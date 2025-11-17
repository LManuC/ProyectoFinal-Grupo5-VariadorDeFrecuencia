#ifndef __MCP23017_DEFS_H__

#define __MCP23017_DEFS_H__

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdbool.h>

#define __MCP23017_BANK_SEQUENTIAL__            0                           /*!< Modo de banqueo de moemora separado por puertos */
#define __MCP23017_BANK_SPLIT__                 1                           /*!< Modo de banqueo de moemora entrelazando puertos para manejo con entreos de 16 bits */

#define __MCP23017_MIRROR_UNCONNECTED__         0                           /*!< Los pines de interrupción no están internamente conectados */
#define __MCP23017_MIRROR_CONNECTED__           1                           /*!< Los pines de interrupción están internamente conectados */

#define __MCP23017_SEQOP_ENABLED__              0                           /*!< El puntero de direccionamiento de memoria incrementa en cada acceso */
#define __MCP23017_SEQOP_DISABLED__             1                           /*!< El puntero de direccionamiento de memoria no incrementa en cada acceso */

#define __MCP23017_DISSLW_DISABLED__            0                           /*!< Slew rate deshabilitado */
#define __MCP23017_DISSLW_ENABLED__             1                           /*!< Slew rate habilitado */

#define __MCP23017_ODR_OPEN_DRAIN_OUTPUT__      1                           /*!< Flags de interrupción de los puerto A y B configuradas open drain */
#define __MCP23017_ODR_ACTIVE_DRIVER_OUTPUT__   0                           /*!< Flags de interrupción de los puerto A y B configuradas como salidas activas */

#define __MCP23017_INTPOL_POLARITY_LOW__        1                           /*!< Flags de interrupción de los puerto A y B en 0 significa una nueva interrupción*/
#define __MCP23017_INTPOL_POLARITY_HIGH__       0                           /*!< Flags de interrupción de los puerto A y B en 1 significa una nueva interrupción*/

#define __MCP23017_GPPU_PULL_UP_ENABLE__        1                           /*!< Activación de pull up de GPIO */
#define __MCP23017_GPPU_PULL_UP_DISABLE__       0                           /*!< DESctivación de pull up de GPIO */

#define __MCP23017_IODIR_INPUT__                1                           /*!< GPIO configurada como entrada */
#define __MCP23017_IODIR_OUTPUT__               0                           /*!< GPIO configurada como salida */

#define __MCP23017_GPINTEN_ENABLE__             1                           /*!< Interrupción de GPIO habilitada */
#define __MCP23017_GPINTEN_DISABLE__            0                           /*!< Interrupción de GPIO deshabilitada */

#define __MCP23017_INTCON_DEFVAL__              1                           /*!< Interrupción dispara por diferencia con valor default */
#define __MCP23017_INTCON_CHANGE__              0                           /*!< Interrupción dispara por diferencia con útlimo valor */

#define __MCP23017_WRITE__                      0                           /*!< Función de escritura del MCP23017 - Se inserta en el opcode */
#define __MCP23017_READ__                       1                           /*!< Función de lectura del MCP23017 - Se inserta en el opcode */

#define __MCP23017_POSITIVE_LOGIC__             0                           /*!< Configuración de la lógica de los GPIO como positiva */
#define __MCP23017_OPPOSITE_LOGIC__             1                           /*!< Configuración de la lógica de los GPIO como negativa */

#define __MCP23017_INTERRUPT_ENABLE__           0                           /*!< Deshabilitación de la interrupción del GPIO */
#define __MCP23017_INTERRUPT_DISABLE__          1                           /*!< Habilitación de la interrupción del GPIO */

#define __MCP23017_ADDRESS__                    0b010                       /*!< CONFIGURADO POR HARDWARE: Address del MCP23017 =  0b010 ----> OPCODE: 0100AAA0 -> 01000000 | __MCP23017_ADDRESS__ << 1 | R/W */ 

#define __MCP23017_READ_OPPCODE__           0b0100 << 4 | __MCP23017_ADDRESS__ << 1 | __MCP23017_READ__     /*!< Código de operación para lectura en el MCP23017 */
#define __MCP23017_WRITE_OPPCODE__          0b0100 << 4 | __MCP23017_ADDRESS__ << 1 | __MCP23017_WRITE__    /*!< Código de operación para escritura en el MCP23017 */
#define __MCP23017_FUNC_MODE__              __MCP23017_BANK_SEQUENTIAL__                                    /*!< Modo de funcionamieto del MCP23017 que funcionará en el sistema */

