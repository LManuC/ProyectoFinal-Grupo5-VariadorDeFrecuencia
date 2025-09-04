/*
 * GestorTimers.h
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#ifndef GESTOR_TIMERS_GESTORTIMERS_H_
#define GESTOR_TIMERS_GESTORTIMERS_H_



typedef enum {
    SVM_Timer = 0,
    Dinamic_Timer
} TimersEnum;

void GestorTimers_Init();
void GestorTimers_IniciarTimer(TimersEnum timerEnum);   // Esta queda deprecada
void GestorTimers_DetenerTimer(TimersEnum timerEnum);   // Esta queda deprecada

void GestorTimers_IniciarTimerSVM();
void GestorTimers_DetenerTimerSVM();

#endif /* GESTOR_TIMERS_GESTORTIMERS_H_ */
