#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <string.h>
#include <math.h>
#include "esp_system.h"
#include "LVFV_system.h"

#include "driver/i2c.h"
#include "./SH1106/display.h"
#include "./SH1106/sh1106_i2c.h"
#include "./SH1106/sh1106_graphics.h"

#include "./MCP23017/io_control.h"

#include "./variable_admin.h"

#define I2C_MASTER_NUM                                  I2C_NUM_0
#define I2C_MASTER_SDA_IO                               18       // GPIO21 -> SDA
#define I2C_MASTER_SCL_IO                               5        // GPIO22 -> SCL
#define I2C_MASTER_FREQ_HZ                              400000   // 400kHz
#define I2C_MASTER_TX_BUF_DISABLE                       0
#define I2C_MASTER_RX_BUF_DISABLE                       0

#define SCREEN_MAIN                     0
#define SCREEN_SELECT_VARIABLE          1
#define SCREEN_TIME_EDIT                2
#define SCREEN_SECURITY_EDIT            3
#define SCREEN_FREQUENCY_EDIT           4


extern QueueHandle_t MCP23017_evt_queue;
extern QueueHandle_t button_evt_queue;
extern QueueHandle_t isolated_inputs_queue;

extern uint8_t blink;
static uint8_t screen_displayed = SCREEN_MAIN;

static const uint8_t init_seq[] = {
    0xAE,  // DISPLAYOFF
    0xD5,  // SETDISPLAYCLOCKDIV
    0x80,
    0xA8,  // SETMULTIPLEX
    0x3F,
    0xD3,  // SETDISPLAYOFFSET
    0x00,
    0x40,  // SETSTARTLINE
    0xAD,  // SETCHARGEPUMP
    0x8B,
    0xA1,  // SEGREMAP
    0xC8,  // COMSCANDEC
    0xDA,  // SETCOMPINS
    0x12,
    0x81,  // SETCONTRAST
    0xCF,
    0xD9,  // SETPRECHARGE
    0xF1,
    0xDB,  // SETVCOMDETECT
    0x40,
    0xA4,  // DISPLAYALLON_RESUME
    0xA6,  // NORMALDISPLAY
    0xAF   // DISPLAYON
};

sh1106_t oled;

static const char *TAG = "DISPLAY";

static void set_frequency_table();
static void accelerating(void *pvParameters );
static void desaccelerating( void *pvParameters );
static void i2c_master_init(void);
static void sh1106_clear_buffer();
static esp_err_t sh1106_refresh();
static void sh1106_print_frame();
static void sh1106_time_edit_variables(time_settings_SH1106_t *variables);
static void sh1106_security_edit_variables(seccurity_settings_SH1106_t *variables);
static void sh1106_frequency_edit_variables(frequency_settings_SH1106_t *variables);
static void sh1106_select_edit_variables(sh1106_variable_lines_e variable);
static void sh1106_splash_screen();
static void sh1106_main_screen();

system_status_t system_status;
uint16_t frequency_table[8];

static TaskHandle_t accelerating_handle = NULL;
static TaskHandle_t desaccelerating_handle = NULL;

static void set_frequency_table( ) {
    if ( system_frequency_settings.input_variable == 1 ) { //lineal
        uint16_t frequency_gap = system_frequency_settings.freq_regime /8;
        frequency_table[7] = frequency_gap;
        for (uint8_t i = 7; i > 0; i-- ) {
            frequency_table[i] = frequency_gap + frequency_table[i + 1];
            ESP_LOGI(TAG, "frequency_table[%d] = %d", i, frequency_table[i]);
        }
        frequency_table[0] = system_frequency_settings.freq_regime;
        ESP_LOGI(TAG, "frequency_table[0] = %d", frequency_table[0]);
    } else if ( system_frequency_settings.input_variable == 2 ) { //Cuadratica
        for (uint8_t i = 0; i < 8; i++) {
            uint32_t num = (uint32_t)system_frequency_settings.freq_regime * (uint32_t)(i + 1) * (uint32_t)(i + 1);
            uint16_t fi  = (uint16_t)((num + 32) / 64);      // +32 para redondeo
            if (fi == 0)
                fi = 1;                             // evita 0 si querés
            frequency_table[7 - i] = fi;
            ESP_LOGI(TAG, "frequency_table[%d] = %d", 7 - i, frequency_table[7 - i]);
        }
    }
}

