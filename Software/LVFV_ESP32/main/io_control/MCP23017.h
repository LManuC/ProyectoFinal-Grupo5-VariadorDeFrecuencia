/**
 * @file MCP23017.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones de inicialización, lectura y escritura a través del puerto I2C el dispositivo MCP23017
 * @version 2.0
 * @date 2025-11-06
 * 
 */
    
#ifndef __MCP23017_H__
#define __MCP23017_H__

#include "esp_mac.h"
#include "mcp23017_defs.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdbool.h>


/**
 * @fn esp_err_t MCP23017_INIT( void );
 *
 * @brief Función dedicada a inicializar y configurar el MCP23017. El chip utiliza i2C.
 *
 * @details Inicializa el MCP32017. Configura las entradas IO2, IO3, IO4, IO5 y IO6 del puerto A (Todas ellas sin pull up activo) y las entradas IO0, IO1, IO2 y IO3 del puerto B (Todas ellas con pull up activo); configura como salidas IO0, IO1 y IO7 del puerto A y IO4, IO5, IO6 y IO7 del puerto B. Todas las entradas generan una interrpción que se informa por los pines INTA e INTB del chip por separado (Las interrupciones del puerto A genera un cambio en el pin INTA; misma explicación para INTB). Las interrupciones se generarán siempre cuando haya un cambio en la entrada (Ya sea un flanco positivo o negativo). Los pines INTA e INTB son salidas activas que se pondrán en alto en caso de tener una interrupción.
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t MCP23017_INIT( void );

/**
 * @fn esp_err_t mcp_get_on_interrupt_input(enum mcp_port_e port, uint8_t *data);
 *
 * @brief Función que lee el registro INTCAP del MCP23017
 *
 * @details El registro INTCAP retiene el valor de las entradas al generarse una interrupción.
 *
 * @param[in] port
 *      Puede tomar los valores PORTA o PORTB, dependiendo del puerto que se quiera acceder.
 *    
 * @param[out] data
 *      Puntero donde se almaecnará la lectura del registro INTCAP.
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t mcp_get_on_interrupt_input(enum mcp_port_e port, uint8_t *data);

/**
 * @fn esp_err_t mcp_interrupt_flag(enum mcp_port_e port, uint8_t *data);
 *
 * @brief Función que lee el registro INTF del MCP23017
 *
 * @details El registro INTF retiene el valor de la entrada que generó la interrupción. Luego de haber leído este registro, el pin INTx vuelve a su estado de reposo.
 *
 * @param[in] port
 *      Puede tomar los valores PORTA o PORTB, dependiendo del puerto que se quiera acceder.
 *    
 * @param[out] data
 *      Puntero donde se almaecnará la lectura del registro INTF.
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t mcp_interrupt_flag(enum mcp_port_e port, uint8_t *data);

/**
 * @fn esp_err_t mcp_write_output_pin(enum mcp_port_e port, uint8_t pin, bool state);
 *
 * @brief Función que lee el registro GPIO, escribe @p state en el @p pin del @p port escribe el registro OLAT del MCP23017
 *
 * @details El registro OLAT es el que exterioriza el estado del pin en etados lógicos altos o bajos.
 *
 * @param[in] port
 *      Puede tomar los valores PORTA o PORTB, dependiendo del puerto que se quiera escribir.
 *    
 * @param[in] pin
 *      Pin físico que quiere ser modificado su estado
 *    
 * @param[in] state
 *      Estado True o False que puede tomar el pin que quiere ser modificado
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t mcp_write_output_pin(enum mcp_port_e port, uint8_t pin, bool state);

/**
 * @fn esp_err_t mcp_read_port(enum mcp_port_e port, uint8_t *data);
 *
 * @brief Función que lee el registro GPIO del @p port del MCP23017 y lo devuelve en @p data
 *
 * @details El registro GPIO es el que representa la exteriorización de los estados de los etados lógicos altos o bajos de los pines.
 *
 * @param[in] port
 *      Puede tomar los valores PORTA o PORTB, dependiendo del puerto que se quiera escribir.
 *    
 * @param[out] data
 *      Puntero donde se almacena el estado del @p puerto del MCP23017.
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t mcp_read_port(enum mcp_port_e port, uint8_t *data);

/**
 * @fn esp_err_t mcp_write_output_port(enum mcp_port_e port, uint8_t data);
 *
 * @brief Función que escribe @p data en el escribe el registro OLAT del @p port del MCP23017.
 *
 * @details Escribe @p data entero, no pin a pin.
 *
 * @param[in] port
 *      Puede tomar los valores PORTA o PORTB, dependiendo del puerto que se quiera escribir.
 *    
 * @param[in] data
 *      Estado True o False que puede tomar los pines del @p port
 *    
 * @return
 *      - ESP_OK en caso de éxito.
 *      - ESP_ERR_INVALID_ARG Error de parámetro
 *      - ESP_FAIL Error al enviar comando, el esclavo no envió ACK a la transferencia.
 *      - ESP_ERR_INVALID_STATE I2C El driver no fue inicializado o no está en modo master.
 *      - ESP_ERR_TIMEOUT Timueout de la operación porque el bus está ocupado.
 */
esp_err_t mcp_write_output_port(enum mcp_port_e port, uint8_t data);

#endif
