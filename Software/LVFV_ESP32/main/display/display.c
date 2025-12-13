/**
 * @file display.c
 * @author Andrenacci - Carra
 * @brief Funciones de consulta de los seteos hechos por el usuario, tarea que controla el display y posteo de eventos para controlar el display.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
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
#include "./display/display.h"
#include "./display/sh1106_i2c.h"
#include "./display/sh1106_graphics.h"

#include "./io_control/io_control.h"
#include "../system/SysControl.h"
#include "../system/SysAdmin.h"
#include "../rtc/rtc.h"
#include "../nvs/nvs.h"

#define I2C_DISPLAY_MASTER_NUM                          I2C_NUM_0   // Número de puerto I2C usado
#define I2C_MASTER_SDA_IO                               18          // Pin SDA del puerto I2C -> GPIO21
#define I2C_MASTER_SCL_IO                               5           // Pin SCL del puerto I2C -> GPIO22
#define I2C_MASTER_FREQ_HZ                              400000      // Frecuencia de clock del puerto I2C -> 400kHz
#define I2C_MASTER_TX_BUF_DISABLE                       0           // Largo del buffer de transmisión del puerto I2C - Como se usa en modo master, es una variable ignorada
#define I2C_MASTER_RX_BUF_DISABLE                       0           // Largo del buffer de recepción del puerto I2C - Como se usa en modo master, es una variable ignorada

#define FREQUENCY_LINEAR_VARIABLE       0
#define FREQUENCY_CUADRATIC_VARIABLE    1

#define FREC_LINE_INDX                  VARIABLE_FIRST
#define VBUS_LINE_INDX                  VARIABLE_SECOND
#define IBUS_LINE_INDX                  VARIABLE_THIRD
#define HINI_LINE_INDX                  VARIABLE_FOURTH
#define HFIN_LINE_INDX                  VARIABLE_FIFTH

#define EDIT_FREQUENCY_INDX             VARIABLE_FIRST
#define EDIT_ACCELERATION_INDX          VARIABLE_SECOND
#define EDIT_DESACCELERATION_INDX       VARIABLE_THIRD
#define EDIT_INPUT_VARIATION_INDX       VARIABLE_FOURTH

#define EDIT_TIME_LINE_INDX             VARIABLE_SECOND
#define EDIT_HINI_LINE_INDX             VARIABLE_THIRD
#define EDIT_HFIN_LINE_INDX             VARIABLE_FOURTH

#define EDIT_VBUS_LINE_INDX             VARIABLE_SECOND
#define EDIT_IBUS_LINE_INDX             VARIABLE_THIRD

/**
 * @enum screen_selected_e
 * 
 * @brief Listado de posibles pantallas que puede ver el usuario
 */
typedef enum {
    SCREEN_MAIN,                                                    // 0 - Pantalla principal
    SCREEN_SELECT_VARIABLE,                                         // 1 - Pantalla de selección de pantallas de edición
    SCREEN_TIME_EDIT,                                               // 2 - Pantalla de edición de variables de tiempo
    SCREEN_SECURITY_EDIT,                                           // 3 - Pantalla de edición de variables de seguridad
    SCREEN_FREQUENCY_EDIT                                           // 4 - Pantalla de edición de variables de frecuencia, aceleración y desaceleración
} screen_selected_e;

QueueHandle_t button_evt_queue;                                     // Cola de comandos de las entradas digitales. Tiene capacidad para un solo mensaje del tamaño systemSignal_e

frequency_settings_t system_frequency_settings;                     // Estructura con las variables de frecuencia del sistema
time_settings_t system_time_settings;                               // Estructura con las variables de tiempo del sistema
seccurity_settings_t system_seccurity_settings;                     // Estructura con las variables de seguridad del sistema

static screen_selected_e screen_displayed = SCREEN_MAIN;            // Pantalla actualmente mostrada

