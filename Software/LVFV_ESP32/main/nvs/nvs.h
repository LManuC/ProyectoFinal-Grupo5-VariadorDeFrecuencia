/**
 * @file nvs.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones de lectura y escritura de memoria no volatil. Útiles para los reset y no tener que reconfigurar siempre las variables básicas del sistema.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef VARIABLE_ADMIN_H
#define VARIABLE_ADMIN_H

#include <stdint.h>
#include "../LVFV_system.h"
/**
 * @fn esp_err_t nvs_init_once(void);
 *
 * @brief Inicializa la memoria NVS una única vez.
 *
 * @details Intenta inicializar la NVS mediante nvs_flash_init(). Si la memoria está llena
 *          (ESP_ERR_NVS_NO_FREE_PAGES) o se detecta una nueva versión de NVS
 *          (ESP_ERR_NVS_NEW_VERSION_FOUND), se borra la partición NVS y se vuelve a
 *          inicializar. No realiza inicialización repetida si ya fue hecha por el sistema.
 *
 * @retval
 *      - ESP_OK: Si la inicialización fue exitosa.
 *      - ESP_ERR_NVS_NO_FREE_PAGES: Sin páginas libres; requiere borrado y reinicialización.
 *      - ESP_ERR_NVS_NEW_VERSION_FOUND: Versión de NVS incompatible; requiere borrado.
 *      - ESP_ERR_NVS_NOT_INITIALIZED: si la memoria no pudo inicializarse.
 *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición con la etiqueta "nvs" no se encuentra.
 *      - ESP_ERR_NOT_ALLOWED: Si la partición es solo lectura.
 *      - Otros códigos de error propagados por nvs_flash_init() o nvs_flash_erase().
 */
esp_err_t nvs_init_once(void);

/**
 * @fn esp_err_t load_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings);
 *
 * @brief Carga desde NVS los parámetros de frecuencia, seguridad y ventanas horarias.
 *
 * @details Utiliza las funciones internas de lectura (load_16) para obtener cada
 *          parámetro persistido en NVS. Si alguna lectura falla, retorna el primer
 *          error encontrado. En caso de no existir una clave, se aplica el valor
 *          por defecto definido en cada macro de carga.
 *
 * @param[out] frequency_settings
 *      Estructura de parámetros de frecuencia a cargar.
 *
 * @param[out] time_settings
 *      Estructura de parámetros de tiempo (ventanas start/stop) a cargar.
 *
 * @param[out] seccurity_settings
 *      Estructura de parámetros de seguridad (vbus/ibus) a cargar.
 *
 * @retval
 *      - ESP_OK: Si todas las lecturas fueron exitosas (o claves ausentes con default aplicado).
 *      - ESP_FAIL: si hay un error interno.
 *      - ESP_ERR_NVS_NOT_INITIALIZED: si la NVS no está inicializada.
 *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición "nvs" no se encuentra.
 *      - ESP_ERR_NVS_INVALID_NAME: si alguna clave no cumple restricciones.
 *      - ESP_ERR_NO_MEM / ESP_ERR_NVS_NOT_ENOUGH_SPACE: errores internos de la NVS.
 *      - ESP_ERR_NOT_ALLOWED: si la partición es solo lectura.
 *      - ESP_ERR_INVALID_ARG: si el handler interno de NVS es NULL.
 */
esp_err_t load_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings);

/**
 * @fn esp_err_t save_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings);
 *
 * @brief Guarda en NVS los parámetros de frecuencia, seguridad y ventanas horarias.
 *
 * @details Utiliza las funciones internas de guardado (save_16) para persistir cada
 *          parámetro en la NVS. Si alguna escritura falla, retorna el primer error encontrado.
 *
 * @param[in] frequency_settings
 *      Estructura con parámetros de frecuencia a guardar.
 *
 * @param[in] time_settings
 *      Estructura con parámetros de tiempo (ventanas start/stop) a guardar.
 *
 * @param[in] seccurity_settings
 *      Estructura con parámetros de seguridad (vbus/ibus) a guardar.
 *
 * @retval
 *      - ESP_OK: Si todas las escrituras fueron exitosas.
 *      - ESP_FAIL: si hay un error interno.
 *      - ESP_ERR_NVS_NOT_INITIALIZED: si la NVS no está inicializada.
 *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición "nvs" no se encuentra.
 *      - ESP_ERR_NVS_INVALID_NAME: si alguna clave no cumple restricciones.
 *      - ESP_ERR_NO_MEM / ESP_ERR_NVS_NOT_ENOUGH_SPACE: errores internos de la NVS o espacio insuficiente.
 *      - ESP_ERR_NOT_ALLOWED: si la partición es solo lectura.
 *      - ESP_ERR_INVALID_ARG: si el handler interno de NVS es NULL.
 */
esp_err_t save_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings);

#endif