static void accelerating(void *pvParameters ) {
    vTaskDelay(pdMS_TO_TICKS(200));
    while( system_status.frequency != system_frequency_settings.freq_regime ) {
        if ( (system_status.frequency + system_frequency_settings.acceleration ) > system_frequency_settings.freq_regime ) {
            system_status.frequency = system_frequency_settings.freq_regime;
        } else {
            system_status.frequency += system_frequency_settings.acceleration;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    accelerating_handle = NULL;
    vTaskDelete(NULL);
}

static void desaccelerating( void *pvParameters ) {
    vTaskDelay(pdMS_TO_TICKS(200));
    while( system_status.frequency ) {
        if ( system_status.frequency > system_frequency_settings.desacceleration ) {
            system_status.frequency -= system_frequency_settings.desacceleration;
        } else {
            system_status.frequency = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    desaccelerating_handle = NULL;
    vTaskDelete(NULL);
}

uint16_t engine_start() {
    if ( system_status.emergency_status == 0 ) {
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        if ( desaccelerating_handle != NULL ) {
            vTaskDelete( desaccelerating_handle );
            desaccelerating_handle = NULL;
        }
        xTaskCreatePinnedToCore(accelerating, "accelerating", 1024, NULL, 9, &accelerating_handle, tskNO_AFFINITY);
    }
    return system_status.frequency_destiny;
}

void engine_stop() {
    system_status.frequency_destiny = 0;
    system_status.emergency_status = 0;
    if ( accelerating_handle != NULL ) {
        vTaskDelete( accelerating_handle );
        accelerating_handle = NULL;
    }
    xTaskCreatePinnedToCore(desaccelerating, "desaccelerating", 1024, NULL, 9, &desaccelerating_handle, tskNO_AFFINITY);
}

void engine_emergency_stop() {
    if ( accelerating_handle != NULL ) {
        vTaskDelete( accelerating_handle );
        accelerating_handle = NULL;
    }
    if ( desaccelerating_handle != NULL ) {
        vTaskDelete( desaccelerating_handle );
        desaccelerating_handle = NULL;
    }
    system_status.frequency_destiny = 0;
    system_status.frequency = 0;
    system_status.emergency_status = 1;
}

uint16_t change_frequency(uint8_t speed_slector) {
    system_status.inputs_status = speed_slector;
    if ( system_status.frequency_destiny > frequency_table[system_status.inputs_status] ) {
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        if ( accelerating_handle != NULL ) {
            vTaskDelete( accelerating_handle );
            accelerating_handle = NULL;
        }
        xTaskCreatePinnedToCore(desaccelerating, "desaccelerating", 1024, NULL, 9, &desaccelerating_handle, tskNO_AFFINITY);
        ESP_LOGI( TAG, "Cambiando la frecuencia de regimen a %d. Desacelerando", system_status.frequency_destiny);
    } else if ( system_status.frequency_destiny < frequency_table[system_status.inputs_status] ) {
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        if ( desaccelerating_handle != NULL ) {
            vTaskDelete( desaccelerating_handle );
            desaccelerating_handle = NULL;
        }
        xTaskCreatePinnedToCore(accelerating, "accelerating", 1024, NULL, 9, &accelerating_handle, tskNO_AFFINITY);
        ESP_LOGI( TAG, "Cambiando la frecuencia de regimen a %d. Acelerando", system_status.frequency_destiny);
    }
    return system_status.frequency_destiny;
}

uint16_t get_frequency_destiny() {
    return system_status.frequency_destiny;
}

uint16_t get_frequency() {
    return system_status.frequency;
}

void update_meas(seccurity_settings_t bus_meas) {
    uint16_t vbus, ibus;
    vbus = bus_meas.vbus_min;
    ibus = bus_meas.ibus_max;
    system_status.vbus_min = bus_meas.vbus_min;
    system_status.ibus_max = bus_meas.ibus_max;

    if ( ibus > system_seccurity_settings.ibus_max ) {
        if ( system_status.emergency_status == 0 ) {
            ESP_LOGI( TAG, "Disparo de emergencia por sobre corriente. Mando senal");
            uint32_t emergency = SECURITY_EXCEDED;
            xQueueSend(isolated_inputs_queue, &emergency, 0);
            system_status.emergency_status = 1;
        }
    }

    if ( vbus < system_seccurity_settings.vbus_min ) {
        if ( system_status.emergency_status == 0 ) {
            ESP_LOGI( TAG, "system_status.vbus_min = %d", system_status.vbus_min);
            ESP_LOGI( TAG, "system_seccurity_settings.vbus_min = %d", system_seccurity_settings.vbus_min);
            ESP_LOGI( TAG, "Disparo de emergencia por baja tensión. Mando senal");
            uint32_t emergency = SECURITY_EXCEDED;
            xQueueSend(isolated_inputs_queue, &emergency, 0);
            system_status.emergency_status = 1;
        }
    }
}

esp_err_t sh1106_init() {
    
    oled.width = 128;
    oled.height = 64;
    oled.rotation = 0;

    i2c_master_init();
    for (size_t i = 0; i < sizeof(init_seq); i++) {
        if ( sh1106_write_command(init_seq[i]) != ESP_OK ) {
            ESP_LOGE(TAG, "Error enviando comando de inicialización %zu: 0x%02X", i, init_seq[i]);
            return ESP_FAIL;
        }
    }
    sh1106_clear_buffer();
    return sh1106_refresh();
}

void task_display(void *pvParameters) {
    uint8_t new_button;                                                         // Variable Queue

    sh1106_variable_lines_e top_variable = first, bottom_variable = fifth;      // Identificador de la variable seleccionada
    sh1106_variable_lines_e variable_lines = VARIABLE_FIRST;

    uint8_t top_multiplier = 100, bottom_multiplier = 1;                          // Incremental de la variable seleccionada
    uint8_t multiplier = 1;

    uint32_t edit_variable_min = 0, edit_variable_max = 0;                       // Variable seleccionada
    uint16_t *edit_variable = NULL;
    
    uint8_t edit = 0;                                                           // Edición o selección

    struct tm timestamp_edit;
    struct tm time_start_edit;
    struct tm time_stop_edit;
    
    sh1106_splash_screen();

    time_settings_SH1106_t time_edit;
    seccurity_settings_SH1106_t seccurity_edit;
    frequency_settings_SH1106_t frequency_edit;
    
    frequency_edit.multiplier = &multiplier;
    frequency_edit.edit = &variable_lines;
    frequency_edit.edit_flag = &edit;

    seccurity_edit.multiplier = &multiplier;
    seccurity_edit.edit = &variable_lines;
    seccurity_edit.edit_flag = &edit;

    time_edit.multiplier = &multiplier;
    time_edit.edit_flag = &edit;
    time_edit.time_settings.timeinfo = &timestamp_edit;
    time_edit.time_settings.time_start = &time_start_edit;
    time_edit.time_settings.time_stop = &time_stop_edit;
    time_edit.edit = &variable_lines;
    time_edit.time_settings.time_start = &time_start_edit;
    time_edit.time_settings.time_stop = &time_stop_edit;

    nvs_init_once();

    load_frequency( (int16_t*) &(system_frequency_settings.freq_regime) );
    load_acceleration( (int16_t*) &(system_frequency_settings.acceleration) );
    load_desacceleration( (int16_t*) &(system_frequency_settings.desacceleration) );
    load_input_variable( (int16_t*) &(system_frequency_settings.input_variable) );
    load_vbus_min( (int16_t*) &(system_seccurity_settings.vbus_min) );
    load_ibus_max( (int16_t*) &(system_seccurity_settings.ibus_max) );
    load_hour_ini( (int16_t*) &(system_time_settings.time_start->tm_hour) );
    load_min_ini( (int16_t*) &(system_time_settings.time_start->tm_min) );
    load_hour_fin( (int16_t*) &(system_time_settings.time_stop->tm_hour) );
    load_min_fin( (int16_t*) &(system_time_settings.time_stop->tm_min) );

    setTime( &timeinfo );
    rtc_schedule_alarms();
    set_frequency_table();

    while( MCP23017_evt_queue == NULL ) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while( button_evt_queue == NULL ) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while( isolated_inputs_queue == NULL ) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (1) {
        blink++;
        if ( blink >= 24 ) {
            blink = 0;
            // UBaseType_t hw = uxTaskGetStackHighWaterMark2(NULL);
            // ESP_LOGI(TAG, "HighWaterMark = %u words (~%u bytes)", (unsigned)hw, (unsigned)(hw*4));
        }
        if ( xQueueReceive( button_evt_queue, &new_button, pdMS_TO_TICKS(5) ) ) {
            switch (new_button) {
                case BUTTON_MENU:
                    if ( screen_displayed == SCREEN_MAIN ) {
                        screen_displayed = SCREEN_SELECT_VARIABLE;
                        variable_lines = VARIABLE_SECOND;
                        top_variable = VARIABLE_SECOND;
                        bottom_variable = VARIABLE_FOURTH;

                        frequency_edit.frequency_settings.freq_regime = system_frequency_settings.freq_regime;
                        frequency_edit.frequency_settings.acceleration = system_frequency_settings.acceleration;
                        frequency_edit.frequency_settings.desacceleration = system_frequency_settings.desacceleration;
                        frequency_edit.frequency_settings.input_variable = system_frequency_settings.input_variable;

                        seccurity_edit.seccurity_settings.vbus_min = system_seccurity_settings.vbus_min;
                        seccurity_edit.seccurity_settings.ibus_max = system_seccurity_settings.ibus_max;

                        time_edit.time_settings.timeinfo->tm_hour = system_time_settings.timeinfo->tm_hour;
                        time_edit.time_settings.timeinfo->tm_min = system_time_settings.timeinfo->tm_min;
                        time_edit.time_settings.timeinfo->tm_sec = system_time_settings.timeinfo->tm_sec;

                        time_edit.time_settings.time_start->tm_hour = system_time_settings.time_start->tm_hour;
                        time_edit.time_settings.time_start->tm_min = system_time_settings.time_start->tm_min;
                        time_edit.time_settings.time_start->tm_sec = 0;

                        time_edit.time_settings.time_stop->tm_hour = system_time_settings.time_stop->tm_hour;
                        time_edit.time_settings.time_stop->tm_min = system_time_settings.time_stop->tm_min;
                        time_edit.time_settings.time_stop->tm_sec = 0;    
                    }
                    break;
                case BUTTON_OK:
                    if ( screen_displayed == SCREEN_SELECT_VARIABLE ) {
                        if ( variable_lines == VARIABLE_SECOND ) {
                            screen_displayed = SCREEN_FREQUENCY_EDIT;
                            variable_lines = VARIABLE_FIRST;
                            top_variable = VARIABLE_FIRST;
                            bottom_variable = VARIABLE_FOURTH;
                            
                            variable_lines = VARIABLE_FIRST;
                        } else if ( variable_lines == VARIABLE_THIRD ) {
                            top_variable = VARIABLE_SECOND;
                            bottom_variable = VARIABLE_THIRD;
                            screen_displayed = SCREEN_SECURITY_EDIT;
                            variable_lines = VARIABLE_SECOND;

                            variable_lines = VARIABLE_SECOND;
                        } else if ( variable_lines == VARIABLE_FOURTH ) {
                            top_variable = VARIABLE_SECOND;
                            bottom_variable = VARIABLE_EIGTH;
                            variable_lines = VARIABLE_SECOND;
                            screen_displayed = SCREEN_TIME_EDIT;

                            variable_lines = VARIABLE_SECOND;
                        }
                        multiplier = 1;
                    } else if ( screen_displayed == SCREEN_FREQUENCY_EDIT ) {
                        if ( edit_variable == NULL ) {
                            if ( variable_lines == EDIT_FREQUENCY_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.freq_regime);
                                edit_variable_min = 5;
                                edit_variable_max = 450;
                            } else if ( variable_lines == EDIT_ACCELERATION_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.acceleration);
                                edit_variable_min = 1;
                                edit_variable_max = 450;
                            } else if ( variable_lines == EDIT_DESACCELERATION_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.desacceleration);
                                edit_variable_min = 1;
                                edit_variable_max = 450;
                            } else if ( variable_lines == EDIT_INPUT_VARIATION_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.input_variable);
                                edit_variable_min = 1;
                                edit_variable_max = 2;
                                top_multiplier = 1;
                                bottom_multiplier = 1;
                            }
                            edit = 1;
                            multiplier = 1;
                            top_multiplier = 100;
                            bottom_multiplier = 1;
                        } else {
                            edit = 0;
                            edit_variable = NULL;
                            edit_variable_min = 0;
                            edit_variable_max = 0;
                        }
                    } else if ( screen_displayed == SCREEN_SECURITY_EDIT ) {
                        if ( edit_variable == NULL ) {
                            if ( variable_lines == EDIT_VBUS_LINE_INDX ) {
                                edit_variable = (uint16_t*) &(seccurity_edit.seccurity_settings.vbus_min);
                                edit_variable_min = 310;
                                edit_variable_max = 360;
                            } else if ( variable_lines == EDIT_IBUS_LINE_INDX ) {
                                edit_variable = (uint16_t*) &(seccurity_edit.seccurity_settings.ibus_max);
                                edit_variable_min = 500;
                                edit_variable_max = 2000;
                            }
                            edit = 1;
                            multiplier = 1;
                            top_multiplier = 100;
                            bottom_multiplier = 1;
                        } else {
                            edit = 0;
                            edit_variable = NULL;
                            edit_variable_min = 0;
                            edit_variable_max = 0;
                        }
                    } else if ( screen_displayed == SCREEN_TIME_EDIT ) {
                        if ( edit_variable == NULL ) {
                            if ( variable_lines == VARIABLE_SECOND ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.timeinfo->tm_hour);
                                edit_variable_min = 0;
                                edit_variable_max = 23;
                            } else if ( variable_lines == VARIABLE_THIRD ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.timeinfo->tm_min);
                                edit_variable_min = 0;
                                edit_variable_max = 59;
                            } else if ( variable_lines == VARIABLE_FOURTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.timeinfo->tm_sec);
                                edit_variable_min = 0;
                                edit_variable_max = 59;
                            } else if ( variable_lines == VARIABLE_FIFTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_start->tm_hour);
                                edit_variable_min = 0;
                                edit_variable_max = 23;
                            } else if ( variable_lines == VARIABLE_SIXTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_start->tm_min);
                                edit_variable_min = 0;
                                edit_variable_max = 59;
                            } else if ( variable_lines == VARIABLE_SEVENTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_stop->tm_hour);
                                edit_variable_min = 0;
                                edit_variable_max = 23;
                            } else if ( variable_lines == VARIABLE_EIGTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_stop->tm_min);
                                edit_variable_min = 0;
                                edit_variable_max = 59;
                            }
                            edit = 1;
                            multiplier = 1;
                            top_multiplier = 10;
                            bottom_multiplier = 1;
                        } else {
                            edit = 0;
                            edit_variable = NULL;
                            edit_variable_min = 0;
                            edit_variable_max = 0;
                        }
                    }
                    break;
                case BUTTON_BACK:
                    if ( screen_displayed == SCREEN_SELECT_VARIABLE ) {
                        screen_displayed = SCREEN_MAIN;
                    } else if ( screen_displayed == SCREEN_SECURITY_EDIT || screen_displayed == SCREEN_FREQUENCY_EDIT || screen_displayed == SCREEN_TIME_EDIT ) {
                        screen_displayed = SCREEN_SELECT_VARIABLE;
                        variable_lines = VARIABLE_SECOND;
                        top_variable = VARIABLE_SECOND;
                        bottom_variable = VARIABLE_FOURTH;
                        edit_variable = NULL;
                        edit_variable_min = 0;
                        edit_variable_max = 0;
                    }
                    break;
                case BUTTON_UP:
                    if ( edit_variable != NULL ) {
                        (*edit_variable) += multiplier;
                        if ( *edit_variable >= edit_variable_max ) {
                            *edit_variable = edit_variable_max;
                        }
                    } else {
                        if ( variable_lines == top_variable ) {
                            variable_lines = bottom_variable;
                        } else {
                            variable_lines -= LINE_INCREMENT;
                        }
                    }
                    break;
                case BUTTON_DOWN:
                    if ( edit_variable != NULL ) {
                        (*edit_variable) -= multiplier;
                        if ( *edit_variable < edit_variable_min || *edit_variable > edit_variable_max ) {
                            *edit_variable = edit_variable_min;
                        }
                    } else {
                        if ( variable_lines == bottom_variable ) {
                            variable_lines = top_variable;
                        } else {
                            variable_lines += LINE_INCREMENT;
                        }
                    }
                    break;
                case BUTTON_LEFT:
                    if ( multiplier < top_multiplier ) {
                        multiplier *= 10;
                    }
                    if ( multiplier > top_multiplier ) {
                        multiplier = top_multiplier;
                    }

                    break;
                case BUTTON_RIGHT:
                    if ( multiplier > bottom_multiplier ) {
                        multiplier /= 10;
                    } 
                    if (multiplier < bottom_multiplier ) {
                        multiplier = bottom_multiplier;
                    }
                    break;
                case BUTTON_SAVE:
                    ESP_LOGI(TAG,"Guardando valores...");

                    system_frequency_settings.freq_regime = frequency_edit.frequency_settings.freq_regime;
                    system_frequency_settings.acceleration = frequency_edit.frequency_settings.acceleration;
                    system_frequency_settings.desacceleration = frequency_edit.frequency_settings.desacceleration;
                    system_frequency_settings.input_variable = frequency_edit.frequency_settings.input_variable;
                    system_seccurity_settings.vbus_min = seccurity_edit.seccurity_settings.vbus_min;
                    system_seccurity_settings.ibus_max = seccurity_edit.seccurity_settings.ibus_max;
                    system_time_settings.timeinfo->tm_hour = time_edit.time_settings.timeinfo->tm_hour;
                    system_time_settings.timeinfo->tm_min = time_edit.time_settings.timeinfo->tm_min;
                    system_time_settings.timeinfo->tm_sec = time_edit.time_settings.timeinfo->tm_sec;
                    system_time_settings.time_start->tm_hour = time_edit.time_settings.time_start->tm_hour;
                    system_time_settings.time_start->tm_min = time_edit.time_settings.time_start->tm_min;
                    system_time_settings.time_start->tm_sec = 0;
                    system_time_settings.time_stop->tm_hour = time_edit.time_settings.time_stop->tm_hour;
                    system_time_settings.time_stop->tm_min = time_edit.time_settings.time_stop->tm_min;
                    system_time_settings.time_stop->tm_sec = 0;

                    save_frequency( (int16_t) system_frequency_settings.freq_regime);
                    save_acceleration( (int16_t) system_frequency_settings.acceleration);
                    save_desacceleration( (int16_t) system_frequency_settings.desacceleration);
                    save_input_variable( (int16_t) system_frequency_settings.input_variable);
                    save_vbus_min( (int16_t) system_seccurity_settings.vbus_min);
                    save_ibus_max( (int16_t) system_seccurity_settings.ibus_max);
                    save_hour_ini( (int16_t) system_time_settings.time_start->tm_hour);
                    save_min_ini( (int16_t) system_time_settings.time_start->tm_min);
                    save_hour_fin( (int16_t) system_time_settings.time_stop->tm_hour);
                    save_min_fin( (int16_t) system_time_settings.time_stop->tm_min);

                    setTime( &timeinfo );
                    set_frequency_table();
                    rtc_schedule_alarms();

                    screen_displayed = SCREEN_MAIN;
                    edit = 0;
                    edit_variable = NULL;
                    edit_variable_min = 0;
                    edit_variable_max = 0;
                    break;
            }
        }

        switch (screen_displayed) {
            case SCREEN_MAIN:
                sh1106_main_screen();
                break;
            case SCREEN_SELECT_VARIABLE:
                sh1106_select_edit_variables(variable_lines);
                break;
            case SCREEN_TIME_EDIT:
                sh1106_time_edit_variables(&time_edit);
                break;
            case SCREEN_SECURITY_EDIT:
                sh1106_security_edit_variables(&seccurity_edit);
                break;
            case SCREEN_FREQUENCY_EDIT:
                sh1106_frequency_edit_variables(&frequency_edit);
                break;
        }
    }
}

