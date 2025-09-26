
#ifndef __ESP32_H__

#define __ESP32_H__

#define GPIO_INPUT_IO_1                 1
#define GPIO_INPUT_IO_2                 2
#define GPIO_INPUT_IO_3                 3
#define GPIO_INPUT_IO_4                 4
#define GPIO_INPUT_IO_5                 5
#define GPIO_INPUT_IO_6                 6
#define GPIO_INPUT_IO_7                 7
#define GPIO_INPUT_IO_8                 8
#define GPIO_INPUT_IO_9                 9
#define GPIO_INPUT_IO_10                10
#define GPIO_INPUT_IO_11                11
#define GPIO_INPUT_IO_12                12
#define GPIO_INPUT_IO_13                13
#define GPIO_INPUT_IO_14                14
#define GPIO_INPUT_IO_15                15
#define GPIO_INPUT_IO_16                16
#define GPIO_INPUT_IO_17                17
#define GPIO_INPUT_IO_18                18
#define GPIO_INPUT_IO_19                19
#define GPIO_INPUT_IO_20                20
#define GPIO_INPUT_IO_21                21
#define GPIO_INPUT_IO_22                22
#define GPIO_INPUT_IO_23                23
#define GPIO_INPUT_IO_24                24
#define GPIO_INPUT_IO_25                25
#define GPIO_INPUT_IO_26                26
#define GPIO_INPUT_IO_27                27
#define GPIO_INPUT_IO_28                28
#define GPIO_INPUT_IO_29                29
#define GPIO_INPUT_IO_30                30
#define GPIO_INPUT_IO_31                31
#define GPIO_INPUT_IO_32                32
#define GPIO_INPUT_IO_33                33
#define GPIO_INPUT_IO_34                34
#define GPIO_INPUT_IO_35                35

#define ESP_INTR_FLAG_DEFAULT           0

typedef enum {
    CMD_CAMBIO_VELOCIDAD = 0,
    CMD_ARRANQUE,
    CMD_ARRANQUE_PROGRAMADO,
    CMD_PARADA_EMERGENCIA,
    CMD_PARADA_ROTAURA,
    CMD_PARADA_ROTAURA_LOW_VOLTAGE,
    CMD_PARADA_ROTAURA_HIGH_VOLTAGE,
    CMD_PARADA_ROTAURA_LOW_CURRENT,
    CMD_PARADA_ROTAURA_HIGH_CURRENT,
    CMD_PARADA_ROTAURA_HIGH_TEMPERATURE
} comando_t;

typedef union {
    uint8_t all;
    struct {
        uint8_t start_btn: 1;
        uint8_t stop_btn: 1;
        uint8_t termo_sw: 1;
        uint8_t current_sens: 1;
        uint8_t voltage_sens: 1;
        uint8_t reserved: 3;
    } bits;
} status_t;

#endif