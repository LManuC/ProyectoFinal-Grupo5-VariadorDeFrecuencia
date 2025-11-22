/**
 * @file adc.c
 * @author Andrenacci - Carra
 * @brief Funcion de inicialización y tarea lectura del ADC 
 * @version 2.0
 * @date 2025-11-06
 * 
 */
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
#include "../adc/adc.h"
#include "../System/SysAdmin.h"

#define ADC_PERIOD_MS          100

#define ADC_UNIT_USED          ADC_UNIT_1
#define ADC_CH_GPIO34          ADC_CHANNEL_6   // GPIO34
#define ADC_CH_GPIO35          ADC_CHANNEL_7   // GPIO35
#define ADC_CH_GPIO36          ADC_CHANNEL_0   // GPIO36
#define ADC_CH_GPIO39          ADC_CHANNEL_3   // GPIO39

#define ADC_ATTEN_USED         ( (adc_atten_t) 3 ) // ~3.3–3.6 V FS aprox.

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t s_adc = NULL;

static adc_cali_handle_t calibration_bus_voltage = NULL;
static adc_cali_handle_t calibration_current = NULL;
static adc_cali_handle_t calibration_5V_source = NULL;
static adc_cali_handle_t calibration_3V3_source = NULL;

static bool calibration_bus_voltage_ok = false;
static bool calibration_current_ok = false;
static bool calibration_5V_source_ok = false;
static bool calibration_3V3_source_ok = false;

QueueHandle_t bus_meas_evt_queue = NULL;

/**
 * @fn static esp_err_t adc_cali_try_init(adc_unit_t unit, adc_channel_t ch, adc_atten_t atten, adc_cali_handle_t *out);
 *
 * @brief Estandariza las mediciones del ADC en función de la tensión que representa el pin físico en mili volts.
 *
 * @details Denera la configuración necesaria para que las mediciones pasen de cuentas a mili volts desde 0 a 3125 mili volts
 *
 * @param[in] uint
 *      Unidad del ADC utilizada. Puede tomar valores ADC_UNIT_1 o ADC_UNIT_2
 *
 * @param[in] ch
 *      Canal del ADC utilizado. Puede tomar valores ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_8 o ADC_CHANNEL_9.
 *
 * @param[in] atten
 *      Atenuación configurada en el ADC. Puede tomar valores ADC_ATTEN_DB_0, ADC_ATTEN_DB_2_5, ADC_ATTEN_DB_6 o ADC_ATTEN_DB_12
 *
 * @param[in] out
 *      Puntero al handler del ADC utilizado.
 *
 * @retval 
 *      - ESP_OK: Si la calibración del ADC fue satisfactoria
 *      - ESP_ERR_INVALID_ARG: Si los argumentos de calibración fueron incorrectos
 *      - ESP_ERR_NO_MEM: No hay suficiente memoria
 *      - ESP_ERR_NOT_SUPPORTED: Los eFose no están correctamente inicializados
 *      - ESP_ERR_INVALID_ARG: Si out es un puntero nulo
 */
static esp_err_t adc_cali_try_init(adc_unit_t unit, adc_channel_t ch, adc_atten_t atten, adc_cali_handle_t *out) {
    esp_err_t err;

    if ( out == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_cali_line_fitting_config_t cfg2 = {
        .unit_id  = unit,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_line_fitting(&cfg2, out);
    
    return err;
}

esp_err_t adc_init(void) {

    esp_err_t retval;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_USED,
    };

    adc_oneshot_new_unit(&unit_cfg, &s_adc);

    adc_oneshot_chan_cfg_t ch_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_USED,
    };

    retval = adc_oneshot_config_channel(s_adc, ADC_CH_GPIO34, &ch_cfg);
    if ( retval != ESP_OK ) {
        return retval;
    }

    retval = adc_oneshot_config_channel(s_adc, ADC_CH_GPIO35, &ch_cfg);
    if ( retval != ESP_OK ) {
        return retval;
    }

    retval = adc_oneshot_config_channel(s_adc, ADC_CH_GPIO36, &ch_cfg);
    if ( retval != ESP_OK ) {
        return retval;
    }

    retval = adc_oneshot_config_channel(s_adc, ADC_CH_GPIO39, &ch_cfg);
    if ( retval != ESP_OK ) {
        return retval;
    }

    /* Calibración (si el chip lo soporta) */
    calibration_5V_source_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO36, ADC_ATTEN_USED, &calibration_5V_source);
    calibration_3V3_source_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO39, ADC_ATTEN_USED, &calibration_3V3_source);
    calibration_bus_voltage_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO34, ADC_ATTEN_USED, &calibration_bus_voltage);
    calibration_current_ok = adc_cali_try_init(ADC_UNIT_USED, ADC_CH_GPIO35, ADC_ATTEN_USED, &calibration_current);

    ESP_LOGI(TAG, "Calibración: 5v Source = %s", calibration_5V_source_ok == ESP_OK ? "ON" : "OFF");
    ESP_LOGI(TAG, "Calibración: 3.3v Source = %s", calibration_3V3_source_ok == ESP_OK ? "ON" : "OFF");
    ESP_LOGI(TAG, "Calibración: DC bus voltage = %s", calibration_bus_voltage_ok == ESP_OK ? "ON" : "OFF");
    ESP_LOGI(TAG, "Calibración: DC bus current = %s", calibration_current_ok == ESP_OK ? "ON" : "OFF");
    
    return ESP_OK;
}

