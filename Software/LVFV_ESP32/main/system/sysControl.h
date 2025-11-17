/**
 * @file SysControl.h
 * @author Andrenacci - Carra
 * @brief Declraraciones de funciones que controlan el sistema del STM32. Además se encontrarán los comandos y respuestas que admite el STM32
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef SPI_MODULE_H
#define SPI_MODULE_H

#include "../LVFV_system.h"

/**
 * @enum SPI_Request
 *
 * @brief Posibles comandos que puede enviar el STM32
 */
typedef enum {
    SPI_REQUEST_START = 10,                     // 10 - Comando de arranque de motor
    SPI_REQUEST_STOP,                           // 11 - Comando de parada de motor
    SPI_REQUEST_SET_FREC,                       // 12 - Comando para configurar la frecuencia de régimen
    SPI_REQUEST_SET_ACEL,                       // 13 - Comando para setear la aceleración hasta la frecuencia de régimen
    SPI_REQUEST_SET_DESACEL,                    // 14 - Comando para setear la desaceleración a 0Hz o frecuencia de régimen
    SPI_REQUEST_SET_DIR,                        // 15 - Comando para setear la dirección - TODO
    SPI_REQUEST_GET_FREC,                       // 16 - Comando de consulta de frecuencia de régimen
    SPI_REQUEST_GET_ACEL,                       // 17 - Comando de consulta de aceleración
    SPI_REQUEST_GET_DESACEL,                    // 18 - Comando de consulta de desaceleración
    SPI_REQUEST_GET_DIR,                        // 19 - Comando de consulta de dirección de giro
    SPI_REQUEST_IS_STOP,                        // 20 - Comando de consulta para saber si el motor está parado o en movimiento
    SPI_REQUEST_EMERGENCY,                      // 21 - Comando para entrar en estado de emergencia - deja de conmutar la salida
    SPI_REQUEST_RESPONSE = 0x50                 // 80 - Comando para pedirle al STM32 la respuesta al comando enviado
} SPI_Request;

/**
 * @enum SPI_Response
 *
 * @brief Posibles respuesta que puede enviar el STM32 en consecuencia por los request enviados
 */
typedef enum {
    SPI_RESPONSE_ERR = 0xA0,                    // 160 - Error en la transmisión del comando por problemas de inicialización del handler. El comando no sale del ESP32
    SPI_RESPONSE_ERR_CMD_UNKNOWN,               // 161 - El comando enviado no está dentro los posibles
    SPI_RESPONSE_ERR_NO_COMMAND,                // 162 - Llego mensaje pero sin comando
    SPI_RESPONSE_ERR_MOVING,                    // 163 - Se está intentando enviar un comando de start con el motor en movimiento
    SPI_RESPONSE_ERR_NOT_MOVING,                // 164 - Se está intentando enviar un comando de stop con el motor detenido
    SPI_RESPONSE_ERR_DATA_MISSING,              // 165 - Faltó enviar un dato dentro de la trama
    SPI_RESPONSE_ERR_DATA_INVALID,              // 166 - Los datos enviados como argumento dentro del comando están fuera de rango
    SPI_RESPONSE_ERR_DATA_OUT_RANGE,            // 167 - Comando con datos fuera de rango permitido
    SPI_RESPONSE_OK = 0xFF,                     // 255 - La comunicación entre ESP32 y STM32 fue exitosa
} SPI_Response;

/**
 * @struct spi_cmd_item_t
 * 
 * @brief Estructura de datos que contiene un comando a enviar por SPI, un argumento que se desea enviar al STM32 y un dato que que pueda recibirse como respuesta en consecuencia.
 *
 * @note getValue no siempre recibe un dato como respuesta, solo en los comandos "GET"
 */
typedef struct {
    SPI_Request request;                        // Comando a enviar al STM32
    int getValue;                               // Variable devuelta por el STM32 en caso de ser un comando get
    int setValue;                               // Dato enviado con los comandos set
} spi_cmd_item_t;

/**
 * @fn esp_err_t SPI_Init(void);
 *
 * @brief Inicializa el módulo SPI del ESP32
 *
 * @details Con figura los pines PIN_NUM_CS en el pin IO12, PIN_NUM_CLK en el pin IO14, PIN_NUM_MISO en el pin IO26 y PIN_NUM_MOSI en el pin IO25; además configura el clock del SPI en 1MHz y tendrá solo un comando para encolar
 *
 * @return
 *  - ESP_ERR_INVALID_ARG Si la configuración es inválida. La combinación de los parámetros puede resultar inválida.
 *  - ESP_ERR_INVALID_STATE Si el host ya se encontraba en uso, si la fuente de clock es inviable o si el SPI no fue inicializado correctamente.
 *  - ESP_ERR_NOT_FOUND if there is no available DMA channel 
 *  - ESP_ERR_NO_MEM Si no hay memoria suficiente para inicializar el módulo
 *  - ESP_OK si la configuración fue exitosa
 */
esp_err_t SPI_Init(void);

/**
 * @fn void SPI_communication(void *arg);
 *
 * @brief Tarea que controla en arranque, parada y emergencia del sistema.
 *
 * @details Espera comandos a través de la cola system_event_queue para evaluar si enviar comandos de start, stop, emergencia, cambios de velocidad, etc. Hace indirectamente el polling de las mediciones del ADC para evaluar si está en estado de emergencia o no.
 *
 * @param[inout] arg
 *      Sin uso
 */
void SPI_communication(void *arg);

/**
 * @fn esp_err_t SystemEventPost(systemSignal_e event);
 *
 * @brief Encola comandos en la cola system_event_queue
 *
 * @param[in] event
 *      Evento que se desea encolar en el sistema
 *
 * @return
 *  - pdTRUE Si el comando se posteó correctamente
 *  - Cualquier otra respuesta implica que la cola está llena
 */
esp_err_t SystemEventPost(systemSignal_e event);

#endif // SPI_MODULE_H