#if __MCP23017_FUNC_MODE__ == __MCP23017_SEQUENTIAL_MODE__
    #define MCP23017_IODIRA_REGISTER            0x00                       /*!< Dirección de memoria del registro IODIRA */
    #define MCP23017_IODIRB_REGISTER            0x01                       /*!< Dirección de memoria del registro IODIRB */
    #define MCP23017_IPOLA_REGISTER             0x02                       /*!< Dirección de memoria del registro IPOLA */
    #define MCP23017_IPOLB_REGISTER             0x03                       /*!< Dirección de memoria del registro IPOLB */
    #define MCP23017_GPINTENA_REGISTER          0x04                       /*!< Dirección de memoria del registro GPINTENA */
    #define MCP23017_GPINTENB_REGISTER          0x05                       /*!< Dirección de memoria del registro GPINTENB */
    #define MCP23017_DEFVALA_REGISTER           0x06                       /*!< Dirección de memoria del registro DEFVALA */
    #define MCP23017_DEFVALB_REGISTER           0x07                       /*!< Dirección de memoria del registro DEFVALB */
    #define MCP23017_INTCONA_REGISTER           0x08                       /*!< Dirección de memoria del registro INTCONA */
    #define MCP23017_INTCONB_REGISTER           0x09                       /*!< Dirección de memoria del registro INTCONB */
    #define MCP23017_IOCON_REGISTER             0x0A                       /*!< Dirección de memoria del registro IOCON */
    // #define MCP23017_IOCON_REGISTER             0x0B                       /*!< Dirección de memoria del registro IOCON */
    #define MCP23017_GPPUA_REGISTER             0x0C                       /*!< Dirección de memoria del registro GPPUA */
    #define MCP23017_GPPUB_REGISTER             0x0D                       /*!< Dirección de memoria del registro GPPUB */
    #define MCP23017_INTFA_REGISTER             0x0E                       /*!< Dirección de memoria del registro INTFA */
    #define MCP23017_INTFB_REGISTER             0x0F                       /*!< Dirección de memoria del registro INTFB */
    #define MCP23017_INTCAPA_REGISTER           0x10                       /*!< Dirección de memoria del registro INTCAPA */
    #define MCP23017_INTCAPB_REGISTER           0x11                       /*!< Dirección de memoria del registro INTCAPB */
    #define MCP23017_GPIOA_REGISTER             0x12                       /*!< Dirección de memoria del registro GPIOA */
    #define MCP23017_GPIOB_REGISTER             0x13                       /*!< Dirección de memoria del registro GPIOB */
    #define MCP23017_OLATA_REGISTER             0x14                       /*!< Dirección de memoria del registro OLATA */
    #define MCP23017_OLATB_REGISTER             0x15                       /*!< Dirección de memoria del registro OLATB */
#elif __MCP23017_FUNC_MODE__ == __MCP23017_BYTE_MODE__
    #define MCP23017_IODIRA_REGISTER             0x00                       /*!< Dirección de memoria del registro IODIRA */
    #define MCP23017_IODIRB_REGISTER             0x10                       /*!< Dirección de memoria del registro IODIRB */
    #define MCP23017_IPOLA_REGISTER              0x01                       /*!< Dirección de memoria del registro IPOLA */
    #define MCP23017_IPOLB_REGISTER              0x11                       /*!< Dirección de memoria del registro IPOLB */
    #define MCP23017_GPINTENA_REGISTER           0x02                       /*!< Dirección de memoria del registro GPINTENA */
    #define MCP23017_GPINTENB_REGISTER           0x12                       /*!< Dirección de memoria del registro GPINTENB */
    #define MCP23017_DEFVALA_REGISTER            0x03                       /*!< Dirección de memoria del registro DEFVALA */
    #define MCP23017_DEFVALB_REGISTER            0x13                       /*!< Dirección de memoria del registro DEFVALB */
    #define MCP23017_INTCONA_REGISTER            0x04                       /*!< Dirección de memoria del registro INTCONA */
    #define MCP23017_INTCONB_REGISTER            0x14                       /*!< Dirección de memoria del registro INTCONB */
    #define MCP23017_IOCON_REGISTER              0x05                       /*!< Dirección de memoria del registro IOCON */
    // #define MCP23017_IOCON_REGISTER              0x15                       /*!< Dirección de memoria del registro IOCON */
    #define MCP23017_GPPUA_REGISTER              0x06                       /*!< Dirección de memoria del registro GPPUA */
    #define MCP23017_GPPUB_REGISTER              0x16                       /*!< Dirección de memoria del registro GPPUB */
    #define MCP23017_INTFA_REGISTER              0x07                       /*!< Dirección de memoria del registro INTFA */
    #define MCP23017_INTFB_REGISTER              0x17                       /*!< Dirección de memoria del registro INTFB */
    #define MCP23017_INTCAPA_REGISTER            0x08                       /*!< Dirección de memoria del registro INTCAPA */
    #define MCP23017_INTCAPB_REGISTER            0x18                       /*!< Dirección de memoria del registro INTCAPB */
    #define MCP23017_GPIOA_REGISTER              0x09                       /*!< Dirección de memoria del registro GPIOA */
    #define MCP23017_GPIOB_REGISTER              0x19                       /*!< Dirección de memoria del registro GPIOB */
    #define MCP23017_OLATA_REGISTER              0x0A                       /*!< Dirección de memoria del registro OLATA */
    #define MCP23017_OLATB_REGISTER              0x1A                       /*!< Dirección de memoria del registro OLATB */
