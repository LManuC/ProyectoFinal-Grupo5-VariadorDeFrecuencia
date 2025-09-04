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

// Comandos SPI
#define CMD_START      0x10
#define CMD_STOP       0x11
#define CMD_SET_FREC   0x20
#define CMD_SET_ACEL   0x21
#define CMD_SET_DIR    0x22
#define CMD_GET_FREC   0x30
#define CMD_GET_ACEL   0x31
#define CMD_GET_DIR    0x32
#define CMD_IS_STOP    0x33
// Commando dummy del master para buscar la respuesta
#define CMD_DUMMY      0x50          


// Respuestas SPI
//#define RESP_OK   0xFF
//#define RESP_ERR  0x00
//#define RESP_ERR_MOVING 0x10

typedef enum {
    RESP_OK = 0xFF,
    RESP_ERR = 0xA0,
    RESP_ERR_CMD_UNKNOWN = 0xA1,        // Comando desconocido
    RESP_ERR_NO_COMMAND = 0xA2,         // Llego mensaje pero sin comando
    RESP_ERR_MOVING = 0xA3,             // Comando Start pero motor ya esta en movimiento
    RESP_ERR_NOT_MOVING = 0xA4,         // Comando Stop pero motor no esta en movimiento    
    RESP_ERR_DATA_MISSING = 0x5,       // Comando que requiere datos pero no llegaron
    RESP_ERR_DATA_INVALID = 0xA6,       // Comando que tiene datos invalidos
} RespSPI;  



#define CMD_LEN 3   // cantidad de bytes que envía el master



// Variables privadas al modulo

// Buffer para almacenar bytes de SPI por DMA (copia directa a memoria)
uint8_t rxDMABuffer[SPI_BUF_SIZE];
// Buffer para almacenar la respuesta por DMA
uint8_t txDMABuffer[SPI_BUF_SIZE];


// Buffer para almacenar la cadena recibida con el comando
volatile uint8_t rxBuffer[SPI_BUF_SIZE];
volatile int rxIndex = 0;

volatile uint8_t tx_buffer[CMD_LEN] = {0xAA, 0xBB, 0xCC}; // respuesta inicial
volatile uint8_t rx_index = 0;

SPI_HandleTypeDef* hspi;



// Prototipos de funciones privadas al modulo

RespSPI SPI_ProcesarComando(uint8_t* buffer, int index, uint8_t* bufferResponse);




void SPI_Init(void* _hspi) {

    /* 
        Modificacion de la configuracion del DMA 
        Se baja la prioridad 
    */
    
    /* DMA1_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    /* DMA1_Channel5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    hspi = (SPI_HandleTypeDef*)_hspi;

    // Limpieza de memoria de ambos buffers
    memset(rxDMABuffer, 0, sizeof(rxDMABuffer));
    memset(txDMABuffer, 0, sizeof(txDMABuffer));

    txDMABuffer[0] = RESP_OK;
    txDMABuffer[1] = ';';

    // Inicio de el DMA
    HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);
}



void SPI_Loop(void) {

    //static uint16_t lastPos = 0;
	//uint8_t byteRx = 0;
    //int respComando = 0;
    

    //uint16_t pos = SPI_BUF_SIZE - __HAL_DMA_GET_COUNTER(hspi->hdmarx);

    /*
    if(hspi->State != HAL_SPI_STATE_READY) {
        // No termino el DMA
        return;        
    }
    */

    

    /*
    while(lastPos != pos) {

        byteRx = rxDMABuffer[lastPos];


        if(byteRx != ';') {
            if(rxIndex < SPI_BUF_SIZE - 1) {
                rxBuffer[rxIndex] = byteRx;
                rxIndex++;
            }
        }else if(rxIndex > 0) {  // En caso de que sea un mensaje dummy se descarta
            rxBuffer[rxIndex] = '\0'; // Fin de comando
            printf("Comando: %s\n", rxBuffer);

            // Se procesa el comando
            respComando = SPI_ProcesarComando(rxBuffer, rxIndex);
            if(respComando == 0) {  // Comando procesado correctamente
                txDMABuffer[0] = RESP_OK;
            }else {
                txDMABuffer[0] = RESP_ERR;
            }
            // Se vuelve a cero el contador
            rxIndex = 0;
        }

        // Incrementamos la posicion del buffer de DMA
        lastPos = (lastPos + 1) % SPI_BUF_SIZE;
    }


    */



}



