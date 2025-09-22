#include "UART_Module.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BUF_SIZE 128
#define UART_PORT UART_NUM_0




void UART_Init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);
}


int UART_GetComando(int* buffer_out) {
    char aux[BUF_SIZE];
    char ch;
    int idx = 0;
    int idxOut = 0;
    int n, i;

    while (1) {
        
        ch = 0;

        // Se lee un byte de la consola
        uart_read_bytes(UART_PORT, &ch, 1, portMAX_DELAY);

        
        if(ch != ' ' && ch != ';') {
            // Se usa un buffer auxiliar para guardar un numero en un solo byte
            aux[idx] = ch;
            aux[idx+1] = '\0';
            idx++;

        }else {
            n = 0;

            // Se calculo el numero del ultimo buffer auxiliar completado
            for(i = 0; i < idx; i++) {
                n = n * 10 + (int)(aux[i] - '0');  // Convertir ASCII a número y acumular
            }

            // Se chequea que no se vaya de los limites
            if(n > 512) {
                n = 512;
            }else if(n < 0) {
                n = 0;
            }

            // Se guardan los numeros en nuestro buffer de retorno
            buffer_out[idxOut] = n;
            idx = 0;
            idxOut++;

            // Si lleva un ; se devuelve lo que se tiene
            if(ch == ';') {
                buffer_out[idxOut] = 0;
                return 0;
            }
        }
    }
    return 1;
}


