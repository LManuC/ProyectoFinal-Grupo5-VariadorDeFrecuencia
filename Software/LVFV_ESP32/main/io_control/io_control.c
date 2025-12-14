/**
 * @file io_control.c
 * @author Andrenacci - Carra
 * @brief Funciones de control del relay, buzzer y tarea del uso del expansor de entradas y salidas y entradas y salidas aisladas directas al ESP32
 * @details Las tareas de relay y buzzer son tareas aisladas del resto del sistema y son accedidas a través de las funciones RelayEvantPost y BuzzerEvantPost. Esto genera que no sea necesaria que la cola de comandos deba ser utilizada en diversos archivos, pretege los valores que envía antes de generar el procesamiento del dato desde las respectivas tareas.
 Cuenta además con la selección, elección y control de entradas que le corresponde a cada tecla del teclado, cada entrada digital aislada. Trabaja con el antirrubote de las entradas, evitando que se filtren pulsos de ruido que hayan sido detectadas como cambios en las entradas o evitar que se generen más de un evento cuando en realidad era uno solo.
 También corre la tarea que controla la salida de 0 a 10 volts de contínua, la señal que representa indirectamente la frecuencia de operación del motor.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "display/display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "mcp23017_defs.h"

#include "./MCP23017.h"
#include "./io_control.h"
#include "../system/sysAdmin.h"
#include "../system/sysControl.h"

static const char *TAG = "IO_CTRL";                                                                                     /*!< Varaible que se imprime en los ESP_LOG */

#define INT_A_PIN                   GPIO_NUM_22                                                                         /*!< Configuración de pin físico correspondiente al flag de interrupciones del puerto A del MCP23017 */

#define BUZZER_PIN                  0                                                                                   /*!< Pin del puerto A que controla el buzzer de la HMI */
#define RELAY_PIN                   1                                                                                   /*!< Pin del puerto A que controla el relé de la HMI */
#define FREQ_SEL_1_PIN              2                                                                                   /*!< Pin del puerto A que controla la selección de frecuencia 1 del variador de frecuencia */
#define FREQ_SEL_2_PIN              3                                                                                   /*!< Pin del puerto A que controla la selección de frecuencia 2 del variador de frecuencia */
#define FREQ_SEL_3_PIN              4                                                                                   /*!< Pin del puerto A que controla la selección de frecuencia 3 del variador de frecuencia */
#define STOP_PIN                    5                                                                                   /*!< Pin del puerto A que controla el botón de parada delvariador de frecuencia */
#define TERMO_SW_PIN                6                                                                                   /*!< Pin del puerto A que controla el interruptor térmico del variador de frecuencia */

#define FREQ_SEL_MASK               ( (1 << FREQ_SEL_3_PIN) | (1 << FREQ_SEL_2_PIN) | (1 << FREQ_SEL_1_PIN) )           /*!< Máscara que, comparada con el registro de flags de interrupción, permite identificar cambios en las entradas del variador de frecuencia */
#define TERMO_SW_MASK               (1 << TERMO_SW_PIN)                                                                 /*!< Máscara que, comparada con el registro de flags de interrupción, permite identificar cambios en el interruptor térmico del variador de frecuencia */
#define STOP_SW_MASK                (1 << STOP_PIN)                                                                     /*!< Máscara que, comparada con el registro de flags de interrupción, permite identificar cambios en el botón de parada del variador de frecuencia */

#define INT_B_PIN                   GPIO_NUM_23                                                                         /*!< Configuración de pin físico correspondiente al flag de interrupciones del puerto B del MCP23017 */

#define MATRIX_SW1                  0x17                                                                                /*!< Representación de switch impreso como SW1 en la serigrafía por 0x17  */
#define MATRIX_SW2                  0x27                                                                                /*!< Representación de switch impreso como SW2 en la serigrafía por 0x27  */
#define MATRIX_SW3                  0x1B                                                                                /*!< Representación de switch impreso como SW3 en la serigrafía por 0x1B  */
#define MATRIX_SW4                  0x2B                                                                                /*!< Representación de switch impreso como SW4 en la serigrafía por 0x2B  */
#define MATRIX_SW5                  0x1D                                                                                /*!< Representación de switch impreso como SW5 en la serigrafía por 0x1D  */
#define MATRIX_SW6                  0x2D                                                                                /*!< Representación de switch impreso como SW6 en la serigrafía por 0x2D  */
#define MATRIX_SW7                  0x1E                                                                                /*!< Representación de switch impreso como SW7 en la serigrafía por 0x1E  */
#define MATRIX_SW8                  0x2E                                                                                /*!< Representación de switch impreso como SW8 en la serigrafía por 0x2E  */

