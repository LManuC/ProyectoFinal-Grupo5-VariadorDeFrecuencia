#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "NVS";

esp_err_t nvs_init_once(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t save_32 (const char *var_tag, int32_t value) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Guardando %s = %ld", var_tag, (long)value);
    err = nvs_set_i32(h, var_tag, value);     // <-- i32 correcto
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}

esp_err_t save_16 (const char *var_tag, int16_t value) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "Guardando %s = %d", var_tag, (int)value);
    err = nvs_set_i16(h, var_tag, value);
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}

esp_err_t load_32(const char *var_tag, int32_t *value, int32_t defval) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &h);
    if (err != ESP_OK) { *value = defval; return err; }

    err = nvs_get_i32(h, var_tag, value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value = defval;
        ESP_LOGW(TAG, "Clave %s no encontrada, usando default=%ld", var_tag, (long)defval);
        err = ESP_OK;
    } else if (err == ESP_OK) {
        ESP_LOGI(TAG, "Cargado %s = %ld", var_tag, (long)*value);   // <-- *value
    }

    nvs_close(h);
    return err;
}

esp_err_t load_16(const char *var_tag, int16_t *value, int16_t defval) {
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &h);
    if (err != ESP_OK) { *value = defval; return err; }

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