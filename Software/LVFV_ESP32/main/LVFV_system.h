/**
 * @file LVFV_system.h
 * @author Andrenacci - Carra
 * @brief Declaraciones de estructuras, variables, definiciones y estados generales del sistema utilizados en varios de los archivos.
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef LVFV_SYSTEM_H
#define LVFV_SYSTEM_H

#include <inttypes.h>
#include <time.h>

#define LINE_INCREMENT                  9

#define VARIABLE_FIRST                  20
#define VARIABLE_SECOND                 29
#define VARIABLE_THIRD                  38
#define VARIABLE_FOURTH                 47
#define VARIABLE_FIFTH                  56
#define VARIABLE_SIXTH                  65
#define VARIABLE_SEVENTH                74
#define VARIABLE_EIGTH                  83

/**    
 * @enum systemSignal_e
 *
 * @brief Variable dedicada a la identificación del tipo de señal que es recibida por las entradas digitales y analógicas del sistema para que se tome una desición a nivel de sistema
 */
typedef enum {
    BUTTON_MENU = 1,
    BUTTON_OK,
    BUTTON_BACK,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_SAVE,
    EMERGENCI_STOP_PRESSED,
    EMERGENCI_STOP_RELEASED,
    START_PRESSED,
    START_RELEASED,
    TERMO_SW_PRESSED,
    TERMO_SW_RELEASED,
    STOP_PRESSED,
    STOP_RELEASED,
    SPEED_SELECTOR_0,
    SPEED_SELECTOR_1,
    SPEED_SELECTOR_2,
    SPEED_SELECTOR_3,
    SPEED_SELECTOR_4,
    SPEED_SELECTOR_5,
    SPEED_SELECTOR_6,
    SPEED_SELECTOR_7,
    SPEED_SELECTOR_8,
    SPEED_SELECTOR_9,
    SECURITY_EXCEDED,
    SECURITY_OK
} systemSignal_e;

/**
 * @enum system_status_e
 *
 * @brief Posibles estados general que puede tomar el sistema
 */
typedef enum system_status_e {
    SYSTEM_IDLE,
    SYSTEM_ACCLE_DESACCEL,
    SYSTEM_REGIME,
    SYSTEM_BREAKING,
    SYSTEM_EMERGENCY,
    SYSTEM_EMERGENCY_SENT,
    SYSTEM_EMERGENCY_OK
} system_status_e;

typedef enum sh1106_variable_lines_e{
    first = VARIABLE_FIRST,
    second = VARIABLE_SECOND,
    third = VARIABLE_THIRD,
    fourth = VARIABLE_FOURTH,   
    fifth = VARIABLE_FIFTH, 
    sixth = VARIABLE_SIXTH,
    seventh = VARIABLE_SEVENTH
} sh1106_variable_lines_e;

typedef struct frequency_settings_t {
    uint16_t freq_regime;
    uint16_t acceleration;
    uint16_t desacceleration;
    uint16_t input_variable;
} frequency_settings_t;

typedef struct time_settings_t {
    struct tm *time_system;
    struct tm *time_start;
    struct tm *time_stop;
} time_settings_t;

typedef struct seccurity_settings_t {
    uint16_t vbus_min;
    uint16_t ibus_max;
} seccurity_settings_t;

/**
 * @struct system_status_t
 *
 * @brief Estructura con las variables necesarias para establecer las condiciones de trabajo del motor
 */
typedef struct system_status_t {
    uint16_t frequency;                                     // Frecuencia actual de giro del motor
    uint16_t frequency_destiny;                             // Frecuencia a la que terminará funcionando el motor
    uint16_t vbus_min;                                      // Tensión mínima del bus de contínua en la que será una condición buena
    uint16_t ibus_max;                                      // Corriente máxima del bus de contínua en la que será una condición buena
    uint16_t acceleration;                                  // Velocidad de aceleración del motor configurada por el usuario
    uint16_t desacceleration;                               // Velocidad de desaceleración del motor configurada por el usuario
    uint8_t inputs_status;                                  // Índice de entrada aislada seleccionado para la variación de velocidad
    system_status_e status;                                 // Estado general del sistema
} system_status_t;

/*
 * Definiciones para display 
 */
#define SH1106_SIZE_1           0
#define SH1106_SIZE_2           1

typedef struct frequency_settings_SH1106_t {
    frequency_settings_t frequency_settings;
    sh1106_variable_lines_e *edit;
    uint8_t edit_variable;
    uint8_t *edit_flag;
    uint8_t *multiplier;
} frequency_settings_SH1106_t;

typedef struct time_settings_SH1106_t {
    time_settings_t time_settings;
    sh1106_variable_lines_e *edit;
    uint8_t edit_variable;
    uint8_t *edit_flag;
    uint8_t *multiplier;
} time_settings_SH1106_t;

typedef struct seccurity_settings_SH1106_t {
    seccurity_settings_t seccurity_settings;
    sh1106_variable_lines_e *edit;
    uint8_t edit_variable;
    uint8_t *edit_flag;
    uint8_t *multiplier;
} seccurity_settings_SH1106_t;

#endif