// Inicializar bus I2C (driver nuevo)
static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM,
                                       conf.mode,
                                       I2C_MASTER_RX_BUF_DISABLE,
                                       I2C_MASTER_TX_BUF_DISABLE, 0));
}

static void sh1106_clear_buffer() {
    memset(oled.buffer, 0, sizeof(oled.buffer));
}

static esp_err_t sh1106_refresh() {
    // El SH1106 espera que se setee la dirección de la columna
    // luego se manden datos página por página
    for (uint8_t page = 0; page < 8; page++) {
        // Dirección de página
        ESP_ERROR_CHECK(sh1106_write_command(0xB0 + page));

        // Dirección de columna baja y alta (offset 2 que ajusta SH1106)
        ESP_ERROR_CHECK(sh1106_write_command(0x02));    // col lo (2)
        ESP_ERROR_CHECK(sh1106_write_command(0x10));    // col hi

        // Enviar 128 bytes de datos para esta página
        uint8_t *page_data = &oled.buffer[page * 128];
        ESP_ERROR_CHECK(sh1106_write_data(page_data, 128));
    }
    return ESP_OK;
}

static void sh1106_print_frame() {
    char str[13];
    sh1106_draw_utn_logo( &oled, 1, 1);
    sh1106_draw_line( &oled, 0, 17);
    strftime(str, sizeof(str), "%H:%M:%S", &timeinfo);
    sh1106_draw_text( &oled, str, 34, 0, SH1106_SIZE_2);
}

