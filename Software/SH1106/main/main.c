/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "sh1106.h"
#include "esp_log.h"
#include "driver/gpio.h"

/*void app_main(void)
{
    printf("Hello world!\n");

    // Print chip information 
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
*/
static const char *TAG = "UTN-CA-PF2025";

#define BTN_INCREMENTO                  0xFE
#define BTN_DECREMENTO                  0xFD
#define BTN_IZQUIERDA_MULT             0xFB
#define BTN_DERECHA_MULT             0xF7
#define BTN_EDIT                        0xEF

void sh1106_splash_screen(sh1106_t *);

static const gpio_num_t botones[] = {
    GPIO_NUM_12,  // Pulsador 1
    GPIO_NUM_13,  // Pulsador 2
    GPIO_NUM_14,  // Pulsador 3
    GPIO_NUM_25,  // Pulsador 4
    GPIO_NUM_26,  // Pulsador 5
    GPIO_NUM_27,  // Pulsador 6
    GPIO_NUM_32,  // Pulsador 7 (ejemplo extra si querés otro pin válido)
    GPIO_NUM_33   // Pulsador 8
};

#define NUM_BOTONES (sizeof(botones)/sizeof(botones[0]))

#define LINE_INCREMENT              9
#define FREC_LINE_INDX              20
#define VBUS_LINE_INDX              29
#define IBUS_LINE_INDX              38
#define HINI_LINE_INDX              47
#define HFIN_LINE_INDX              56

void print_binary(uint8_t value) {
    char buffer[9]; // 8 bits + terminador
    for (int i = 7; i >= 0; i--) {
        buffer[7 - i] = (value & (1 << i)) ? '1' : '0';
    }
    buffer[8] = '\0'; // terminador de string

    ESP_LOGI(TAG, "Valor: %s", buffer);
}

