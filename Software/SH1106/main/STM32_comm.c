// ⚠️ Aviso importante (ESP32 clásico): GPIO12 (MTDI) es pin de strap que define el voltaje del flash. 
// Si queda alto durante el boot, el chip puede intentar 1.8 V y no arrancar. Usarlo como CS es arriesgado 
// (CS suele estar alto en reposo). Si podés, cambiá CS a un pin no-strap (p. ej. GPIO27). Si debés usar 
// GPIO12: agregá un pull-down fuerte (10 k) a GND y asegurate de que el dispositivo esclavo no lo tire 
// alto en el reset.

// spi_comm.c
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "./MCP23017/io_control.h"
#include "./LVFV_system.h"
#include "./SH1106/display.h"

// ===== Pines =====
#define PIN_NUM_CS     12   // ⚠️ Strap pin MTDI (ver nota más abajo)
#define PIN_NUM_SCK    14
#define PIN_NUM_MISO   26
#define PIN_NUM_MOSI   25

// ===== SPI =====
#define SPI_HOST_USED      SPI2_HOST     // HSPI
#define SPI_CLOCK_HZ       (1*1000*1000) // 1 MHz
#define SPI_QUEUE_TX_DEPTH 1

static const char *TAG = "SPI";

static spi_device_handle_t s_spi_dev = NULL;

extern QueueHandle_t isolated_inputs_queue;
extern QueueHandle_t bus_meas_evt_queue;

typedef enum {
    SPI_REQUEST_START = 10,
    SPI_REQUEST_STOP,
    SPI_REQUEST_SET_FREC,
    SPI_REQUEST_SET_ACEL,
    SPI_REQUEST_SET_DESACEL,
    SPI_REQUEST_SET_DIR,
    SPI_REQUEST_GET_FREC,
    SPI_REQUEST_GET_ACEL,
    SPI_REQUEST_GET_DESACEL,
    SPI_REQUEST_GET_DIR,
    SPI_REQUEST_IS_STOP,
    SPI_REQUEST_EMERGENCY,
} SPI_Request;

typedef enum {
    SPI_RESPONSE_ERR = 0xA0,
    SPI_RESPONSE_ERR_CMD_UNKNOWN,               // Comando desconocido
    SPI_RESPONSE_ERR_NO_COMMAND,                // Llego mensaje pero sin comando
    SPI_RESPONSE_ERR_MOVING,                    // Comando Start pero motor ya esta en movimiento
    SPI_RESPONSE_ERR_NOT_MOVING,                // Comando Stop pero motor no esta en movimiento    
    SPI_RESPONSE_ERR_DATA_MISSING,              // Comando que requiere datos pero no llegaron
    SPI_RESPONSE_ERR_DATA_INVALID,              // Comando que tiene datos invalidos
    SPI_RESPONSE_ERR_DATA_OUT_RANGE,            // Comando con datos fuera de rango permitido
    SPI_RESPONSE_OK = 0xFF,
} SPI_Response;

#define SPI_REQUEST_RESPONSE   0x50  // Comando para recibir la respuesta

spi_device_handle_t spi_handle;

/* Item en cola: comando + valor (usar -1 si no aplica) */
typedef struct {
    SPI_Request request;
    int getValue;      // 0..999 para setpoints; -1 si no aplica
    int setValue;
} spi_cmd_item_t;


static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item);

/* =================== Inicialización del bus =================== */
esp_err_t SPI_Init(void) {
    esp_err_t err;

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };

    err = spi_bus_initialize(SPI_HOST_USED, &buscfg, SPI_DMA_DISABLED);

    if (err != ESP_OK) {
        return err;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,                    // CPOL=0, CPHA=0
        .spics_io_num = PIN_NUM_CS,   // ⚠️ ver nota sobre GPIO12
        .queue_size = SPI_QUEUE_TX_DEPTH,
    };

    err = spi_bus_add_device(SPI_HOST_USED, &devcfg, &s_spi_dev);

    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "SPI OK: CS=%d SCK=%d MISO=%d MOSI=%d clk=%d Hz",
             PIN_NUM_CS, PIN_NUM_SCK, PIN_NUM_MISO, PIN_NUM_MOSI, SPI_CLOCK_HZ);
    return ESP_OK;
}