static void sh1106_time_edit_variables(time_settings_SH1106_t *variables) {
    char hora_str[12];
    uint8_t blink_compare = 4;
    if ( *(variables->edit_flag) ) {
        blink_compare = 12;
    }

    sh1106_clear_buffer();
    sh1106_print_frame();

    sh1106_draw_text( &oled, "Time:", 15, EDIT_TIME_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Ini:", 15, EDIT_HINI_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Fin:", 15, EDIT_HFIN_LINE_INDX, SH1106_SIZE_1);

    if ( *(variables->edit) >= VARIABLE_SECOND && *(variables->edit) <= VARIABLE_FOURTH ) {
        sh1106_draw_arrow( &oled, 1, (int) VARIABLE_SECOND);
    } else if ( *(variables->edit) >= VARIABLE_FIFTH && *(variables->edit) <= VARIABLE_SIXTH ) {
        sh1106_draw_arrow( &oled, 1, (int) VARIABLE_THIRD);
    } else if ( *(variables->edit) >= VARIABLE_SEVENTH && *(variables->edit) <= VARIABLE_EIGTH ) {
        sh1106_draw_arrow( &oled, 1, (int) VARIABLE_FOURTH);
    }

    if ( *(variables->edit) == VARIABLE_SECOND && blink < blink_compare) {
        sprintf(hora_str, "  :%02d:%02d", variables->time_settings.timeinfo->tm_min, variables->time_settings.timeinfo->tm_sec);
    } else if( *(variables->edit) == VARIABLE_THIRD && blink < blink_compare) {
        sprintf(hora_str, "%02d:  :%02d", variables->time_settings.timeinfo->tm_hour, variables->time_settings.timeinfo->tm_sec);
    } else if( *(variables->edit) == VARIABLE_FOURTH && blink < blink_compare) {
        sprintf(hora_str, "%02d:%02d:  ", variables->time_settings.timeinfo->tm_hour, variables->time_settings.timeinfo->tm_min);
    } else {
        strftime(hora_str, sizeof(hora_str), "%H:%M:%S", variables->time_settings.timeinfo);
    }
    sh1106_draw_text( &oled, hora_str, 64, EDIT_TIME_LINE_INDX, SH1106_SIZE_1);

    if ( *(variables->edit) == VARIABLE_FIFTH && blink < blink_compare ) {
        sprintf(hora_str, "  :%02d", variables->time_settings.time_start->tm_min);
    } else if( *(variables->edit) == VARIABLE_SIXTH && blink < blink_compare ) {
        sprintf(hora_str, "%02d:  ", variables->time_settings.time_start->tm_hour);
    } else {
        sprintf(hora_str, "%02d:%02d", variables->time_settings.time_start->tm_hour, variables->time_settings.time_start->tm_min);
    }
    sh1106_draw_text( &oled, hora_str, 64, EDIT_HINI_LINE_INDX, SH1106_SIZE_1);

    if ( *(variables->edit) == VARIABLE_SEVENTH && blink < blink_compare ) {
        sprintf(hora_str, "  :%02d", variables->time_settings.time_stop->tm_min);
    } else if ( *(variables->edit) == VARIABLE_EIGTH && blink < blink_compare ) {
        sprintf(hora_str, "%02d:  ", variables->time_settings.time_stop->tm_hour);
    } else {
        sprintf(hora_str, "%02d:%02d", variables->time_settings.time_stop->tm_hour, variables->time_settings.time_stop->tm_min);
    }
    sh1106_draw_text( &oled, hora_str, 64, EDIT_HFIN_LINE_INDX, SH1106_SIZE_1);

    ESP_ERROR_CHECK(sh1106_refresh());
}

