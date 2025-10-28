// adc_twice_esp_adc.c
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"

#include "../LVFV_system.h"

#define ADC_PERIOD_MS          50

#define ADC_UNIT_USED          ADC_UNIT_1
#define ADC_CH_GPIO34          ADC_CHANNEL_6   // GPIO34
#define ADC_CH_GPIO35          ADC_CHANNEL_7   // GPIO35
#define ADC_CH_GPIO36          ADC_CHANNEL_0   // GPIO36
#define ADC_CH_GPIO39          ADC_CHANNEL_3   // GPIO39

#define ADC_ATTEN_USED         ((adc_atten_t)3) // ~3.3–3.6 V FS aprox.

static const char *TAG = "ADC_ESPADC";

QueueHandle_t bus_meas_evt_queue = NULL;

static adc_oneshot_unit_handle_t s_adc = NULL;

static adc_cali_handle_t calibration_bus_voltage = NULL;
static adc_cali_handle_t calibration_current = NULL;
static adc_cali_handle_t calibration_5V_source = NULL;
static adc_cali_handle_t calibration_3V3_source = NULL;

static bool calibration_bus_voltage_ok = false, calibration_current_ok = false, calibration_5V_source_ok = false, calibration_3V3_source_ok = false;

seccurity_settings_t bus_meas;

static bool adc_cali_try_init(adc_unit_t unit, adc_channel_t ch, adc_atten_t atten, adc_cali_handle_t *out) {
    esp_err_t err;
    adc_cali_line_fitting_config_t cfg2 = {
        .unit_id  = unit,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_line_fitting(&cfg2, out);
    if (err == ESP_OK) return true;
    return false;
}

esp_err_t adc_init_two_channels(void) {

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_USED,
    };

    adc_oneshot_new_unit(&unit_cfg, &s_adc);

    adc_oneshot_chan_cfg_t ch_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_USED,
    };

    adc_oneshot_config_channel(s_adc, ADC_CH_GPIO34, &ch_cfg);
    adc_oneshot_config_channel(s_adc, ADC_CH_GPIO35, &ch_cfg);
    adc_oneshot_config_channel(s_adc, ADC_CH_GPIO36, &ch_cfg);
    adc_oneshot_config_channel(s_adc, ADC_CH_GPIO39, &ch_cfg);

    /* Calibración (si el chip lo soporta) */
    calibration_bus_voltage_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO34, ADC_ATTEN_USED, &calibration_bus_voltage);
    calibration_current_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO35, ADC_ATTEN_USED, &calibration_current);
    calibration_5V_source_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO36, ADC_ATTEN_USED, &calibration_5V_source);
    calibration_3V3_source_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO39, ADC_ATTEN_USED, &calibration_3V3_source);

    ESP_LOGI(TAG, "Calibración: CH34=%s, CH35=%s, CH36=%s, CH39=%s", calibration_bus_voltage_ok ? "ON" : "OFF", calibration_current_ok ? "ON" : "OFF", calibration_5V_source_ok ? "ON" : "OFF", calibration_3V3_source_ok ? "ON" : "OFF");

    return ESP_OK;
}

void adc_task(void *arg) {

    uint16_t vbus_vector[20], ibus_vector[20];
    uint16_t vector_index = 0;

    int raw_meas_bus_voltage = 0, raw_meas_current = 0, raw_meas_5V_source = 0, raw_meas_3V3_source = 0;
    int meas_bus_voltage  = 0, meas_current  = 0, meas_5V_source  = 0, meas_3V3_source  = 0;

    if (bus_meas_evt_queue == NULL) {
        bus_meas_evt_queue = xQueueCreate(1, sizeof(seccurity_settings_t));
        if (bus_meas_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de mediciones de bus");
            return;
        }
    }

    while (1) {
        if (adc_oneshot_read(s_adc, ADC_CH_GPIO34, &raw_meas_bus_voltage) == ESP_OK) {
            if (calibration_bus_voltage_ok) {
                adc_cali_raw_to_voltage(calibration_bus_voltage, raw_meas_bus_voltage, &meas_bus_voltage);
                // ESP_LOGI(TAG, "Bus voltage = %4f mV", meas_bus_voltage / 0.009014);
                // ESP_LOGI(TAG, "Bus voltage = %4d V", (int) truncf(meas_bus_voltage / 9.014) );
                uint32_t vbus_prom = 0;
                vbus_vector[vector_index] = (int) truncf(meas_bus_voltage / 9.014);
                for ( uint8_t i = 0; i < 20; i++ ) {
                    vbus_prom += vbus_vector[i];
                }
                bus_meas.vbus_min = (uint16_t) vbus_prom/20;
            } else {
                // ESP_LOGI(TAG, "Bus voltage raw = %4d", raw_meas_bus_voltage);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura tensión");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO35, &raw_meas_current) == ESP_OK) {

            if (calibration_current_ok) {
                adc_cali_raw_to_voltage(calibration_current, raw_meas_current, &meas_current);
                // ESP_LOGI(TAG, "Bus current = %4f mA", meas_current * 0.6656);
                // ESP_LOGI(TAG, "Bus current = %4d mA", (int) truncf(meas_current * 0.6656) );
                uint32_t ibus_prom = 0;
                ibus_vector[vector_index] = (int) truncf(meas_current * 0.6656);
                for ( uint8_t i = 0; i < 20; i++ ) {
                    ibus_prom += ibus_vector[i];
                }
                bus_meas.ibus_max = (uint16_t) ibus_prom/20;
            } else {
                // ESP_LOGI(TAG, "Bus current raw = %4d", raw_meas_current);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO39, &raw_meas_3V3_source) == ESP_OK) {

            if (calibration_3V3_source_ok) {
                adc_cali_raw_to_voltage(calibration_3V3_source, raw_meas_3V3_source, &meas_3V3_source);
                // ESP_LOGI(TAG, "3V3 source = %4f mV", meas_3V3_source * 4.4);
            } else {
                // ESP_LOGI(TAG, "3V3 source raw = %4d", raw_meas_3V3_source);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO36, &raw_meas_5V_source) == ESP_OK) {

            if (calibration_5V_source_ok) {
                adc_cali_raw_to_voltage(calibration_5V_source, raw_meas_5V_source, &meas_5V_source);
                // ESP_LOGI(TAG, "5V source = %4f mV", meas_5V_source * 6.66);
            } else {
                // ESP_LOGI(TAG, "5V source raw = %4d", raw_meas_5V_source);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        xQueueSend(bus_meas_evt_queue, &bus_meas, 0);
        vector_index++;
        if ( vector_index == 20 ) {
            vector_index = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(ADC_PERIOD_MS));
    }
}

