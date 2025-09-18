#ifndef __MCP23017_H__
#define __MCP23017_H__

#include "esp_mac.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "C:/Users/User/esp/v5.5/esp-idf/components/driver/i2c/include/driver/i2c.h"

#define I2C_MASTER_TIMEOUT_MS   100
#define I2C_MASTER_NUM          1


// IOCON pins defines
#define __MCP23017_BYTE_MODE__                  1
#define __MCP23017_SEQUENTIAL_MODE__            0

#define __MCP23017_BANK_SEQUENTIAL__            0
#define __MCP23017_BANK_SPLIT__                 1

#define __MCP23017_MIRROR_UNCONNECTED__         0
#define __MCP23017_MIRROR_CONNECTED__           1

#define __MCP23017_SEQOP_ENABLED__              0
#define __MCP23017_SEQOP_DISABLED__             1

#define __MCP23017_DISSLW_DISABLED__            0
#define __MCP23017_DISSLW_ENABLED__             1

// #define __MCP23017_HEAN_ENABLED__              0
// #define __MCP23017_HEAN_DISABLED__             1

#define __MCP23017_ODR_OPEN_DRAIN_OUTPUT__      1
#define __MCP23017_ODR_ACTIVE_DRIVER_OUTPUT__   0

#define __MCP23017_INTPOL_POLARITY_LOW__        1
#define __MCP23017_INTPOL_POLARITY_HIGH__       0

#define __MCP23017_GPPU_PULL_UP_ENABLE__        1
#define __MCP23017_GPPU_PULL_UP_DISABLE__       0

#define __MCP23017_IODIR_INPUT__        1
#define __MCP23017_IODIR_OUTPUT__       0

#define __MCP23017_GPINTEN_ENABLE__        1
#define __MCP23017_GPINTEN_DISABLE__       0




#define __MCP23017_WRITE__                  0
#define __MCP23017_READ__                   1

#define __MCP23017_IO_OUTPUT__              0
#define __MCP23017_IO_INPUT__               1

#define __MCP23017_POSITIVE_LOGIC__         0
#define __MCP23017_OPPOSITE_LOGIC__         1

#define __MCP23017_INTERRUPT_ENABLE__       0
#define __MCP23017_INTERRUPT_DISABLE__      1

#define __MCP23017_INT_CHANGE__             0
#define __MCP23017_INT_DEFAULT_COMP__       1

#define __MCP23017_ADDRESS__                0b010   // CONFIGURADO POR HARDWARE ----> OPCODE: 0100AAA0 -> 01000000 | __MCP23017_ADDRESS__ << 1 | R/W


#define __MCP23017_READ_OPPCODE__           0b0100 << 4 | __MCP23017_ADDRESS__ << 1 | __MCP23017_READ__
#define __MCP23017_WRITE_OPPCODE__          0b0100 << 4 | __MCP23017_ADDRESS__ << 1 | __MCP23017_WRITE__
#define __MCP23017_FUNC_MODE__              __MCP23017_BYTE_MODE__




#if __MCP23017_FUNC_MODE__ == __MCP23017_SEQUENTIAL_MODE__
    #define MCP23017_IODIRA_REGISTER            0x00
    #define MCP23017_IODIRB_REGISTER            0x01
    #define MCP23017_IPOLA_REGISTER             0x02
    #define MCP23017_IPOLB_REGISTER             0x03
    #define MCP23017_GPINTENA_REGISTER          0x04
    #define MCP23017_GPINTENB_REGISTER          0x05
    #define MCP23017_DEFVALA_REGISTER           0x06
    #define MCP23017_DEFVALB_REGISTER           0x07
    #define MCP23017_INTCONA_REGISTER           0x08
    #define MCP23017_INTCONB_REGISTER           0x09
    #define MCP23017_IOCON_REGISTER             0x0A
    // #define MCP23017_IOCON_REGISTER             0x0B
    #define MCP23017_GPPUA_REGISTER             0x0C
    #define MCP23017_GPPUB_REGISTER             0x0D
    #define MCP23017_INTFA_REGISTER             0x0E
    #define MCP23017_INTFB_REGISTER             0x0F
    #define MCP23017_INTCAPA_REGISTER           0x10
    #define MCP23017_INTCAPB_REGISTER           0x11
    #define MCP23017_GPIOA_REGISTER             0x12
    #define MCP23017_GPIOB_REGISTER             0x13
    #define MCP23017_OLATA_REGISTER             0x14
    #define MCP23017_OLATB_REGISTER             0x15
#elif __MCP23017_FUNC_MODE__ == __MCP23017_BYTE_MODE__
    #define MCP23017_IODIRA_REGISTER             0x00
    #define MCP23017_IODIRB_REGISTER             0x10
    #define MCP23017_IPOLA_REGISTER              0x01
    #define MCP23017_IPOLB_REGISTER              0x11
    #define MCP23017_GPINTENA_REGISTER           0x02
    #define MCP23017_GPINTENB_REGISTER           0x12
    #define MCP23017_DEFVALA_REGISTER            0x03
    #define MCP23017_DEFVALB_REGISTER            0x13
    #define MCP23017_INTCONA_REGISTER            0x04
    #define MCP23017_INTCONB_REGISTER            0x14
    #define MCP23017_IOCON_REGISTER              0x05
    // #define MCP23017_IOCON_REGISTER              0x15
    #define MCP23017_GPPUA_REGISTER              0x06
    #define MCP23017_GPPUB_REGISTER              0x16
    #define MCP23017_INTFA_REGISTER              0x07
    #define MCP23017_INTFB_REGISTER              0x17
    #define MCP23017_INTCAPA_REGISTER            0x08
    #define MCP23017_INTCAPB_REGISTER            0x18
    #define MCP23017_GPIOA_REGISTER              0x09
    #define MCP23017_GPIOB_REGISTER              0x19
    #define MCP23017_OLATA_REGISTER              0x0A
    #define MCP23017_OLATB_REGISTER              0x1A