static void sh1106_security_edit_variables(seccurity_settings_SH1106_t *variables) {
    char num_str[12];

    sh1106_clear_buffer( &oled );
    sh1106_print_frame();

    sh1106_draw_text( &oled, "V. min:", 15, EDIT_VBUS_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "V",           117, EDIT_VBUS_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "I. max:", 15, EDIT_IBUS_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "A",         117, EDIT_IBUS_LINE_INDX, SH1106_SIZE_1);
    sh1106_draw_arrow( &oled, 1, (int) *(variables->edit));

    if ( *(variables->edit) == EDIT_VBUS_LINE_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            sprintf(num_str, "%03d", variables->seccurity_settings.vbus_min);
            sh1106_draw_text( &oled, num_str, 85, EDIT_VBUS_LINE_INDX, SH1106_SIZE_1);
        }
    } else {
        sprintf(num_str, "%03d", variables->seccurity_settings.vbus_min);
        sh1106_draw_text( &oled, num_str, 85, EDIT_VBUS_LINE_INDX, SH1106_SIZE_1);
    }

    if ( *(variables->edit) == EDIT_IBUS_LINE_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            sprintf(num_str, "%03d", variables->seccurity_settings.ibus_max);
            sh1106_draw_text( &oled, num_str, 85, EDIT_IBUS_LINE_INDX, SH1106_SIZE_1);
        }
    } else {
        sprintf(num_str, "%03d", variables->seccurity_settings.ibus_max);
        sh1106_draw_text( &oled, num_str, 85, EDIT_IBUS_LINE_INDX, SH1106_SIZE_1);
    }
    sh1106_draw_arrow( &oled, 1, (int) variables->edit);

    ESP_ERROR_CHECK(sh1106_refresh());
}

