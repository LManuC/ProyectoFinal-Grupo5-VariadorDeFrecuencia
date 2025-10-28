/**
 * @file MCP23017.c
 * @author Andrenacci - Carra
 * @brief MCP23017 es un expansor de entradas y salidas que se controla a través de puerto I2C. 
 * @details Este chip permite usar sus dos puertos de 8 bits por separado o como uno solo de 16 bits. Tiene la función de reportar una interrupción configurable por puerto a través de 2 pines, uno por puerto, que informar si hubo algún cambio en las entradas si así lo deseo; estos pines de interrupción pueden ser configurados también para que actúen en conjunto o separado.
 Para su funcionamiento de direccionamiento tiene dos modos: "Byte mode" o "Sequencial mode" que se configuran en el registro IOCON que, aunque en el mapa de registros esté duplicado, es compartido entre ambos puerto, por lo que acceder por una dirección u otra es indiferente:
    - Byte mode: no incrementa el puntero interno de direcciones en cada comando enviado, sino que en cada escritura/lectura se le debe indicar a que dirección de memoria se desea acceder si no se desea acceder a la última dirección de memoria accedida. Esto permite hacer polling en el mismo registro, con la intención de estar monitoreando contínuamente la/las entradas de interés. Utilizando el modo ICON.BANK = 0, habilita una función especial que permite tooglear entre los registros del puerto A y B secuencialmente en cada acceso al chip.
    - Sequenctial mode: El puntero de dirección interno del chip se incrementa a cada byte de datos enviado. Cuando llega al último registro del mapa de registro hace un rollover al registro 0x00.

La secuencia de escritura se define de la siguiente amnera:
    S 0100AAA0 ADDR DIN P
 * @version 0.1
 * @date 2025-08-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "MCP23017.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/semphr.h"

static esp_err_t mcp_register_read(uint8_t register_address, uint8_t *data);
static esp_err_t mcp_register_write(uint8_t register_address, uint8_t data);

static const char *TAG = "MCP23017";
SemaphoreHandle_t xI2C_Mutex = NULL;


#define I2C_MASTER_TIMEOUT_MS   100
#define I2C_MASTER_NUM          1
#define I2C_MASTER_SCL_IO           15      /*!< GPIO para SCL */
#define I2C_MASTER_SDA_IO           2       /*!< GPIO para SDA */
#define I2C_MASTER_NUM              1       /*!< I2C port number */
#define I2C_MASTER_FREQ_HZ          100000  /*!< frecuencia estándar */
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                                       I2C_MASTER_RX_BUF_DISABLE,
                                       I2C_MASTER_TX_BUF_DISABLE, 0));
    if (xI2C_Mutex == NULL) {
        xI2C_Mutex = xSemaphoreCreateMutex();
        if (xI2C_Mutex == NULL) {
            ESP_LOGE(TAG, "No se pudo crear el mutex de I2C");
        }
    }

    return ESP_OK;
}

static bool i2c_is_hardware_free() {
    while ( xSemaphoreTake(xI2C_Mutex, pdMS_TO_TICKS(100)) != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        return false;
    }
    return true;
}

static void i2c_release_hardware() {
    xSemaphoreGive(xI2C_Mutex);
}

/**
 * @fn esp_err_t MCP23017_INIT( void )
 * 
 * @brief Inicializa el MCP23017 escribiendo en los diferentes registros
 * 
 * @details La función está explícitamente pensada para el PCB del variador de frecuencia. Setea pull-ups en los pines de entrada de la matriz de pulsadores y en las entradas aisladas a través de los optoacopladores, configura las salidas, interrupciones y los valores lógicos que disparan la interrupción.
 *
 * @return esp_err_t 
 */