#endif

/**
 * @typedef MCP23017_IODIR_t
 *
 * @brief Unión que define al registro IODIR y los bits que lo componen.
 *
 * @details IODIR es el registro que define la dirección de cada pin, si es entrada o salida.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t IO0 : 1;                                                    /*!< Bit 0 de IODIR */
        uint8_t IO1 : 1;                                                    /*!< Bit 1 de IODIR */
        uint8_t IO2 : 1;                                                    /*!< Bit 2 de IODIR */
        uint8_t IO3 : 1;                                                    /*!< Bit 3 de IODIR */
        uint8_t IO4 : 1;                                                    /*!< Bit 4 de IODIR */
        uint8_t IO5 : 1;                                                    /*!< Bit 5 de IODIR */
        uint8_t IO6 : 1;                                                    /*!< Bit 6 de IODIR */
        uint8_t IO7 : 1;                                                    /*!< Bit 7 de IODIR */
    } bits;
} MCP23017_IODIR_t;

/**
 * @typedef MCP23017_IPOL_t
 *
 * @brief Unión que define al registro IPOL y los bits que lo componen.
 *
 * @details IPOL es el registro que define si el valor del registro GPIO representa la misma lógica de estado del pin físico o su estado opuesto.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t IP0 : 1;                                                    /*!< Bit 0 de IPOL */
        uint8_t IP1 : 1;                                                    /*!< Bit 1 de IPOL */
        uint8_t IP2 : 1;                                                    /*!< Bit 2 de IPOL */
        uint8_t IP3 : 1;                                                    /*!< Bit 3 de IPOL */
        uint8_t IP4 : 1;                                                    /*!< Bit 4 de IPOL */
        uint8_t IP5 : 1;                                                    /*!< Bit 5 de IPOL */
        uint8_t IP6 : 1;                                                    /*!< Bit 6 de IPOL */
        uint8_t IP7 : 1;                                                    /*!< Bit 7 de IPOL */
    } bits;
} MCP23017_IPOL_t;

/**
 * @typedef MCP23017_GPINTEN_t
 *
 * @brief Unión que define al registro GPINTEN y los bits que lo componen.
 *
 * @details GPINTEN es el registro que activa o desactiva las interrupciones de cada pin.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t GPINT0 : 1;                                                    /*!< Bit 0 de GPINTEN */
        uint8_t GPINT1 : 1;                                                    /*!< Bit 1 de GPINTEN */
        uint8_t GPINT2 : 1;                                                    /*!< Bit 2 de GPINTEN */
        uint8_t GPINT3 : 1;                                                    /*!< Bit 3 de GPINTEN */
        uint8_t GPINT4 : 1;                                                    /*!< Bit 4 de GPINTEN */
        uint8_t GPINT5 : 1;                                                    /*!< Bit 5 de GPINTEN */
        uint8_t GPINT6 : 1;                                                    /*!< Bit 6 de GPINTEN */
        uint8_t GPINT7 : 1;                                                    /*!< Bit 7 de GPINTEN */
    } bits;
} MCP23017_GPINTEN_t;