static void sh1106_frequency_edit_variables(frequency_settings_SH1106_t *variables) {
    char num_str[12];

    sh1106_clear_buffer();
    sh1106_print_frame();

    sh1106_draw_text( &oled, "Frec:",        15, VARIABLE_FIRST, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Hz",           95, VARIABLE_FIRST, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Acel:",        15, VARIABLE_SECOND, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Hz/s",         95, VARIABLE_SECOND, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Desa:",        15, VARIABLE_THIRD, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Hz/s",         95, VARIABLE_THIRD, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Var entradas", 18, VARIABLE_FOURTH, SH1106_SIZE_1);

    if ( *(variables->edit) != VARIABLE_FOURTH ) {
        sh1106_draw_arrow( &oled, 1, (int) *(variables->edit));
    } else {
        sh1106_draw_arrow( &oled, 1, (int) VARIABLE_FIFTH);
    }

    if ( *(variables->edit) == EDIT_FREQUENCY_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            sprintf(num_str, "%03d", variables->frequency_settings.freq_regime);
            sh1106_draw_text( &oled, num_str, 70, VARIABLE_FIRST, SH1106_SIZE_1);
        }
    } else {
        sprintf(num_str, "%03d", variables->frequency_settings.freq_regime);
        sh1106_draw_text( &oled, num_str, 70, VARIABLE_FIRST, SH1106_SIZE_1);
    } 

    if ( *(variables->edit) == EDIT_ACCELERATION_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            sprintf(num_str, "%02d", variables->frequency_settings.acceleration);
            sh1106_draw_text( &oled, num_str, 70, VARIABLE_SECOND, SH1106_SIZE_1);
        }
    } else {
        sprintf(num_str, "%02d", variables->frequency_settings.acceleration);
        sh1106_draw_text( &oled, num_str, 70, VARIABLE_SECOND, SH1106_SIZE_1);
    }

    if ( *(variables->edit) == EDIT_DESACCELERATION_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            sprintf(num_str, "%02d", variables->frequency_settings.desacceleration);
            sh1106_draw_text( &oled, num_str, 70, VARIABLE_THIRD, SH1106_SIZE_1);
        }
    } else {
        sprintf(num_str, "%02d", variables->frequency_settings.desacceleration);
        sh1106_draw_text( &oled, num_str, 70, VARIABLE_THIRD, SH1106_SIZE_1);
    }
    if ( *(variables->edit) == EDIT_INPUT_VARIATION_INDX && *(variables->edit_flag) ) {
        if ( blink >= 12 ) {
            if ( variables->frequency_settings.input_variable == 1 ) {
                sh1106_draw_text( &oled, "   Lineal", 18, VARIABLE_FIFTH, SH1106_SIZE_1);
            } else if ( variables->frequency_settings.input_variable == 2 ) {
                sh1106_draw_text( &oled, " Cuadratica", 18, VARIABLE_FIFTH, SH1106_SIZE_1);
            }
        }
    } else {
        if ( variables->frequency_settings.input_variable == 1 ) {
            sh1106_draw_text( &oled, "   Lineal", 18, VARIABLE_FIFTH, SH1106_SIZE_1);
        } else if ( variables->frequency_settings.input_variable == 2 ) {
            sh1106_draw_text( &oled, " Cuadratica", 18, VARIABLE_FIFTH, SH1106_SIZE_1);
        }
    }

    ESP_ERROR_CHECK(sh1106_refresh());
}

