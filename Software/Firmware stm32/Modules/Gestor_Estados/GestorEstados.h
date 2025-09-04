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
    ACTION_INIT_DONE,
    ACTION_TO_IDLE,
    ACTION_START,
    ACTION_STOP,
    ACTION_MOTOR_STOPPED,
    ACTION_EMERGENCY,
    ACTION_SET_FREC,
    ACTION_SET_ACEL,
    ACTION_SET_DIR,
    ACTION_TO_CONST_RUNNING,    // Llego de la aceleracion o desaceleracion a una velocidad constante distinta de cero
} SystemAction;

typedef enum {
    ACTION_RESP_OK = 0,
    ACTION_RESP_ERR = 1,
    ACTION_RESP_MOVING = 2,
    ACTION_RESP_NOT_MOVING,
} SystemActionResponse;

void GestorEstados_Init();
void GestorEstados_Run();
SystemActionResponse GestorEstados_Action(SystemAction sysAct, int value);

#endif /* MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_ */
