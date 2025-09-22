#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "SPI_Module.h"

#define SPI_REQUEST_RESPONSE   0x50  // Comando para recibir la respuesta

#define PIN_NUM_MISO 19     // rojo
#define PIN_NUM_MOSI 23     // marron
#define PIN_NUM_CLK  18     // naranja
#define PIN_NUM_CS   5      // amarillo


spi_device_handle_t spi_handle;

void SPI_Init() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1*1000*1000,           // 1 MHz
        .mode = 0,                              // SPI mode 0
        .spics_io_num = PIN_NUM_CS,             // CS pin
        .queue_size = 1,
        .flags = SPI_DEVICE_NO_DUMMY            // Sin dummy bits
    };

    // Inicializar bus SPI
    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);

    // Añadir dispositivo
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_handle);
    ESP_ERROR_CHECK(ret);
}

SPI_Response SPI_SendRequest(SPI_Request request, int setValue, int* getValue) {
    
    uint8_t tx_buffer[4];
    uint8_t rx_buffer[4];
    esp_err_t ret;
    int flagDevolverValor;      // Se necesita devolver el valor por parametro

    printf("[SPI Module] Request %d\n", request);

    // Chequeo de errores fuera de rango y comando desconocido
    if(setValue > 512 || setValue < 0) {
        return SPI_RESPONSE_ERR_DATA_INVALID;
    }

    if(request < SPI_REQUEST_START || request >= SPI_REQUEST_LAST) {
        printf("[SPI Module] Comando desconocido\n");
        return SPI_RESPONSE_ERR_CMD_UNKNOWN;
    }

    // Es de los comandos que deben devolver un valor por parametro?
    if(request >= SPI_REQUEST_GET_FREC && request <= SPI_REQUEST_IS_STOP) {
        flagDevolverValor = 1;
    }else {
        flagDevolverValor = 0;
    }

    // Se arma la cadena a enviar
    tx_buffer[0] = request;

    if(setValue > 255) {
        tx_buffer[1] = 255;
        tx_buffer[2] = setValue - 255;
        tx_buffer[3] = ';';
    }else {
        tx_buffer[1] = setValue;
        tx_buffer[2] = 0;
        tx_buffer[3] = ';';
    }


    // Limpiamos el buffer de recepcion
    memset(rx_buffer, 0, sizeof(rx_buffer));

    // Siempre envio y recibo 4 bytes
    spi_transaction_t t = {
        .length = 8 * 4,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
        .rxlength = 8 * 4,
    };

    ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        printf("[ESP32] Error en SPI transmit: %d\n", ret);
        return SPI_RESPONSE_ERR;
    }

    tx_buffer[0] = SPI_REQUEST_RESPONSE;
    tx_buffer[1] = ';';

    t.length = 8 * 4;
    t.tx_buffer = tx_buffer;
    t.rx_buffer = rx_buffer;
    t.rxlength = 8 * 4;

    vTaskDelay(pdMS_TO_TICKS(200));

    ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        printf("[ESP32] Error en SPI transmit: %d\n", ret);
        return SPI_RESPONSE_ERR;
    }



    if(flagDevolverValor && rx_buffer[0] == SPI_RESPONSE_OK) {
        
        *getValue = rx_buffer[1] + rx_buffer[2];
        printf("RxValores: %d - %d", rx_buffer[1], rx_buffer[2]);
        
    }else {
        *getValue = 0;
    }
    
    return rx_buffer[0];
}