static const uint8_t init_seq[] = {                                 // Secuencia de inicialización del display
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

sh1106_t oled;                                                      // Estructura con la configuración del display

static const char *TAG = "DISPLAY";                                 // Tag de ESP_LOG
static uint8_t blink;                                               // Variable de parpadeo para edición de variables         

/**
 * @fn static void i2c_master_init(void);
 *
 * @brief Inicializa el bus I2C para comunicarse con el display SH1106
 */
static void i2c_master_init(void);
/**
 * @fn static void sh1106_clear_buffer();
 *
 * @brief Vacía el buffer de la pantalla para iniciar un nuevo dibujo
 */
static void sh1106_clear_buffer();
/**
 * @fn static esp_err_t sh1106_refresh();
 *
 * @brief Función que imprime la pantalla con los elementos agregados por el usuario
 *
 * @retval ESP_OK si todo funciona bien
 */
static esp_err_t sh1106_refresh();
/**
 * @fn static void sh1106_print_frame();
 *
 * @brief Función que agrega al buffer de la pantalla un marco fijo que incluye la hora, la línea horizontal divisoria y el logo de la UTN
 */
static void sh1106_print_frame();
/**
 * @fn static void sh1106_time_edit_variables(time_settings_SH1106_t *variables);
 *
 * @brief Imprime en pantalla las variables de tiempo a editar: Hora del sistema, hora de inicio y hora de fin
 *
 * @param[inout] variables
 *     Puntero a la estructura de veriables a modificar por el usuario
 */
static void sh1106_time_edit_variables(time_settings_SH1106_t *variables);
/**
 * @fn static void sh1106_security_edit_variables(seccurity_settings_SH1106_t *variables);
 *
 * @brief  Imprime en pantalla las variables de seguridad a editar: Tensión mínima en la bus de contíncua y corriente máxima en el bus de contínua
 *
 * @param[inout] variables
 *     Puntero a la estructura de veriables a modificar por el usuario
 */
static void sh1106_security_edit_variables(seccurity_settings_SH1106_t *variables);
/**
 * @fn static void sh1106_frequency_edit_variables(frequency_settings_SH1106_t *variables);
 *
 * @brief  Imprime en pantalla las variables de frecuencia a editar: Frecuencia de régimen, aceleración, desaceleración y variación de las entradas aisladas para seleccionar la velocidad de régimen
 *
 * @param[inout] variables
 *     Puntero a la estructura de veriables a modificar por el usuario
 */
static void sh1106_frequency_edit_variables(frequency_settings_SH1106_t *variables);
/**
 * @fn static void sh1106_select_edit_variables(sh1106_variable_lines_e variable);
 *
 * @brief Permite al usuario seleccionar entre las tres pantallas de edición de variables: Frecuencia, tiempo y seguridad
 *
 * @param[inout] variables
 *     Selector de menúes
 */
static void sh1106_select_edit_variables(sh1106_variable_lines_e variable);
/**
 * @fn static void sh1106_splash_screen();
 *
 * @brief Pantalla de splash que se imprime al iniciar el sistema
 */
static void sh1106_splash_screen();
/**
 * @fn static void sh1106_main_screen( );
 *
 * @brief Pantalla principal dell sistema. Allí se muestra la hora del sistema, la frecuencia a la que se encuentra el variador, la hora de inicio y fin de movimiento del motor 
 */
static void sh1106_main_screen( );

static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_DISPLAY_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_DISPLAY_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
}

static void sh1106_clear_buffer() {
    memset(oled.buffer, 0, sizeof(oled.buffer));
}

static esp_err_t sh1106_refresh() {
    uint8_t page;
    // El SH1106 espera que se setee la dirección de la columna luego se manden datos página por página
    for ( page = 0; page < 8; page++) {
        // Dirección de página
        uint8_t command = 0xB0 + page;
        ESP_ERROR_CHECK(sh1106_write(&command, 1, SH1106_COMM_CMD));
        // Dirección de columna baja y alta (offset 2 que ajusta SH1106)
        command = 0x02;
        ESP_ERROR_CHECK(sh1106_write(&command, 1, SH1106_COMM_CMD));    // col lo (2)
        command = 0x10;
        ESP_ERROR_CHECK(sh1106_write(&command, 1, SH1106_COMM_CMD));    // col hi

        // Enviar 128 bytes de datos para esta página
        uint8_t *page_data = &oled.buffer[page * 128];
        ESP_ERROR_CHECK(sh1106_write(page_data, 128, SH1106_COMM_DATA));
    }
    if ( page == 8 ) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static void sh1106_print_frame() {
    char str[13];
    sh1106_draw_utn_logo( &oled, 1, 1);
    sh1106_draw_line( &oled, 0, 17);
    strftime(str, sizeof(str), "%H:%M:%S", system_time_settings.time_system);
    sh1106_draw_text( &oled, str, 34, 0, SH1106_SIZE_2);
}

static void sh1106_print_emergency() {
    sh1106_clear_buffer();
    sh1106_print_frame();
    sh1106_draw_text( &oled, "  Parada", 25, 25, SH1106_SIZE_2);
    sh1106_draw_text( &oled, "EMERGENCIA", 25, VARIABLE_FOURTH, SH1106_SIZE_2);

    ESP_ERROR_CHECK(sh1106_refresh());
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
        sprintf(hora_str, "  :%02d:%02d", variables->time_settings.time_system->tm_min, variables->time_settings.time_system->tm_sec);
    } else if( *(variables->edit) == VARIABLE_THIRD && blink < blink_compare) {
        sprintf(hora_str, "%02d:  :%02d", variables->time_settings.time_system->tm_hour, variables->time_settings.time_system->tm_sec);
    } else if( *(variables->edit) == VARIABLE_FOURTH && blink < blink_compare) {
        sprintf(hora_str, "%02d:%02d:  ", variables->time_settings.time_system->tm_hour, variables->time_settings.time_system->tm_min);
    } else {
        strftime(hora_str, sizeof(hora_str), "%H:%M:%S", variables->time_settings.time_system);
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
    system_status_t s_e;
    get_status(&s_e);

    sh1106_clear_buffer();
    sh1106_print_frame();

    sh1106_draw_text( &oled, "Frecuen.:", 5, VARIABLE_FIRST, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "V. Bus DC:" , 5, VARIABLE_SECOND, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "I. Out DC:" , 5, VARIABLE_THIRD, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Arranque:"  , 5, VARIABLE_FOURTH, SH1106_SIZE_1);
    sh1106_draw_text( &oled, "Parada:"    , 5, VARIABLE_FIFTH, SH1106_SIZE_1);

    sprintf(hora_str, "%03d", s_e.frequency);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FIRST, SH1106_SIZE_1);
    sprintf(hora_str, "%03d", s_e.vbus_min);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_SECOND, SH1106_SIZE_1);
    sprintf(hora_str, "%04d", s_e.ibus_max);
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_THIRD, SH1106_SIZE_1);
    sprintf(hora_str, "%02d:%02d", system_time_settings.time_start->tm_hour , system_time_settings.time_start->tm_min );
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FOURTH, SH1106_SIZE_1);
    sprintf(hora_str, "%02d:%02d", system_time_settings.time_stop->tm_hour , system_time_settings.time_stop->tm_min );
    sh1106_draw_text( &oled, hora_str, 86, VARIABLE_FIFTH, SH1106_SIZE_1);

    ESP_ERROR_CHECK(sh1106_refresh());
}

