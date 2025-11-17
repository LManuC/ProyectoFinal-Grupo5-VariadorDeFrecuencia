/**
 * @file sh1106_i2c.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones de hardware para el puerto I2C
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __SH1106_I2C_H__

#define __SH1106_I2C_H__

/**
 * @enum sh1106_comm_type_t
 *
 * @brief Tipo de comunicación I2C con el display SH1106: Comando o Dato
 */
typedef enum {
    SH1106_COMM_CMD = 0,
    SH1106_COMM_DATA
} sh1106_comm_type_t;

/**
 * @fn 
 *
 * @brief Función que envía datos o comandos al displat SH1106 a través del puerto I2C
 *
 * @param[in] data
 *      Dato que se desea escribir en el display
 *
 * @param[in] len
 *      Largo del dato que se quiere escribir en el display
 *
 * @param[in] comm_type
 *      Tipo de comunuicación que se desea: Dato o Comando
 *
 * @retval
 */
esp_err_t sh1106_write(const uint8_t *data, size_t len, sh1106_comm_type_t comm_type);

#endif