#define SWITCH_BUTTON_BACK          MATRIX_SW1                                                                          /*!< Asignación de boton al SW1 representado por 0x17 */
#define SWITCH_BUTTON_UP            MATRIX_SW2                                                                          /*!< Asignación de boton al SW2 representado por 0x27 */
#define SWITCH_BUTTON_LEFT          MATRIX_SW3                                                                          /*!< Asignación de boton al SW3 representado por 0x1B */
#define SWITCH_BUTTON_RIGHT         MATRIX_SW4                                                                          /*!< Asignación de boton al SW4 representado por 0x2B */
#define SWITCH_BUTTON_DOWN          MATRIX_SW5                                                                          /*!< Asignación de boton al SW5 representado por 0x1D */
#define SWITCH_BUTTON_SAVE          MATRIX_SW6                                                                          /*!< Asignación de boton al SW6 representado por 0x2D */
#define SWITCH_BUTTON_OK            MATRIX_SW7                                                                          /*!< Asignación de boton al SW7 representado por 0x1E */
#define SWITCH_BUTTON_MENU          MATRIX_SW8                                                                          /*!< Asignación de boton al SW8 representado por 0x2E */

#define STOP_BUTTON_PIN             GPIO_NUM_16                                                                         /*!< Configuración de pin físico correspondiente al pulsado exterior de parada del motor */
#define START_BUTTON_PIN            GPIO_NUM_17                                                                         /*!< Configuración de pin físico correspondiente al pulsado exterior de arranque del motor */

#define PWM_GPIO                    4                                                                                   /*!< GPIO4 es la utilizada para la salida de 0-10V en el PCB */
#define PWM_FREQ_HZ                 1000                                                                                /*!< Frecuencia de operación del timer para la salida 0-10V en 1KHz */
#define PWM_TIMER                   LEDC_TIMER_0                                                                        /*!< Timer utilizado para el PWM que controla la salida 0-10V */
#define PWM_MODE                    LEDC_LOW_SPEED_MODE                                                                 /*!< El requerimiento del timer para el control de los 0-10V no amerita un timer de alta velocidad */
#define PWM_CHANNEL                 LEDC_CHANNEL_0                                                                      /*!< Canal del timer utilizado para el PWM que controla la salida 0-10V */
#define PWM_RES                     LEDC_TIMER_10_BIT                                                                   /*!< Resolución de 10 bits (0–1023) para el ADC que  */
#define PWM_STEP_MS                 250                                                                                 /*!< Cambio cada 500 ms */
#define DUTY_MIN_PCT                54                                                                                  /*!< Mínimo duty que puede alcanzar el PWM para lograr los 10V de salida*/
#define DUTY_MAX_PCT                98                                                                                  /*!< Máximo duty que puede alcanzar el PWM para lograr los 0V de salida */

QueueHandle_t buzzer_evt_queue = NULL;                                                                                  /*!< Cola de eventos para el control del buzzer. Espera tiempo de activación del buzzer, tiempo en el que se lo escuchará sonar*/
QueueHandle_t relay_evt_queue = NULL;                                                                                   /*!< Cola de eventos para el control del relay. Admite solo 1 para energizarlo y 0 para desenergizarlo */
QueueHandle_t GPIO_evt_queue = NULL;                                                                                    /*!< Cola de eventos para las entradas digitales aisladas y directas sobre el ESP32 y las que llegan al MCP23017. Solo puede encolar una acción desde dentro de este archivo */

uint8_t mcp_porta;                                                                                                      /*!< Estado de entradas del puerto A del MCP23017 */
uint8_t mcp_portb;                                                                                                      /*!< Estado de entradas del puerto B del MCP23017 */

