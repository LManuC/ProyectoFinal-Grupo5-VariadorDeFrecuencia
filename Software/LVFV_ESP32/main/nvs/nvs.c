/**
 * @file nvs.c
 * @author Andrenacci - Carra
 * @brief Funciones de lectura y escritura de variables no volátiles. Carga en sistema además las variables de seguridad al momento de ser guardadas en la memoria.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include "../LVFV_system.h"
#include "./nvs.h"

#define save_frequency(freq_op)                                 save_16("freq_freq", (freq_op) )
#define save_acceleration(freq_acceleration)                    save_16("freq_acce", (freq_acceleration) )
#define save_desacceleration(freq_desacceleration)              save_16("freq_desa", (freq_desacceleration) )
#define save_input_variable(freq_input_variable)                save_16("freq_input", (freq_input_variable) )
#define save_vbus_min(vbus_min)                                 save_16("vbus_min", (vbus_min) )
#define save_ibus_max(ibus_max)                                 save_16("ibus_max", (ibus_max) )
#define save_hour_ini(hour_ini)                                 save_16("hour_ini", (hour_ini) )
#define save_min_ini(min_ini)                                   save_16("min_ini", (min_ini) )
#define save_hour_fin(hour_fin)                                 save_16("hour_fin", (hour_fin) )
#define save_min_fin(min_fin)                                   save_16("min_fin", (min_fin) )

#define load_frequency(freq_op)                                 load_16("freq_freq", (freq_op), 50 )
#define load_acceleration(freq_acceleration)                    load_16("freq_acce", (freq_acceleration), 5 )
#define load_desacceleration(freq_desacceleration)              load_16("freq_desa", (freq_desacceleration), 3 )
#define load_input_variable(freq_input_variable)                load_16("freq_input", (freq_input_variable), 1 )
#define load_vbus_min(vbus_min)                                 load_16("vbus_min", (vbus_min), 310 )
#define load_ibus_max(ibus_max)                                 load_16("ibus_max", (ibus_max), 2000 )
#define load_hour_ini(hour_ini)                                 load_16("hour_ini", (hour_ini), 20 )
#define load_min_ini(min_ini)                                   load_16("min_ini", (min_ini), 30 )
#define load_hour_fin(hour_fin)                                 load_16("hour_fin", (hour_fin), 20 )
#define load_min_fin(min_fin)                                   load_16("min_fin", (min_fin), 35 )

static const char *TAG = "NVS";

// /**
//  * @fn static esp_err_t save_32 (const char *var_tag, int32_t value);
//  *
//  * @brief Accede a la memoria NVS para guardar un dato de 32 bits en memoria no volatil.
//  *
//  * @details Abre la memoria NVS con el namespace "storage" y guarda la variable cuyo namespace es @p var_tag para guardarla en @p value.
//  *
//  * @param[in] var_tag
//  *      Nombre del namespace donde se almacenará la variable que se está intentando guardar. Es un string.
//  *
//  * @param[out] value
//  *      Puntero donde se guardará el dato que se desea guardar de la memoria.
//  *
//  * @retval 
//  *      - ESP_OK: Si la escritura de la variable fue exitosa
//  *      - ESP_FAIL: si hay un interno error; generalmente ocurre cuando la memoria tiene una oartición corrupta.
//  *      - ESP_ERR_NVS_NOT_INITIALIZED: si la memoria no fue inicializada.
//  *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición con la etiqueta "nvs" no se encuentra.
//  *      - ESP_ERR_NVS_NOT_FOUND: id namespace doesn't exist yet and mode is NVS_READONLY.
//  *      - ESP_ERR_NVS_INVALID_NAME: si el nombre del namespace no cumple con las restricciones.
//  *      - ESP_ERR_NO_MEM: en caso que la memoria no puda ser guardada por la estructura interna.
//  *      - ESP_ERR_NVS_NOT_ENOUGH_SPACE: Si no hay espacia para une nueva entrad o si hay demasiados namespaces diferentes (max 254).
//  *      - ESP_ERR_NOT_ALLOWED: Si la partición es solo lectura y se abrió en modo NVS_READWRITE.
//  *      - ESP_ERR_INVALID_ARG: Si el handler de la NVS es NULL.
//  */
// static esp_err_t save_32 (const char *var_tag, int32_t value) {
//     nvs_handle_t h;
//     esp_err_t err;
//     err = nvs_open("storage", NVS_READWRITE, &h);
//     if (err != ESP_OK) {
//         return err;
//     }
//     ESP_LOGI(TAG, "Guardando %s = %ld", var_tag, (long)value);
//     err = nvs_set_i32(h, var_tag, value);     // <-- i32 correcto
//     if (err == ESP_OK) err = nvs_commit(h);
//     nvs_close(h);
//     return err;
// }

