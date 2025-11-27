/*
*   SPIModule.c
*
*   Created on: 11 ago. 2025
*       Author: elian
*/


#define SPI_BUF_SIZE 16
#define SPI_TRANSMITION_SIZE 4

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "../Gestor_Estados/GestorEstados.h"

// Definiciones privadas al modulo

// Consultas SPI
typedef enum {
    SPI_REQUEST_START       = 10,
    SPI_REQUEST_STOP        = 11,
    SPI_REQUEST_SET_FREC    = 12,
    SPI_REQUEST_SET_ACEL    = 13,
    SPI_REQUEST_SET_DESACEL = 14,
    SPI_REQUEST_SET_DIR     = 15,
    SPI_REQUEST_GET_FREC    = 16,
    SPI_REQUEST_GET_ACEL    = 17,
    SPI_REQUEST_GET_DESACEL = 18,
    SPI_REQUEST_GET_DIR     = 19,
    SPI_REQUEST_IS_STOP     = 20,
    SPI_REQUEST_EMERGENCY   = 21,
} SPI_Request;

// Commando dummy del master para buscar la respuesta
#define SPI_REQUEST_RESPONSE  0x50   

// Respuestas SPI
typedef enum {
    SPI_RESPONSE_OK = 0xFF,
    SPI_RESPONSE_ERR = 0xA0,
    SPI_RESPONSE_ERR_CMD_UNKNOWN = 0xA1,            // Comando desconocido
    SPI_RESPONSE_ERR_NO_COMMAND = 0xA2,             // Llego mensaje pero sin comando
    SPI_RESPONSE_ERR_MOVING = 0xA3,                 // Comando Start pero motor ya esta en movimiento
    SPI_RESPONSE_ERR_NOT_MOVING = 0xA4,             // Comando Stop pero motor no esta en movimiento    
    SPI_RESPONSE_ERR_DATA_MISSING = 0xA5,           // Comando que requiere datos pero no llegaron
    SPI_RESPONSE_ERR_DATA_INVALID = 0xA6,           // Comando que tiene datos invalidos
    SPI_RESPONSE_ERR_DATA_OUT_RANGE = 0xA7,         // Comando con datos fuera de rango permitido
    SPI_RESPONSE_ERR_EMERGENCY_ACTIVE = 0xA8,       // Emergencia activa
    SPI_LAST_VALUE,                                 // Los mensajes deben tener un valor consecutivo uno de otro
} SPI_Response;



#define CMD_LEN 3   // cantidad de bytes que envía el master

uint8_t rxDMABuffer[SPI_BUF_SIZE];					// Buffer para almacenar bytes de SPI por DMA (copia directa a memoria)
uint8_t txDMABuffer[SPI_BUF_SIZE];					// Buffer para almacenar la respuesta por DMA

volatile uint8_t rxBuffer[SPI_BUF_SIZE];			// Buffer para almacenar la cadena recibida con el comando
volatile int rxIndex = 0;

volatile uint8_t tx_buffer[CMD_LEN] = {0xAA, 0xBB, 0xCC}; // respuesta inicial
volatile uint8_t rx_index = 0;

SPI_HandleTypeDef* hspi;

static void SPI_ProcesarComando(uint8_t* buffer, int index, uint8_t* bufferResponse);