/**
 * @typedef MCP23017_GPPU_t
 *
 * @brief Unión que define al registro GPPU y los bits que lo componen.
 *
 * @details GPPU es el registro que activa o desactiva los pull ups de cada pin.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t PU0 : 1;                                                    /*!< Bit 0 de GPPU */
        uint8_t PU1 : 1;                                                    /*!< Bit 1 de GPPU */
        uint8_t PU2 : 1;                                                    /*!< Bit 2 de GPPU */
        uint8_t PU3 : 1;                                                    /*!< Bit 3 de GPPU */
        uint8_t PU4 : 1;                                                    /*!< Bit 4 de GPPU */
        uint8_t PU5 : 1;                                                    /*!< Bit 5 de GPPU */
        uint8_t PU6 : 1;                                                    /*!< Bit 6 de GPPU */
        uint8_t PU7 : 1;                                                    /*!< Bit 7 de GPPU */
    } bits;
} MCP23017_GPPU_t;

/**
 * @typedef MCP23017_GPIO_t
 *
 * @brief Unión que define al registro GPIO y los bits que lo componen.
 *
 * @details GPIO es el registro que representa fielmente el estado del pin.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t GP0 : 1;                                                    /*!< Bit 0 de GPIO */
        uint8_t GP1 : 1;                                                    /*!< Bit 1 de GPIO */
        uint8_t GP2 : 1;                                                    /*!< Bit 2 de GPIO */
        uint8_t GP3 : 1;                                                    /*!< Bit 3 de GPIO */
        uint8_t GP4 : 1;                                                    /*!< Bit 4 de GPIO */
        uint8_t GP5 : 1;                                                    /*!< Bit 5 de GPIO */
        uint8_t GP6 : 1;                                                    /*!< Bit 6 de GPIO */
        uint8_t GP7 : 1;                                                    /*!< Bit 7 de GPIO */
    } bits;
} MCP23017_GPIO_t;

/**
 * @typedef MCP23017_OLAT_t
 *
 * @brief Unión que define al registro OLAT y los bits que lo componen.
 *
 * @details OLAT es el registro que exteriorizará el estado de las entradas. Escribiendo un 1 se activará la salida; escribiendo un 0 se desactivará la salida. Se valor no representa fielmente el estado del pin.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t OL0 : 1;                                                    /*!< Bit 0 de OLAT */
        uint8_t OL1 : 1;                                                    /*!< Bit 1 de OLAT */
        uint8_t OL2 : 1;                                                    /*!< Bit 2 de OLAT */
        uint8_t OL3 : 1;                                                    /*!< Bit 3 de OLAT */
        uint8_t OL4 : 1;                                                    /*!< Bit 4 de OLAT */
        uint8_t OL5 : 1;                                                    /*!< Bit 5 de OLAT */
        uint8_t OL6 : 1;                                                    /*!< Bit 6 de OLAT */
        uint8_t OL7 : 1;                                                    /*!< Bit 7 de OLAT */
    } bits;
} MCP23017_OLAT_t;

/**
 * @typedef MCP23017_DEFVAL_t
 *
 * @brief Unión que define al registro DEFVAL y los bits que lo componen.
 *
 * @details DEFVAL es el registro que compara con el estado lógico de cada entrada para generar o no una interrupción cuando su INTCON está activo. Cuando el DEFVAL es diferente al estado del pin se generará una interrupción.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t DEF0 : 1;                                                    /*!< Bit 0 de DEFVAL */
        uint8_t DEF1 : 1;                                                    /*!< Bit 1 de DEFVAL */
        uint8_t DEF2 : 1;                                                    /*!< Bit 2 de DEFVAL */
        uint8_t DEF3 : 1;                                                    /*!< Bit 3 de DEFVAL */
        uint8_t DEF4 : 1;                                                    /*!< Bit 4 de DEFVAL */
        uint8_t DEF5 : 1;                                                    /*!< Bit 5 de DEFVAL */
        uint8_t DEF6 : 1;                                                    /*!< Bit 6 de DEFVAL */
        uint8_t DEF7 : 1;                                                    /*!< Bit 7 de DEFVAL */
    } bits;
} MCP23017_DEFVAL_t;

/**
 * @typedef MCP23017_INTCON_t
 *
 * @brief Unión que define al registro INTCON y los bits que lo componen.
 *
 * @details INTCON es el registro de configuración interrupciones. Desactivarlo implica una interrupción por cambio en la entradas, dejarlo activo implica una interrupción cuando DEFVAL y el valor del pin son diferentes.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t IOC0 : 1;                                                    /*!< Bit 0 de INTCON */
        uint8_t IOC1 : 1;                                                    /*!< Bit 1 de INTCON */
        uint8_t IOC2 : 1;                                                    /*!< Bit 2 de INTCON */
        uint8_t IOC3 : 1;                                                    /*!< Bit 3 de INTCON */
        uint8_t IOC4 : 1;                                                    /*!< Bit 4 de INTCON */
        uint8_t IOC5 : 1;                                                    /*!< Bit 5 de INTCON */
        uint8_t IOC6 : 1;                                                    /*!< Bit 6 de INTCON */
        uint8_t IOC7 : 1;                                                    /*!< Bit 7 de INTCON */
    } bits;
} MCP23017_INTCON_t;