uint16_t get_system_frequency() {
    return system_frequency_settings.freq_regime;
}

uint16_t get_system_acceleration() {
    return system_frequency_settings.acceleration;
}

uint16_t get_system_desacceleration() {
    return system_frequency_settings.desacceleration;
}

esp_err_t sh1106_init() {
    
    oled.width = 128;
    oled.height = 64;
    oled.rotation = 0;

    i2c_master_init();

    for (size_t i = 0; i < sizeof(init_seq); i++) {
        if ( sh1106_write(&init_seq[i], 1, SH1106_COMM_CMD) != ESP_OK ) {
            ESP_LOGE(TAG, "Error enviando comando de inicialización %zu: 0x%02X", i, init_seq[i]);
            return ESP_FAIL;
        }
    }
    sh1106_clear_buffer();
    sh1106_splash_screen();
    return sh1106_refresh();
}

esp_err_t system_variables_save(frequency_settings_SH1106_t *frequency_settings, time_settings_SH1106_t *time_settings, seccurity_settings_SH1106_t *seccurity_settings) {
    ESP_LOGI(TAG, "Guardando variables del sistema desde el display");
    if ( frequency_settings == NULL || time_settings == NULL || seccurity_settings == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    if ( time_settings->time_settings.time_system == NULL || time_settings->time_settings.time_start == NULL || time_settings->time_settings.time_stop == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    system_frequency_settings.freq_regime = frequency_settings->frequency_settings.freq_regime;
    system_frequency_settings.acceleration = frequency_settings->frequency_settings.acceleration;
    system_frequency_settings.desacceleration = frequency_settings->frequency_settings.desacceleration;
    system_frequency_settings.input_variable = frequency_settings->frequency_settings.input_variable;
    system_seccurity_settings.vbus_min = seccurity_settings->seccurity_settings.vbus_min;
    system_seccurity_settings.ibus_max = seccurity_settings->seccurity_settings.ibus_max;
    system_time_settings.time_system->tm_hour = time_settings->time_settings.time_system->tm_hour;
    system_time_settings.time_system->tm_min = time_settings->time_settings.time_system->tm_min;
    system_time_settings.time_system->tm_sec = time_settings->time_settings.time_system->tm_sec;
    system_time_settings.time_start->tm_hour = time_settings->time_settings.time_start->tm_hour;
    system_time_settings.time_start->tm_min = time_settings->time_settings.time_start->tm_min;
    system_time_settings.time_start->tm_sec = 0;
    system_time_settings.time_stop->tm_hour = time_settings->time_settings.time_stop->tm_hour;
    system_time_settings.time_stop->tm_min = time_settings->time_settings.time_stop->tm_min;
    system_time_settings.time_stop->tm_sec = 0;
    
    setTime(  system_time_settings.time_system );
    set_frequency_table(system_frequency_settings.input_variable, system_frequency_settings.freq_regime);
    rtc_schedule_alarms(&system_time_settings);

    if ( save_variables( &system_frequency_settings, &system_time_settings, &system_seccurity_settings) != ESP_OK ) {
        ESP_LOGE(TAG, "Algo falló guardando los valores en NVS");
    }
    set_system_settings( &system_frequency_settings, &system_seccurity_settings);    
    ESP_LOGI(TAG, "Configuración guardada correctamente");
    return ESP_OK;
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

    struct tm timeinfo = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };
    struct tm time_start = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };
    struct tm time_stop = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };

    struct tm timestamp_edit = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };
    struct tm time_start_edit = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };
    struct tm time_stop_edit = {
        .tm_hour = 0,
        .tm_min = 0,
        .tm_sec = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0
    };
    
    if ( sh1106_init() != ESP_OK ) {
        ESP_LOGE(TAG, "Error al inicializar el display SH1106");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    ESP_LOGI(TAG, "Inicialización del display SH1106 exitosa");


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
    time_edit.time_settings.time_system = &timestamp_edit;
    time_edit.time_settings.time_start = &time_start_edit;
    time_edit.time_settings.time_stop = &time_stop_edit;
    time_edit.edit = &variable_lines;
    time_edit.time_settings.time_start = &time_start_edit;
    time_edit.time_settings.time_stop = &time_stop_edit;

    system_time_settings.time_start = &time_start;
    system_time_settings.time_system = &timeinfo;
    system_time_settings.time_stop = &time_stop;
    ESP_LOGI(TAG, "Variables de edición inicializadas");

    setTime( &timeinfo );
    rtc_schedule_alarms(&system_time_settings);
    ESP_LOGI(TAG, "Variables temportales inicializadas");
    set_frequency_table(system_frequency_settings.input_variable, system_frequency_settings.freq_regime);
    ESP_LOGI(TAG, "Tablas de frecuencia inicializadas");

    if (button_evt_queue == NULL) {
        button_evt_queue = xQueueCreate(1, sizeof(uint32_t));
        if (button_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            ESP_ERROR_CHECK(ESP_FAIL);
        }
    }
    ESP_LOGI(TAG, "Queue de eventos inicializadas");

    load_variables( &system_frequency_settings, &system_time_settings, &system_seccurity_settings);
    set_system_settings( &system_frequency_settings, &system_seccurity_settings);

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        getTime(&timeinfo);
        blink++;
        if ( blink >= 24 ) {
            blink = 0;
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

                        time_edit.time_settings.time_system->tm_hour = system_time_settings.time_system->tm_hour;
                        time_edit.time_settings.time_system->tm_min = system_time_settings.time_system->tm_min;
                        time_edit.time_settings.time_system->tm_sec = system_time_settings.time_system->tm_sec;

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
                                edit_variable_max = 150;
                            } else if ( variable_lines == EDIT_ACCELERATION_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.acceleration);
                                edit_variable_min = 1;
                                edit_variable_max = 150;
                            } else if ( variable_lines == EDIT_DESACCELERATION_INDX ) {
                                edit_variable = (uint16_t*) &(frequency_edit.frequency_settings.desacceleration);
                                edit_variable_min = 1;
                                edit_variable_max = 150;
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
                                edit_variable_min = 250;
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
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_system->tm_hour);
                                edit_variable_min = 0;
                                edit_variable_max = 23;
                            } else if ( variable_lines == VARIABLE_THIRD ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_system->tm_min);
                                edit_variable_min = 0;
                                edit_variable_max = 59;
                            } else if ( variable_lines == VARIABLE_FOURTH ) {
                                edit_variable = (uint16_t*) &(time_edit.time_settings.time_system->tm_sec);
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
                    } else if ( screen_displayed == SCREEN_MAIN ) {
                        SystemEventPost(START_PRESSED);
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
                    } else if ( screen_displayed == SCREEN_MAIN ) {
                        SystemEventPost(STOP_PRESSED);
                        RelayEvantPost( 0 );
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

                    system_variables_save(&frequency_edit, &time_edit, &seccurity_edit);

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
                system_status_t s_e;
                get_status(&s_e);
                if ( ( s_e.status == SYSTEM_EMERGENCY || s_e.status == SYSTEM_EMERGENCY_OK ) && blink < 12 ) {
                    sh1106_print_emergency();
                } else {
                    sh1106_main_screen();
                }
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

esp_err_t DisplayEventPost(systemSignal_e event) {
    if (button_evt_queue == NULL) {
        return ESP_FAIL;
    }
    return xQueueSend(button_evt_queue, &event, 0);
}