/**
 * @file SPIModule.h
 * @brief Interfaz del módulo SPI (esclavo) con DMA.
 * @details
 * El maestro (ESP32) envía solicitudes @ref SPI_Request y el STM32 responde con
 * un código @ref SPI_Response en la **siguiente** transacción DMA.  
 * Este módulo actúa como capa de transporte sobre la lógica de estados implementada en
 * @ref GestorEstados_Action (ver @ref SystemAction y @ref SystemState).
 *
 * **Formato típico de tramas**
 * - **Master → Slave**: `CMD [DATOS...] ';'` (ver @ref SPI_Request)
 * - **Slave → Master**: `RESP [';'] [opcional: datos]` (ver @ref SPI_Response)
 *
 * @see GestorEstados_Action
 * @see SystemAction
 * @see SystemState
 * @see SystemActionResponse
 */

#ifndef SPI_MODULE_H_
#define SPI_MODULE_H_

/**
 * @enum SPI_Request
 * @brief Comandos aceptados por el esclavo a través de SPI.
 * @details
 * Cada entrada se mapea a una acción de @ref SystemAction procesada por
 * @ref GestorEstados_Action. Los comandos **SET\_*** llevan datos; **GET\_*** devuelven valores.
 */
typedef enum {
    SPI_REQUEST_START       = 10,   /** Solicita arranque → equivale a @ref ACTION_START. */
    SPI_REQUEST_STOP,               /** Solicita parada → equivale a @ref ACTION_STOP. */
    SPI_REQUEST_SET_FREC,           /** Setea frecuencia de régimen → @ref ACTION_SET_FREC (requiere datos). */
    SPI_REQUEST_SET_ACEL,           /** Setea aceleración → @ref ACTION_SET_ACEL (requiere datos). */
    SPI_REQUEST_SET_DESACEL,        /** Setea desaceleración → @ref ACTION_SET_DESACEL (requiere datos). */
    SPI_REQUEST_SET_DIR,            /** Setea dirección de giro → @ref ACTION_SET_DIR (requiere datos). */
    SPI_REQUEST_GET_FREC,           /** Consulta frecuencia actual → lectura sin cambio de @ref SystemState. */
    SPI_REQUEST_GET_ACEL,           /** Consulta aceleración actual. */
    SPI_REQUEST_GET_DESACEL,        /** Consulta desaceleración actual. */
    SPI_REQUEST_GET_DIR,            /** Consulta dirección actual. */
    SPI_REQUEST_IS_STOP,            /** Consulta si el motor está detenido → usa @ref ACTION_IS_MOTOR_STOP. */
    SPI_REQUEST_EMERGENCY,          /** Fuerza emergencia → @ref ACTION_EMERGENCY. */
    SPI_REQUEST_RESPONSE    = 0x50  /** Ping/placeholder para obtener la última respuesta. */
} SPI_Request;

/**
 * @enum SPI_Response
 * @brief Códigos de respuesta emitidos por el esclavo STM32.
 * @details
 * Conceptualmente mapean los resultados de @ref GestorEstados_Action (ver @ref SystemActionResponse)
 * hacia el enlace SPI. En comandos **GET\_*** pueden incluir bytes de datos adicionales.
 */
typedef enum {
    SPI_RESPONSE_OK = 0xFF,                 /** Operación válida/exitosa → similar a @ref ACTION_RESP_OK. */
    SPI_RESPONSE_ERR = 0xA0,                /** Error genérico → similar a @ref ACTION_RESP_ERR. */
    SPI_RESPONSE_ERR_CMD_UNKNOWN,           /** Comando desconocido (no mapea a acción válida). */
    SPI_RESPONSE_ERR_NO_COMMAND,            /** Trama sin comando o sin ';'. */
    SPI_RESPONSE_ERR_MOVING,                /** Incompatible por movimiento → similar a @ref ACTION_RESP_MOVING. */
    SPI_RESPONSE_ERR_NOT_MOVING,            /** Incompatible por estar detenido → similar a @ref ACTION_RESP_NOT_MOVING. */
    SPI_RESPONSE_ERR_DATA_MISSING,          /** Faltan datos en un comando que los requiere. */
    SPI_RESPONSE_ERR_DATA_INVALID,          /** Datos inválidos. */
    SPI_RESPONSE_ERR_DATA_OUT_RANGE,        /** Datos fuera de rango → similar a @ref ACTION_RESP_OUT_RANGE. */
    SPI_RESPONSE_ERR_EMERGENCY_ACTIVE,      /** No permitido en emergencia → similar a @ref ACTION_RESP_EMERGENCY_ACTIVE. */
    SPI_LAST_VALUE                          /** Marcador final (no usar como respuesta). */
} SPI_Response;

/**
 * @fn void SPI_Init(void* hspi)
 * @brief Inicializa el módulo SPI esclavo con DMA y deja activa la primera transacción.
 * @details
 * - Configura prioridades/enable de IRQ para DMA1 Channel 4/5 (Tx/Rx).
 * - Limpia buffers RX/TX y precarga una respuesta por defecto (`SPI_RESPONSE_OK;`).
 * - Llama a `HAL_SPI_TransmitReceive_DMA()` para iniciar el ciclo continuo DMA
 *   (el tamaño de paquete es el configurado en el fuente, p.ej. `SPI_TRANSMITION_SIZE`).
 * - El parseo y la construcción de @ref SPI_Response se realizan en el callback
 *   `HAL_SPI_TxRxCpltCallback`, invocando @ref GestorEstados_Action según corresponda.
 *
 * @param[in] hspi  Handler de SPI inicializado por HAL (tipo `SPI_HandleTypeDef*`, p.ej. `&hspi2`).
 *
 * @see GestorEstados_Action
 * @see SPI_Request
 * @see SPI_Response
 */
void SPI_Init(void* hspi);

#endif /* SPI_MODULE_H_ */