/**
 * @fn static esp_err_t save_16 (const char *var_tag, int16_t value);
 *
 * @brief Accede a la memoria NVS para guardar un dato de 16 bits en memoria no volatil.
 *
 * @details Abre la memoria NVS con el namespace "storage" y guarda la variable cuyo namespace es @p var_tag para guardarla en @p value.
 *
 * @param[in] var_tag
 *      Nombre del namespace donde se almacenará la variable que se está intentando guardar. Es un string.
 *
 * @param[out] value
 *      Puntero donde se guardará el dato que se desea guardar de la memoria.
 *
 * @retval 
 *      - ESP_OK: Si la escritura de la variable fue exitosa
 *      - ESP_FAIL: si hay un interno error; generalmente ocurre cuando la memoria tiene una oartición corrupta.
 *      - ESP_ERR_NVS_NOT_INITIALIZED: si la memoria no fue inicializada.
 *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición con la etiqueta "nvs" no se encuentra.
 *      - ESP_ERR_NVS_NOT_FOUND: id namespace doesn't exist yet and mode is NVS_READONLY.
 *      - ESP_ERR_NVS_INVALID_NAME: si el nombre del namespace no cumple con las restricciones.
 *      - ESP_ERR_NO_MEM: en caso que la memoria no puda ser guardada por la estructura interna.
 *      - ESP_ERR_NVS_NOT_ENOUGH_SPACE: Si no hay espacia para une nueva entrad o si hay demasiados namespaces diferentes (max 254).
 *      - ESP_ERR_NOT_ALLOWED: Si la partición es solo lectura y se abrió en modo NVS_READWRITE.
 *      - ESP_ERR_INVALID_ARG: Si el handler de la NVS es NULL.
 */
static esp_err_t save_16 (const char *var_tag, int16_t value) {
    nvs_handle_t h;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Guardando %s = %d", var_tag, (int)value);
    err = nvs_set_i16(h, var_tag, value);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}

// /**
//  * @fn static esp_err_t load_32(const char *var_tag, int32_t *value, int32_t defval);
//  *
//  * @brief Accede a la memoria NVS para obtener un dato de 32 bits guardado en memoria no volatil.
//  *
//  * @details Abre la memoria NVS con el namespace "storage" y intenta leer la variable cuyo namespace es @p var_tag para guardarla en @p value. En caso de error, guarda @p defval como valor default en @p value.
//  *
//  * @param[in] var_tag
//  *      Nombre del namespace donde se almacena la variable que se está intentando leer. Es un string.
//  *
//  * @param[out] value
//  *      Puntero donde se guardará el dato que se desea leer de la memoria.
//  *
//  * @param[in] defval
//  *      Valor default que se utiliza en caso que nunca se haya creado la variable o si existe algún problema al abrir la NVS.
//  *
//  * @retval 
//  *      - ESP_OK: Si la lectura de la variable fue exitosa
//  *      - ESP_FAIL: si hay un interno error; generalmente ocurre cuando la memoria tiene una oartición corrupta.
//  *      - ESP_ERR_NVS_NOT_INITIALIZED: si la memoria no fue inicializada.
//  *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición con la etiqueta "nvs" no se encuentra.
//  *      - ESP_ERR_NVS_NOT_FOUND: id namespace doesn't exist yet and mode is NVS_READONLY.
//  *      - ESP_ERR_NVS_INVALID_NAME: si el nombre del namespace no cumple con las restricciones.
//  *      - ESP_ERR_NO_MEM: en caso que la memoria no puda ser guardada por la estructura interna.
//  *      - ESP_ERR_NVS_NOT_ENOUGH_SPACE: Si no hay espacia para une nueva entrad o si hay demasiados namespaces diferentes (max 254).
//  *      - ESP_ERR_NOT_ALLOWED: Si la partición es solo lectura y se abrió en modo NVS_READWRITE.
//  *      - ESP_ERR_INVALID_ARG: Si el handler de la NVS es NULL.
//  */
// static esp_err_t load_32(const char *var_tag, int32_t *value, int32_t defval) {
//     nvs_handle_t h;
//     esp_err_t err;
//     if ( var_tag == NULL ) {
//         return ESP_FAIL;
//     }
//     err = nvs_open("storage", NVS_READWRITE, &h);
//     if (err != ESP_OK) {
//         return err;
//     }
//     if (err != ESP_OK) { 
//         *value = defval;
//         return err;
//     }
//     err = nvs_get_i32(h, var_tag, value);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         *value = defval;
//         ESP_LOGW(TAG, "Clave %s no encontrada, usando default=%ld", var_tag, (long)defval);
//         err = ESP_OK;
//     } else if (err == ESP_OK) {
//         ESP_LOGI(TAG, "Cargado %s = %ld", var_tag, (long)*value);   // <-- *value
//     }
//     nvs_close(h);
//     return err;
// }

