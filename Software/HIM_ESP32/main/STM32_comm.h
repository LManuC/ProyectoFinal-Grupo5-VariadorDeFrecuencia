
#ifndef __STM32_COM_H__

#define __STM32_COM_H__

extern QueueHandle_t cmd_queue;

void spi_task(void *arg);

extern status_t status;

#endif