/*
 * GestorEstados.h
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#ifndef MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_
#define MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_

typedef enum {
    ACTION_NONE = 0,
    ACTION_TO_IDLE,
    ACTION_START,
    ACTION_STOP,
    ACTION_EMERGENCY,
    ACTION_VELOCITY_CHANGE,
} SystemAction;

void GestorEstados_Init();
void GestorEstados_Run();
int GestorEstados_Action(SystemAction sysAct);

#endif /* MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_ */
