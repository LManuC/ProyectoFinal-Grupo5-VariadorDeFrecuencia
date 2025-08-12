/*
 * GestorEstados.c
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#include "GestorEstados.h"

typedef enum {
    STATE_INIT = 0, // En init se estan iniciando el sistema
    STATE_IDLE,     // Finalizo la inicializacion o se encuentra inactivo
    STATE_RUNNING,  // El motor esta girando a velocidad constante
    STATE_VEL_CHANGE, // El sistema esta cambiando la velocidad
    STATE_BRAKING,  // El motor se esta deteniendo, bajando la velocidad
    STATE_EMERGENCY // El sistema se encuentra en estado de emergencia, esta girando o no
} SystemState;

static SystemState currentState = STATE_INIT;

void GestorEstados_Init() {

}

void GestorEstados_Run() {
    switch(currentState) {
        case STATE_INIT:
            // Implementar la logica de inicializacion
            // Por ejemplo, iniciar los timers, configurar los GPIOs, etc.
            currentState = STATE_IDLE; // Cambiar al estado inactivo una vez que se complete la inicializacion
            break;
        case STATE_IDLE:
            // Implementar la logica para el estado inactivo
            // Esperar a que se reciba una accion para cambiar de estado
            break;
        case STATE_RUNNING:
            // Implementar la logica para el estado de ejecucion
            // Por ejemplo, controlar el motor a una velocidad constante
            break;
        case STATE_VEL_CHANGE:
            // Implementar la logica para el estado de cambio de velocidad
            break;
        case STATE_BRAKING:
            // Implementar la logica para el estado de frenado
            // Por ejemplo, controlar el frenado del motor
            // Si se detiene el motor y la velocidad es cero pasa al estado idle
            if(veloity == 0) {
                currentState = STATE_IDLE;
            }
            break;
        case STATE_EMERGENCY:
            // Implementar la logica para el estado de emergencia
            break;
    }
}

int GestorEstados_Action(SystemAction sysAct) {
    switch(sysAct) {
        case ACTION_TO_IDLE:
            // Cambio de estado a idle lo tenemos luego de la inicializacion
            if(currentState == STATE_INIT) {
                // Si no estamos en estado idle, cambiamos
                currentState = STATE_IDLE;
                return 0;   // Retornar 0 si la accion fue exitosa
            }
            
            return 1;   // Retornar 1 si la accion no se ejecuto
        case ACTION_START:
            // Si estamos en estado idle podemos pasar a start
            if(currentState == STATE_IDLE) {
                currentState = STATE_RUNNING;
                return 0; // Retornar 0 si la accion fue exitosa
            }
            return 1;
        case ACTION_STOP:
            if(currentState == STATE_RUNNING || currentState == STATE_VEL_CHANGE) {
                currentState = STATE_BRAKING;
                // Aca tengo que poner un timer para que calcule el tiempo aprox de frenado.
                return 0;
            }
            // Si tenemos una emergencia y luego usamos el boton de stop
            if(currentState == STATE_EMERGENCY) {
                currentState = STATE_IDLE;
                return 0;   // Retornar 0 si la accion fue exitosa
            }
            return 1;   // Retornar 1 si la accion no se ejecuto
        case ACTION_EMERGENCY:
            // Implementar la logica para manejar una emergencia
            return 0; // Retornar 0 si la accion fue exitosa
        case ACTION_VELOCITY_CHANGE:
            // Implementar la logica para cambiar la velocidad
            return 0; // Retornar 0 si la accion fue exitosa
        default:
            return -1; // Retornar -1 si no se reconoce la accion
    }
}