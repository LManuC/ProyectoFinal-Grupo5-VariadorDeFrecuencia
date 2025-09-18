#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t rotation;
    uint8_t buffer[128 * 8]; // 128 × 64 píxeles, 8 páginas de 8 pixeles verticales
} sh1106_t;

// Inicializa el bus I²C y el display
esp_err_t sh1106_init(sh1106_t *oled);

// Limpia el buffer interno (no refresca aún la pantalla)
void sh1106_clear_buffer(sh1106_t *oled);

// Manda el buffer a la pantalla (refresco)
esp_err_t sh1106_refresh(sh1106_t *oled);

// Dibuja un bitmap monocromo (1 = pixel encendido, 0 = pixel apagado)
void sh1106_draw_bitmap(sh1106_t *oled, const uint8_t *bitmap,
                        uint8_t w, uint8_t h, int x, int y);

// Dibuja un texto simple con fuente monoespaciada (placeholder)
void sh1106_draw_text(sh1106_t *oled, const char *text,
                      int x, int y);

void sh1106_draw_utn_logo(sh1106_t *oled, int x, int y);
void sh1106_draw_arrow(sh1106_t *oled, int x, int y);
void sh1106_draw_fail(sh1106_t *oled, int x, int y);
void sh1106_draw_line(sh1106_t *oled, int x, int y);
void sh1106_print_hour(sh1106_t *oled, const char *text);
void sh1106_draw_number(sh1106_t *oled, uint16_t value, int x, int y, uint8_t dots);

#endif // SH1106_H
