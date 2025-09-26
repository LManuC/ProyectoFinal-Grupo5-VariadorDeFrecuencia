
#ifndef __IO_CONTROL_H__

#define __IO_CONTROL_H__

#define ESP_INTR_FLAG_DEFAULT           0

extern QueueHandle_t MCP23017_evt_queue;
extern uint8_t mcp_porta;
extern uint8_t mcp_portb;

void MCP23017_interrupt_attendance(void* arg);

#endif