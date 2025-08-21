/*
 * GestorSoftTimers.h
 *
 *  Created on: 11 ago. 2025
 *      Author: elian
 */

#ifndef MODULES_GESTOR_TIMERS_GESTORSOFTTIMERS_H_
#define MODULES_GESTOR_TIMERS_GESTORSOFTTIMERS_H_


#include <stdint.h>
#include <stdbool.h>

typedef enum {

    PERIODICO,
    UNICO_DISPARO
}tipos_timer;

typedef enum {

    TIMER_1 = 0,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    TIMER_5,
    TIMER_6,
    TIMER_7,
    TIMER_8,
    TIMER_9,
    TIMER_10,
    TIMER_11,
    TIMER_12,
    TIMER_13,
    TIMER_14,
    TIMER_15,
    TIMER_16,
    TIMER_17,
    TIMER_18,
    TIMER_19,
    TIMER_20,
    TIMERS_MAX_CANT
}timer_id;

void SoftTimer_Inicio();
void SoftTimer_Set(timer_id idTimer, tipos_timer tipo, uint32_t tiempo, void (*handler)(void));
void SoftTimer_Stop(timer_id idTimer);
void SoftTimer_Toggle(timer_id idTimer, tipos_timer tipo, uint32_t tiempo, bool* var);
void SoftTimer_Loop();



#endif /* MODULES_GESTOR_TIMERS_GESTORSOFTTIMERS_H_ */
