#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "SH1106/display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// #include "esp32.h"
// #include "STM32_comm.h"

#include "./MCP23017/MCP23017.h"

static const char *TAG = "IO_CTRL";

#define ESP_INTR_FLAG_DEFAULT       0

#define INT_A_PIN                   GPIO_NUM_22

#define BUZZER_PIN                  0
#define _SAVE_CTRL_PIN              1
#define FREQ_SEL_1_PIN              2
#define FREQ_SEL_2_PIN              3
#define FREQ_SEL_3_PIN              4
#define STOP_PIN                    5
#define TERMO_SW_PIN                6

// #define FREQ_SEL_MASK               ( (1 << FREQ_SEL_4_PIN) | (1 << FREQ_SEL_3_PIN) | (1 << FREQ_SEL_2_PIN) | (1 << FREQ_SEL_1_PIN) )
#define FREQ_SEL_MASK               ( (1 << FREQ_SEL_3_PIN) | (1 << FREQ_SEL_2_PIN) | (1 << FREQ_SEL_1_PIN) )
#define TERMO_SW_MASK               (1 << TERMO_SW_PIN)
#define STOP_SW_MASK                (1 << STOP_PIN)

#define INT_B_PIN                   GPIO_NUM_23

#define MATRIX_SW1                  0x17
#define MATRIX_SW2                  0x27
#define MATRIX_SW3                  0x1B
#define MATRIX_SW4                  0x2B
#define MATRIX_SW5                  0x1D
#define MATRIX_SW6                  0x2D
#define MATRIX_SW7                  0x1E
#define MATRIX_SW8                  0x2E

#define BUTTON_MENU                 MATRIX_SW1
#define BUTTON_DOWN                 MATRIX_SW2
#define BUTTON_RIGHT                MATRIX_SW3
#define BUTTON_LEFT                 MATRIX_SW4
#define BUTTON_UP                   MATRIX_SW5
#define BUTTON_OK                   MATRIX_SW6
#define BUTTON_SAVE                MATRIX_SW7
#define BUTTON_BACK                 MATRIX_SW8

#define BUTTON_MENU_SIGNAL                      1
#define BUTTON_OK_SIGNAL                        2
#define BUTTON_BACK_SIGNAL                      3
#define BUTTON_UP_SIGNAL                        4
#define BUTTON_DOWN_SIGNAL                      5
#define BUTTON_LEFT_SIGNAL                      6
#define BUTTON_RIGHT_SIGNAL                     7
#define BUTTON_SAVE_SIGNAL                      8
#define STOP_EMERGENCI_PRESSED_SIGNAL           9
#define STOP_EMERGENCI_RELEASED_SIGNAL          10
#define START_PRESSED_SIGNAL                    11
#define START_RELEASED_SIGNAL                   12
#define TERMO_SW_PRESSED_SIGNAL                 13
#define TERMO_SW_RELEASED_SIGNAL                14
#define STOP_PRESSED_SIGNAL                     15
#define STOP_RELEASED_SIGNAL                    16
#define SPEED_SELECTOR_SIGNAL(x)                17 + (x)

#define STOP_BUTTON_PIN             GPIO_NUM_16
#define START_BUTTON_PIN            GPIO_NUM_17

QueueHandle_t MCP23017_buzzer_event = NULL;
QueueHandle_t MCP23017_relay_event = NULL;

QueueHandle_t MCP23017_evt_queue = NULL;
QueueHandle_t button_evt_queue = NULL;
QueueHandle_t isolated_inputs_queue = NULL;

uint8_t mcp_porta;
uint8_t mcp_portb;
uint8_t signal_to_screen;
uint32_t relay_state;

static gpio_config_t io_conf_stop_start;
static gpio_config_t io_conf;

static void MCP23017_buzzer_control(void* arg);
static void MCP23017_relay_control(void* arg);
static void MCP23017_keyboard_control(void* arg);
static void gpio_init_interrupts(void);

#define PWM_GPIO        4
#define PWM_FREQ_HZ     1000
#define PWM_TIMER       LEDC_TIMER_0
#define PWM_MODE        LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL     LEDC_CHANNEL_0
#define PWM_RES         LEDC_TIMER_10_BIT   // Resolución de 10 bits (0–1023)
#define PWM_STEP_MS     250                 // Cambio cada 500 ms
#define DUTY_MIN_PCT    54 // -> 10V
#define DUTY_MAX_PCT    98 // -> 0V