#endif

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t IO0 : 1;  // bit 0
        uint8_t IO1 : 1;  // bit 1
        uint8_t IO2 : 1;
        uint8_t IO3 : 1;
        uint8_t IO4 : 1;
        uint8_t IO5 : 1;
        uint8_t IO6 : 1;
        uint8_t IO7 : 1;  // bit 7
    } bits;
} MCP23017_IODIR_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t IP0 : 1;  // bit 0
        uint8_t IP1 : 1;  // bit 1
        uint8_t IP2 : 1;
        uint8_t IP3 : 1;
        uint8_t IP4 : 1;
        uint8_t IP5 : 1;
        uint8_t IP6 : 1;
        uint8_t IP7 : 1;  // bit 7
    } bits;
} MCP23017_IPOL_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t GPINT0 : 1;  // bit 0
        uint8_t GPINT1 : 1;  // bit 1
        uint8_t GPINT2 : 1;
        uint8_t GPINT3 : 1;
        uint8_t GPINT4 : 1;
        uint8_t GPINT5 : 1;
        uint8_t GPINT6 : 1;
        uint8_t GPINT7 : 1;  // bit 7
    } bits;
} MCP23017_GPINTEN_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t PU0 : 1;  // bit 0
        uint8_t PU1 : 1;  // bit 1
        uint8_t PU2 : 1;
        uint8_t PU3 : 1;
        uint8_t PU4 : 1;
        uint8_t PU5 : 1;
        uint8_t PU6 : 1;
        uint8_t PU7 : 1;  // bit 7
    } bits;
} MCP23017_GPPU_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t GP0 : 1;  // bit 0
        uint8_t GP1 : 1;  // bit 1
        uint8_t GP2 : 1;
        uint8_t GP3 : 1;
        uint8_t GP4 : 1;
        uint8_t GP5 : 1;
        uint8_t GP6 : 1;
        uint8_t GP7 : 1;  // bit 7
    } bits;
} MCP23017_GPIO_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t OL0 : 1;  // bit 0
        uint8_t OL1 : 1;  // bit 1
        uint8_t OL2 : 1;
        uint8_t OL3 : 1;
        uint8_t OL4 : 1;
        uint8_t OL5 : 1;
        uint8_t OL6 : 1;
        uint8_t OL7 : 1;  // bit 7
    } bits;
} MCP23017_OLAT_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t DEF0 : 1;  // bit 0
        uint8_t DEF1 : 1;  // bit 1
        uint8_t DEF2 : 1;
        uint8_t DEF3 : 1;
        uint8_t DEF4 : 1;
        uint8_t DEF5 : 1;
        uint8_t DEF6 : 1;
        uint8_t DEF7 : 1;  // bit 7
    } bits;
} MCP23017_DEFVAL_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t IOC0 : 1;  // bit 0
        uint8_t IOC1 : 1;  // bit 1
        uint8_t IOC2 : 1;
        uint8_t IOC3 : 1;
        uint8_t IOC4 : 1;
        uint8_t IOC5 : 1;
        uint8_t IOC6 : 1;
        uint8_t IOC7 : 1;  // bit 7
    } bits;
} MCP23017_INTCON_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t reserved : 1;  // bit 0
        uint8_t INTPOL : 1;  // bit 1
        uint8_t ODR : 1;
        uint8_t reserved_2 : 1;       // HEAN: Reserver for MCP23S17
        uint8_t DISSLW : 1;
        uint8_t SEQOP : 1;
        uint8_t MIRROR : 1;
        uint8_t BANK : 1;  // bit 7
    } bits;
} MCP23017_IOCON_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t INT0 : 1;  // bit 0
        uint8_t INT1 : 1;  // bit 1
        uint8_t INT2 : 1;
        uint8_t INT3 : 1;
        uint8_t INT4 : 1;
        uint8_t INT5 : 1;
        uint8_t INT6 : 1;
        uint8_t INT7 : 1;  // bit 7
    } bits;
} MCP23017_INTF_t;

typedef union {
    uint8_t all;  // acceso al registro completo
    struct {
        uint8_t ICP0 : 1;  // bit 0
        uint8_t ICP1 : 1;  // bit 1
        uint8_t ICP2 : 1;
        uint8_t ICP3 : 1;
        uint8_t ICP4 : 1;
        uint8_t ICP5 : 1;
        uint8_t ICP6 : 1;
        uint8_t ICP7 : 1;  // bit 7
    } bits;
} MCP23017_INTCAP_t;

esp_err_t register_read(uint8_t register_address, uint8_t *data);
esp_err_t register_write(uint8_t register_address, uint8_t data);
esp_err_t MCP23017_INIT( void );

#endif
