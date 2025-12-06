
#ifndef __MAIN_WIFI_WIFI_H__
#define __MAIN_WIFI_WIFI_H__

#include "esp_wifi.h"
#include "esp_http_server.h"

httpd_handle_t start_webserver(void);
void wifi_init_softap(void);
// void start_mdns(void);

#endif 