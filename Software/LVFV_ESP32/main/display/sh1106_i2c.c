/**
 * @file sh1106_i2c.c
 * @author Andrenacci - Carra
 * @brief Funciones de hardware para el puerto I2C
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "./sh1106_i2c.h"

#define I2C_DISPLAY     I2C_NUM_0       // Npumero de puerto I2C utilizado para el display
#define SH1106_ADDR     0x3C            // Addess del display. Puede variar: 0x3C o 0x3D según como se configure el hardware
#define CMD_CONTROL     0x00            // Control byte para comandos
#define DATA_CONTROL    0x40            // Control byte para datos

esp_err_t sh1106_write(const uint8_t *data, size_t len, sh1106_comm_type_t comm_type) {
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
    i2c_master_write_byte(cmdh, (SH1106_ADDR << 1) | I2C_MASTER_WRITE, true);

    if ( comm_type == SH1106_COMM_CMD ) {
        i2c_master_write_byte(cmdh, CMD_CONTROL, true);
        i2c_master_write_byte(cmdh, *data, true);
    } else {
        i2c_master_write_byte(cmdh, DATA_CONTROL, true);
        i2c_master_write(cmdh, data, len, true);
    }
    
    i2c_master_stop(cmdh);
    esp_err_t ret = i2c_master_cmd_begin(I2C_DISPLAY, cmdh, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmdh);
    return ret;
}