/**
 * @fn static esp_err_t freq_0_10V_output();
 *
 * @brief Inicializa el timer para el correcto funcionamiento del PWM que permite obtener la salida de 0 a 10V analógicos.
 *    
 * @return
 *      - ESP_OK: En caso de éxito.
 *      - ESP_ERR_INVALID_ARG: Error al pasar parámetros de configuración de timer o canal de PWM.
 *      - ESP_FAIL: No se pudo encontrar un pre divisor apropiado para la base de frecuencia y resolución de duty dado.
 *      - ESP_ERR_INVALID_STATE: Timer no pudo se desconfigurado poruq el timer no se configuró previamente o está pausado.
 */
static esp_err_t freq_0_10V_output();

/**    
 * @fn static esp_err_t gpio_init_interrupts(void);
 *
 * @brief Inicializa las interrupciones para INTA y INTB del MCP23017 y para los botones aislados stop y start.
 *
 * @details Las interrupciones de INTA y INTB se dispararán cada vez que los pines pasen de estado alto a bajo, ninguno de ellos tendrá activo el pull-up o pull-down ya que los pines del MCP23017 trabajarán como pines activos. Las interrupciones de start y stop serán disparadas por flanco alto o bajo y tampoco tendrán activos ni sus pull-ups ni pull-downs
 *    
 * @return
 *  - ESP_OK: en caso de éxito
 *  - ESP_FAIL: No se pudo inicializar alguna de las colas de eventos.
 *  - ESP_ERR_INVALID_STATE: El handler de interrupcionmes no fue inicializado correctamente o se intentó inicializar la interrupción más de una vez
 *  - ESP_ERR_INVALID_ARG: Error en la configuración de los parámetros o hay un error en los GPIO
 *  - ESP_ERR_NO_MEM: No hay memoria suficiente para instalar las interrupciones
 *  - ESP_ERR_NOT_FOUND: No hay interrupciones disponibles para instalar con la configuración solicitada
 */
static esp_err_t gpio_init_interrupts(void);

/**    
 * @fn static void MCP23017_buzzer_control(void* arg);
 *
 * @brief Tarea que espera comandos a través de la cola buzzer_evt_queue para hacer sonar al buzzer.
 *
 * @details Lo que espera la tarea a través de la cola, es el tiempo en milisegundos que hará sonar el buzzer.
 *
 */
static void MCP23017_buzzer_control(void* arg);

/**    
 * @fn static void MCP23017_keyboard_control(void* arg);
 *
 * @brief Tarea que controla las salidas del MCP23017 para hacer funcionar al teclado.
 *
 * @details Alternadamente activa y desactiva los pines 4 y 5 del puerto B siempre y cuando ninguna tecla esté siendo presionada.
 *
 */
static void MCP23017_keyboard_control(void* arg);

/**    
 * @fn static void MCP23017_relay_control(void* arg);
 *
 * @brief Tarea que espera comandos a través de la cola relay_evt_queue para activar o desactivar el relay.
 *
 * @details Lo que espera la tarea a través de la cola, es un uno para activar el relay; cualquier otro número recibido desenergizará la bobina del relay.
 *
 */
static void MCP23017_relay_control(void* arg);


/**    
 * @fn void IRAM_ATTR gpio_isr_handler(void* arg);
 *
 * @brief Función interrupción que envía una señal a la tarea GPIO_interrupt_attendance_task
 *
 * @details Utiliza la cola de comandos GPIO_evt_queue
 *
 * @related GPIO_interrupt_attendance_task
 *
 */
 void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(GPIO_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t freq_0_10V_output() {
    esp_err_t retval;
    // Configurar el temporizador del PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = PWM_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution  = PWM_RES,
        .freq_hz          = PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    retval = ledc_timer_config(&ledc_timer);

    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar el timer de la salida 0-10V");
        return retval;
    }

    // Configurar el canal
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PWM_GPIO,
        .speed_mode     = PWM_MODE,
        .channel        = PWM_CHANNEL,
        .timer_sel      = PWM_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };

    retval = ledc_channel_config(&ledc_channel);

    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar el canal de la salida 0-10V");
        return retval;
    }

    return ESP_OK;
}

