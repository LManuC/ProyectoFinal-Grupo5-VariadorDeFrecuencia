/**
 * @file sh1106_graphics.c
 * @author Andrenacci - Carra
 * @brief Funciones que permiten operar sobre el display
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include <string.h>
#include "esp_err.h"
#include "./sh1106_graphics.h"
#include "../LVFV_system.h"

static const unsigned char logo16_utn_bmp[] =       {0x71,0x8E,0x71,0x8E,0x79,0x9E,0x3D,0xBC,0x1F,0xF8,0x07,0xE0,0x07,0xE0,0x1F,0xF8,0x3D,0xBC,0x79,0x9E,0x71,0x8E,0x71,0x8E};                      // Logo de la UTN 16 x 12
static const unsigned char fail_bmp[] =             {0x03,0xC0,0x0C,0x30,0x33,0xD8,0x6E,0x6C,0xDC,0x06,0xDE,0x03,0xCF,0xE3,0xC7,0xF3,0xC0,0x7B,0x60,0x76,0x36,0x6C,0x1B,0xD8,0x0C,0x30,0x03,0xC0 }; // Icono de fail 24 x 26
static const unsigned char slash[] =                {0x06,0x0E,0x1C,0x38,0x70,0xE0,0xC0};                                                                                                           // Slash para separar miles 7 x 7
static const unsigned char line_bmp[] =             {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};                                                              // Línea horizontal 16 x 8
static const unsigned char arrow_bmp[] =            {0x00,0x20,0x00,0x30,0x00,0x38,0x1F,0xFC,0x00,0x38,0x00,0x30,0x00,0x20};                                                                        // Flecha hacia la derecha 15 x 7
static const uint8_t font5x7_space[7] =             {0x00,0x00,0x00,0x00,0x00,0x00,0x00};                                                                                                           // Espacio 5 x 7
static const uint8_t font8x14_space[14] =           {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};                                                                        // Espacio 8 x 14
static const uint8_t font5x7_double_dot[7] =        {0x18,0x18,0x00,0x00,0x18,0x18,0x00};                                                                                                           // Dos puntos 5 x 7
static const uint8_t font8x14_double_dot[14] =      {0x00,0x38,0x38,0x38,0x00,0x00,0x00,0x00,0x00,0x38,0x38,0x38,0x00,0x00};                                                                        // Dos puntos 8 x 14
static const uint8_t font5x7_dot[7] =               {0x00,0x00,0x00,0x00,0x00,0x18,0x18};                                                                                                           // Punto 5 x 7
static const uint8_t font8x14_dot[14] =             {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x38};                                                                        // Punto 8 x 14
static const uint8_t font5x7_comma[7] =             {0x00,0x00,0x00,0x00,0x18,0x18,0x30};                                                                                                           // Coma 5 x 7
static const uint8_t font8x14_comma[14] =           {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x38,0x70};                                                                        // Coma 8 x 14
static const uint8_t font5x7_close_quest[7] =       {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18};                                                                                                           // Signo de interrogación cerrado 5 x 7
static const uint8_t font8x14_close_quest[14] =     {0x3C,0x3C,0x66,0x66,0x06,0x06,0x0C,0x0C,0x18,0x18,0x00,0x00,0x18,0x18};                                                                        // Signo de interrogación cerrado 8 x 14
static const uint8_t font5x7_close_excl[7] =        {0x18,0x18,0x18,0x18,0x18,0x00,0x18};                                                                                                           // Signo de exclamación cerrado 5 x 7
static const uint8_t font8x14_close_excl[14] =      {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18};                                                                        // Signo de exclamación cerrado 8 x 14
static const uint8_t font5x7_minus[7] =             {0x00,0x00,0x00,0x7E,0x00,0x00,0x00};                                                                                                           // Menos 5 x 7
static const uint8_t font8x14_minus[14] =           {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00};                                                                        // Menos 8 x 14
static const uint8_t font5x7_rowmajor[][7] = {                                                                                                                                                      // Alfabeto mayúsculas 5 x 7
    // A-Z
    {0x1E,0x33,0x33,0x3F,0x33,0x33,0x33},                                                                                                                                                           // A
    {0x3E,0x33,0x33,0x3E,0x33,0x33,0x3E},                                                                                                                                                           // B
    {0x1E,0x33,0x30,0x30,0x30,0x33,0x1E},                                                                                                                                                           // C
    {0x3C,0x36,0x33,0x33,0x33,0x36,0x3C},                                                                                                                                                           // D
    {0x3F,0x30,0x30,0x3E,0x30,0x30,0x3F},                                                                                                                                                           // E
    {0x3F,0x30,0x30,0x3E,0x30,0x30,0x30},                                                                                                                                                           // F
    {0x1E,0x33,0x30,0x37,0x33,0x33,0x1F},                                                                                                                                                           // G
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33},                                                                                                                                                           // H
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E},                                                                                                                                                           // I
    {0x0F,0x06,0x06,0x06,0x06,0x36,0x1C},                                                                                                                                                           // J
    {0x33,0x36,0x3C,0x38,0x3C,0x36,0x33},                                                                                                                                                           // K
    {0x30,0x30,0x30,0x30,0x30,0x30,0x3F},                                                                                                                                                           // L
    {0x33,0x3F,0x3F,0x33,0x33,0x33,0x33},                                                                                                                                                           // M
    {0x33,0x3B,0x3F,0x37,0x33,0x33,0x33},                                                                                                                                                           // N
    {0x1E,0x33,0x33,0x33,0x33,0x33,0x1E},                                                                                                                                                           // O
    {0x3E,0x33,0x33,0x3E,0x30,0x30,0x30},                                                                                                                                                           // P
    {0x1E,0x33,0x33,0x33,0x37,0x36,0x1D},                                                                                                                                                           // Q
    {0x3E,0x33,0x33,0x3E,0x3C,0x36,0x33},                                                                                                                                                           // R
    {0x1E,0x33,0x30,0x1E,0x03,0x33,0x1E},                                                                                                                                                           // S
    {0x3F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C},                                                                                                                                                           // T
    {0x33,0x33,0x33,0x33,0x33,0x33,0x1E},                                                                                                                                                           // U
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C},                                                                                                                                                           // V
    {0x33,0x33,0x33,0x33,0x3F,0x3F,0x33},                                                                                                                                                           // W
    {0x33,0x33,0x1E,0x0C,0x1E,0x33,0x33},                                                                                                                                                           // X
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x0C},                                                                                                                                                           // Y
    {0x3F,0x03,0x06,0x0C,0x18,0x30,0x3F}                                                                                                                                                            // Z
};
static const uint8_t font5x7_rowminor[][7] = {                                                                                                                                                      // Alfabeto minúsculas 5 x 7
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E},                                                                                                                                                           // a
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C},                                                                                                                                                           // b
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C},                                                                                                                                                           // c
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E},                                                                                                                                                           // d
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C},                                                                                                                                                           // e
    {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30},                                                                                                                                                           // f
    {0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C},                                                                                                                                                           // g
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66},                                                                                                                                                           // h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C},                                                                                                                                                           // i
    {0x06,0x00,0x06,0x06,0x66,0x66,0x3C},                                                                                                                                                           // j
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66},                                                                                                                                                           // k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C},                                                                                                                                                           // l
    {0x00,0x00,0x6C,0x7E,0x7E,0x6C,0x6C},                                                                                                                                                           // m
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66},                                                                                                                                                           // n
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C},                                                                                                                                                           // o
    {0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},                                                                                                                                                           // p
    {0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},                                                                                                                                                           // q
    {0x00,0x00,0x6C,0x76,0x60,0x60,0x60},                                                                                                                                                           // r
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C},                                                                                                                                                           // s
    {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C},                                                                                                                                                           // t
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E},                                                                                                                                                           // u
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18},                                                                                                                                                           // v
    {0x00,0x00,0x66,0x66,0x7E,0x7E,0x6C},                                                                                                                                                           // w
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66},                                                                                                                                                           // x
    {0x00,0x66,0x66,0x66,0x3E,0x06,0x7C},                                                                                                                                                           // y
    {0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}                                                                                                                                                            // z
};
static const uint8_t font5x7_rownumber[][7] = {                                                                                                                                                     // Números 5 x 7
    {0x1E,0x33,0x37,0x3B,0x33,0x33,0x1E},                                                                                                                                                           // 0
    {0x0C,0x1C,0x0C,0x0C,0x0C,0x0C,0x1E},                                                                                                                                                           // 1
    {0x1E,0x33,0x03,0x0E,0x18,0x30,0x3F},                                                                                                                                                           // 2
    {0x1E,0x33,0x03,0x0E,0x03,0x33,0x1E},                                                                                                                                                           // 3
    {0x06,0x0E,0x1E,0x36,0x3F,0x06,0x06},                                                                                                                                                           // 4
    {0x3F,0x30,0x3E,0x03,0x03,0x33,0x1E},                                                                                                                                                           // 5
    {0x1E,0x30,0x3E,0x33,0x33,0x33,0x1E},                                                                                                                                                           // 6
    {0x3F,0x03,0x06,0x0C,0x18,0x18,0x18},                                                                                                                                                           // 7
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E},                                                                                                                                                           // 8
    {0x1E,0x33,0x33,0x1F,0x03,0x03,0x1E}                                                                                                                                                            // 9
};
static const uint8_t font8x14_rowmajor[][14] = {                                                                                                                                                    // Alfabeto mayúsculas 8 x 14
    {0x1E,0x1E,0x33,0x33,0x33,0x33,0x3F,0x3F,0x33,0x33,0x33,0x33,0x33,0x33},                                                                                                                        // A
    {0x3E,0x3E,0x33,0x33,0x33,0x33,0x3E,0x3E,0x33,0x33,0x33,0x33,0x3E,0x3E},                                                                                                                        // B
    {0x1E,0x1E,0x33,0x33,0x30,0x30,0x30,0x30,0x30,0x30,0x33,0x33,0x1E,0x1E},                                                                                                                        // C
    {0x3C,0x3C,0x36,0x36,0x33,0x33,0x33,0x33,0x33,0x33,0x36,0x36,0x3C,0x3C},                                                                                                                        // D
    {0x3F,0x3F,0x30,0x30,0x30,0x30,0x3E,0x3E,0x30,0x30,0x30,0x30,0x3F,0x3F},                                                                                                                        // E
    {0x3F,0x3F,0x30,0x30,0x30,0x30,0x3E,0x3E,0x30,0x30,0x30,0x30,0x30,0x30},                                                                                                                        // F
    {0x1E,0x1E,0x33,0x33,0x30,0x30,0x37,0x37,0x33,0x33,0x33,0x33,0x1F,0x1F},                                                                                                                        // G
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x3F,0x33,0x33,0x33,0x33,0x33,0x33},                                                                                                                        // H
    {0x1E,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x1E},                                                                                                                        // I
    {0x0F,0x0F,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x36,0x36,0x1C,0x1C},                                                                                                                        // J
    {0x33,0x33,0x36,0x36,0x3C,0x3C,0x38,0x38,0x3C,0x3C,0x36,0x36,0x33,0x33},                                                                                                                        // K
    {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3F,0x3F},                                                                                                                        // L
    {0x33,0x33,0x3F,0x3F,0x3F,0x3F,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33},                                                                                                                        // M
    {0x33,0x33,0x3B,0x3B,0x3F,0x3F,0x37,0x37,0x33,0x33,0x33,0x33,0x33,0x33},                                                                                                                        // N
    {0x1E,0x1E,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x1E,0x1E},                                                                                                                        // O
    {0x3E,0x3E,0x33,0x33,0x33,0x33,0x3E,0x3E,0x30,0x30,0x30,0x30,0x30,0x30},                                                                                                                        // P
    {0x1E,0x1E,0x33,0x33,0x33,0x33,0x33,0x33,0x37,0x37,0x36,0x36,0x1D,0x1D},                                                                                                                        // Q
    {0x3E,0x3E,0x33,0x33,0x33,0x33,0x3E,0x3E,0x3C,0x3C,0x36,0x36,0x33,0x33},                                                                                                                        // R
    {0x1E,0x1E,0x33,0x33,0x30,0x30,0x1E,0x1E,0x03,0x03,0x33,0x33,0x1E,0x1E},                                                                                                                        // S
    {0x3F,0x3F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C},                                                                                                                        // T
    {0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x1E,0x1E},                                                                                                                        // U
    {0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x1E,0x1E,0x0C,0x0C},                                                                                                                        // V
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x3F,0x3F,0x3F,0x33,0x33,0x33,0x33},                                                                                                                        // W
    {0x33,0x33,0x33,0x33,0x1E,0x1E,0x0C,0x0C,0x1E,0x1E,0x33,0x33,0x33,0x33},                                                                                                                        // X
    {0x33,0x33,0x33,0x33,0x1E,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C},                                                                                                                        // Y
    {0x3F,0x3F,0x03,0x03,0x06,0x06,0x0C,0x0C,0x18,0x18,0x30,0x30,0x3F,0x3F},                                                                                                                        // Z
};
static const uint8_t font8x14_rowminor[][14] = {                                                                                                                                                    // Alfabeto minúsculas 8 x 14
    {0x00,0x00,0x00,0x00,0x3C,0x3C,0x06,0x06,0x3E,0x3E,0x66,0x66,0x3E,0x3E},                                                                                                                        // a
    {0x60,0x60,0x60,0x60,0x7C,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x7C,0x7C},                                                                                                                        // b
    {0x00,0x00,0x00,0x00,0x3C,0x3C,0x66,0x66,0x60,0x60,0x66,0x66,0x3C,0x3C},                                                                                                                        // c
    {0x06,0x06,0x06,0x06,0x3E,0x3E,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x3E},                                                                                                                        // d
    {0x00,0x00,0x00,0x00,0x3C,0x3C,0x66,0x66,0x7E,0x7E,0x60,0x60,0x3C,0x3C},                                                                                                                        // e
    {0x1C,0x1C,0x30,0x30,0x30,0x30,0x7C,0x7C,0x30,0x30,0x30,0x30,0x30,0x30},                                                                                                                        // f
    {0x00,0x00,0x3E,0x3E,0x66,0x66,0x66,0x66,0x3E,0x3E,0x06,0x06,0x7C,0x7C},                                                                                                                        // g
    {0x60,0x60,0x60,0x60,0x7C,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66},                                                                                                                        // h
    {0x18,0x18,0x00,0x00,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x3C},                                                                                                                        // i
    {0x06,0x06,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x3C},                                                                                                                        // j
    {0x60,0x60,0x60,0x60,0x66,0x66,0x6C,0x6C,0x78,0x78,0x6C,0x6C,0x66,0x66},                                                                                                                        // k
    {0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x3C},                                                                                                                        // l
    {0x00,0x00,0x00,0x00,0x6C,0x6C,0x7E,0x7E,0x7E,0x7E,0x6C,0x6C,0x6C,0x6C},                                                                                                                        // m
    {0x00,0x00,0x00,0x00,0x7C,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66},                                                                                                                        // n
    {0x00,0x00,0x00,0x00,0x3C,0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x3C},                                                                                                                        // o
    {0x00,0x00,0x7C,0x7C,0x66,0x66,0x66,0x66,0x7C,0x7C,0x60,0x60,0x60,0x60},                                                                                                                        // p
    {0x00,0x00,0x3E,0x3E,0x66,0x66,0x66,0x66,0x3E,0x3E,0x06,0x06,0x06,0x06},                                                                                                                        // q
    {0x00,0x00,0x00,0x00,0x6C,0x6C,0x76,0x76,0x60,0x60,0x60,0x60,0x60,0x60},                                                                                                                        // r
    {0x00,0x00,0x00,0x00,0x3E,0x3E,0x60,0x60,0x3C,0x3C,0x06,0x06,0x7C,0x7C},                                                                                                                        // s
    {0x30,0x30,0x30,0x30,0x7C,0x7C,0x30,0x30,0x30,0x30,0x30,0x30,0x1C,0x1C},                                                                                                                        // t
    {0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x3E},                                                                                                                        // u
    {0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x3C,0x18,0x18},                                                                                                                        // v
    {0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x7E,0x7E,0x7E,0x7E,0x6C,0x6C},                                                                                                                        // w
    {0x00,0x00,0x00,0x00,0x66,0x66,0x3C,0x3C,0x18,0x18,0x3C,0x3C,0x66,0x66},                                                                                                                        // x
    {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x3E,0x06,0x06,0x7C,0x7C},                                                                                                                        // y
    {0x00,0x00,0x00,0x00,0x7E,0x7E,0x0C,0x0C,0x18,0x18,0x30,0x30,0x7E,0x7E},                                                                                                                        // z
};
static const uint8_t font8x14_rownumber[][14] = {                                                                                                                                                   // Números 8 x 14
    {0x3C,0x66,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0x66,0x3C,0x00},                                                                                                                        // 0
    {0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},                                                                                                                        // 1
    {0x3C,0x66,0xC3,0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0xC3,0xFF,0xFE,0x00},                                                                                                                        // 2
    {0x7E,0xC3,0x03,0x03,0x06,0x3C,0x06,0x03,0x03,0x03,0xC3,0x66,0x3C,0x00},                                                                                                                        // 3
    {0x06,0x0E,0x1E,0x36,0x66,0xC6,0x86,0xFF,0xFF,0x06,0x06,0x06,0x06,0x00},                                                                                                                        // 4
    {0xFE,0xC0,0xC0,0xC0,0xFC,0xC6,0x03,0x03,0x03,0x03,0xC3,0x66,0x3C,0x00},                                                                                                                        // 5
    {0x3C,0x66,0xC3,0xC0,0xC0,0xFC,0xC6,0xC3,0xC3,0xC3,0xC3,0x66,0x3C,0x00},                                                                                                                        // 6
    {0xFF,0xC3,0x03,0x06,0x06,0x0C,0x18,0x18,0x30,0x30,0x60,0x60,0x60,0x00},                                                                                                                        // 7
    {0x3C,0x66,0xC3,0xC3,0xC3,0x66,0x3C,0x66,0xC3,0xC3,0xC3,0x66,0x3C,0x00},                                                                                                                        // 8
    {0x3C,0x66,0xC3,0xC3,0xC3,0x67,0x3F,0x03,0x03,0x03,0xC3,0x66,0x3C,0x00}                                                                                                                         // 9
};

/**
 * @struct bitmap_t
 *
 * @brief Estructura que representa un caracter cualquiera. Donde de le debe inicializar un puntero a alguno de los caracteres, el alto, ancho, y su posición en x e y
 */
