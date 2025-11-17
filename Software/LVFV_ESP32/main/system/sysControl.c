/**
 * @file SysControl.c
 * @author Andrenacci - Carra
 * @brief Archivo de funciones que controlan el sistema físico a través del puerto SPI contra el STM32. En función de las respuestas del STM32, es que se controlan las variables internas de sistema del ESP32
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "./io_control/io_control.h"
#include "../LVFV_system.h"
#include "./display/display.h"
#include "./adc/adc.h"
#include "./SysAdmin.h"
#include "./SysControl.h"

// ===== Pines =====
#define PIN_NUM_CS                      12                      /** @def PIN_NUM_CS @brief Chip select del SPI para comunicación con el STM32. IO12 */
#define PIN_NUM_CLK                     14                      /** @def PIN_NUM_CLK @brief Clock del SPI para comunicación con el STM32. IO14 */
#define PIN_NUM_MISO                    26                      /** @def PIN_NUM_MISO @brief master input slave output del SPI para comunicación con el STM32. IO26 */
#define PIN_NUM_MOSI                    25                      /** @def PIN_NUM_MOSI @brief master output slave input del SPI para comunicación con el STM32. IO25 */

// ===== SPI =====
#define SPI_HOST_USED                   SPI2_HOST               /** @def SPI_HOST_USED @brief don't know */
#define SPI_CLOCK_HZ                    1*1000*1000             /** @def SPI_CLOCK_HZ @brief Velocidad de clock: 1 MHz */
#define SPI_QUEUE_TX_DEPTH              1                       /** @def SPI_QUEUE_TX_DEPTH @brief Profundidad de comandos para el puerto SPI */

static const char *TAG = "sysControl";                          /** @var TAG @brief Etiqueta para imprimir con ESP_LOG */

QueueHandle_t system_event_queue = NULL;                        /** @var system_event_queue @brief Cola de eventos para enviar comandos al STM32 */

static spi_device_handle_t spi_handle = NULL;                   // Handler del puerto SPI para comunicación con el STM32. Inicializado en esp_err_t SPI_Init(void)

/**
 * @fn static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item);
 *
 * @brief Envía el comando configurado en @p spi_cmd_item->request con el argumento  @p spi_cmd_item->setValue en caso de ser necesario y obtiene los datos que responde el STM32 en  @p spi_cmd_item->getValue si corresponde 
 *
 * @details El ESP32 debe enviar comandos en dos instancias. La primera envía el comando deseado por el usuario, se detiene la ejecución durante 200 mili segundos y luego se envía un nuevo comando para consultarle al STM32 si el comando enviado por el usuario fue recibido correctamente o no.
 *
 * @param[inout] spi_cdm_item
 *      Estrutura de datos con los parámetros necesarios para llevar a cabo la comunicación. Allí se almacena el comando, valores a enviar y valores recibidos.
 *
 * @return
 *  - SPI_RESPONSE_ERR: Error en la transmisión del comando por problemas de inicialización del handler. El comando no sale del ESP32
 *  - SPI_RESPONSE_ERR_CMD_UNKNOWN: El comando enviado no está dentro los posibles
 *  - SPI_RESPONSE_ERR_MOVING: Se está intentando enviar un comando de start con el motor en movimiento
 *  - SPI_RESPONSE_ERR_NOT_MOVING: Se está intentando enviar un comando de stop con el motor detenido
 *  - SPI_RESPONSE_ERR_DATA_MISSING: Faltó enviar un dato dentro de la trama
 *  - SPI_RESPONSE_ERR_DATA_INVALID: Los datos enviados como argumento dentro del comando están fuera de rango
 *  - SPI_RESPONSE_OK: La comunicación entre ESP32 y STM32 fue exitosa
 */
static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item);

