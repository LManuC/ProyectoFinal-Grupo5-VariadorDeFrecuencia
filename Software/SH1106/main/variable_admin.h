#ifndef VARIABLE_ADMIN_H
#define VARIABLE_ADMIN_H

#include <stdint.h>

esp_err_t nvs_init_once(void);
esp_err_t save_32 (const char *var_tag, int32_t variable);
esp_err_t save_16 (const char *var_tag, int16_t variable);
esp_err_t load_32(const char *var_tag, int32_t *value, int32_t defval);
esp_err_t load_16(const char *var_tag, int16_t *value, int16_t defval);

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

#endif