static void sh1106_select_edit_variables(sh1106_variable_lines_e variable) {

    sh1106_clear_buffer();
    sh1106_print_frame();

    sh1106_draw_arrow( &oled, 1, (int) variable);
    sh1106_draw_text( &oled, "Frecuencias", 15, VARIABLE_SECOND, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Seguridad", 15, VARIABLE_THIRD, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Horarios", 15, VARIABLE_FOURTH, SH1106_SIZE_1);
    ESP_ERROR_CHECK(sh1106_refresh());
}

static void sh1106_splash_screen() {

    sh1106_clear_buffer();
    sh1106_draw_utn_logo( &oled, 56, 10);

    sh1106_draw_text( &oled, "    UTN FRBA", 0, 30, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Proy. Final 2025", 0, 40, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Andrenacci-Carra", 0, 50, SH1106_SIZE_1);
    ESP_ERROR_CHECK(sh1106_refresh());
}

static void sh1106_main_screen( ) {

    char hora_str[13];

    sh1106_clear_buffer();
    sh1106_print_frame();

    sh1106_draw_text( &oled, "Frecuen.:", 5, VARIABLE_FIRST, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "V. Bus AC:" , 5, VARIABLE_SECOND, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "I. Out AC:" , 5, VARIABLE_THIRD, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Arranque:"  , 5, VARIABLE_FOURTH, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Parada:"    , 5, VARIABLE_FIFTH, SH1106_SIZE_1);

    sprintf(hora_str, "%03d", system_status.frequency);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FIRST, SH1106_SIZE_1);
    sprintf(hora_str, "%03d", system_status.vbus_min);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_SECOND, SH1106_SIZE_1);
    sprintf(hora_str, "%04d", system_status.ibus_max);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_THIRD, SH1106_SIZE_1);
    sprintf(hora_str, "%02d:%02d", system_time_settings.time_start->tm_hour , system_time_settings.time_start->tm_min );
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FOURTH, SH1106_SIZE_1);
    sprintf(hora_str, "%02d:%02d", system_time_settings.time_stop->tm_hour , system_time_settings.time_stop->tm_min );
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FIFTH, SH1106_SIZE_1);

    ESP_ERROR_CHECK(sh1106_refresh());
}