/**
 * @typedef MCP23017_IOCON_t
 *
 * @brief Unión que define al registro IOCON y los bits que lo componen.
 *
 * @details IOCON es el registro de configuración del chip. En el se define el comportamiento de los pines de interrupción, el direccionamiento de memoria de los registros, cual será la dirección de memoria de cada uno de ellos y como funcionan las interrupciónes.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t reserved : 1;                                                /*!< Bit 0 de IOCON - Sin uso*/
        uint8_t INTPOL : 1;                                                  /*!< Bit 1 de IOCON - Configuración de polaridad de las entradas*/
        uint8_t ODR : 1;                                                     /*!< Bit 2 de IOCON - Configuración de formato de salida de pines INT*/
        uint8_t reserved_2 : 1;                                              /*!< Bit 3 de IOCON - Sin uso */
        uint8_t DISSLW : 1;                                                  /*!< Bit 4 de IOCON - Configuración de velocidad del MCP23017 */
        uint8_t SEQOP : 1;                                                   /*!< Bit 5 de IOCON - Configuración de autoincremento de banco de memoria luego de un acceso */
        uint8_t MIRROR : 1;                                                  /*!< Bit 6 de IOCON - Configuración de espejado de pines de interrupción */
        uint8_t BANK : 1;                                                    /*!< Bit 7 de IOCON - Configuración del banco de memoria. Separación o no de Puertos A y B */
    } bits;
} MCP23017_IOCON_t;

/**
 * @typedef MCP23017_INTF_t
 *
 * @brief Unión que define al registro INTF y los bits que lo componen.
 *
 * @details INTF guarda el valor del pin que generó la interrupción. Escribir en el registro no tendrá efecto
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t INT0 : 1;                                                    /*!< Bit 0 de INTF */
        uint8_t INT1 : 1;                                                    /*!< Bit 1 de INTF */
        uint8_t INT2 : 1;                                                    /*!< Bit 2 de INTF */
        uint8_t INT3 : 1;                                                    /*!< Bit 3 de INTF */
        uint8_t INT4 : 1;                                                    /*!< Bit 4 de INTF */
        uint8_t INT5 : 1;                                                    /*!< Bit 5 de INTF */
        uint8_t INT6 : 1;                                                    /*!< Bit 6 de INTF */
        uint8_t INT7 : 1;                                                    /*!< Bit 7 de INTF */
    } bits;
} MCP23017_INTF_t;

/**
 * @typedef MCP23017_INTCAP_t
 *
 * @brief Unión que define al registro INTCAP y los bits que lo componen.
 *
 * @details INTCAP guarda el valor del puerto al momento de haber generado la interrupción. Al leer este registro, se reinicia el estado de interrupción del chip.
 */
typedef union {
    uint8_t all;                                                            /*!< Unificación de la vaviable */
    struct {
        uint8_t ICP0 : 1;                                                    /*!< Bit 0 de INTCAP */
        uint8_t ICP1 : 1;                                                    /*!< Bit 1 de INTCAP */
        uint8_t ICP2 : 1;                                                    /*!< Bit 2 de INTCAP */
        uint8_t ICP3 : 1;                                                    /*!< Bit 3 de INTCAP */
        uint8_t ICP4 : 1;                                                    /*!< Bit 4 de INTCAP */
        uint8_t ICP5 : 1;                                                    /*!< Bit 5 de INTCAP */
        uint8_t ICP6 : 1;                                                    /*!< Bit 6 de INTCAP */
        uint8_t ICP7 : 1;                                                    /*!< Bit 7 de INTCAP */
    } bits;
} MCP23017_INTCAP_t;

/**
 * @enum mcp_port_e
 *
 * @brief Selector de puertos del MCP23017. Puede ser PORTA (0) o PORTB (1). De esta manera el usuario se abtrae en conocer como se llaman los registros del chip, solo con las funciones específicas de lectura y escritura, datos, pines y esta definición de puerto.
 */
enum mcp_port_e {
    PORTA = 0,                                                              /*!< Puerto A del MCP23017 */
    PORTB = 1                                                               /*!< Puerto B del MCP23017 */
};

#endif