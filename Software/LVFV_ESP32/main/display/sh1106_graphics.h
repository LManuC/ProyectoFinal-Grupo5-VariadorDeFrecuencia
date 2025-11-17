/**
 * @file sh1106_graphics.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones que permiten operar sobre el display
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __SH1106_GRAPHICS_H__

#define __SH1106_GRAPHICS_H__

typedef struct sh1106_t{
    uint8_t width;
    uint8_t height;
    uint8_t rotation;
    uint8_t buffer[128 * 8]; // 128 × 64 píxeles, 8 páginas de 8 pixeles verticales
} sh1106_t;

/**
 * @fn void sh1106_draw_text( sh1106_t *oled, const char *text, int x, int y, uint8_t size);
 *
 * @brief
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 * @param[in] x
 *      Coordenada en x donde se desea diujar el texto de caracteres alfanuméricos
 *
 * @param[in] y
 *      Coordenada en y donde se desea diujar el texto de caracteres alfanuméricos
 *
 * @param[in] size
 *      Largo del texto que se desea imprimir expresado en caracteres
 */
void sh1106_draw_text( sh1106_t *oled, const char *text, int x, int y, uint8_t size);
/**
 * @fn void sh1106_draw_utn_logo( sh1106_t *oled, int x, int y);
 *
 * @brief
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 * @param[in] x
 *      Coordenada en x donde se desea diujar el texto de caracteres alfanuméricos
 *
 * @param[in] y
 *      Coordenada en y donde se desea diujar el texto de caracteres alfanuméricos
 *
 */
void sh1106_draw_utn_logo( sh1106_t *oled, int x, int y);
/**
 * @fn void sh1106_draw_arrow( sh1106_t *oled, int x, int y);
 *
 * @brief
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 * @param[in] x
 *      Coordenada en x donde se desea diujar el logo de la UTN
 *
 * @param[in] y
 *      Coordenada en y donde se desea diujar el logo de la UTN
 *
 */
void sh1106_draw_arrow( sh1106_t *oled, int x, int y);
/**
 * @fn void sh1106_draw_fail( sh1106_t *oled );
 *
 * @brief Función que dibuja un signo de exclamación en la esquina superior derecha de la pantalla para expresar una falla en el sistema
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 */
void sh1106_draw_fail( sh1106_t *oled );
/**
 * @fn void sh1106_draw_line( sh1106_t *oled, int x, int y);
 *
 * @brief
 *
 * @param[out] oled
 *      Puntero a la estructura que con la información que se enviará al display
 *
 * @param[in] x
 *      Coordenada en x donde se desea diujar una línea del ancho del display
 *
 * @param[in] y
 *      Coordenada en y donde se desea diujar una línea del ancho del display
 *
 */
void sh1106_draw_line( sh1106_t *oled, int x, int y);

#endif