typedef struct bitmap_t {
    const uint8_t *bitmap;          /*!< Puntero al caracter */
    uint8_t w;                      /*!< Ancho del caracter */
    uint8_t h;                      /*!< Alto del caracter */
    int x;                          /*!< Posición en X del caracter */
    int y;                          /*!< Posición en Y del caracter */
} bitmap_t;

/**
 * @fn static void sh1106_draw_bitmap( sh1106_t *oled, bitmap_t *bitmap);
 *
 * @brief La función escribe en el bitmap de la pantalla los caracteres que el usuario desea imprimir
 *
 * @details Copiando bit a bit el buffer en las posiciones x e y indicadas
 *
 * @param[out] oled
 *      Puntero a la estructura que representa todo lo que se enviará al display
 *
 * @param[in] bitmap
 *      Puntero a la estructura del caracter que se desea escribir en el display
 */
static void sh1106_draw_bitmap( sh1106_t *oled, bitmap_t *bitmap);

static void sh1106_draw_bitmap( sh1106_t *oled, bitmap_t *bitmap) {
    // Dibujado sencillo: copia bit por bit en el buffer
    // (Implementar según formato del bitmap)
    for (uint8_t by = 0; by < bitmap->h; by++) {
        for (uint8_t bx = 0; bx < bitmap->w; bx++) {
            uint8_t byte = bitmap->bitmap[by * ((bitmap->w + 7) / 8) + (bx / 8)];
            uint8_t bit = (byte >> (7 - (bx % 8))) & 0x01;
            if (bit) {
                int px = bitmap->x + bx;
                int py = bitmap->y + by;
                if (px >= 0 && px < oled->width && py >= 0 && py < oled->height) {
                    uint8_t page = py / 8;
                    uint8_t bitpos = py % 8;
                    oled->buffer[page * oled->width + px] |= (1 << bitpos);
                }
            }
        }
    }
}