esp_err_t MCP23017_INIT( void ) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Iniciando modulo");
    if ( i2c_master_init() == ESP_OK ) {
        ESP_LOGI(TAG, "I2C Inicializado correctamente");
    } else {
        ESP_LOGE(TAG, "Error al inicializar I2C");
        return ESP_FAIL;
    }
    MCP23017_IOCON_t iocon;
    MCP23017_GPPU_t gppua, gppub;
    MCP23017_IODIR_t iodira, iodirb;
    MCP23017_GPINTEN_t gpintena, gpintenb;
    MCP23017_DEFVAL_t defvala, defvalb;
    MCP23017_INTCON_t intcona, intconb;

    iocon.all = 0x00;
    iocon.bits.INTPOL = __MCP23017_INTPOL_POLARITY_HIGH__;          // Los pines de interrupción se ponen en 1 cuando hay una interrupción
    iocon.bits.ODR = __MCP23017_ODR_ACTIVE_DRIVER_OUTPUT__;         // Los pines de interrupción configuran como salidas activas
    iocon.bits.DISSLW = __MCP23017_DISSLW_ENABLED__;                // Activo el Slew Rate en el SDA para una mejor comunicación
    iocon.bits.SEQOP = __MCP23017_SEQOP_DISABLED__;                 // No activo el incremento del puntero de direcciones para poder leer constantemente el mismo registro si quiero
    iocon.bits.MIRROR = __MCP23017_MIRROR_UNCONNECTED__;            // Los pines de interrupción A y B se controlan por separado
    iocon.bits.BANK = __MCP23017_FUNC_MODE__;                       // Uso el modo Byte para tratar los puertos como si guesen de 8 bits y no de 16

    gppua.all = __MCP23017_GPPU_PULL_UP_DISABLE__;

    gppub.all = __MCP23017_GPPU_PULL_UP_DISABLE__;
    gppub.bits.PU0 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppub.bits.PU1 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppub.bits.PU2 = __MCP23017_GPPU_PULL_UP_ENABLE__;
    gppub.bits.PU3 = __MCP23017_GPPU_PULL_UP_ENABLE__;

    iodira.all = 0x00;
    iodira.bits.IO0 = __MCP23017_IODIR_OUTPUT__;
    iodira.bits.IO1 = __MCP23017_IODIR_OUTPUT__;
    iodira.bits.IO2 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO3 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO4 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO5 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO6 = __MCP23017_IODIR_INPUT__;
    iodira.bits.IO7 = __MCP23017_IODIR_OUTPUT__;

    iodirb.all = 0x00;
    iodirb.bits.IO0 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO1 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO2 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO3 = __MCP23017_IODIR_INPUT__;
    iodirb.bits.IO4 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO5 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO6 = __MCP23017_IODIR_OUTPUT__;
    iodirb.bits.IO7 = __MCP23017_IODIR_OUTPUT__;

    gpintena.all = 0x00;
    gpintena.bits.GPINT2 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT3 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT4 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT5 = __MCP23017_GPINTEN_ENABLE__;
    gpintena.bits.GPINT6 = __MCP23017_GPINTEN_ENABLE__;

    gpintenb.all = 0x00;
    gpintenb.bits.GPINT0 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT1 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT2 = __MCP23017_GPINTEN_ENABLE__;
    gpintenb.bits.GPINT3 = __MCP23017_GPINTEN_ENABLE__;

    defvala.all = 0x00;
    defvala.bits.DEF2 = 1;
    defvala.bits.DEF3 = 1;
    defvala.bits.DEF4 = 1;
    defvala.bits.DEF5 = 1;
    defvala.bits.DEF6 = 0;

    defvalb.all = 0x0F;

    intcona.all = __MCP23017_INTCON_CHANGE__;

    intconb.all = __MCP23017_INTCON_CHANGE__;

    ret = mcp_register_write( (uint8_t) MCP23017_IOCON_REGISTER     , iocon.all     );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro IOCON");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_IODIRA_REGISTER    , iodira.all    );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro IODIRA");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_IODIRB_REGISTER    , iodirb.all    );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro IODIRB");
        return ret;
    }
    // ret = mcp_register_write( (uint8_t) MCP23017_IPOLB_REGISTER     , iodira.all );  // Mantienen reset val
    ret = mcp_register_write( (uint8_t) MCP23017_GPINTENA_REGISTER  , gpintena.all  );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro GPINTENA");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_GPINTENB_REGISTER  , gpintenb.all  );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro GPINTENB");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_DEFVALA_REGISTER   , defvala.all   );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALA");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_DEFVALB_REGISTER   , defvalb.all   );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALB");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_INTCONA_REGISTER   , intcona.all   );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALB");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_INTCONB_REGISTER   , intconb.all   );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALB");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_GPPUA_REGISTER     , gppua.all     );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALB");
        return ret;
    }
    ret = mcp_register_write( (uint8_t) MCP23017_GPPUB_REGISTER     , gppub.all     );  //
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "Error al escribir en el registro DEFVALB");
        return ret;
    }
    // ret = mcp_register_write( (uint8_t) MCP23017_INTFA_REGISTER     , iodira.all    );   // Read only
    // ret = mcp_register_write( (uint8_t) MCP23017_INTFB_REGISTER     , iodirb.all    );   // Read only
    // ret = mcp_register_write( (uint8_t) MCP23017_INTCAPA_REGISTER   , gpintena.all  );   // Read only
    // ret = mcp_register_write( (uint8_t) MCP23017_INTCAPB_REGISTER   , gpintenb.all  );   // Read only
    // ret = mcp_register_write( (uint8_t) MCP23017_GPIOA_REGISTER     , defvala.all   );   // Operational registers
    // ret = mcp_register_write( (uint8_t) MCP23017_GPIOB_REGISTER     , defvalb.all   );   // Operational registers
    // ret = mcp_register_write( (uint8_t) MCP23017_OLATA_REGISTER     , intcona.all   );   // Operational registers
    // ret = mcp_register_write( (uint8_t) MCP23017_OLATB_REGISTER     , intconb.all   );   // Operational registers

    return ret;
}

