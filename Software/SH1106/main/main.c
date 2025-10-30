/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "./SH1106/display.h"
#include "./MCP23017/MCP23017.h"
#include "./MCP23017/io_control.h"
#include "./SPI/SPI_module.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "LVFV_system.h"
#include "esp_sleep.h"
#include "esp_clk_tree.h"

frequency_settings_t system_frequency_settings;
time_settings_t system_time_settings;
seccurity_settings_t system_seccurity_settings;

struct tm timeinfo;
struct tm time_start;
struct tm time_stop;

uint8_t blink;

#define NUM_BOTONES (sizeof(botones)/sizeof(botones[0]))

static const char *TAG = "UTN-CA-PF2025";


void app_main(void) {

    if ( adc_init_two_channels() == ESP_OK) {
        xTaskCreatePinnedToCore(adc_task, "adc_task", 4096, NULL, 9, NULL, tskNO_AFFINITY);
    } else {
        ESP_LOGE(TAG, "adc_init_two_channels falló; no se inicializaron correctamente los ADC");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if ( SPI_Init() == ESP_OK) {
        xTaskCreatePinnedToCore(SPI_communication, "SPI_communication", 2048, NULL, 9, NULL, tskNO_AFFINITY);
    } else {
        ESP_LOGE(TAG, "SPI_Init falló; no creo SPI_communication");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if ( sh1106_init() == ESP_OK ) {
        xTaskCreatePinnedToCore( task_display, "Tarea Display", 4096, NULL, 10, NULL, 1);
        ESP_LOGI(TAG, "Display SH1106 inicializado correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar el display SH1106");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if ( initRTC() == ESP_OK ) {
        ESP_LOGI(TAG, "RTC inicializado correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar el RTC");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if (MCP23017_INIT() == ESP_OK ) {
        xTaskCreate(MCP23017_interrupt_attendance, "MCP23017_interrupt_attendance", 2048, NULL, 10, NULL);
        ESP_LOGI(TAG, "MCP23017 Tarea iniciada correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar el MCP23017");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    xTaskCreatePinnedToCore(adc_task, "adc_task", 2048, NULL, 9, NULL, tskNO_AFFINITY);

    system_time_settings.time_start = &time_start;
    system_time_settings.timeinfo = &timeinfo;
    system_time_settings.time_stop = &time_stop;

    while (1) {

        getTime(&timeinfo);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