static esp_err_t gpio_init_interrupts(void) {
    esp_err_t retval;
    gpio_config_t io_conf_stop_start;                                                                                   /*!<  */
    gpio_config_t io_conf;                                                                                              /*!<  */


    if (GPIO_evt_queue == NULL) {
        GPIO_evt_queue = xQueueCreate(1, sizeof(uint32_t));
        if (GPIO_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            return ESP_FAIL;
        }
    }

    retval = gpio_install_isr_service(0);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar la interrupción de GPIO para INTA y INTB del MCP23017");
        return retval;
    }

    // Configuración de pines
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<INT_A_PIN) | (1ULL<<INT_B_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    retval = gpio_config(&io_conf);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar los GPIO");
        return retval;
    }

    // Asocio pines a la rutina ISR
    retval = gpio_isr_handler_add(INT_A_PIN, gpio_isr_handler, (void*) (uintptr_t) INT_A_PIN);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar la interrupción de GPIO para pin INTA del MCP23017");
        return retval;
    }

    retval = gpio_isr_handler_add(INT_B_PIN, gpio_isr_handler, (void*) (uintptr_t) INT_B_PIN);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar la interrupción de GPIO para pin INTB del MCP23017");
        return retval;
    }

    ESP_LOGI(TAG, "Interrupciones GPIO configuradas en %d (Int A MCP) y %d (Int B MCP)", INT_A_PIN, INT_B_PIN);

    // Configuración de pines
    io_conf_stop_start.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_stop_start.mode = GPIO_MODE_INPUT;
    io_conf_stop_start.pin_bit_mask = (1ULL<<STOP_BUTTON_PIN) | (1ULL<<START_BUTTON_PIN);
    io_conf_stop_start.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_stop_start.pull_up_en = GPIO_PULLUP_DISABLE;

    retval = gpio_config(&io_conf_stop_start);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar los GPIO para botones aislados stop y start");
        return retval;
    }

    // Asocio pines a la rutina ISR
    retval = gpio_isr_handler_add(STOP_BUTTON_PIN,  gpio_isr_handler, (void*) (uintptr_t) STOP_BUTTON_PIN);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar la interrupción de GPIO para botones aislados stop");
        return retval;
    }

    retval = gpio_isr_handler_add(START_BUTTON_PIN, gpio_isr_handler, (void*) (uintptr_t) START_BUTTON_PIN);
    if ( retval != ESP_OK ) {
        ESP_LOGE(TAG, "Error al configurar la interrupción de GPIO para botones aislados start");
        return retval;
    }

    ESP_LOGI(TAG, "Interrupciones GPIO configuradas en %d (Stop) y %d (Start)", STOP_BUTTON_PIN, START_BUTTON_PIN);
    return ESP_OK;
}

static void MCP23017_buzzer_control(void* arg) {
    
    uint32_t delay_time;
    uint16_t buzzer_time;

    if (buzzer_evt_queue == NULL) {
        buzzer_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        if (buzzer_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de buzzer");
            return;
        }
    }
    
    mcp_write_output_pin(PORTA, 0, 0);
    for(;;) {
        if(xQueueReceive(buzzer_evt_queue, &delay_time, portMAX_DELAY)) {
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
    
    uint8_t io_num;

    if (relay_evt_queue == NULL) {
        relay_evt_queue = xQueueCreate(1, sizeof(uint8_t));
        if (relay_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de relay");
            return;
        }
    }
    
    mcp_write_output_pin(PORTA, 1, 0);
    for(;;) {
        if(xQueueReceive(relay_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "New relay state: %d", io_num);
            if ( io_num == 1 ) {
                mcp_write_output_pin(PORTA, 1, 1);
            } else {
                mcp_write_output_pin(PORTA, 1, 0);
            }
        }
    }
}

esp_err_t set_freq_output(uint16_t freq)
{
    const uint16_t F_MAX    = 150;   // Hz
    const uint16_t DUTY_MAX = 98;    // %
    const uint16_t DUTY_MIN = 54;    // %
    const uint32_t PWM_MAX  = 1023;  // ticks (10 bits)

    if (freq > F_MAX) freq = F_MAX;

    // duty_pct = 98 - round( (44*freq)/150 )
    uint16_t drop = (uint16_t)((44u * freq + 75u) / 150u);   // +75 para redondeo
    int duty_pct  = (int)DUTY_MAX - (int)drop;

    if (duty_pct < DUTY_MIN) duty_pct = DUTY_MIN;
    if (duty_pct > DUTY_MAX) duty_pct = DUTY_MAX;

    // ticks = round( duty_pct/100 * 1023 )
    uint32_t ticks = (uint32_t)((duty_pct * PWM_MAX + 50u) / 100u);

    if (ledc_get_duty(PWM_MODE, PWM_CHANNEL) != ticks) {
        ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, ticks));
        ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));
        ESP_LOGI("PWM", "Freq=%u Hz, Duty=%d%% (ticks=%lu)", freq, duty_pct, (unsigned long)ticks);
    }
    return ESP_OK;
}

