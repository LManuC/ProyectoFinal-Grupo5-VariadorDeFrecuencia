
#ifndef __MAIN_WIFI_WIFI_H__
#define __MAIN_WIFI_WIFI_H__

#include "esp_wifi.h"
#include "esp_http_server.h"

/**
 * @fn httpd_handle_t start_webserver(void);
 * 
 * @brief Arranca el servidor HTTP y registra endpoints.
 *        - GET  "/"     → formulario
 *        - POST "/save" → procesa y responde
 */
httpd_handle_t start_webserver(void);

/**
 * @fn void wifi_init_softap(void);
 * 
 * @brief Configura ESP-NETIF + Wi-Fi en modo AP y arranca SSID/clave. (Canal 1, máx 1 cliente, WPA/WPA2)
 */
void wifi_init_softap(void);
// void start_mdns(void);

#endif 