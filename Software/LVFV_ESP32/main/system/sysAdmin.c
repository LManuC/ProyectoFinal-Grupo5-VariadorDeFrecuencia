/**
 * @file SysAdmin.c
 * @author Andrenacci - Carra
 * @brief Archivo de funciones que permiten controlar las variables fundamentales del sistema que controla el motor, solo para poder ser representadas en el display. Las variables reales de sistema se encuentran el el STM32
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"

#include "SysAdmin.h"
#include "SysControl.h"
#include "../display/display.h"

static const char *TAG = "sysAdmin";                            /** @var TAG @brief Etiqueta para imprimir con ESP_LOG */
static system_status_t system_status;                           /** @var system_status @brief Status general del sistema. */
static seccurity_settings_t system_seccurity_settings;          /** @var system_seccurity_settings @brief Estructura con las variables de seguridad del sistema */
static uint16_t frequency_table[8];                             /** @var frequency_table @brief Tabla de valores de frecuencias para cambios de frecuencia con las entradas aisladas */
static TaskHandle_t accelerating_handle = NULL;                 /** @var accelerating_handle @brief Handeler de la tarea de aceleración. Permite que una tarea termine a la otra o que sea cerrada desde otra función */
static TaskHandle_t desaccelerating_handle = NULL;              /** @var desaccelerating_handle @brief Handeler de la tarea de desaceleración. Permite que una tarea termine a la otra o que sea cerrada desde otra función */

/** 
 * @fn static void accelerating(void *pvParameters );
 *
 * @brief Tareas dedicada a incrementar la frecuencia que es impresa en el display de acuerdo a la aceleración y hasta la frecuencia de regimen
 *
 * @details Es una tarea que debe ser creada cada vez que se necesita incrementar la frecuencia impresa ya que, al terminar de llegar a régimen, termina su propia ejecución. Al terminar su ejecución pasa de SYSTEM_ACCLE_DESACCEL a SYSTEM_REGIME
 *
 * @param[in] pvParameters
 *      Sin uso
 */
static void accelerating(void *pvParameters );

/** 
 * @fn static void desaccelerating( void *pvParameters );
 *
 * @brief Tareas dedicada a decrementar la frecuencia que es impresa en el display de acuerdo a la desaceleración y hasta la frecuencia de regimen o 0Hz
 *
 * @details Es una tarea que debe ser creada cada vez que se necesita decrementar la frecuencia impresa ya que, al terminar de llegar a régimen, termina su propia ejecución. Al terminar su ejecución pasa de SYSTEM_BREAKING a SYSTEM_REGIME o de SYSTEM_ACCLE_DESACCEL a SYSTEM_REGIME. En caso de estar en SYSTEM_EMERGENCY no cambia el status
 *
 * @param[in] pvParameters
 *      Sin uso
 */
static void desaccelerating( void *pvParameters );

