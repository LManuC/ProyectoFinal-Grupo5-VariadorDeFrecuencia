#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
// #include "mdns.h"

#include "esp_netif.h"

// Tus headers con las structs y funciones:
#include "../LVFV_system.h"
#include "../system/sysControl.h"
#include "../display/display.h"
#include "./wifi.h"
#include "form.h"

static const char *TAG = "WEB_CFG";

/* ---------- Helpers para parsear POST x-www-form-urlencoded ---------- */

static int hex2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return 0;
}

static void url_decode(char *dst, const char *src) {
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            int hi = hex2int(src[1]);
            int lo = hex2int(src[2]);
            *dst++ = (char)((hi << 4) | lo);
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Busca un key en cuerpo "key1=val1&key2=val2..." y lo copia decodificado a dest
static bool get_param_value(const char *body, const char *key, char *dest, size_t dest_len) {
    size_t key_len = strlen(key);
    const char *p = body;

    while (p && *p) {
        const char *eq = strchr(p, '=');
        if (!eq) break;

        // Nombre de parámetro
        size_t this_key_len = (size_t)(eq - p);

        if (this_key_len == key_len && strncmp(p, key, key_len) == 0) {
            // Encontramos la clave. Buscar fin del valor (& o final de cadena)
            const char *val_start = eq + 1;
            const char *amp = strchr(val_start, '&');
            size_t val_len = amp ? (size_t)(amp - val_start) : strlen(val_start);
            if (val_len >= dest_len) val_len = dest_len - 1;

            char encoded[256];
            if (val_len >= sizeof(encoded)) val_len = sizeof(encoded) - 1;

            memcpy(encoded, val_start, val_len);
            encoded[val_len] = '\0';

            url_decode(dest, encoded);
            return true;
        }

        // Ir al siguiente par key=value
        const char *amp = strchr(eq + 1, '&');
        if (!amp) break;
        p = amp + 1;
    }

    return false;
}

/* --------------------- Handlers HTTP --------------------- */

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char body[512];
    char cmd[16] = {0};

    time_settings_SH1106_t time_edit;
    seccurity_settings_SH1106_t seccurity_edit;
    frequency_settings_SH1106_t frequency_edit;

    if (req->content_len >= sizeof(body)) {
        ESP_LOGW(TAG, "Body demasiado grande (%d)", req->content_len);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Body too large");
        return ESP_FAIL;
    }

    int received = httpd_req_recv(req, body, req->content_len);
    if (received <= 0) {
        ESP_LOGW(TAG, "Error recibiendo body");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive error");
        return ESP_FAIL;
    }
    body[received] = '\0';

    ESP_LOGI(TAG, "Body: %s", body);

    char buf[64];

    /* -------- Leer parámetros -------- */

    int frequency = 0, accel = 0, decel = 0, variation = 0;
    int imax = 0, vmin = 0;
    char variation_str[16] = {0};
    char sys_time_str[16] = {0};
    char start_time_str[16] = {0};
    char stop_time_str[16] = {0};

    if (get_param_value(body, "frequency", buf, sizeof(buf))) {
        frequency = atoi(buf);
        ESP_LOGI(TAG, "Frecuencia: %d", frequency);
    }
    if (get_param_value(body, "accel", buf, sizeof(buf))) {
        accel = atoi(buf);
        ESP_LOGI(TAG, "Aceleración: %d", accel);
    }
    if (get_param_value(body, "decel", buf, sizeof(buf))) {
        decel = atoi(buf);
        ESP_LOGI(TAG, "Desaceleración: %d", decel);
    }
    if (get_param_value(body, "imax", buf, sizeof(buf))) {
        imax = atoi(buf);
        ESP_LOGI(TAG, "Corriente maxima: %d", imax);
    }
    if (get_param_value(body, "vmin", buf, sizeof(buf))) {
        vmin = atoi(buf);
        ESP_LOGI(TAG, "Tension minima: %d", vmin);
    }
    if (get_param_value(body, "variation_str", variation_str, sizeof(variation_str))) {
        ESP_LOGI(TAG, "Variacion de entradas %s", variation_str);
        if ( strcmp(variation_str, "cuadratica") == 0 ) {
            variation = 1;
        } else {
            variation = 2;
        }
        // variation_str = "linear" o "quadratic"
    }
    get_param_value(body, "sys_time",   sys_time_str,   sizeof(sys_time_str));
    get_param_value(body, "start_time", start_time_str, sizeof(start_time_str));
    get_param_value(body, "stop_time",  stop_time_str,  sizeof(stop_time_str));

    /* -------- Parsear horas -------- */

    int sys_h=0, sys_m=0, sys_s=0;
    int start_h=0, start_m=0;
    int stop_h=0, stop_m=0;

    if (sscanf(sys_time_str, "%2d:%2d:%2d", &sys_h, &sys_m, &sys_s) != 3) {
        ESP_LOGW(TAG, "Hora de sistema inválida: %s", sys_time_str);
    }
    if (sscanf(start_time_str, "%2d:%2d", &start_h, &start_m) != 2) {
        ESP_LOGW(TAG, "Hora de arranque inválida: %s", start_time_str);
    }
    if (sscanf(stop_time_str, "%2d:%2d", &stop_h, &stop_m) != 2) {
        ESP_LOGW(TAG, "Hora de parada inválida: %s", stop_time_str);
    }
    ESP_LOGI(TAG, "Hora de sistema: %02d:%02d:%02d", sys_h, sys_m, sys_s);
    ESP_LOGI(TAG, "Hora de arranque: %02d:%02d", start_h, start_m);
    ESP_LOGI(TAG, "Hora de parada: %02d:%02d", stop_h, stop_m);


    struct tm time_system;
    time_system.tm_hour = sys_h;
    time_system.tm_min = sys_m;
    time_system.tm_sec = sys_s;
    struct tm time_start;
    time_start.tm_hour = start_h;
    time_start.tm_min = start_m;
    struct tm time_stop;
    time_stop.tm_hour = stop_h;
    time_stop.tm_min = stop_m;

    frequency_edit.frequency_settings.freq_regime = frequency;
    frequency_edit.frequency_settings.acceleration = accel;
    frequency_edit.frequency_settings.desacceleration = decel;
    frequency_edit.frequency_settings.input_variable = variation;

    seccurity_edit.seccurity_settings.ibus_max = imax;
    seccurity_edit.seccurity_settings.vbus_min = vmin;

    time_edit.time_settings.time_system = &time_system;
    time_edit.time_settings.time_start = &time_start;
    time_edit.time_settings.time_stop = &time_stop;

    if (get_param_value(body, "cmd", cmd, sizeof(cmd))) {
        ESP_LOGI(TAG, "Comando: %s", cmd);
        if (strcmp(cmd, "save") == 0) {
            system_variables_save(&frequency_edit, &time_edit, &seccurity_edit);
        } else if (strcmp(cmd, "start") == 0) {
            SystemEventPost(START_PRESSED);
        } else if (strcmp(cmd, "stop") == 0) {
            SystemEventPost(STOP_PRESSED);
        }
    }

    /* -------- Respuesta al navegador -------- */

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

/* ---------------------- Inicio HTTPD ---------------------- */

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t save_uri = {
            .uri      = "/save",
            .method   = HTTP_POST,
            .handler  = save_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &save_uri);
    }
    return server;
}

/* ---------------------- WiFi AP ---------------------- */

void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "LVFV_2025",
            .ssid_len = 0,
            .channel = 1,
            .password = "LVFV_2025",
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen((char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP iniciado. SSID: %s, pass: %s", wifi_config.ap.ssid, wifi_config.ap.password);
}

// void start_mdns(void) {
//     // Iniciar mDNS
//     mdns_init();
//     // hostname -> variador.local
//     mdns_hostname_set("variador");
//     // Nombre descriptivo
//     mdns_instance_name_set("Variador ESP32");

//     // Opcional: anunciar servicio HTTP
//     mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, NULL, 0);
// }

// /* ---------------------- main ---------------------- */

// void app_main(void)
// {
//     // Inicializar NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ESP_ERROR_CHECK(nvs_flash_init());
//     }

//     wifi_init_softap();
//     start_webserver();

//     ESP_LOGI(TAG, "Servidor web listo.");
// }
