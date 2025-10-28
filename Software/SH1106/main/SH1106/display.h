#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>
#include <stdbool.h>
#include "../LVFV_system.h"
#include "./display_typedef.h"

void task_display(void *pvParameters);
esp_err_t sh1106_init();



uint16_t engine_start();
void engine_stop();
void engine_emergency_stop();
uint16_t change_frequency(uint8_t speed_slector);
void update_meas(seccurity_settings_t bus_meas);
uint16_t get_frequency_destiny();
uint16_t get_frequency();

#endif