static void accelerating(void *pvParameters ) {
    // uint16_t acceleration = *((uint16_t*)pvParameters);
    vTaskDelay(pdMS_TO_TICKS(200));
    while( system_status.frequency != system_status.frequency_destiny ) {
        if ( (system_status.frequency + system_status.acceleration ) > system_status.frequency_destiny ) {
            system_status.frequency = system_status.frequency_destiny;
        } else {
            system_status.frequency += system_status.acceleration;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    system_status.status = SYSTEM_REGIME;
    accelerating_handle = NULL;
    vTaskDelete(NULL);
}

static void desaccelerating( void *pvParameters ) {
    // uint16_t desacceleration = *((uint16_t*)pvParameters);
    vTaskDelay(pdMS_TO_TICKS(200));
    while( system_status.frequency != system_status.frequency_destiny ) {
        if ( system_status.frequency - system_status.desacceleration < system_status.frequency_destiny ) {
            system_status.frequency = system_status.frequency_destiny;
        } else if ( system_status.frequency > system_status.desacceleration ) {
            system_status.frequency -= system_status.desacceleration;
        } else {
            system_status.frequency = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if ( system_status.status != SYSTEM_EMERGENCY ) {
        if( system_status.status != SYSTEM_BREAKING ) {
            system_status.status = SYSTEM_REGIME;
        } else {
            system_status.status = SYSTEM_IDLE;
        }
    }
    desaccelerating_handle = NULL;
    vTaskDelete(NULL);
}

uint16_t engine_start() {
    if ( system_status.status != SYSTEM_EMERGENCY ) {
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        system_status.status = SYSTEM_ACCLE_DESACCEL;
        if ( desaccelerating_handle != NULL ) {
            vTaskDelete( desaccelerating_handle );
            desaccelerating_handle = NULL;
        }
        xTaskCreatePinnedToCore(accelerating, "accelerating", 1024, (void*) &system_status.acceleration, 9, &accelerating_handle, 1);
    }
    return system_status.frequency_destiny;
}

uint16_t engine_stop() {
    if ( system_status.status == SYSTEM_EMERGENCY ) {
    } else if ( system_status.status == SYSTEM_EMERGENCY_OK ) {
        system_status.status = SYSTEM_IDLE;
    } else {
        system_status.acceleration = get_system_acceleration();
        system_status.desacceleration = get_system_desacceleration();
        system_status.frequency_destiny = 0;
        if ( accelerating_handle != NULL ) {
            vTaskDelete( accelerating_handle );
            accelerating_handle = NULL;
        }
        system_status.status = SYSTEM_BREAKING;
        xTaskCreatePinnedToCore(desaccelerating, "desaccelerating", 1024, (void*) &system_status.desacceleration, 9, &desaccelerating_handle, 1);
    }
    return system_status.frequency_destiny;
}

bool engine_emergency_stop_release(uint8_t signal) {
    bool retval = false;
    system_status.emergency_signals &= ~signal;
    if ( system_status.emergency_signals == 0 ) {
        system_status.status = SYSTEM_EMERGENCY_OK;
        retval = true;
    } else {
        ESP_LOGI(TAG, "Estado de las señales de emergencia: %d", system_status.emergency_signals );
    }
    return retval;
}

void engine_emergency_stop(uint8_t signal) {
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
    system_status.status = SYSTEM_EMERGENCY;
    system_status.emergency_signals |= signal;
}

uint16_t change_frequency(uint8_t speed_slector) {
    if (speed_slector > 8 ) {
        ESP_LOGE(TAG, "El numero del indice del selector de velocidad es muy grnade %d", speed_slector);
        return 0;
    }
    system_status.inputs_status = speed_slector;
    if ( system_status.frequency > frequency_table[system_status.inputs_status] ) {

        if ( accelerating_handle != NULL ) {
            vTaskDelete( accelerating_handle );
            accelerating_handle = NULL;
        }
        uint16_t desacceleration = get_system_acceleration();
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        if ( desaccelerating_handle == NULL ) {
            xTaskCreatePinnedToCore(desaccelerating, "desaccelerating", 1024, (void*) &desacceleration, 9, &desaccelerating_handle, 1);
        }
        ESP_LOGI( TAG, "Cambiando la frecuencia de regimen a %d. Desacelerando", system_status.frequency_destiny);
    } else if ( system_status.frequency < frequency_table[system_status.inputs_status] ) {
        if ( desaccelerating_handle != NULL ) {
            vTaskDelete( desaccelerating_handle );
            desaccelerating_handle = NULL;
        }
        uint16_t acceleration = get_system_acceleration();
        system_status.frequency_destiny = frequency_table[system_status.inputs_status];
        if ( accelerating_handle == NULL ) {
            xTaskCreatePinnedToCore(accelerating, "accelerating", 1024, (void*) &(acceleration), 9, &accelerating_handle, 1);
        }
        ESP_LOGI( TAG, "Cambiando la frecuencia de regimen a %d. Acelerando", system_status.frequency_destiny);
    }
    system_status.status = SYSTEM_ACCLE_DESACCEL;
    return system_status.frequency_destiny;
}

esp_err_t get_status(system_status_t *s_e) {
    if (s_e == NULL)
        return ESP_ERR_NOT_ALLOWED;
    s_e->frequency = system_status.frequency;
    s_e->frequency_destiny = system_status.frequency_destiny;
    s_e->vbus_min = system_status.vbus_min;
    s_e->ibus_max = system_status.ibus_max;
    s_e->inputs_status = system_status.inputs_status;
    s_e->status = system_status.status;
    return ESP_OK;
}

system_status_e update_meas(uint16_t vbus_meas, uint16_t ibus_meas) {
    bool in_emergency = false;
    system_status.vbus_min = vbus_meas;
    system_status.ibus_max = ibus_meas;

    if ( system_status.ibus_max > system_seccurity_settings.ibus_max ) {
        in_emergency = true;
        if ( system_status.status != SYSTEM_EMERGENCY ) {
            ESP_LOGE( TAG, "Disparo de emergencia por sobre corriente.");
        }
    }

    if ( system_status.vbus_min < system_seccurity_settings.vbus_min ) {
        in_emergency = true;
        if ( system_status.status != SYSTEM_EMERGENCY ) {
            ESP_LOGE( TAG, "Disparo de emergencia por baja tensión.");
        }
    }

    if ( in_emergency == true ) {
        if ( system_status.status != SYSTEM_EMERGENCY ) {
            SystemEventPost(SECURITY_EXCEDED);
            ESP_LOGI( TAG, "Mando senal.");
        }
    } else if ( system_status.status == SYSTEM_EMERGENCY && ( system_status.emergency_signals & 0b010 ) ) {
        SystemEventPost(SECURITY_OK);
        ESP_LOGI( TAG, "Salgo de emergencia por seguridad.");
    }

    return system_status.status;
}

void set_frequency_table( uint16_t input_variable, uint16_t freq_regime ) {
    if ( input_variable == 1 ) { //lineal
        uint16_t frequency_gap = freq_regime / 8;
        frequency_table[7] = frequency_gap;
        for (uint8_t i = 6; i > 0; i-- ) {
            frequency_table[i] = frequency_table[i + 1] + frequency_gap;
            ESP_LOGI(TAG, "frequency_table[%d] = %d", i, frequency_table[i]);
        }
        frequency_table[0] = freq_regime;
        ESP_LOGI(TAG, "frequency_table[0] = %d", frequency_table[0]);
    } else if ( input_variable == 2 ) { //Cuadratica
        for (uint8_t i = 0; i < 8; i++) {
            uint32_t num = (uint32_t)freq_regime * (uint32_t)(i + 1) * (uint32_t)(i + 1);
            uint16_t fi  = (uint16_t)((num + 32) / 64);
            if (fi == 0)
                fi = 1;
            frequency_table[7 - i] = fi;
            ESP_LOGI(TAG, "frequency_table[%d] = %d", 7 - i, frequency_table[7 - i]);
        }
    }
}

void set_system_settings( frequency_settings_t *f_s, seccurity_settings_t *s_s ) {
    system_status.acceleration = f_s->acceleration;
    system_status.desacceleration = f_s->desacceleration;
    system_seccurity_settings.vbus_min = s_s->vbus_min;
    system_seccurity_settings.ibus_max = s_s->ibus_max;
    set_frequency_table( f_s->input_variable, f_s->freq_regime );
}