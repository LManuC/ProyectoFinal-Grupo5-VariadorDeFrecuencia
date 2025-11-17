/**
 * @file main.c
 * @author Andrenacci - Carra
 * @brief Inicialización de tareas y periféricos que no tienen una tarea específica. El resto de los periféricos se inicializan en las tareas que son utilizados.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_clk_tree.h"

#include "driver/gpio.h"

#include "./display/display.h"
#include "./io_control/io_control.h"
#include "./System/SysControl.h"
#include "./adc/adc.h"
#include "./nvs/nvs.h"
#include "./rtc/rtc.h"

#include "LVFV_system.h"

static const char *TAG = "UTN-CA-PF2025";                       /// Tag de logs del sistema

/**
 * @brief Función principal de la aplicación
 * 
 */
void app_main(void) {

    nvs_init_once();
    if ( initRTC() == ESP_OK ) {
        ESP_LOGI(TAG, "RTC inicializado correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar el RTC");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    xTaskCreatePinnedToCore( task_display,                  "Tarea Display",                    4096, NULL, 10, NULL, 1);

    xTaskCreatePinnedToCore(adc_task,                       "adc_task",                         4096, NULL, 9,  NULL, 0);
    xTaskCreatePinnedToCore(SPI_communication,              "SPI_communication",                2048, NULL, 9,  NULL, 0);
    xTaskCreatePinnedToCore(GPIO_interrupt_attendance_task, "GPIO_interrupt_attendance_task",   2048, NULL, 10, NULL, 0);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
