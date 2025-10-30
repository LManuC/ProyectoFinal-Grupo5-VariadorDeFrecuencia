#ifndef LVFV_SYSTEM_H
#define LVFV_SYSTEM_H

#include <inttypes.h>
#include <time.h>

        
#define START_BUTTON                    GPIO_NUM_27
#define STOP_BUTTON                     GPIO_NUM_28

#define FREQUENCY_LINEAR_VARIABLE       0
#define FREQUENCY_CUADRATIC_VARIABLE    1

#define LINE_INCREMENT                  9

#define VARIABLE_FIRST                  20
#define VARIABLE_SECOND                 29
#define VARIABLE_THIRD                  38
#define VARIABLE_FOURTH                 47
#define VARIABLE_FIFTH                  56
#define VARIABLE_SIXTH                  65
#define VARIABLE_SEVENTH                74
#define VARIABLE_EIGTH                  83

#define FREC_LINE_INDX                  VARIABLE_FIRST
#define VBUS_LINE_INDX                  VARIABLE_SECOND
#define IBUS_LINE_INDX                  VARIABLE_THIRD
#define HINI_LINE_INDX                  VARIABLE_FOURTH
#define HFIN_LINE_INDX                  VARIABLE_FIFTH

#define EDIT_FREQUENCY_INDX             VARIABLE_FIRST
#define EDIT_ACCELERATION_INDX          VARIABLE_SECOND
#define EDIT_DESACCELERATION_INDX       VARIABLE_THIRD
#define EDIT_INPUT_VARIATION_INDX       VARIABLE_FOURTH

#define EDIT_TIME_LINE_INDX             VARIABLE_SECOND
#define EDIT_HINI_LINE_INDX             VARIABLE_THIRD
#define EDIT_HFIN_LINE_INDX             VARIABLE_FOURTH

#define EDIT_VBUS_LINE_INDX             VARIABLE_SECOND
#define EDIT_IBUS_LINE_INDX             VARIABLE_THIRD

/*
 * Definiciones para sistema 
 */  
typedef enum system_status_e {
    SYSTEM_IDLE,
    SYSTEM_ACCLE_DESACCEL,
    SYSTEM_REGIME,
    SYSTEM_BREAKING,
    SYSTEM_EMERGENCY
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
    struct tm *timeinfo;
    struct tm *time_start;
    struct tm *time_stop;
} time_settings_t;

typedef struct seccurity_settings_t {
    uint16_t vbus_min;
    uint16_t ibus_max;
} seccurity_settings_t;

typedef struct system_status_t {
    uint16_t frequency;
    uint16_t frequency_destiny;
    uint16_t vbus_min;
    uint16_t ibus_max;
    uint8_t inputs_status;
    system_status_e status;
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

typedef struct sh1106_t{
    uint8_t width;
    uint8_t height;
    uint8_t rotation;
    uint8_t buffer[128 * 8]; // 128 × 64 píxeles, 8 páginas de 8 pixeles verticales
} sh1106_t;

/*
 * Variables globales
 */  

extern struct tm timeinfo;
extern struct tm time_start;
extern struct tm time_stop;

extern frequency_settings_t system_frequency_settings;
extern time_settings_t system_time_settings;
extern seccurity_settings_t system_seccurity_settings;

esp_err_t initRTC();
esp_err_t setTime(struct tm *setting_time);
esp_err_t rtc_schedule_alarms();
esp_err_t adc_init_two_channels(void);
void getTime();
void adc_task(void *arg);
void analog_output ( void *arg);

#endif