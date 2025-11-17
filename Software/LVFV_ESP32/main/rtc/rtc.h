/**
 * @file RTC.h
 * @author Andrenacci - Carra
 * @brief Header con prototipos de funciones básicas para el lectura y escritura del RTC y alarmas
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __RTC_H__

#define __RTC_H__

#include "../LVFV_system.h"
#include "esp_err.h"

/**
 * @fn esp_err_t initRTC();
 *
 * @brief Inicializa el RTC en la hora 00:00:00.
 *
 * @details No es importante el día configurado para el sistema, pero carga la hora 0 de UNIX_TIME.
 *
 * @return
 *      - ESP_OK siempre.
 */
esp_err_t initRTC();

/**
 * @fn esp_err_t setTime(struct tm *setting_time);
 *
 * @brief Inicializa el RTC en la hora cargada en setting_time.
 *
 * @details No es importante el día configurado para el sistema, pero carga la hora 0 de UNIX_TIME.
 *
 * @param[in] setting_time
 *      Esturctura tm que es cargada en el RTC del ESP32 según su año, mes, día, hora, minuto y segundo.
 *
 * @return
 *      - ESP_OK si carga el horario correctamente.
 *      - ESP_ERR_INVALID_ARG en caso de recibir un puntero NULL
 */
esp_err_t setTime(struct tm *setting_time);

/**
 * @fn esp_err_t rtc_schedule_alarms(time_settings_t *time_settings);
 *
 * @brief Inicializa las alarmas de inicio y fin de funcionamiento del motor.
 *
 * @details La hora y minuto cargada en time_start es la que dará inicio al funcionamiento del motor, mientas que la cargada en time_stop será la que finalizará el mismo.
 *
 * @param[in] time_settings
 *      Esturctura time_settings_t que contiene los horarios de alarma de inicio y fin. Además contiene la hora actual, una variable que no es utilizada por la función.
 *
 * @return
 *      - ESP_OK si Configura las alarmas exitosamente.
 *      - ESP_FAIL si falla en la creación de las alarmas.
 */
esp_err_t rtc_schedule_alarms(time_settings_t *time_settings);

/**
 * @fn esp_err_t getTime(struct tm *current_timeinfo);
 *
 * @brief Carga la hora actuak desde el RTC del ESP32.
 *
 * @details Lee la hora del RTC del ESP32 expresada en unix time y la convierte en un formato entendible por el usuario en la estructura @p current_timeinfo
 *
 * @param[in] current_timeinfo
 *      Esturctura tm en donde se cargará la fecha y hora del sistema.
 *
 * @return
 *      - ESP_OK si carga la hora exitosamente.
 *      - ESP_ERR_INVALID_ARG si @p current_timeinfo es NULL
 */
esp_err_t getTime(struct tm *current_timeinfo);

#endif