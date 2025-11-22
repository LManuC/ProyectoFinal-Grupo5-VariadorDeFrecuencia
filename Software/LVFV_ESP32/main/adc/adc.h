/**
 * @file adc.h
 * @author Andrenacci - Carra
 * @brief Declraración de funcion de inicialización y tarea lectura del ADC 
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __ADC_H__

#define __ADC_H__

#include "../LVFV_system.h"

/**
 * @fn void adc_task(void *arg);
 *
 * @brief Función programada como alarma para finalizar el funcionamiento del motor.
 *
 * @details Lee por las entradas el canal 0 (GPIO36) el nivel de tensión de fuente de 5V, 3 (GPIO39) el nivel de tensión de fuente de 3.3V, 6 (GPIO34) tensión de bus de contínua y 7 (GPIO35) corriente de bus de contínua cada @def ADC_PERIOD_MS
 *
 * @param[in] arg
 *      Sin uso.
 */
void adc_task(void *arg);

/**
 * @fn esp_err_t adc_init(void);
 *
 * @brief Configura los 4 ADC necesarios para el sistema y los calibra para obtener las mediciones en desde 0 a 3125mV.
 *
 * @details Configura por las entradas el canal 0 (GPIO36) el nivel de tensión de fuente de 5V, 3 (GPIO39) el nivel de tensión de fuente de 3.3V, 6 (GPIO34) tensión de bus de contínua y 7 (GPIO35) corriente de bus de contínua cada @def ADC_PERIOD_MS
 *
 * @retval 
 *      - ESP_OK: Si inicialización y calibración de todos los ADC fue satisfactoria
 *      - ESP_ERR_INVALID_ARG: Si la iniciaulización de algún ADC falló
 *      - ESP_ERR_INVALID_STATE: Si las calibraciones de algún ADC falló
 */
esp_err_t adc_init(void);

/**
 * @fn system_status_e readADC(void);
 *
 * @brief Desencola mediciones obtenidas desde el ADC para tensión y corriente del bus de contínua, compara con los parámetros seteados y dispara el estado de emergencia si es necesario.
 *
 * @details Consulta si existe alguna medición encolada en bus_meas_evt_queue sin ser bloqueante y envía esas mediciones a analizar por el sistema para evaluar si corresponde o no un estado de emergencia.
 *
 * @retval 
 *      - SYSTEM_EMERGENCY: Si las mediciones de tensión o corriente del bus de DC está fuera de los parámetros esperados
 *      - SYSTEM_EMERGENCY_SENT: Si las mediciones de tensión o corriente del bus de DC continúan por fuera de los parámetros esperados
 *      - Otros estados: Si la medición está dentro de los parámetros aceptables
 */
system_status_e readADC(void);

#endif