static void freq_0_10V_output() {
    // Configurar el temporizador del PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = PWM_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configurar el canal
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PWM_GPIO,
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void set_freq_output(uint16_t freq) {
    if (freq > 450) {
        freq = 450;
    }

    // Calcular duty linealmente (en %)
    float duty_percent = -0.0977f * freq + 98.0f;

    // Convertir duty en ticks de PWM (0..1023)
    uint32_t duty = (uint32_t)((duty_percent / 100.0f) * 1023.0f);
    if ( ledc_get_duty(PWM_MODE, PWM_CHANNEL) != duty ) {
        ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
        ledc_update_duty(PWM_MODE, PWM_CHANNEL);
        ESP_LOGI("PWM", "Freq = %dHz, Duty = %f%%", freq, duty_percent);
    }
}

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(MCP23017_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void gpio_init_interrupts(void) {

    if (MCP23017_evt_queue == NULL) {
        MCP23017_evt_queue = xQueueCreate(1, sizeof(uint32_t));
        if (MCP23017_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            return;
        }
    }

    if (button_evt_queue == NULL) {
        button_evt_queue = xQueueCreate(1, sizeof(uint32_t));
        if (button_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            return;
        }
    }

    if (isolated_inputs_queue == NULL) {
        isolated_inputs_queue = xQueueCreate(1, sizeof(uint32_t));
        if (isolated_inputs_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            return;
        }
    }

    // Configuración de pines
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<INT_A_PIN) | (1ULL<<INT_B_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK( gpio_config(&io_conf) );

    // Instalo servicio de ISR (si no está instalado)
    // Nota: llamar a gpio_install_isr_service sólo una vez en el sistema.
    static bool isr_installed = false;
    if (!isr_installed) {
        ESP_ERROR_CHECK( gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT) );
        isr_installed = true;
    }

    // Asocio pines a la rutina ISR
    ESP_ERROR_CHECK( gpio_isr_handler_add(INT_A_PIN, gpio_isr_handler, (void*) (uintptr_t) INT_A_PIN) );
    ESP_ERROR_CHECK( gpio_isr_handler_add(INT_B_PIN, gpio_isr_handler, (void*) (uintptr_t) INT_B_PIN) );
    ESP_LOGI(TAG, "Interrupciones GPIO configuradas en %d (Int A MCP) y %d (Int B MCP)", INT_A_PIN, INT_B_PIN);

    // Configuración de pines
    io_conf_stop_start.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_stop_start.mode = GPIO_MODE_INPUT;
    io_conf_stop_start.pin_bit_mask = (1ULL<<STOP_BUTTON_PIN) | (1ULL<<START_BUTTON_PIN);
    io_conf_stop_start.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_stop_start.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK( gpio_config(&io_conf_stop_start) );

    // Asocio pines a la rutina ISR
    ESP_ERROR_CHECK( gpio_isr_handler_add(STOP_BUTTON_PIN,  gpio_isr_handler, (void*) (uintptr_t) STOP_BUTTON_PIN) );
    ESP_ERROR_CHECK( gpio_isr_handler_add(START_BUTTON_PIN, gpio_isr_handler, (void*) (uintptr_t) START_BUTTON_PIN) );

    ESP_LOGI(TAG, "Interrupciones GPIO configuradas en %d (Stop) y %d (Start)", STOP_BUTTON_PIN, START_BUTTON_PIN);
}

void MCP23017_interrupt_attendance(void* arg) {
    uint32_t io_num;
    uint32_t last_interrupt = 0;
    uint8_t scratch_data;
    uint16_t buzzer_time = 500;
    uint8_t speed_selector;

    gpio_init_interrupts();
    freq_0_10V_output();
    set_freq_output(0);
    
    xTaskCreate(MCP23017_buzzer_control, "MCP23017_buzzer_control", 2048, NULL, 10, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(MCP23017_relay_control, "MCP23017_relay_control", 2048, NULL, 10, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(MCP23017_keyboard_control, "MCP23017_keyboard_control", 2048, NULL, 10, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    buzzer_time = 50;
    for(;;) {
        set_freq_output(get_frequency());
        if(xQueueReceive(MCP23017_evt_queue, &io_num, pdMS_TO_TICKS(200))) {
            if ( io_num == INT_A_PIN ) {
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_interrupt_flag(PORTA, &scratch_data) );
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_get_on_interrupt_input(PORTA, &mcp_porta) );

                if ( scratch_data & TERMO_SW_MASK ) {
                    if ( mcp_porta & TERMO_SW_MASK ) {
                        last_interrupt = TERMO_SW_RELEASED_SIGNAL;
                    } else {
                        last_interrupt = TERMO_SW_PRESSED_SIGNAL;
                    }
                } else if ( scratch_data & FREQ_SEL_MASK ) {
                    mcp_porta = ~mcp_porta;
                    speed_selector = mcp_porta & FREQ_SEL_MASK;                                                              // 00ss ss00
                    speed_selector = ( speed_selector >> ( FREQ_SEL_1_PIN ) );                                        // 0000 ssss
                    last_interrupt = SPEED_SELECTOR_SIGNAL(speed_selector);
                } else if ( scratch_data & STOP_SW_MASK ) {
                    if ( mcp_porta & STOP_SW_MASK ) {
                        last_interrupt = STOP_RELEASED_SIGNAL;
                    } else {
                        last_interrupt = STOP_PRESSED_SIGNAL;
                    }
                }
            } else if ( io_num == INT_B_PIN ) {
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_read_port(PORTB, &mcp_portb) );
                switch (mcp_portb){
                    case BUTTON_MENU:
                        last_interrupt = BUTTON_MENU_SIGNAL;
                        break;
                    case BUTTON_DOWN:
                        last_interrupt = BUTTON_DOWN_SIGNAL;
                        break;
                    case BUTTON_RIGHT:
                        last_interrupt = BUTTON_RIGHT_SIGNAL;
                        break;
                    case BUTTON_LEFT:
                        last_interrupt = BUTTON_LEFT_SIGNAL;
                        break;
                    case BUTTON_UP:
                        last_interrupt = BUTTON_UP_SIGNAL;
                        break;
                    case BUTTON_OK:
                        last_interrupt = BUTTON_OK_SIGNAL;
                        break;
                    case BUTTON_SAVE:
                        last_interrupt = BUTTON_SAVE_SIGNAL;
                        break;
                    case BUTTON_BACK:
                        last_interrupt = BUTTON_BACK_SIGNAL;
                        break;
                }
            } else if ( io_num == STOP_BUTTON_PIN ) {
                if ( !gpio_get_level(io_num) ) {
                    last_interrupt = STOP_EMERGENCI_PRESSED_SIGNAL;
                } else {
                    last_interrupt = STOP_EMERGENCI_RELEASED_SIGNAL;
                }
            } else if ( io_num == START_BUTTON_PIN ) {
                if ( !gpio_get_level(io_num) ) {
                    ESP_LOGI(TAG, "Start button pressed");
                    last_interrupt = START_PRESSED_SIGNAL;
                } else {
                    ESP_LOGI(TAG, "Start button released");
                    last_interrupt = START_RELEASED_SIGNAL;
                }
            }
            continue;
        }
        switch (last_interrupt) {
            case BUTTON_MENU_SIGNAL:
                signal_to_screen = BUTTON_MENU_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_DOWN_SIGNAL:
                signal_to_screen = BUTTON_DOWN_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_RIGHT_SIGNAL:
                signal_to_screen = BUTTON_RIGHT_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_LEFT_SIGNAL:
                signal_to_screen = BUTTON_LEFT_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_UP_SIGNAL:
                signal_to_screen = BUTTON_UP_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_OK_SIGNAL:
                signal_to_screen = BUTTON_OK_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_SAVE_SIGNAL:
                signal_to_screen = BUTTON_SAVE_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case BUTTON_BACK_SIGNAL:
                signal_to_screen = BUTTON_BACK_SIGNAL;
                xQueueSend(button_evt_queue, &signal_to_screen, 0);
                xQueueSend(MCP23017_buzzer_event, &buzzer_time, 0);
                break;
            case STOP_EMERGENCI_PRESSED_SIGNAL:
                signal_to_screen = STOP_EMERGENCI_PRESSED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                break;
            case STOP_EMERGENCI_RELEASED_SIGNAL:
                signal_to_screen = STOP_EMERGENCI_RELEASED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                break;
            case START_PRESSED_SIGNAL:
                signal_to_screen = START_PRESSED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                break;
            case START_RELEASED_SIGNAL:
                signal_to_screen = START_RELEASED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                break;
            case TERMO_SW_PRESSED_SIGNAL:
                signal_to_screen = TERMO_SW_PRESSED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                relay_state = 1;
                xQueueSend(MCP23017_relay_event, &relay_state, 0);
                break;
            case TERMO_SW_RELEASED_SIGNAL:
                signal_to_screen = TERMO_SW_RELEASED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                relay_state = 0;
                xQueueSend(MCP23017_relay_event, &relay_state, 0);
                break;
            case STOP_PRESSED_SIGNAL:
                signal_to_screen = STOP_PRESSED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                relay_state = 1;
                xQueueSend(MCP23017_relay_event, &relay_state, 0);
                break;
            case STOP_RELEASED_SIGNAL:
                signal_to_screen = STOP_RELEASED_SIGNAL;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                relay_state = 0;
                xQueueSend(MCP23017_relay_event, &relay_state, 0);
                break;
            case SPEED_SELECTOR_SIGNAL(0):
            case SPEED_SELECTOR_SIGNAL(1):
            case SPEED_SELECTOR_SIGNAL(2):
            case SPEED_SELECTOR_SIGNAL(3):
            case SPEED_SELECTOR_SIGNAL(4):
            case SPEED_SELECTOR_SIGNAL(5):
            case SPEED_SELECTOR_SIGNAL(6):
            case SPEED_SELECTOR_SIGNAL(7):
            case SPEED_SELECTOR_SIGNAL(8):
            case SPEED_SELECTOR_SIGNAL(9):
                signal_to_screen = last_interrupt;
                xQueueSend(isolated_inputs_queue, &signal_to_screen, 0);
                break;
        }
        last_interrupt = 0;
    }
}

static void MCP23017_buzzer_control(void* arg) {
    
    uint32_t delay_time;
    uint16_t buzzer_time;

    if (MCP23017_buzzer_event == NULL) {
        MCP23017_buzzer_event = xQueueCreate(10, sizeof(uint32_t));
        if (MCP23017_buzzer_event == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de buzzer");
            return;
        }
    }
    
    mcp_write_output_pin(PORTA, 0, 0);
    for(;;) {
        if(xQueueReceive(MCP23017_buzzer_event, &delay_time, portMAX_DELAY)) {
            buzzer_time = (uint16_t) delay_time;
            mcp_write_output_pin(PORTA, 0, 1);
            vTaskDelay(pdMS_TO_TICKS(buzzer_time));
            mcp_write_output_pin(PORTA, 0, 0);
        }
    }
}

static void MCP23017_keyboard_control(void* arg) {
    ESP_LOGI(TAG, "Iniciando tarea de control de teclado");
    
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if ( mcp_portb & 0x10 ) {
            mcp_portb = mcp_portb & (~0x10);
            mcp_portb = mcp_portb | (0x20);
        } else {
            mcp_portb = mcp_portb & (~0x20);
            mcp_portb = mcp_portb | (0x10);
        }
        if ( ( mcp_portb & 0x0F ) != 0x0F ) {
            // ESP_LOGI(TAG, "Puerto B: %X. Botón pulsado...", mcp_portb);
            mcp_read_port(PORTB, &mcp_portb);
            continue;
        }
        mcp_write_output_port(PORTB, mcp_portb);
    }
}

static void MCP23017_relay_control(void* arg) {
    
    uint32_t io_num;

    if (MCP23017_relay_event == NULL) {
        MCP23017_relay_event = xQueueCreate(10, sizeof(uint32_t));
        if (MCP23017_relay_event == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de relay");
            return;
        }
    }
    
    mcp_write_output_pin(PORTA, 1, 0);
    for(;;) {
        if(xQueueReceive(MCP23017_relay_event, &io_num, portMAX_DELAY)) {
            if ( io_num == 1 ) {
                mcp_write_output_pin(PORTA, 1, 1);
            } else {
                mcp_write_output_pin(PORTA, 1, 0);
            }
        }
    }
}

