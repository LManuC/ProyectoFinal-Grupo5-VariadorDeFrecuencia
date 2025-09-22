#ifndef UART_MODULE_H
#define UART_MODULE_H

// Funcion de inicializacion
void UART_Init(void);

// Funcion bloqueante que devuelve lo que se escribe en pantalla
// Es bloqueante solo por propositos de la prueba
int UART_GetComando(int* buffer_out);


#endif