void GPIO_interrupt_attendance_task(void* arg) {
    uint32_t io_num;
    uint32_t last_interrupt = 0;
    uint8_t scratch_data;
    uint8_t speed_selector;

    if (MCP23017_INIT() == ESP_OK ) {
        ESP_LOGI(TAG, "MCP23017 Tarea iniciada correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar el MCP23017");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    gpio_init_interrupts();
    freq_0_10V_output();
    set_freq_output(0);
    
    xTaskCreate(MCP23017_buzzer_control, "MCP23017_buzzer_control", 2048, NULL, 10, NULL);
    xTaskCreate(MCP23017_relay_control, "MCP23017_relay_control", 2048, NULL, 10, NULL);
    xTaskCreate(MCP23017_keyboard_control, "MCP23017_keyboard_control", 2048, NULL, 10, NULL);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    for(;;) {
        system_status_t s_e;
        get_status(&s_e);
        set_freq_output(s_e.frequency);
        if(xQueueReceive(GPIO_evt_queue, &io_num, pdMS_TO_TICKS(50))) {
            if ( io_num == INT_A_PIN ) {
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_interrupt_flag(PORTA, &scratch_data) );
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_read_port(PORTA, &mcp_porta) );

                if ( scratch_data & TERMO_SW_MASK ) {
                    if ( mcp_porta & TERMO_SW_MASK ) {
                        last_interrupt = TERMO_SW_RELEASED;
                    } else {
                        last_interrupt = TERMO_SW_PRESSED;
                    }
                } else if ( scratch_data & FREQ_SEL_MASK ) {
                    mcp_porta = ~mcp_porta;
                    speed_selector = mcp_porta & FREQ_SEL_MASK;                                                              // 00ss ss00
                    speed_selector = ( speed_selector >> ( FREQ_SEL_1_PIN ) );                                        // 0000 ssss
                    last_interrupt = SPEED_SELECTOR_0 + speed_selector;
                } else if ( scratch_data & STOP_SW_MASK ) {
                    if ( mcp_porta & STOP_SW_MASK ) {
                        last_interrupt = STOP_RELEASED;
                    } else {
                        last_interrupt = STOP_PRESSED;
                    }
                }
            } else if ( io_num == INT_B_PIN ) {
                ESP_ERROR_CHECK_WITHOUT_ABORT( mcp_read_port(PORTB, &mcp_portb) );
                switch (mcp_portb){
                    case SWITCH_BUTTON_MENU:
                        last_interrupt = BUTTON_MENU;
                        break;
                    case SWITCH_BUTTON_DOWN:
                        last_interrupt = BUTTON_DOWN;
                        break;
                    case SWITCH_BUTTON_RIGHT:
                        last_interrupt = BUTTON_RIGHT;
                        break;
                    case SWITCH_BUTTON_LEFT:
                        last_interrupt = BUTTON_LEFT;
                        break;
                    case SWITCH_BUTTON_UP:
                        last_interrupt = BUTTON_UP;
                        break;
                    case SWITCH_BUTTON_OK:
                        last_interrupt = BUTTON_OK;
                        break;
                    case SWITCH_BUTTON_SAVE:
                        last_interrupt = BUTTON_SAVE;
                        break;
                    case SWITCH_BUTTON_BACK:
                        last_interrupt = BUTTON_BACK;
                        break;
                }
            } else if ( io_num == STOP_BUTTON_PIN ) {
                if ( gpio_get_level(io_num) ) {
                    last_interrupt = EMERGENCI_STOP_PRESSED;
                } else {
                    last_interrupt = EMERGENCI_STOP_RELEASED;
                }
            } else if ( io_num == START_BUTTON_PIN ) {
                if ( !gpio_get_level(io_num) ) {
                    last_interrupt = START_PRESSED;
                } else {
                    last_interrupt = START_RELEASED;
                }
            }
            continue;
        }
        mcp_read_port(PORTA, &mcp_porta);
        mcp_read_port(PORTB, &mcp_portb);

        switch (last_interrupt) {
            case BUTTON_MENU:
                if ( mcp_portb == SWITCH_BUTTON_MENU ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_DOWN:
                if ( mcp_portb == SWITCH_BUTTON_DOWN ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_RIGHT:
                if ( mcp_portb == SWITCH_BUTTON_RIGHT ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_LEFT:
                if ( mcp_portb == SWITCH_BUTTON_LEFT ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_UP:
                if ( mcp_portb == SWITCH_BUTTON_UP ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_OK:
                if ( mcp_portb == SWITCH_BUTTON_OK ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_SAVE:
                if ( mcp_portb == SWITCH_BUTTON_SAVE ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case BUTTON_BACK:
                if ( mcp_portb == SWITCH_BUTTON_BACK ) {
                    DisplayEventPost(last_interrupt);
                    BuzzerEvantPost( 50 );
                }
                break;
            case TERMO_SW_PRESSED:
                if ( !(mcp_porta & TERMO_SW_MASK) ) {
                    SystemEventPost(TERMO_SW_PRESSED);
                }
                break;
            case TERMO_SW_RELEASED:
                if ( mcp_porta & TERMO_SW_MASK ) {
                    SystemEventPost(TERMO_SW_RELEASED);
                }
                break;
            case STOP_PRESSED:
                if ( !(mcp_porta & STOP_SW_MASK) ) {
                    SystemEventPost(STOP_PRESSED);
                }
                break;
            case STOP_RELEASED:
                if ( mcp_porta & STOP_SW_MASK ) {
                    SystemEventPost(STOP_RELEASED);
                }
                break;
            case SPEED_SELECTOR_0:
            case SPEED_SELECTOR_1:
            case SPEED_SELECTOR_2:
            case SPEED_SELECTOR_3:
            case SPEED_SELECTOR_4:
            case SPEED_SELECTOR_5:
            case SPEED_SELECTOR_6:
            case SPEED_SELECTOR_7:
            case SPEED_SELECTOR_8:
            case SPEED_SELECTOR_9:
                mcp_porta = ~mcp_porta;
                speed_selector = mcp_porta & FREQ_SEL_MASK;                                                              // 00ss ss00
                speed_selector = ( speed_selector >> ( FREQ_SEL_1_PIN ) );                                        // 0000 ssss
                if ( speed_selector + SPEED_SELECTOR_0 == last_interrupt ) {
                    SystemEventPost(last_interrupt);
                }
                break;
            case EMERGENCI_STOP_PRESSED:
                if ( gpio_get_level(STOP_BUTTON_PIN) ) {
                    SystemEventPost(EMERGENCI_STOP_PRESSED);
                }
                break;
            case EMERGENCI_STOP_RELEASED:
                if ( !gpio_get_level(STOP_BUTTON_PIN) ) {
                    SystemEventPost(EMERGENCI_STOP_RELEASED);
                }
                break;
            case START_PRESSED:
                if ( !gpio_get_level(START_BUTTON_PIN) ) {
                    SystemEventPost(START_PRESSED);
                }
                break;
            case START_RELEASED:
                if ( gpio_get_level(START_BUTTON_PIN) ) {
                    SystemEventPost(START_RELEASED);
                }
                break;
        }
        last_interrupt = 0;
    }
}

esp_err_t RelayEvantPost( uint8_t relay_event ) {
    uint8_t event = relay_event;
    if ( event == 1 || event == 0 ) {
        return xQueueSend(relay_evt_queue, &event, 0);
    }
    return ESP_FAIL;
}

esp_err_t BuzzerEvantPost( uint32_t buzzer_time_on ) {
    if ( buzzer_time_on > 3000 ) {
        buzzer_time_on = 3000;
    }
    return xQueueSend(buzzer_evt_queue, &buzzer_time_on, 0);
}