void adc_task(void *arg) {

    uint16_t vbus_vector[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, ibus_vector[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uint16_t vector_index = 0;

    int raw_meas_bus_voltage = 0, raw_meas_current = 0, raw_meas_5V_source = 0, raw_meas_3V3_source = 0;
    int meas_bus_voltage  = 0, meas_current  = 0, meas_5V_source  = 0, meas_3V3_source  = 0;

    uint32_t vbus_sum = 0, ibus_sum = 0;

    bool vector_filled = false;

    seccurity_settings_t bus_meas;

    if ( adc_init() != ESP_OK) {
        ESP_LOGE(TAG, "adc_init falló; no se inicializaron correctamente los ADC");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if (bus_meas_evt_queue == NULL) {
        bus_meas_evt_queue = xQueueCreate(1, sizeof(seccurity_settings_t));
        if (bus_meas_evt_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de mediciones de bus");
            return;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        if (adc_oneshot_read(s_adc, ADC_CH_GPIO34, &raw_meas_bus_voltage) == ESP_OK) {
            if ( calibration_bus_voltage_ok == ESP_OK ) {
                adc_cali_raw_to_voltage(calibration_bus_voltage, raw_meas_bus_voltage, &meas_bus_voltage);

                vbus_sum -= vbus_vector[vector_index];
                vbus_vector[vector_index] = (int) truncf(meas_bus_voltage / 9.014);
                vbus_sum += vbus_vector[vector_index];

                if ( vector_filled ) {
                    bus_meas.vbus_min = (uint16_t) (vbus_sum / 20);
                }
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura tensión");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO35, &raw_meas_current) == ESP_OK) {

            if ( calibration_current_ok == ESP_OK ) {
                adc_cali_raw_to_voltage(calibration_current, raw_meas_current, &meas_current);

                ibus_sum -= ibus_vector[vector_index];
                ibus_vector[vector_index] = abs(meas_current - 2500);
                ibus_sum += ibus_vector[vector_index];

                if ( vector_filled ) {
                    bus_meas.ibus_max = (uint16_t) (ibus_sum / 20) * 5;
                }
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO39, &raw_meas_3V3_source) == ESP_OK) {
            if ( calibration_3V3_source_ok == ESP_OK ) {
                adc_cali_raw_to_voltage(calibration_3V3_source, raw_meas_3V3_source, &meas_3V3_source);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        if (adc_oneshot_read(s_adc, ADC_CH_GPIO36, &raw_meas_5V_source) == ESP_OK) {
            if ( calibration_5V_source_ok == ESP_OK ) {
                adc_cali_raw_to_voltage(calibration_5V_source, raw_meas_5V_source, &meas_5V_source);
            }
        } else {
            ESP_LOGE(TAG, "Error de lectura corriente");
        }

        vector_index++;
        if (vector_index >= 20) {
            vector_index = 0;
            vector_filled = true;
        }

        if ( vector_filled ) {
            xQueueSend(bus_meas_evt_queue, &bus_meas, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_PERIOD_MS));
    }
}

system_status_e readADC(void) {
    seccurity_settings_t bus_meas;
    system_status_t s_e;
    
    get_status(&s_e);

    if ( xQueueReceive( bus_meas_evt_queue, &bus_meas, pdMS_TO_TICKS(0) ) ) {
        s_e.status = update_meas(bus_meas.vbus_min, bus_meas.ibus_max);
    }

    if ( s_e.status == SYSTEM_EMERGENCY ) {
        s_e.status = SYSTEM_EMERGENCY_SENT;
    }

    return s_e.status;
}