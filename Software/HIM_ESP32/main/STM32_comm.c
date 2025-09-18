#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp32.h"
#include "crc8.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdint.h>

#define PIN_NUM_MISO  23
#define PIN_NUM_MOSI  24
#define PIN_NUM_CLK   22
#define PIN_NUM_CS    21

/*
 *
 * ************************************ ESTRUCTURA DEL MENSAJE ***********************************
 *                HEADER  |   LEN |   STATUS  |   CMD |   DATA0   |   CRC8    |   FOOTER
 * ********************************************* LEN *********************************************
 *                                      1     |    2  |     3     |     4     |     5
 * ******************************************** STATUS *******************************************
 *                                  xxxxxxxx
 * ************************************** CAMBIO DE VELOCIDAD ************************************
 *                   >    |    7  |   STATUS  |   CMD | VELOCIDAD |   CRC8    |     <
 * ********************************************* ARRANQUE ****************************************
 *                   >    |    6  |   STATUS  |   CMD |   CRC8    |     <
 * ********************************************* PARADA ******************************************
 *                   >    |    6  |   STATUS  |   CMD |   CRC8    |     <
 * ************************************** PARADA POR ROTURA **************************************
 *                   >    |    7  |   STATUS  |   CMD |  REASON   |   CRC8    |     <
 */
 
#define SPI_MSJ_HEADER          '>'
#define SPI_MSJ_FOOTER          '<'
#define SPI_CMD_LENGTH          10

#define SPI_MSJ_HEADER_INDEX        0
#define SPI_MSJ_LENGTH_INDEX        1
#define SPI_MSJ_STATUS_INDEX        2
#define SPI_MSJ_COMMAND_INDEX       3
#define SPI_MSJ_DATA0_INDEX         4
#define SPI_MSJ_CRC8_INDEX(len)     5 - ( 7 - (len) )   //  5 - ( 7 - 7) = 5 - (0) = 5 ->   5 - ( 7 - 6) = 5 - (1) = 4
#define SPI_MSJ_FOOTER_INDEX(len)   6 - ( 7 - (len) )   //  6 - ( 7 - 7) = 6 - (0) = 6 ->   6 - ( 7 - 6) = 6 - (1) = 5

static const char *TAG = "SPI_COM";

QueueHandle_t cmd_queue;
spi_device_handle_t spi;
status_t status;
uint8_t freq_out_speed;

extern spi_device_handle_t spi;  // handle creado en spi_init()

void spi_send_buffer(const uint8_t *data, size_t len) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = len * 8;   // longitud en bits
    t.tx_buffer = data;

    esp_err_t ret = spi_device_transmit(spi, &t);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Comando enviado");
        xQueueSend(data[len-2], &cmd, portMAX_DELAY);
    } else {
        ESP_LOGE(TAG, "Error enviando: %s", esp_err_to_name(ret));
    }    
}

void spi_init(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 115200,              // baudrate
        .mode = 0,                             // SPI mode 0
        .spics_io_num = PIN_NUM_CS,            // CS
        .queue_size = 5,                       // transacciones en cola
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    ESP_LOGI(TAG, "SPI inicializado en pines MISO=%d, MOSI=%d, SCK=%d, CS=%d",
             PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
}

void spi_task(void *arg) {
    uint8_t rx_data;
    comando_t cmd;
    uint8_t trama[SPI_CMD_LENGTH];

    spi_init();

    while (1) {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY)) {
            trama[SPI_MSJ_HEADER_INDEX] = SPI_MSJ_HEADER;
            switch(cmd) {
                case CMD_ARRANQUE:
                case CMD_ARRANQUE_PROGRAMADO:
                case CMD_PARADA_EMERGENCIA:
                    trama[SPI_MSJ_LENGTH_INDEX] = 4;
                    break;
                case CMD_PARADA_ROTAURA:
                    trama[SPI_MSJ_LENGTH_INDEX] = 5;
                    trama[SPI_MSJ_DATA0_INDEX] = freq_out_speed;
                    
                    break;
                case CMD_CAMBIO_VELOCIDAD:
                    trama[SPI_MSJ_LENGTH_INDEX] = 5;
                    trama[SPI_MSJ_DATA0_INDEX] = freq_out_speed;
                    break;
            }
            trama[SPI_MSJ_STATUS_INDEX] = status.all;
            trama[3] = cmd;


            // if (cmd == CMD_PARADA_EMERGENCIA) {
            //     ESP_LOGW(TAG, "⚠️ PARADA DE EMERGENCIA");
            //     // acción inmediata
            // } else {
            //     ESP_LOGI(TAG, "Comando recibido: %d", cmd);
            //     // procesar otros comandos
            // }
        }
        memset(&t, 0, sizeof(t));
        t.length = 8;             // 8 bits
        t.rx_buffer = &rx_data;

        // esperar un byte desde SPI
        esp_err_t ret = spi_device_transmit(spi, &t);
        if (ret == ESP_OK) {
            comando_t cmd = (comando_t) rx_data;

            // encolar el comando
            xQueueSend(cmd_queue, &cmd, portMAX_DELAY);
        }
    }
}