void sh1106_draw_text( sh1106_t *oled, const char *text, int x, int y, uint8_t size) {
    uint8_t y_diff = 0;
    bitmap_t bitmap;
    if (size == SH1106_SIZE_1) {
        bitmap.w = 8;
        bitmap.h = 7;
    } else if ( size == SH1106_SIZE_2 ) {
        bitmap.w = 8;
        bitmap.h = 14;
    } else {
        return;
    }
    bitmap.x = x;
    bitmap.y = y;
    for(uint8_t x_diff = 0, letter = 0; letter < strlen(text); x_diff++, letter++) {

        if (x + 8 * x_diff > 120) {
            x_diff = 0;
            y_diff += 9;
        }
        bitmap.x = x + 8 * x_diff;
        bitmap.y = y + y_diff;

        if ( text[letter] >= 'A' && text[letter] <= 'Z') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_rowmajor[text[letter] - 'A'];
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_rowmajor[text[letter] - 'A'];
            }
        } else if ( text[letter] >= 'a' && text[letter] <= 'z') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_rowminor[text[letter] - 'a'];
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_rowminor[text[letter] - 'a'];
            }
        } else if ( text[letter] >= '0' && text[letter] <= '9') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_rownumber[text[letter] - '0'];
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_rownumber[text[letter] - '0'];
            }
        } else if ( text[letter] == ':') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_double_dot;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_double_dot;
            }
        } else if ( text[letter] == '.') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_dot;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_dot;
            }
        } else if ( text[letter] == ',') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_comma;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_comma;
            }
        } else if ( text[letter] == '?') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_close_quest;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_close_quest;
            }
        } else if ( text[letter] == '!') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_close_excl;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_close_excl;
            }
        } else if ( text[letter] == '-') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_minus;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_minus;
            }
        } else if ( text[letter] == '/') {
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = slash;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_space;         // No existe caracter '/' en tamaño 2
            }
        } else {                        // Si es un espacio u otro caracter no soportado
            if ( size == SH1106_SIZE_1 ) {
                bitmap.bitmap = font5x7_space;
            } else if ( size == SH1106_SIZE_2 ) {
                bitmap.bitmap = font8x14_space;
            }
        }
        sh1106_draw_bitmap( oled, &bitmap);
    }
}

void sh1106_draw_utn_logo( sh1106_t *oled, int x, int y) {
    bitmap_t bitmap = {
        .bitmap = logo16_utn_bmp,
        .w = 16,
        .h = 12,
        .x = x,
        .y = y
    };
    sh1106_draw_bitmap( oled, &bitmap);
}

void sh1106_draw_arrow( sh1106_t *oled, int x, int y) {
    bitmap_t bitmap = {
        .bitmap = arrow_bmp,
        .w = 16,
        .h = 7,
        .x = x,
        .y = y
    };
    sh1106_draw_bitmap( oled, &bitmap);
}

void sh1106_draw_fail( sh1106_t *oled ) {
    bitmap_t bitmap = {
        .bitmap = fail_bmp,
        .w = 16,
        .h = 14,
        .x = 109,
        .y = 1
    };
    sh1106_draw_bitmap( oled, &bitmap);
}

void sh1106_draw_line( sh1106_t *oled, int x, int y) {
    bitmap_t bitmap = {
        .bitmap = line_bmp,
        .w = 128,
        .h = 1,
        .x = x,
        .y = y
    };
    sh1106_draw_bitmap( oled, &bitmap);
}