static SPI_Response SPI_SendRequest(spi_cmd_item_t *spi_cmd_item) {
    
    uint8_t tx_buffer[4];
    uint8_t rx_buffer[4];
    esp_err_t ret;
    int flagDevolverValor;      // Se necesita devolver el valor por parametro

    ESP_LOGI( TAG, "[SPI Module] Request %d", spi_cmd_item->request);

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
        ESP_LOGI( TAG, "[ESP32] Error en SPI transmit: %d", ret);
        return SPI_RESPONSE_ERR;
    }

    tx_buffer[0] = SPI_REQUEST_RESPONSE;
    tx_buffer[1] = ';';

    t.length = 8 * 4;
    t.tx_buffer = tx_buffer;
    t.rx_buffer = rx_buffer;
    t.rxlength = 8 * 4;

    vTaskDelay(pdMS_TO_TICKS(400));

    ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGI( TAG, "[ESP32] Error en SPI transmit: %d", ret);
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
    if ( ret != ESP_OK ) {
        return ret;
    }
    ESP_ERROR_CHECK(ret);

    // Añadir dispositivo
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_handle);
    ESP_ERROR_CHECK(ret);
    return ESP_OK;
}

void SPI_communication(void *arg) {
    spi_cmd_item_t item;

    systemSignal_e new_button;

    if ( SPI_Init() != ESP_OK) {
        ESP_LOGE(TAG, "SPI_Init falló; no creo SPI_communication");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    if (system_event_queue == NULL) {
        system_event_queue = xQueueCreate(1, sizeof(systemSignal_e));
        if (system_event_queue == NULL) {
            ESP_LOGE(TAG, "No se pudo crear la cola de interrupciones");
            ESP_ERROR_CHECK(ESP_FAIL);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(4000));

    do {
        item.request = SPI_REQUEST_STOP;
        item.setValue = 0;
        item.getValue = 0;
        vTaskDelay(pdMS_TO_TICKS(100));
    } while ( SPI_SendRequest(&item) != SPI_RESPONSE_ERR_NOT_MOVING);

    engine_emergency_stop();
    for (uint8_t i = 0; i < 10; i++) { 
        readADC();
        xQueueReceive( system_event_queue, &new_button, pdMS_TO_TICKS(20) );
    }
    engine_stop();

    ESP_LOGI(TAG, "SPI_communication lista. Esperando comandos...");

    while (1) {
        system_status_t s_e;
        get_status( &s_e );
        readADC();
        if ( xQueueReceive( system_event_queue, &new_button, pdMS_TO_TICKS(20) ) ) {
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

                    if ( s_e.status == SYSTEM_IDLE ) {
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
                        uint16_t dest = engine_start();
                        ESP_LOGI(TAG, "Motor arrancado. Frecuencia destino: %d", dest);
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
                    RelayEvantPost( 1 );
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
                    
                    if ( s_e.status == SYSTEM_REGIME || s_e.status == SYSTEM_ACCLE_DESACCEL ) {
                        ESP_LOGI(TAG, "Cambio de velocidad: %d", new_button - SPEED_SELECTOR_0);
                        uint8_t old_input_status = s_e.inputs_status;
                        item.request = SPI_REQUEST_SET_FREC;
                        item.setValue = change_frequency( new_button - SPEED_SELECTOR_0 );
                        item.getValue = 0;
                        if ( SPI_SendRequest(&item) != SPI_RESPONSE_OK ) {
                            change_frequency( old_input_status );
                            ESP_LOGE(TAG, "El STM32 no respondio correctamente");
                            xQueueSend(system_event_queue, &new_button, pdMS_TO_TICKS(1000));
                        }
                    } else {
                        ESP_LOGI(TAG, "No puede cambiar la velocidad, status = %d", s_e.status);
                    }
                    break;
                default:
                    break;
            }            
        }
    }
}

esp_err_t SystemEventPost(systemSignal_e event) {
    if (system_event_queue == NULL) {
        return ESP_FAIL;
    }
    return xQueueSend(system_event_queue, &event, 0);
}