// En esta funcion vamos a procesar la cadena
// Generalmente vamos a atacar al modulo gestor de estados
// Ademas aca tenemos que cargar el buffer Tx del DMA 
// Si tenemos que responder con un valor lo vamos a pasar por bufferResponse
RespSPI SPI_ProcesarComando(uint8_t* buffer, int cantBytes, uint8_t* bufferResponse) {

    int resp;
    int val;

    // Limpieza del buffer de respuesta
    bufferResponse[0] = '\0';

    switch(buffer[0]) {
        case CMD_START:

            // Comprobacion que el mensaje tiene 1 byte, solo el comando
            if(cantBytes != 1) {return RESP_ERR_DATA_MISSING;}

            resp = GestorEstados_Action(ACTION_START, 0);

            if(resp == ACTION_RESP_OK) {
                return RESP_OK;
            }else if(resp == ACTION_RESP_MOVING) {
                return RESP_ERR_MOVING;
            }else {
                return RESP_ERR;
            }

            break;
        case CMD_STOP:

            // Comprobacion que el mensaje tiene 1 byte, solo el comando
            if(cantBytes != 1) {return RESP_ERR_DATA_MISSING;}

            resp = GestorEstados_Action(ACTION_STOP, 0);
            
            if(resp == ACTION_RESP_OK) {
                return RESP_OK;
            }else if(resp == ACTION_RESP_NOT_MOVING) {
                return RESP_ERR_NOT_MOVING;
            }else {
                return RESP_ERR;
            }

            break;
        case CMD_SET_FREC:

            // Comprobacion que el mensaje tiene 3 byte, dos de datos
            if(cantBytes != 3) {return RESP_ERR_DATA_MISSING;}

            val = (int)buffer[0] + (int)buffer[1];

            resp = GestorEstados_Action(ACTION_SET_FREC, val);
            
            if(resp == ACTION_RESP_OK) {
                return RESP_OK;
            }else {
                return RESP_ERR;
            }

            break;
        case CMD_SET_ACEL:

            // Comprobacion que el mensaje tiene 3 byte, dos de datos
            if(cantBytes != 3) {return RESP_ERR_DATA_MISSING;}

            val = (int)buffer[0] + (int)buffer[1];
            
            resp = GestorEstados_Action(ACTION_SET_ACEL, val);

            if(resp == ACTION_RESP_OK) {
                return RESP_OK;
            }else {
                return RESP_ERR;
            }
            
            break;
        case CMD_SET_DIR:

            // Comprobacion que el mensaje tiene 2 byte, uno de datos
            if(cantBytes != 2) {return RESP_ERR_DATA_MISSING;}

            val = (int)buffer[0];

            if(val != 1 && val != 0) {return RESP_ERR_DATA_INVALID;}
        
            resp = GestorEstados_Action(ACTION_SET_DIR, val);

            if(resp == ACTION_RESP_OK) {
                return RESP_OK;
            }else {
                return RESP_ERR;
            }

            break;
        case CMD_GET_FREC:
            break;
        case CMD_GET_ACEL:
            break;
        case CMD_GET_DIR:
            break;
        case CMD_IS_STOP:
            break;
        case CMD_DUMMY:
            return RESP_OK;

        default:
            return RESP_ERR_CMD_UNKNOWN;

    }

    return RESP_ERR;
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *_hspi) {
    int len = 0;
    int found = 0;
    RespSPI resp;
    //uint8_t bufferResponse[10];
    uint8_t txDMABufferLen = 0;

    // Limpiamos el buffer de envio
    memset(txDMABuffer, 0, sizeof(txDMABuffer));

    if (_hspi->Instance == SPI2) {

        // Buscamos ';' en el buffer recibido
        for (int i = 0; i < SPI_BUF_SIZE; i++) {
            if (rxDMABuffer[i] == ';') {
                found = 1;
                len = i; // cantidad de bytes validos, sin incluir el ';'
                break;
            }
        }

        if(found) {
            resp = SPI_ProcesarComando(rxDMABuffer, len, txDMABuffer);
            txDMABufferLen = strlen((char*)txDMABuffer);

            // Si se debe responder con el bufferResponse
            if(txDMABufferLen != 0 && resp == RESP_OK) {
                txDMABuffer[txDMABufferLen] = ';'; // Aseguramos el fin de cadena
            }else{
                // Si se debe responder con resp
                txDMABuffer[0] = resp;
                txDMABuffer[1] = ';';
            }
        }else {
            txDMABuffer[0] = RESP_ERR_NO_COMMAND;
            txDMABuffer[1] = ';';
            
        }

        memset(rxDMABuffer, 0, sizeof(rxDMABuffer));

        HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_TRANSMITION_SIZE);

        // Procesas lo que llego en rx_buffer
        // Preparas la proxima respuesta en tx_buffer
        //HAL_SPI_TransmitReceive_DMA(hspi, txDMABuffer, rxDMABuffer, SPI_BUF_SIZE);
    }
}
