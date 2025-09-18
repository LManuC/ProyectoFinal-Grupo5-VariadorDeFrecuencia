#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp32.h"
#include "STM32_comm.h"

#include "./MCP23017/MCP23017.h"

static const char *TAG = "IO_CTRL";

#define INT_A_PIN           GPIO_INPUT_IO_15

#define BUZZER_PIN          0
#define RELAY_CTRL_PIN      1
#define FREQ_SEL_1_PIN      2
#define FREQ_SEL_2_PIN      3
#define FREQ_SEL_3_PIN      4
#define FREQ_SEL_4_PIN      5
#define TERMO_SW_PIN        6

#define FREQ_SEL_MASK       (1 << FREQ_SEL_4_PIN) || (1 << FREQ_SEL_3_PIN) || (1 << FREQ_SEL_2_PIN) || (1 << FREQ_SEL_1_PIN)
#define TERMO_SW_MASK       (1 << TERMO_SW_PIN)

#define INT_B_PIN           GPIO_INPUT_IO_2

#define MATRIX_0_PIN        0
#define MATRIX_1_PIN        1
#define MATRIX_2_PIN        2
#define MATRIX_3_PIN        3
#define MATRIX_4_PIN        4
#define MATRIX_5_PIN        5

QueueHandle_t MCP23017_evt_queue = NULL;
uint8_t mcp_porta;
uint8_t mcp_portb;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(MCP23017_evt_queue, &gpio_num, NULL);
}

void gpio_init_interrupts(void) {
    // Selecciono máscara de pines
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,    // interrupción en flanco ascendente
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_15),
        .pull_down_en = 0,
        .pull_up_en = 1
    };

    gpio_config(&io_conf);

    // Instalo servicio de ISR
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // Asocio pines a la rutina ISR
    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*) GPIO_INPUT_IO_2);
    gpio_isr_handler_add(GPIO_INPUT_IO_15, gpio_isr_handler, (void*) GPIO_INPUT_IO_15);

    ESP_LOGI(TAG, "Interrupciones GPIO configuradas en IO2 y IO15");
}

void MCP23017_interrupt_attendance(void* arg) {
    
    MCP23017_INIT();
    gpio_init_interrupts();
    
    uint32_t io_num;
    uint8_t scratch_data;
    
    for(;;) {
        if(xQueueReceive(MCP23017_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Interrupción en GPIO[%ld], estado: %d", io_num, gpio_get_level(io_num));

            if ( io_num == INT_A_PIN ) {
                mcp_interrupt_flag(PORTA, &scratch_data);
                if ( scratch_data & TERMO_SW_MASK ) {
                    mcp_get_on_interrupt_input(PORTA, &mcp_porta);
                    comando_t cmd = CMD_PARADA_ROTAURA; 
                    xQueueSend(cmd_queue, &cmd, portMAX_DELAY);
                    /*
                     * Encola reporte de parada de emergencia al STM32
                    */
                } else if ( scratch_data & FREQ_SEL_MASK ) {
                    mcp_get_on_interrupt_input(PORTA, &mcp_porta);
                    scratch_data &= FREQ_SEL_MASK;                                                          // 00ss ss00
                    mcp_porta;
                    scratch_data = ( scratch_data >> FREQ_SEL_1_PIN );                                      // 0000 ssss
                    comando_t cmd = CMD_CAMBIO_VELOCIDAD; 
                    xQueueSend(cmd_queue, &cmd, portMAX_DELAY);
                    /*
                     * Encola reporte de cambio de velocidad al STM32
                    */
                }
            } else if ( io_num == INT_B_PIN ) {
                mcp_interrupt_flag(PORTB, &scratch_data);

            }
        }
    }
}

