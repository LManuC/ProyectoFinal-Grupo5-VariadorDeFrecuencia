#ifndef SH1106_GRAPHICS_H
#define SH1106_GRAPHICS_H

#include "./display_typedef.h"

void sh1106_draw_text( sh1106_t *oled, const char *text, int x, int y, uint8_t size);
void sh1106_draw_utn_logo( sh1106_t *oled, int x, int y);
void sh1106_draw_arrow( sh1106_t *oled, int x, int y);
void sh1106_draw_fail( sh1106_t *oled );
void sh1106_draw_line( sh1106_t *oled, int x, int y);
void sh1106_draw_number( sh1106_t *oled, uint16_t value, int x, int y, uint8_t size);

#endif