esp_err_t mcp_get_on_interrupt_input(enum mcp_port_e port, uint8_t *data) {
    while ( i2c_is_hardware_free() != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    esp_err_t esp_err;
    if ( port == __MCP23017_PORT_A__ || port == __MCP23017_PORT_B__ ) {
        esp_err = mcp_register_read(MCP23017_INTCAPA_REGISTER + port, data);
    } else {
        esp_err = ESP_ERR_INVALID_ARG;
    }
    i2c_release_hardware();
    return esp_err;
}

esp_err_t mcp_write_output_pin(enum mcp_port_e port, uint8_t pin, bool state) {
    esp_err_t esp_err;
    uint8_t data;

    while ( i2c_is_hardware_free() != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if ( port == __MCP23017_PORT_A__ || port == __MCP23017_PORT_B__ ) {
        if ( !(esp_err = mcp_register_read(MCP23017_GPIOA_REGISTER + port, &data) ) ) {
            if (state ) {
                data |= (1 << pin);
            } else {
                data &= ~(1 << pin);
            }
            esp_err = mcp_register_write(MCP23017_OLATA_REGISTER + port, data);
        }
    } else {
        esp_err = ESP_ERR_INVALID_ARG;
    }
    i2c_release_hardware();
    return esp_err;
}

esp_err_t mcp_write_output_port(enum mcp_port_e port, uint8_t data) {
    esp_err_t esp_err;

    while ( i2c_is_hardware_free() != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if ( port == __MCP23017_PORT_A__ || port == __MCP23017_PORT_B__ ) {
        esp_err = mcp_register_write(MCP23017_OLATA_REGISTER + port, data);
    } else {
        esp_err = ESP_ERR_INVALID_ARG;
    }
    i2c_release_hardware();
    return esp_err;
}

esp_err_t mcp_read_port(enum mcp_port_e port, uint8_t *data) {
    esp_err_t esp_err;

    while ( i2c_is_hardware_free() != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if ( port == __MCP23017_PORT_A__ || port == __MCP23017_PORT_B__ ) {
        esp_err = mcp_register_read(MCP23017_GPIOA_REGISTER + port, data);
    } else {
        esp_err = ESP_ERR_INVALID_ARG;
    }
    i2c_release_hardware();
    return esp_err;
}

esp_err_t mcp_interrupt_flag(enum mcp_port_e port, uint8_t *data) {
    esp_err_t esp_err;

    while ( i2c_is_hardware_free() != pdTRUE ) {
        ESP_LOGI(TAG, "Espero recursos de hardware I2C...");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if ( port == __MCP23017_PORT_A__ || port == __MCP23017_PORT_B__ ) {
        esp_err = mcp_register_read(MCP23017_INTFA_REGISTER + port, data);
    } else {
        esp_err = ESP_ERR_INVALID_ARG;
    }
    i2c_release_hardware();
    return esp_err;
}

static esp_err_t mcp_register_read(uint8_t register_address, uint8_t *data) {
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_WRITE_OPPCODE__,0);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_READ_OPPCODE__,0);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mcp_register_write(uint8_t register_address, uint8_t data) {
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, __MCP23017_WRITE_OPPCODE__,0);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}