/*
*   SPIModule.c
*
*   Created on: 11 ago. 2025
*       Author: elian
*/

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "../Gestor_Estados/GestorEstados.h"

/* Tamaños de buffers y frame SPI */
#define SPI_BUF_SIZE           16   // Tamaño del buffer circular DMA RX/TX
#define SPI_TRANSMITION_SIZE    4   // Longitud de transacción DMA (bytes)
#define CMD_LEN                 3   // Cantidad de bytes que envía el master en un comando típico

/* Buffers para DMA (HW lee/escribe aquí directamente) */
uint8_t rxDMABuffer[SPI_BUF_SIZE];  // RX por DMA (lectura directa desde periférico)
uint8_t txDMABuffer[SPI_BUF_SIZE];  // TX por DMA (próxima respuesta a enviar)

/* Buffers auxiliares opcionales (no usados con DMA continuo) */
volatile uint8_t rxBuffer[SPI_BUF_SIZE];
volatile int rxIndex = 0;

/* Respuesta inicial dummy (no usada en flujo actual, se mantiene por compatibilidad) */
volatile uint8_t tx_buffer[CMD_LEN] = {0xAA, 0xBB, 0xCC};
volatile uint8_t rx_index = 0;

/* Handle del periférico SPI (inyectado desde fuera con SPI_Init) */
SPI_HandleTypeDef* hspi;

/**
 * @brief Procesa un comando recibido por SPI y construye la respuesta.
 * @param buffer       Puntero al buffer recibido (sin el ';' final).
 * @param cantBytes    Cantidad de bytes válidos en buffer (antes de ';').
 * @param bufferResponse Puntero a un arreglo destino (mín. 4 bytes) donde se
 *                       escribe la respuesta en formato [RESP][';'][dato1][dato2].
 *
 * @note La respuesta se deja en bufferResponse; el callback de DMA la copiará a txDMABuffer.
 * @note Para GET_FREC se empaquetan hasta 2 bytes (valor <= 511 según tu lógica actual).
 */
static void SPI_ProcesarComando(uint8_t* buffer, int cantBytes, uint8_t* bufferResponse) {
    int resp;
    int val;

    // Selección por comando (primer byte del buffer de respuesta
    bufferResponse[0] = '\0';
    switch (buffer[0]) {
        case SPI_REQUEST_START:
            resp = GestorEstados_Action(ACTION_START, 0);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            } else if(resp == ACTION_RESP_EMERGENCY_ACTIVE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_EMERGENCY_ACTIVE;            
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_STOP:
            resp = GestorEstados_Action(ACTION_STOP, 0);
            
            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_NOT_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_NOT_MOVING;
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }
            
            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_EMERGENCY:
            GestorEstados_Action(ACTION_EMERGENCY, 0);
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_SET_FREC:
            val = (uint8_t)buffer[1] + (uint8_t)buffer[2];
            resp = GestorEstados_Action(ACTION_SET_FREC, val);
            
            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            } else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_SET_ACEL:
            resp = GestorEstados_Action(ACTION_SET_ACEL, (uint8_t)buffer[1]);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            } else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_SET_DESACEL:
            resp = GestorEstados_Action(ACTION_SET_DESACEL, (uint8_t)buffer[1]);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            } else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_SET_DIR:
            val = (uint8_t)buffer[1];
            resp = GestorEstados_Action(ACTION_SET_DIR, val);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            } else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            } else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            } else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';
            return;

        case SPI_REQUEST_GET_FREC:
            /* Lee valor actual de frecuencia desde el SVM */
            val = GestorSVM_GetFrec();
            bufferResponse[0] = SPI_RESPONSE_OK;
            /* Empaquetado simple en 2 bytes (LOW/HIGH “capado”) */
            if (val > 255) {
                bufferResponse[1] = 255;
                bufferResponse[2] = (uint8_t)(val - 255);
            } else {
                bufferResponse[1] = (uint8_t)val;
                bufferResponse[2] = 0;
            }
            bufferResponse[3] = ';';
            return;

        case SPI_REQUEST_GET_ACEL:
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = GestorSVM_GetAcel();
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';
            return;

        case SPI_REQUEST_GET_DESACEL:
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = GestorSVM_GetDesacel();
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';
            return;

        case SPI_REQUEST_GET_DIR:
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = GestorSVM_GetDir();
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';
            return;

        case SPI_REQUEST_IS_STOP:
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = (uint8_t)GestorEstados_Action(ACTION_IS_MOTOR_STOP, 0);
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';
            return;

        case SPI_REQUEST_RESPONSE:
            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = ';';
            return;

        default:
            /* Comando desconocido */
            bufferResponse[0] = SPI_RESPONSE_ERR_CMD_UNKNOWN;
            bufferResponse[1] = ';';
            return;
    }

    /* Fallback (no debería alcanzarse) */
    bufferResponse[0] = SPI_RESPONSE_ERR;
    bufferResponse[1] = ';';
}

/**
 * @fn void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *_hspi)
 * 
 * @brief Callback de HAL llamado al completar una transacción TxRx por DMA.
 * @details
 *  - Busca el delimitador ';' en rxDMABuffer para conocer longitud de comando.
 *  - Construye una respuesta por defecto (ERR_NO_COMMAND) si el frame es inválido.
 *  - Llama a SPI_ProcesarComando() si hay un payload válido.
 *  - Copia la respuesta a txDMABuffer para la PRÓXIMA transacción.
 *  - Re-arma la transacción DMA continua con HAL_SPI_TransmitReceive_DMA().
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *_hspi) {
    int len = 0;
    int found = 0;
    
    if (_hspi->Instance == SPI2) {
        /* Buscar fin de comando ';' en el buffer de RX */
        for (int i = 0; i < SPI_BUF_SIZE; i++) {
            if (rxDMABuffer[i] == ';') {
                found = 1;
                len = i;              // bytes válidos (sin incluir ';')
                break;
            }
        }

        /* Respuesta por defecto: “ERR_NO_COMMAND;” */
        uint8_t resp[4] = { SPI_RESPONSE_ERR_NO_COMMAND, ';', 0, 0 };

        if (found && len > 0) {
            SPI_ProcesarComando(rxDMABuffer, len, resp);
        }

        txDMABuffer[0] = resp[0];
        txDMABuffer[1] = resp[1];
        txDMABuffer[2] = resp[2];
        txDMABuffer[3] = resp[3];

        /* Limpiar RX para próxima captura */
        memset(rxDMABuffer, 0, sizeof(rxDMABuffer));

        /* Reiniciar ciclo DMA full-duplex (no bloqueante) */
        HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);
    }
}

void SPI_Init(void* _hspi) {
    /* Prioridades/enable de DMA para SPI2: TX (CH4), RX (CH5) */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    /* Guardar handle de SPI */
    hspi = (SPI_HandleTypeDef*)_hspi;

    /* Inicialización básica de buffers */
    memset(rxDMABuffer, 0, sizeof(rxDMABuffer));
    memset(txDMABuffer, 0, sizeof(txDMABuffer));

    /* Respuesta inicial por defecto: “OK;” (se enviará en la primera TxRx) */
    txDMABuffer[0] = SPI_RESPONSE_OK;
    txDMABuffer[1] = ';';

    /* Iniciar ciclo DMA full-duplex no bloqueante */
    HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);
}