void app_main(void) {
    char hora_str[13];
    uint8_t arrow_cursor = 0;
    uint16_t multiplier_cursor = 0;
    uint8_t h_ini = 2,m_ini = 14;
    uint8_t h_fin = 4,m_fin = 18;
    uint8_t freq = 143;
    uint16_t vbus_min = 409;
    uint16_t ibus_max = 15;
    uint8_t scratch_seg = 0;
    uint8_t scratch_min = 0;
    uint8_t scratch_hor = 0;
    uint16_t scratch = 0;
    uint8_t edit = 0;
    uint8_t blink = 0;
    int prev_nivel = 0, nivel = 0;

    for (int i = 0; i < NUM_BOTONES; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << botones[i],
            .mode = GPIO_MODE_INPUT,
            .intr_type = GPIO_INTR_DISABLE,
        };
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;   // activar pull-up interno
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&io_conf);
        prev_nivel |= gpio_get_level(botones[i]) << i;
    }
    nivel = prev_nivel;

    struct tm timeinfo = {
        .tm_year = 2025 - 1900, // Año desde 1900
        .tm_mon  = 9,           // Mes (0=enero, 8=septiembre)
        .tm_mday = 18,          // Día del mes
        .tm_hour = 0,          // Hora
        .tm_min  = 47,          // Minuto
        .tm_sec  = 30            // Segundo
    };

    time_t now = mktime(&timeinfo);  // Convierte a timestamp UNIX

    struct timeval tv = {
        .tv_sec = now,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL); // Cargar en RTC interno

    sh1106_t oled = {
        .width = 128,
        .height = 64,
        .rotation = 0
    };
    ESP_ERROR_CHECK(sh1106_init(&oled));

    sh1106_splash_screen(&oled);
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        // ESP_LOGI(TAG, "Hora actual: %04d/%02d/%02d %02d:%02d:%02d",
        //          timeinfo.tm_year + 1900,
        //          timeinfo.tm_mon + 1,
        //          timeinfo.tm_mday,
        //          timeinfo.tm_hour,
        //          timeinfo.tm_min,
        //          timeinfo.tm_sec);
        sh1106_clear_buffer(&oled);
        if ( arrow_cursor == FREC_LINE_INDX - LINE_INCREMENT && edit ) {
            sprintf(hora_str, "%02d:%02d:%02d", scratch_hor, scratch_min, scratch_seg);
            if ( multiplier_cursor == 1 ) {
                if ( blink > 12 ) {
                    sh1106_draw_number(&oled, scratch_seg,    76, 0, 0);
                }
                sh1106_draw_number(&oled, scratch_min,     52, 0, 0);
                sh1106_draw_number(&oled, scratch_hor,     28, 0, 0);
            } else if ( multiplier_cursor == 2 ) {
                if ( blink > 12 ) {
                    sh1106_draw_number(&oled, scratch_min,     52, 0, 0);
                }
                sh1106_draw_number(&oled, scratch_seg,    76, 0, 0);
                sh1106_draw_number(&oled, scratch_hor,     28, 0, 0);
            } else if ( multiplier_cursor == 3 ) {
                if ( blink > 12 ) {
                    sh1106_draw_number(&oled, scratch_hor,     28, 0, 0);
                }
                sh1106_draw_number(&oled, scratch_seg,    76, 0, 0);
                sh1106_draw_number(&oled, scratch_min,     52, 0, 0);
            }
            sh1106_draw_text  (&oled, ":",            44, 0);
            sh1106_draw_text  (&oled, ":",            68, 0);
        } else {
            strftime(hora_str, sizeof(hora_str), "%H:%M:%S", &timeinfo);
            sh1106_print_hour(&oled, hora_str);
        }
        sh1106_draw_utn_logo(&oled, 1, 1);
        // sh1106_draw_fail(&oled, 109, 1);
        sh1106_draw_line(&oled, 0, 17);
        if ( arrow_cursor >= FREC_LINE_INDX && arrow_cursor <= HFIN_LINE_INDX ) {
            sh1106_draw_arrow(&oled, 1, arrow_cursor);
        }
        sh1106_draw_text(&oled, "Frec:",    20, FREC_LINE_INDX);
        sh1106_draw_text(&oled, "VBus:",    20, VBUS_LINE_INDX);
        sh1106_draw_text(&oled, "IOut:",    20, IBUS_LINE_INDX);
        sh1106_draw_text(&oled, "H. ini:",  20, HINI_LINE_INDX);
        sh1106_draw_text(&oled, "H. fin:",  20, HFIN_LINE_INDX);

        if ( arrow_cursor == HINI_LINE_INDX && edit ) {
            if ( blink > 12 && multiplier_cursor == 1 ) {
                sh1106_draw_number(&oled, scratch_min,    108, HINI_LINE_INDX, 0);
            } else if ( blink > 12 && multiplier_cursor == 2 ) {
                sh1106_draw_number(&oled, scratch_hor,     85, HINI_LINE_INDX, 0);
            }
            sh1106_draw_text  (&oled, ":",            101, HINI_LINE_INDX);
        } else {
            sh1106_draw_number(&oled, h_ini,            85, HINI_LINE_INDX, 0);
            sh1106_draw_text  (&oled,   ":",           101, HINI_LINE_INDX);
            sh1106_draw_number(&oled, m_ini,           108, HINI_LINE_INDX, 0);
        }
        if ( arrow_cursor == HFIN_LINE_INDX && edit ) {
            if ( blink > 12 && multiplier_cursor == 1 ) {
                sh1106_draw_number(&oled, scratch_hor,     85, HFIN_LINE_INDX, 0);
            } else if ( blink > 12 && multiplier_cursor == 2 ) {
                sh1106_draw_number(&oled, scratch_min,    108, HFIN_LINE_INDX, 0);
            }
            sh1106_draw_text  (&oled, ":",            101, HFIN_LINE_INDX);
        } else {
            sh1106_draw_number(&oled, h_fin,            85, HFIN_LINE_INDX, 0);
            sh1106_draw_text  (&oled, ":",             101, HFIN_LINE_INDX);
            sh1106_draw_number(&oled, m_fin,           108, HFIN_LINE_INDX, 0);
        }
        if ( arrow_cursor == FREC_LINE_INDX && edit ) {
            if ( blink > 12 )
                sh1106_draw_number(&oled, scratch,     85, FREC_LINE_INDX, 0);
        } else {
            sh1106_draw_number(&oled, freq,     85, FREC_LINE_INDX, 0);
        }
        if ( arrow_cursor == VBUS_LINE_INDX && edit ) {
            if ( blink > 12 )
                sh1106_draw_number(&oled, scratch,     85, VBUS_LINE_INDX, 0);
        } else {
            sh1106_draw_number(&oled, vbus_min,     85, VBUS_LINE_INDX, 0);
        }
        if ( arrow_cursor == IBUS_LINE_INDX && edit ) {
            if ( blink > 12 )
                sh1106_draw_number(&oled, scratch,     85, IBUS_LINE_INDX, 0);
        } else {
            sh1106_draw_number(&oled, ibus_max, 85, IBUS_LINE_INDX, 0);
        }
        ESP_ERROR_CHECK(sh1106_refresh(&oled));

        nivel = 0;
        for (int i = 0; i < NUM_BOTONES; i++) {
            nivel |= gpio_get_level(botones[i]) << i;
        }
        if ( nivel != prev_nivel ) {
            for (int i = 0; i < NUM_BOTONES; i++) {
                if ( ( nivel & ( 1 << i ) ) != (prev_nivel & ( 1 << i ) ) ) {
                    ESP_LOGI(TAG, "Boton %d (GPIO %d): %s", i+1, botones[i], nivel & ( 1 << i ) ? "LIBERADO" : "PRESIONADO");
                }
            }
            print_binary(nivel);
            switch ( nivel) {
                case BTN_INCREMENTO:
                    switch ( arrow_cursor ) {
                        case FREC_LINE_INDX - LINE_INCREMENT:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = FREC_LINE_INDX;
                                scratch = freq;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch_seg += 1;
                                    if ( scratch_seg >= 60 ) {
                                        scratch_seg = 0;
                                        scratch_min += 1;
                                        if ( scratch_min >= 60 ) {
                                            scratch_min = 0;
                                            scratch_hor += 1;
                                            if ( scratch_hor >= 24 ) {
                                                scratch_hor = 0;
                                            }
                                        }
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    scratch_min += 1;
                                    if ( scratch_min >= 60 ) {
                                        scratch_min = 0;
                                        scratch_hor += 1;
                                        if ( scratch_hor >= 24 ) {
                                            scratch_hor = 0;
                                        }
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    scratch_hor += 1;
                                    if ( scratch_hor >= 24 ) {
                                        scratch_hor = 0;
                                    }
                                }
                            }
                            break;
                        case FREC_LINE_INDX:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = VBUS_LINE_INDX;
                                scratch = vbus_min;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch += 1;
                                    if ( scratch > 150 ) {
                                        scratch = 0;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch == 150 ) {
                                        scratch = 0;
                                    } else {
                                        scratch += 10;
                                        if ( scratch > 150 ) {
                                            scratch = 150;
                                        }
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    if ( scratch == 150 ) {
                                        scratch = 0;
                                    } else {
                                        scratch += 100;
                                        if ( scratch > 150 ) {
                                            scratch = 150;
                                        }
                                    }
                                }
                            }
                            break;
                        case VBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = IBUS_LINE_INDX;
                                scratch = ibus_max;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch += 1;
                                    if ( scratch > 400 ) {
                                        scratch = 0;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch == 400 ) {
                                        scratch = 0;
                                    } else {
                                        scratch += 10;
                                        if ( scratch > 400 ) {
                                            scratch = 400;
                                        }
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    if ( scratch == 400 ) {
                                        scratch = 0;
                                    } else {
                                        scratch += 100;
                                        if ( scratch > 400 ) {
                                            scratch = 400;
                                        }
                                    }
                                }
                            }
                            break;
                        case IBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = HINI_LINE_INDX;
                                scratch_min = m_ini;
                                scratch_hor = h_ini;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch += 1;
                                    if ( scratch > 20 ) {
                                        scratch = 0;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch == 20 ) {
                                        scratch = 0;
                                    } else {
                                        scratch += 10;
                                        if ( scratch > 20 ) {
                                            scratch = 20;
                                        }
                                    }
                                }
                            }
                            break;
                        case HINI_LINE_INDX:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = HFIN_LINE_INDX;
                                scratch_min = m_fin;
                                scratch_hor = h_fin;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch_min += 1;
                                    if ( scratch_min >= 60 ) {
                                        scratch_min = 0;
                                        scratch_hor += 1;
                                        if ( scratch_hor >= 24 ) {
                                            scratch_hor = 0;
                                        }
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    scratch_hor += 1;
                                    if ( scratch_hor >= 24 ) {
                                        scratch_hor = 0;
                                    }
                                }
                            }
                            break;
                        case HFIN_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = 0;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    scratch_min += 1;
                                    if ( scratch_min >= 60 ) {
                                        scratch_min = 0;
                                        scratch_hor += 1;
                                        if ( scratch_hor >= 24 ) {
                                            scratch_hor = 0;
                                        }
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    scratch_hor += 1;
                                    if ( scratch_hor >= 24 ) {
                                        scratch_hor = 0;
                                    }
                                }
                            }
                            break;
                        default:
                            multiplier_cursor = 1;
                            arrow_cursor = FREC_LINE_INDX - LINE_INCREMENT;
                            scratch_hor = timeinfo.tm_hour;
                            scratch_min = timeinfo.tm_min;
                            scratch_seg = timeinfo.tm_sec;
                            edit = 0;
                            break;
                    }
                    ESP_LOGI(TAG, "scratch: %d", scratch);
                    break;
                case BTN_DECREMENTO:
                    switch ( arrow_cursor ) {
                        case FREC_LINE_INDX - LINE_INCREMENT:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = 0;
                                scratch_hor = timeinfo.tm_hour;
                                scratch_min = timeinfo.tm_min;
                                scratch_seg = timeinfo.tm_sec;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch_seg > 0 ) {
                                        scratch_seg -= 1;
                                    } else if ( scratch_min > 0 ) {
                                        scratch_seg = 59;
                                        scratch_min -= 1;
                                    } else if ( scratch_hor > 0 ) {
                                        scratch_min = 59;
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_seg = 59;
                                        scratch_min = 59;
                                        scratch_hor = 23;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch_min > 0 ) {
                                        scratch_min -= 1;
                                    } else if ( scratch_hor > 0 ) {
                                        scratch_min = 59;
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_min = 59;
                                        scratch_hor = 23;
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    if ( scratch_hor > 0 ) {
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_hor = 23;
                                    }
                                }
                            }
                            break;
                        case FREC_LINE_INDX:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = FREC_LINE_INDX - LINE_INCREMENT;
                                scratch_hor = timeinfo.tm_hour;
                                scratch_min = timeinfo.tm_min;
                                scratch_seg = timeinfo.tm_sec;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch > 0 ) {
                                        scratch -= 1;
                                    } else if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 150;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 150;
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 150;
                                    }
                                }
                            }
                            break;
                        case VBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = FREC_LINE_INDX;
                                scratch = freq;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch > 0 ) {
                                        scratch -= 1;
                                    } else if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 420;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 420;
                                    }
                                } else if ( multiplier_cursor == 3 ) {
                                    if ( scratch > 100 ) {
                                        scratch -= 100;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 420;
                                    }
                                }
                            }
                            break;
                        case IBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = VBUS_LINE_INDX;
                                scratch = vbus_min;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch > 0 ) {
                                        scratch -= 1;
                                    } else if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 20;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch > 10 ) {
                                        scratch -= 10;
                                    } else if ( scratch > 0 ) {
                                        scratch = 0;
                                    } else {
                                        scratch = 20;
                                    }
                                }
                            }
                            break;
                        case HINI_LINE_INDX:
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = IBUS_LINE_INDX;
                                scratch = ibus_max;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch_min > 0 ) {
                                        scratch_min -= 1;
                                    } else if ( scratch_hor > 0 ) {
                                        scratch_min = 59;
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_min = 59;
                                        scratch_hor = 23;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch_hor > 0 ) {
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_hor = 23;
                                    }
                                }
                            }
                            break;
                        case HFIN_LINE_INDX: 
                            if ( edit == 0 ) {
                                multiplier_cursor = 1;
                                arrow_cursor = HINI_LINE_INDX;
                                scratch_min = m_ini;
                                scratch_hor = h_ini;
                            } else {
                                if ( multiplier_cursor == 1 ) {
                                    if ( scratch_min > 0 ) {
                                        scratch_min -= 1;
                                    } else if ( scratch_hor > 0 ) {
                                        scratch_min = 59;
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_min = 59;
                                        scratch_hor = 23;
                                    }
                                } else if ( multiplier_cursor == 2 ) {
                                    if ( scratch_hor > 0 ) {
                                        scratch_hor -= 1;
                                    } else {
                                        scratch_hor = 23;
                                    }
                                }
                            }
                            break;
                        default:
                            multiplier_cursor = 1;
                            arrow_cursor = HFIN_LINE_INDX;
                            scratch_min = m_fin;
                            scratch_hor = h_fin;
                            edit = 0;
                            break;
                    }
                    ESP_LOGI(TAG, "scratch: %d", scratch);
                    break; 
                case BTN_IZQUIERDA_MULT:
                    if ( arrow_cursor <= HFIN_LINE_INDX && arrow_cursor >= FREC_LINE_INDX - LINE_INCREMENT && edit == 1) {
                        if ( multiplier_cursor == 0 ) {
                            multiplier_cursor = 1;
                        } else if ( multiplier_cursor < 3 ) {
                            multiplier_cursor++;
                        }
                    } else {
                        multiplier_cursor = 1;
                    }
                    ESP_LOGI(TAG, "Multiplicador: %d", multiplier_cursor);
                    break;
                case BTN_DERECHA_MULT:
                    if ( arrow_cursor <= HFIN_LINE_INDX && arrow_cursor >= FREC_LINE_INDX - LINE_INCREMENT && edit == 1 ) {
                        if ( multiplier_cursor > 1 ) {
                            multiplier_cursor--;
                        }
                    } else {
                        multiplier_cursor = 1;
                    }
                    ESP_LOGI(TAG, "Multiplicador: %d", multiplier_cursor);
                    break;
                case BTN_EDIT:
                    edit ^= 1;
                    switch ( arrow_cursor ) {
                        case FREC_LINE_INDX - LINE_INCREMENT:
                            if ( edit == 0 ) {
                                ESP_LOGI(TAG,"Parametro guardado");
                                timeinfo.tm_hour = scratch_hor;
                                timeinfo.tm_min  = scratch_min;
                                timeinfo.tm_sec  = scratch_seg;
                                    
                                now = mktime(&timeinfo);  // Convierte a timestamp UNIX
                                tv.tv_sec = now;

                                settimeofday(&tv, NULL); // Cargar en RTC interno

                                ESP_LOGI(TAG, "Hora guardada: %04d/%02d/%02d %02d:%02d:%02d",
                                        timeinfo.tm_year + 1900,
                                        timeinfo.tm_mon + 1,
                                        timeinfo.tm_mday,
                                        timeinfo.tm_hour,
                                        timeinfo.tm_min,
                                        timeinfo.tm_sec);
                            } else {
                                ESP_LOGI(TAG,"Comienza edicion");
                                multiplier_cursor = 1;
                                scratch_hor = timeinfo.tm_hour;
                                scratch_min = timeinfo.tm_min;
                                scratch_seg = timeinfo.tm_sec;
                                break;
                            }
                        case FREC_LINE_INDX:
                            if ( edit == 0 ) {
                                freq = scratch;
                                ESP_LOGI(TAG, "frecuencia guardada: %d", freq);
                            } else {
                                multiplier_cursor = 1;
                                scratch = freq;
                            }
                            break;
                        case VBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                    vbus_min = scratch;
                                    ESP_LOGI(TAG, "VBus guardada: %d", freq);
                            } else {
                                multiplier_cursor = 1;
                                scratch = vbus_min;
                            }
                            break;
                        case IBUS_LINE_INDX: 
                            if ( edit == 0 ) {
                                    ibus_max = scratch;
                                    ESP_LOGI(TAG, "IBus guardada: %d", freq);
                            } else {
                                multiplier_cursor = 1;
                                scratch = ibus_max;
                            }
                            break;
                        case HINI_LINE_INDX:
                            if ( edit == 0 ) {
                                    h_ini = scratch_hor;
                                    m_ini = scratch_min;
                                    ESP_LOGI(TAG, "Horario inicio guardada: %02d:%02d", h_ini, m_ini);
                            } else {
                                multiplier_cursor = 1;
                                scratch_min = m_ini;
                                scratch_hor = h_ini;
                            }
                            break;
                        case HFIN_LINE_INDX: 
                            if ( edit == 0 ) {
                                    h_fin = scratch_hor;
                                    m_fin = scratch_min;
                                    ESP_LOGI(TAG, "Horario fin guardada: %02d:%02d", h_fin, m_fin);
                            } else {
                                multiplier_cursor = 1;
                                scratch_min = m_fin;
                                scratch_hor = h_fin;
                            }
                        default:
                            break;
                    }
                    break;
                case 0xDF:
                    break; 
                case 0xBF:
                    break;
                case 0x7F:
                    break;
            }
            prev_nivel = nivel;
        }
        blink++;
        if ( blink >= 24 )
            blink = 0;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void sh1106_splash_screen(sh1106_t *oled) {
        sh1106_clear_buffer(oled);
        sh1106_draw_utn_logo(oled, 56, 10);
        sh1106_draw_text(oled, "    UTN FRBA", 0, 30);
        sh1106_draw_text(oled, "Proy. Final 2025", 0, 40);
        sh1106_draw_text(oled, "Andrenacci-Carra", 0, 50);
        ESP_ERROR_CHECK(sh1106_refresh(oled));
}