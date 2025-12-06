/**
 * @file display.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones que de consulta de los seteos hechos por el usuario, tarea que controla el display y posteo de eventos para controlar el display
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>
#include <stdbool.h>
#include "../LVFV_system.h"

/**
 * @fn uint16_t get_system_frequency();
 *
 * @brief Entrega el valor de frecuencia de regimen seteado por el usuario
 *
 * @retval
 *      Valor de frecuencia de regimen configurado por el usuario
 */
uint16_t get_system_frequency();
/**
 * @fn uint16_t get_system_acceleration();
 *
 * @brief Entrega el valor de aceleración seteado por el usuario
 *
 * @retval
 *      Valor de aceleración configurado por el usuario
 */
uint16_t get_system_acceleration();
/**
 * @fn uint16_t get_system_desacceleration();
 *
 * @brief Entrega el valor de desaceleración seteado por el usuario
 *
 * @retval
 *      Valor de desaceleración configurado por el usuario
 */
uint16_t get_system_desacceleration();
/**
 * @fn esp_err_t sh1106_init();
 *
 * @brief Función que inicializa el display para comenzar a imprimir
 *
 * @retval
 *      ESP_OK Si inicializa correctamente
 *      ESP_FAIL Si alguno de los comandos de inicialización falla
 */
esp_err_t sh1106_init();
/**
 * @fn void task_display(void *pvParameters);
 *
 * @brief Tarea de display que controla las diferentes pantallas de muestra, selección y edición de las variables de sistema.
 *
 * @param[in] pvParameters
 *      Variable sin uso
 */
void task_display(void *pvParameters);
/**
 * @fn esp_err_t DisplayEventPost(systemSignal_e event);
 *
 * @brief Función que permite encolar acciones para que ejecute la tarea de display
 *
 * @retval
 *      pdTRUE: Si se encoló exitosamente
 *      errQUEUE_FULL: Cualquier tipo de falla
 */
esp_err_t DisplayEventPost(systemSignal_e event);

esp_err_t system_variables_save(frequency_settings_SH1106_t *frequency_settings, time_settings_SH1106_t *time_settings, seccurity_settings_SH1106_t *seccurity_settings);

#endif
