/*
 * GestorSoftTimers.c
 *
 *  Created on: 11 ago. 2025
 *      Author: elian
 */


#include "SoftTimers.h"
// Para tener el HAL_GetTick()
#include "main.h"

static bool timerSeteado[TIMERS_MAX_CANT]; 
static uint32_t ultTiempoTimer[TIMERS_MAX_CANT];
static uint32_t intervaloTimer[TIMERS_MAX_CANT];
static void (*timerHandler[TIMERS_MAX_CANT]) (void);
static tipos_timer tipoTimer[TIMERS_MAX_CANT];

void SoftTimer_Inicio() {
    for(uint8_t i = 0; i < TIMERS_MAX_CANT; i++) {
        timerSeteado[i] = false;
        ultTiempoTimer[i] = 0;
    }
}

void SoftTimer_Loop() {

    for(uint8_t i = 0; i < TIMERS_MAX_CANT; i++) {
        if(timerSeteado[i]) {
            if(HAL_GetTick() - ultTiempoTimer[i] >= intervaloTimer[i]) {
                
                (*timerHandler[i])();
                
                if(tipoTimer[i] == UNICO_DISPARO) {
                    SoftTimer_Stop((timer_id)i);
                }

                ultTiempoTimer[i] = HAL_GetTick();
            }
        }
    }
}


void SoftTimer_Set(timer_id idTimer, tipos_timer tipo, uint32_t tiempo, void (*handler)(void)) {

    timerSeteado[idTimer] = true;
    intervaloTimer[idTimer] = tiempo;
    tipoTimer[idTimer] = tipo;
    timerHandler[idTimer] = handler;
    ultTiempoTimer[idTimer] = HAL_GetTick();
}

void SoftTimer_Stop(timer_id idTimer) {
    timerSeteado[idTimer] = false;
}



