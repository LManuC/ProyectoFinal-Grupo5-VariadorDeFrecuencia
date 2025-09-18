// #include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "sh1106.h"

#define I2C_NUM         I2C_NUM_0
#define SH1106_ADDR     0x3C  // puede variar: 0x3C o 0x3D
#define CMD_CONTROL     0x00  // control byte para comandos
#define DATA_CONTROL    0x40  // control byte para datos

esp_err_t sh1106_write_command(uint8_t cmd) {
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
    i2c_master_write_byte(cmdh, (SH1106_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmdh, CMD_CONTROL, true);
    i2c_master_write_byte(cmdh, cmd, true);
    i2c_master_stop(cmdh);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmdh, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmdh);
    return ret;
}

esp_err_t sh1106_write_data(const uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmdh = i2c_cmd_link_create();
    i2c_master_start(cmdh);
    i2c_master_write_byte(cmdh, (SH1106_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmdh, DATA_CONTROL, true);
    i2c_master_write(cmdh, data, len, true);
    i2c_master_stop(cmdh);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmdh, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmdh);
    return ret;
}
