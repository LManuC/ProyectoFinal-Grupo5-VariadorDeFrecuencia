
#ifndef __IO_CONTROL_H__

#define __IO_CONTROL_H__

typedef enum {
    BUTTON_MENU = 1,
    BUTTON_OK,
    BUTTON_BACK,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_SAVE,
    EMERGENCI_STOP_PRESSED,
    EMERGENCI_STOP_RELEASED,
    START_PRESSED,
    START_RELEASED,
    TERMO_SW_PRESSED,
    TERMO_SW_RELEASED,
    STOP_PRESSED,
    STOP_RELEASED,
    SPEED_SELECTOR_0,
    SPEED_SELECTOR_1,
    SPEED_SELECTOR_2,
    SPEED_SELECTOR_3,
    SPEED_SELECTOR_4,
    SPEED_SELECTOR_5,
    SPEED_SELECTOR_6,
    SPEED_SELECTOR_7,
    SPEED_SELECTOR_8,
    SPEED_SELECTOR_9,
    SECURITY_EXCEDED,
    SECURITY_OK
} button_t;


extern QueueHandle_t MCP23017_evt_queue;
extern QueueHandle_t button_evt_queue;

extern uint8_t mcp_porta;
extern uint8_t mcp_portb;

void MCP23017_interrupt_attendance(void* arg);
void MCP23017_buzzer_control(void* arg);
void MCP23017_relay_control(void* arg);
void set_freq_output(uint16_t freq);

#endif