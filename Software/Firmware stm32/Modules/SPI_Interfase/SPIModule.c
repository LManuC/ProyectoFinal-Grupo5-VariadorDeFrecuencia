/*
*   SPIModule.c
*
*   Created on: 11 ago. 2025
*       Author: elian
*/


#define SPI_BUF_SIZE 16

#include <stdio.h>

uint8_t spi_rx_buf[SPI_BUF_SIZE];
uint8_t spi_tx_buf[SPI_BUF_SIZE];


void SPI_Init(void) {
    HAL_SPI_TransmitReceive_DMA(&hspi1, spi_tx_buf, spi_rx_buf, SPI_BUF_SIZE);
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // Procesar datos en el loop principal, no acá
        spi_data_ready = 1;
        // Reiniciar DMA para la próxima transacción
        HAL_SPI_TransmitReceive_DMA(&hspi1, spi_tx_buf, spi_rx_buf, SPI_BUF_SIZE);
    }
}
