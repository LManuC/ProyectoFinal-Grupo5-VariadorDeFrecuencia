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
#define PIN_NUM_CS     12
#define PIN_NUM_CLK    14
#define PIN_NUM_MISO   26
#define PIN_NUM_MOSI   25

// ===== SPI =====
#define SPI_HOST_USED      SPI2_HOST     // HSPI
#define SPI_CLOCK_HZ       1*1000*1000           // 1 MHz
#define SPI_QUEUE_TX_DEPTH 1

static const char *TAG = "SPI";

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
    SPI_RESPONSE_ERR = 0xA0,                    // 160
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

static spi_device_handle_t spi_handle = NULL;

/* Item en cola: comando + valor (usar -1 si no aplica) */
typedef struct {
    SPI_Request request;
    int getValue;      // 0..999 para setpoints; -1 si no aplica
    int setValue;
} spi_cmd_item_t;


static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item);

/* =================== Inicialización del bus =================== */
esp_err_t SPI_Init(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
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

    vTaskDelay(pdMS_TO_TICKS(2000));

    for (uint8_t i = 0; i < 10; i++) { 
        xQueueReceive( bus_meas_evt_queue, &bus_meas, pdMS_TO_TICKS(portMAX_DELAY ) );
        update_meas(bus_meas);
        xQueueReceive( isolated_inputs_queue, &new_button, pdMS_TO_TICKS(20) );
    }
    engine_stop();
    do {
        item.request = SPI_REQUEST_STOP;
        item.setValue = 0;
        item.getValue = 0;
    } while ( SPI_SendRequest(&item) != SPI_RESPONSE_ERR_NOT_MOVING);
    
    ESP_LOGI(TAG, "SPI_communication lista. Esperando comandos...");

    while (1) {
        // ESP_LOGI(TAG, "STATUS = %d", get_system_status() );
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
                case STOP_PRESSED:
                    engine_stop();
                    ESP_LOGI(TAG, "Botón de Parada presionado");
                    item.request = SPI_REQUEST_STOP;
                    item.setValue = 0;
                    item.getValue = 0;
                    SPI_SendRequest(&item);
                    break;
                case TERMO_SW_RELEASED:
                    ESP_LOGI(TAG, "Termoswitch desactivado");
                    break;
                case EMERGENCI_STOP_RELEASED:
                    ESP_LOGI(TAG, "Botón de EMERGENCIA liberado");
                    break;
                case START_PRESSED:
                    if ( get_system_status() == SYSTEM_IDLE ) {
                        ESP_LOGI(TAG, "Botón de Inicio presionado");
                        SPI_Response SPI_commando_response;
                        
                        item.request = SPI_REQUEST_SET_FREC;
                        item.setValue = get_system_frequency();
                        item.getValue = 0;
                        SPI_commando_response = SPI_SendRequest(&item);
                        if ( SPI_commando_response != SPI_RESPONSE_OK ) {
                            ESP_LOGE(TAG,"Error cargando la frecuencia");
                            break;
                        }
                        
                        item.request = SPI_REQUEST_SET_ACEL;
                        item.setValue = get_system_acceleration();
                        item.getValue = 0;
                        SPI_commando_response = SPI_SendRequest(&item);
                        if ( SPI_commando_response != SPI_RESPONSE_OK ) {
                            ESP_LOGE(TAG,"Error cargando la aceleracion");
                            break;
                        }
                        
                        item.request = SPI_REQUEST_SET_DESACEL;
                        item.setValue = get_system_desacceleration();
                        item.getValue = 0;
                        SPI_commando_response = SPI_SendRequest(&item);
                        if ( SPI_commando_response != SPI_RESPONSE_OK ) {
                            ESP_LOGE(TAG,"Error cargando la desaceleracion");
                            break;
                        }
                        
                        item.request = SPI_REQUEST_START;
                        item.setValue = 0;
                        item.getValue = 0;
                        SPI_commando_response = SPI_SendRequest(&item);
                        if ( SPI_commando_response != SPI_RESPONSE_OK ) {
                            ESP_LOGE(TAG,"Error arrancando el motor");
                            break;
                        }
                        engine_start();
                    } else {
                        ESP_LOGE(TAG,"El motor debe estar detenido para poder arrancarlo");
                    }
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
                    uint8_t old_speed_selector = get_speed_selector();
                    system_status_e status = get_system_status();
                    if ( status == SYSTEM_REGIME || status == SYSTEM_ACCLE_DESACCEL ) {
                        ESP_LOGI(TAG, "Cambio de velocidad: %d", new_button - SPEED_SELECTOR_0);
                        item.request = SPI_REQUEST_SET_FREC;
                        item.setValue = change_frequency( new_button - SPEED_SELECTOR_0 );
                        item.getValue = 0;
                        if ( SPI_SendRequest(&item) != SPI_RESPONSE_OK ) {
                            // change_frequency( old_speed_selector );
                            ESP_LOGE(TAG, "El STM32 no respondio correctamente");
                        }
                    } else {
                        ESP_LOGI(TAG, "No puede cambiar la velocidad, status = &d", status);
                    }
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



    ESP_LOGI( TAG, "rx_buffer[0]: %d", rx_buffer[0]);
    if(flagDevolverValor && rx_buffer[0] == SPI_RESPONSE_OK) {
        
        spi_cmd_item->getValue = rx_buffer[1] + rx_buffer[2];
        ESP_LOGI( TAG, "RxValores: %d - %d", rx_buffer[1], rx_buffer[2]);
        
    }else {
        spi_cmd_item->getValue = 0;
    }
    
    return rx_buffer[0];
}