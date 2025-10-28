#ifndef __SH1106_I2C_H__

#define __SH1106_I2C_H__

esp_err_t sh1106_write_command(uint8_t cmd);
esp_err_t sh1106_write_data(const uint8_t *data, size_t len);

#endif