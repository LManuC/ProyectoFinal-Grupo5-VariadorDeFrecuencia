/**
 * @file MCP23017.c
 * @author Andrenacci - Carra
 * @brief MCP23017 es un expansor de entradas y salidas que se controla a través de puerto I2C. 
 * @details Este chip permite usar sus dos puertos de 8 bits por separado o como uno solo de 16 bits. Tiene la función de reportar una interrupción configurable por puerto a través de 2 pines, uno por puerto, que informar si hubo algún cambio en las entradas si así lo deseo; estos pines de interrupción pueden ser configurados también para que actúen en conjunto o separado.
 Para su funcionamiento de direccionamiento tiene dos modos: "Byte mode" o "Sequencial mode" que se configuran en el registro IOCON que, aunque en el mapa de registros esté duplicado, es compartido entre ambos puerto, por lo que acceder por una dirección u otra es indiferente:
    - Byte mode: no incrementa el puntero interno de direcciones en cada comando enviado, sino que en cada escritura/lectura se le debe indicar a que dirección de memoria se desea acceder si no se desea acceder a la última dirección de memoria accedida. Esto permite hacer polling en el mismo registro, con la intención de estar monitoreando contínuamente la/las entradas de interés. Utilizando el modo ICON.BANK = 0, habilita una función especial que permite tooglear entre los registros del puerto A y B secuencialmente en cada acceso al chip.
    - Sequenctial mode: El puntero de dirección interno del chip se incrementa a cada byte de datos enviado. Cuando llega al último registro del mapa de registro hace un rollover al registro 0x00.

La secuencia de escritura se define de la siguiente amnera:
    S 0100AAA0 ADDR DIN P
 * @version 0.1
 * @date 2025-08-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "MCP23017.h"
#include "esp_log.h"

static const char *TAG = "MCP23017";

esp_err_t MCP23017_INIT( void ) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Iniciando modulo");
    MCP23017_IOCON_t iocon;
    MCP23017_GPPU_t gppua;
    MCP23017_IODIR_t iodira, iodirb;
    MCP23017_GPINTEN_t gpintena, gpintenb;

    iocon.bits.INTPOL = __MCP23017_INTPOL_POLARITY_HIGH__;          // Los pines de interrupción se ponen en 1 cuando hay una interrupción
    iocon.bits.ODR = __MCP23017_ODR_ACTIVE_DRIVER_OUTPUT__;         // Los pines de interrupción configuran como salidas activas
    iocon.bits.DISSLW = __MCP23017_DISSLW_ENABLED__;                // Activo el Slew Rate en el SDA para una mejor comunicación
    iocon.bits.SEQOP = __MCP23017_SEQOP_DISABLED__;                 // No activo el incremento del puntero de direcciones para poder leer constantemente el mismo registro si quiero
    iocon.bits.MIRROR = __MCP23017_MIRROR_UNCONNECTED__;            // Los pines de interrupción A y B se controlan por separado
    iocon.bits.BANK = __MCP23017_FUNC_MODE__;                       // Uso el modo Byte para tratar los puertos como si guesen de 8 bits y no de 16

    gppua.bits.PU2 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppua.bits.PU3 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppua.bits.PU4 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppua.bits.PU5 = __MCP23017_GPPU_PULL_UP_ENABLE__;

    iodira.bits.IO0 = __MCP23017_IODIR_OUTPUT__;
    iodira.bits.IO1 = __MCP23017_IODIR_OUTPUT__;
    iodira.bits.IO2 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO3 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO4 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO5 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO6 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO7 = __MCP23017_IODIR_OUTPUT__;

    iodirb.bits.IO0 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO1 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO2 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO3 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO4 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO5 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO6 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO7 = __MCP23017_IODIR_OUTPUT__;

    gpintena.bits.GPINT2 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT3 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT4 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT5 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT6 = __MCP23017_GPINTEN_ENABLE__;

    gpintenb.bits.GPINT2 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT3 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT4 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT5 = __MCP23017_GPINTEN_ENABLE__;

    ret = register_write( (uint8_t) MCP23017_IOCON_REGISTER, iocon.all);
    ret = register_write( (uint8_t) MCP23017_GPPUA_REGISTER, gppua.all);
    ret = register_write( (uint8_t) MCP23017_IODIRA_REGISTER, iodira.all);
    ret = register_write( (uint8_t) MCP23017_IODIRB_REGISTER, iodirb.all);
    ret = register_write( (uint8_t) MCP23017_GPINTENA_REGISTER, gpintena.all);
    ret = register_write( (uint8_t) MCP23017_GPINTENB_REGISTER, gpintenb.all);

    return ret;
}

esp_err_t register_read(uint8_t register_address, uint8_t *data) {
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_WRITE_OPPCODE__,0);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_READ_OPPCODE__,0);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t register_write(uint8_t register_address, uint8_t data) {
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_WRITE_OPPCODE__,0);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}