/**
 * @fn static esp_err_t load_16(const char *var_tag, int16_t *value, int16_t defval);
 *
 * @brief Accede a la memoria NVS para obtener un dato de 16 bits guardado en memoria no volatil.
 *
 * @details Abre la memoria NVS con el namespace "storage" y intenta leer la variable cuyo namespace es @p var_tag para guardarla en @p value. En caso de error, guarda @p defval como valor default en @p value.
 *
 * @param[in] var_tag
 *      Nombre del namespace donde se almacena la variable que se está intentando leer. Es un string.
 *
 * @param[out] value
 *      Puntero donde se guardará el dato que se desea leer de la memoria.
 *
 * @param[in] defval
 *      Valor default que se utiliza en caso que nunca se haya creado la variable o si existe algún problema al abrir la NVS.
 *
 * @retval 
 *      - ESP_OK: Si la lectura de la variable fue exitosa
 *      - ESP_FAIL: si hay un interno error; generalmente ocurre cuando la memoria tiene una oartición corrupta.
 *      - ESP_ERR_NVS_NOT_INITIALIZED: si la memoria no fue inicializada.
 *      - ESP_ERR_NVS_PART_NOT_FOUND: si la partición con la etiqueta "nvs" no se encuentra.
 *      - ESP_ERR_NVS_NOT_FOUND: id namespace doesn't exist yet and mode is NVS_READONLY.
 *      - ESP_ERR_NVS_INVALID_NAME: si el nombre del namespace no cumple con las restricciones.
 *      - ESP_ERR_NO_MEM: en caso que la memoria no puda ser guardada por la estructura interna.
 *      - ESP_ERR_NVS_NOT_ENOUGH_SPACE: Si no hay espacia para une nueva entrad o si hay demasiados namespaces diferentes (max 254).
 *      - ESP_ERR_NOT_ALLOWED: Si la partición es solo lectura y se abrió en modo NVS_READWRITE.
 *      - ESP_ERR_INVALID_ARG: Si el handler de la NVS es NULL.
 */
static esp_err_t load_16(const char *var_tag, int16_t *value, int16_t defval) {
    nvs_handle_t h;

    esp_err_t err;

    if ( var_tag == NULL ) {
        return ESP_FAIL;
    }

    err = nvs_open("storage", NVS_READWRITE, &h);

    if (err != ESP_OK) {
        return err;
    }

    if (err != ESP_OK) { 
        *value = defval;
        return err;
    }

    err = nvs_get_i16(h, var_tag, value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value = defval;
        ESP_LOGW(TAG, "Clave %s no encontrada, usando default=%d", var_tag, (int)defval);
        err = ESP_OK;
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "Cargado %s = %d", var_tag, (int)*value);     // <-- *value
    }

    nvs_close(h);
    return err;
}

esp_err_t nvs_init_once(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t load_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings) {
    esp_err_t retval;
    retval = load_frequency( (int16_t*) &(frequency_settings->freq_regime) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_acceleration( (int16_t*) &(frequency_settings->acceleration) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_desacceleration( (int16_t*) &(frequency_settings->desacceleration) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_input_variable( (int16_t*) &(frequency_settings->input_variable) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_vbus_min( (int16_t*) &(seccurity_settings->vbus_min) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_ibus_max( (int16_t*) &(seccurity_settings->ibus_max) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_hour_ini( (int16_t*) &(time_settings->time_start->tm_hour) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_min_ini( (int16_t*) &(time_settings->time_start->tm_min) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_hour_fin( (int16_t*) &(time_settings->time_stop->tm_hour) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    

    retval = load_min_fin( (int16_t*) &(time_settings->time_stop->tm_min) );
    if ( retval != ESP_OK ){ 
        return retval;
    }
    
    return retval;
}

esp_err_t save_variables(frequency_settings_t *frequency_settings, time_settings_t *time_settings, seccurity_settings_t *seccurity_settings) {
    esp_err_t retval;
    retval = save_frequency( (int16_t) frequency_settings->freq_regime);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_acceleration( (int16_t) frequency_settings->acceleration);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_desacceleration( (int16_t) frequency_settings->desacceleration);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_input_variable( (int16_t) frequency_settings->input_variable);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_vbus_min( (int16_t) seccurity_settings->vbus_min);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_ibus_max( (int16_t) seccurity_settings->ibus_max);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_hour_ini( (int16_t) time_settings->time_start->tm_hour);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_min_ini( (int16_t) time_settings->time_start->tm_min);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_hour_fin( (int16_t) time_settings->time_stop->tm_hour);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    retval = save_min_fin( (int16_t) time_settings->time_stop->tm_min);
    if ( retval != ESP_OK ){ 
        return retval;
    }

    return retval;
}