/* =================== Tarea de comunicación =================== */
void SPI_communication(void *arg) {
    spi_cmd_item_t item;

    uint8_t new_button;
    seccurity_settings_t bus_meas;

    while( isolated_inputs_queue == NULL ) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "SPI_communication lista. Esperando comandos...");

    while (1) {
        if ( xQueueReceive( bus_meas_evt_queue, &bus_meas, pdMS_TO_TICKS(20) ) ) {
            update_meas(bus_meas);
        }
        if ( xQueueReceive( isolated_inputs_queue, &new_button, pdMS_TO_TICKS(20) ) ) {
            switch ( new_button ) {
                case EMERGENCI_STOP_PRESSED:
                    ESP_LOGI(TAG, "Botón de EMERGENCIA presionado");
                    item.request = SPI_REQUEST_EMERGENCY;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    engine_emergency_stop();
                    break;
                case TERMO_SW_RELEASED:
                case EMERGENCI_STOP_RELEASED:
                    if ( new_button == TERMO_SW_RELEASED ) {
                        ESP_LOGI(TAG, "Termoswitch desactivado");
                    } else {
                        ESP_LOGI(TAG, "Botón de EMERGENCIA liberado");
                    }
                    item.request = SPI_REQUEST_STOP;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    break;
                case STOP_PRESSED:
                    ESP_LOGI(TAG, "Botón de Parada presionado");
                    engine_stop();
                    item.request = SPI_REQUEST_STOP;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    break;
                case STOP_RELEASED:
                    ESP_LOGI(TAG, "Botón de Parada liberado");
                    break;
                case START_PRESSED:
                    ESP_LOGI(TAG, "Botón de Inicio presionado");
                    engine_start();
                    item.request = SPI_REQUEST_START;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    break;
                case START_RELEASED:
                    ESP_LOGI(TAG, "Botón de Inicio liberado");
                    break;
                case SECURITY_EXCEDED:
                    ESP_LOGI(TAG, "Corriente elevada o tensión reducida");
                    item.request = SPI_REQUEST_EMERGENCY;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    engine_emergency_stop();
                    break;
                case TERMO_SW_PRESSED:
                    ESP_LOGI(TAG, "Termoswitch activo");
                    item.request = SPI_REQUEST_EMERGENCY;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    engine_emergency_stop();
                    break;
                case SPEED_SELECTOR_0:
                case SPEED_SELECTOR_1:
                case SPEED_SELECTOR_2:
                case SPEED_SELECTOR_3:
                case SPEED_SELECTOR_4:
                case SPEED_SELECTOR_5:
                case SPEED_SELECTOR_6:
                case SPEED_SELECTOR_7:
                case SPEED_SELECTOR_8:
                case SPEED_SELECTOR_9:
                    ESP_LOGI(TAG, "Cambio de velocidad: %d", new_button - SPEED_SELECTOR_0);
                    item.request = SPI_REQUEST_SET_FREC;
                    item.setValue = change_frequency( new_button - SPEED_SELECTOR_0 );
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    break;
            }            
        }
    }
}

static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item) {
    
    uint8_t tx_buffer[4];
    uint8_t rx_buffer[4];
    esp_err_t ret;
    int flagDevolverValor;      // Se necesita devolver el valor por parametro

    ESP_LOGI( TAG, "[SPI Module] Request %d\n", spi_cmd_item->request);

    // Chequeo de errores fuera de rango y comando desconocido
    if(spi_cmd_item->setValue > 512 || spi_cmd_item->setValue < 0) {
        return SPI_RESPONSE_ERR_DATA_INVALID;
    }

    if(spi_cmd_item->request < SPI_REQUEST_START || spi_cmd_item->request > SPI_REQUEST_EMERGENCY) {
        ESP_LOGI( TAG, "[SPI Module] Comando desconocido\n");
        return SPI_RESPONSE_ERR_CMD_UNKNOWN;
    }

    // Es de los comandos que deben devolver un valor por parametro?
    if(spi_cmd_item->request >= SPI_REQUEST_GET_FREC && spi_cmd_item->request <= SPI_REQUEST_IS_STOP) {
        flagDevolverValor = 1;
    }else {
        flagDevolverValor = 0;
    }

    // Se arma la cadena a enviar
    tx_buffer[0] = spi_cmd_item->request;

    if(spi_cmd_item->setValue > 255) {
        tx_buffer[1] = 255;
        tx_buffer[2] = spi_cmd_item->setValue - 255;
        tx_buffer[3] = ';';
    }else {
        tx_buffer[1] = spi_cmd_item->setValue;
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
        ESP_LOGI( TAG, "[ESP32] Error en SPI transmit: %d\n", ret);
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
        ESP_LOGI( TAG, "[ESP32] Error en SPI transmit: %d\n", ret);
        return SPI_RESPONSE_ERR;
    }



    if(flagDevolverValor && rx_buffer[0] == SPI_RESPONSE_OK) {
        
        spi_cmd_item->getValue = rx_buffer[1] + rx_buffer[2];
        ESP_LOGI( TAG, "RxValores: %d - %d", rx_buffer[1], rx_buffer[2]);
        
    }else {
        spi_cmd_item->getValue = 0;
    }
    
    return rx_buffer[0];
}