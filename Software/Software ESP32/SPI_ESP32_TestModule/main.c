#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_Module.h"
#include "UART_Module.h"
#include <string.h>


void UART_SPI_TestModule();

void app_main(void) {

    UART_Init();
    SPI_Init();
    xTaskCreate(UART_SPI_TestModule, "uart_task", 4096, NULL, 5, NULL);

}

void UART_SPI_TestModule(void *arg) {

    int uartBuffer[6];
    
    int comando;
    int setVal; 
    int getVal;
    SPI_Response retVal;

    while (1) {

        printf("\nIngresar comando: ");
        fflush(stdout);

        memset(uartBuffer, 0, sizeof(uartBuffer));

        UART_GetComando(uartBuffer);

        // El comando siempre esta en el primer elemento
        comando = uartBuffer[0];
        // El valor si esta, es el segundo elemento
        setVal = uartBuffer[1];

        printf("\n[Main] Uart comand: %d, uartSetVal: %d\n", comando, setVal);

        retVal = SPI_SendRequest(comando, setVal, &getVal);

        printf("[Main] RetVal: %d\n", retVal);

        switch(retVal) {
            case SPI_RESPONSE_OK:
                printf("Todo OK - RetVal: %d\n", getVal);
                break;
            case SPI_RESPONSE_ERR:
                printf("Error en mensaje\n");
                break;
            case SPI_RESPONSE_ERR_CMD_UNKNOWN:
                printf("Error: Comando desconocido\n");
                break;
            case SPI_RESPONSE_ERR_NO_COMMAND:
                printf("Error: Sin comando\n");
                break;
            case SPI_RESPONSE_ERR_MOVING:
                printf("Error: Motor en movimiento\n");
                break;
            case SPI_RESPONSE_ERR_NOT_MOVING:
                printf("Error: Motor detenido\n");
                break;
            case SPI_RESPONSE_ERR_DATA_MISSING:
                printf("Error: Falta de datos\n");
                break;
            case SPI_RESPONSE_ERR_DATA_INVALID:
                printf("Error: Datos invalidos\n");
                break; 
            case SPI_RESPONSE_ERR_DATA_OUT_RANGE:
                printf("Error: Datos fuera de rango\n");
                break;
            case SPI_RESPONSE_ERR_EMERGENCY_ACTIVE:
                printf("Error: Emergencia activa\n");
                break;
            default:
                printf("Error: Codigo de respuesta desconocido\n");
                break;
        }
    
        
    }
    
}