// En esta funcion vamos a procesar la cadena
// Generalmente vamos a atacar al modulo gestor de estados
// Ademas aca tenemos que cargar el buffer Tx del DMA 
// Si tenemos que responder con un valor lo vamos a pasar por bufferResponse
void SPI_ProcesarComando(uint8_t* buffer, int cantBytes, uint8_t* bufferResponse) {

    int resp;
    int val;

    // Limpieza del buffer de respuesta
    bufferResponse[0] = '\0';

    switch(buffer[0]) {
        case SPI_REQUEST_START:

            resp = GestorEstados_Action(ACTION_START, 0);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            }else if(resp == ACTION_RESP_EMERGENCY_ACTIVE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_EMERGENCY_ACTIVE;            
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';

            printf("Response OK START");

            return;

        case SPI_REQUEST_STOP:

            resp = GestorEstados_Action(ACTION_STOP, 0);
            
            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_NOT_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_NOT_MOVING;
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }
            
            bufferResponse[1] = ';';

            printf("Response OK STOP");

            return;

        case SPI_REQUEST_EMERGENCY:

            GestorEstados_Action(ACTION_EMERGENCY, 0);

            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = ';';

            return;

            printf("Response OK EMERGENCY");

        case SPI_REQUEST_SET_FREC:

            val = (uint8_t)buffer[1] + (uint8_t)buffer[2];

            resp = GestorEstados_Action(ACTION_SET_FREC, val);
            
            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            }else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';

            printf("Response OK frequency set");

            return;

        case SPI_REQUEST_SET_ACEL:
          
            resp = GestorEstados_Action(ACTION_SET_ACEL, (uint8_t)buffer[1]);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            }else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';

            printf("Response OK acceleration set");

            return;

        case SPI_REQUEST_SET_DESACEL:
            
            resp = GestorEstados_Action(ACTION_SET_DESACEL, (uint8_t)buffer[1]);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            }else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';

            printf("Response OK desacceleration set");

            return;

        case SPI_REQUEST_SET_DIR:

            val = (uint8_t)buffer[1];
        
            resp = GestorEstados_Action(ACTION_SET_DIR, val);

            if(resp == ACTION_RESP_OK) {
                bufferResponse[0] = SPI_RESPONSE_OK;
            }else if(resp == ACTION_RESP_OUT_RANGE) {
                bufferResponse[0] = SPI_RESPONSE_ERR_DATA_OUT_RANGE;
            }else if(resp == ACTION_RESP_MOVING) {
                bufferResponse[0] = SPI_RESPONSE_ERR_MOVING;
            }else {
                bufferResponse[0] = SPI_RESPONSE_ERR;
            }

            bufferResponse[1] = ';';

            printf("Response OK direction set");

            return;

        case SPI_REQUEST_GET_FREC:

            val = GestorEstados_Action(ACTION_GET_FREC, 0);
            
            bufferResponse[0] = SPI_RESPONSE_OK;
            if(val > 255) {
                bufferResponse[1] = 255;
                bufferResponse[2] = val - 255; 
            }else {
                bufferResponse[1] = val;
                bufferResponse[2] = 0;
            }

            bufferResponse[3] = ';';

            printf("Response OK frequency value");

            return;
            
        case SPI_REQUEST_GET_ACEL:

        	bufferResponse[0] = SPI_RESPONSE_OK;
        	bufferResponse[1] = GestorEstados_Action(ACTION_GET_ACEL, 0);
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';

            printf("Response OK acceleration value");

            return;
            
        case SPI_REQUEST_GET_DESACEL:

        	bufferResponse[0] = SPI_RESPONSE_OK;
        	bufferResponse[1] = GestorEstados_Action(ACTION_GET_DESACEL, 0);
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';

            printf("Response OK desacceleration value");

            return;

        case SPI_REQUEST_GET_DIR:

        	bufferResponse[0] = SPI_RESPONSE_OK;
        	bufferResponse[1] = GestorEstados_Action(ACTION_GET_DIR, 0);
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';

            printf("Response OK direction value");

            return;

        case SPI_REQUEST_IS_STOP:

        	bufferResponse[0] = SPI_RESPONSE_OK;
        	bufferResponse[1] = GestorEstados_Action(ACTION_IS_MOTOR_STOP, 0);
            bufferResponse[2] = 0;
            bufferResponse[3] = ';';

            printf("Response OK Stop/Moving");

            return;

        case SPI_REQUEST_RESPONSE:

            bufferResponse[0] = SPI_RESPONSE_OK;
            bufferResponse[1] = ';';

            printf("Response OK");

            return;

        default:
        
            bufferResponse[0] = SPI_RESPONSE_ERR_CMD_UNKNOWN;
            bufferResponse[1] = ';';

            printf("Response Unknown");

            return;

    }

    bufferResponse[0] = SPI_RESPONSE_ERR;
    bufferResponse[1] = ';'; ;
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *_hspi) {
    int len = 0;
    int found = 0;

    // Limpiamos el buffer de envio
    //memset(txDMABuffer, 0, sizeof(txDMABuffer));

    if (_hspi->Instance == SPI2) {

        // Buscamos ';' en el buffer recibido
        for (int i = 0; i < SPI_BUF_SIZE; i++) {
            if (rxDMABuffer[i] == ';') {
                found = 1;
                len = i; // cantidad de bytes validos, sin incluir el ';'
                break;
            }
        }

        // Arrancamos con respuesta por defecto
        uint8_t resp[4] = { SPI_RESPONSE_ERR_NO_COMMAND, ';', 0, 0 };

        if( found && len > 0 ) {
            
            SPI_ProcesarComando(rxDMABuffer, len, resp);

        }

        // Copiar la respuesta al TX para la PRÓXIMA transacción
        txDMABuffer[0] = resp[0];
        txDMABuffer[1] = resp[1];
        txDMABuffer[2] = resp[2];
        txDMABuffer[3] = resp[3];

        memset(rxDMABuffer, 0, sizeof(rxDMABuffer));

        HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);

    }
}

void SPI_Init(void* _hspi) {
    
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    hspi = (SPI_HandleTypeDef*)_hspi;

    memset(rxDMABuffer, 0, sizeof(rxDMABuffer));
    memset(txDMABuffer, 0, sizeof(txDMABuffer));

    txDMABuffer[0] = SPI_RESPONSE_OK;
    txDMABuffer[1] = ';';

    HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);
}