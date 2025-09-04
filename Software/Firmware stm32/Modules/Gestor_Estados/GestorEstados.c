/*
 * GestorEstados.c
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#include "GestorEstados.h"
#include "../Gestor_SVM/GestorSVM.h"

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
            //currentState = STATE_IDLE; // Cambiar al estado inactivo una vez que se complete la inicializacion
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
            //if(veloity == 0) {
                //currentState = STATE_IDLE;
            //}
            break;
        case STATE_EMERGENCY:
            // Implementar la logica para el estado de emergencia
            break;
    }
}

/*
    Diferentes partes del sistema va a mandar acciones a esta funcion para manejar
    el estado y esta va a responder con un codigo

*/
SystemActionResponse GestorEstados_Action(SystemAction sysAct, int value) {

    /*
    ACTION_RESP_OK
ACTION_RESP_ERR
ACTION_RESP_MOVING
    
    */

    int retVal;


    switch(sysAct) {
        case ACTION_INIT_DONE:
            // Cambio de estado a idle lo tenemos luego de la inicializacion
            if(currentState == STATE_INIT) {
                // Si no estamos en estado idle, cambiamos
                currentState = STATE_IDLE;
                return 0;   // Retornar 0 si la accion fue exitosa
            }
            
            return 1;   // Retornar 1 si la accion no se ejecuto

        case ACTION_TO_IDLE:
            if(currentState == STATE_BRAKING) {
                currentState = STATE_IDLE;
                return ACTION_RESP_OK;
            }
            return ACTION_RESP_ERR;

        case ACTION_START:
            // Si estamos en estado idle podemos pasar a start
            if(currentState == STATE_IDLE) {

                // Inicio del motor
                GestorSVM_MotorStart();

                currentState = STATE_RUNNING;
                return ACTION_RESP_OK; // Retornar 0 si la accion fue exitosa
            }else if(currentState == STATE_RUNNING) {
                // Si ya estamos en estado running, no hacemos nada
                return ACTION_RESP_MOVING; // Retornar 2 si ya estamos moviendo
            }
            return ACTION_RESP_ERR;
        case ACTION_STOP:
            if(currentState == STATE_RUNNING || currentState == STATE_VEL_CHANGE) {

                // Motor stop
                GestorSVM_MotorStop();

                currentState = STATE_BRAKING;
                // Aca tengo que poner un timer para que calcule el tiempo aprox de frenado.
                return ACTION_RESP_OK;
            }
            // Si tenemos una emergencia y luego usamos el boton de stop
            if(currentState == STATE_EMERGENCY) {

                // Motor emergency stop
                GestorSVM_Estop();
                currentState = STATE_IDLE;
                return ACTION_RESP_OK;   // Retornar 0 si la accion fue exitosa
            }

            if(currentState == STATE_IDLE) {
                // Si ya estamos en estado idle, no hacemos nada
                return ACTION_RESP_NOT_MOVING; // Retornar 0 si la accion fue exitosa
            }

            return ACTION_RESP_ERR;   // Retornar 1 si la accion no se ejecuto
        case ACTION_EMERGENCY:
            // Implementar la logica para manejar una emergencia
            if(currentState != STATE_EMERGENCY) {
                currentState = STATE_EMERGENCY;
                return ACTION_RESP_OK;   // Retornar 1 si la accion no se ejecuto
            }
            return ACTION_RESP_ERR; // Retornar 0 si la accion fue exitosa

        case ACTION_TO_CONST_RUNNING:
            if(currentState == STATE_VEL_CHANGE) {  // Esta es la unica forma de llegar a este estado
                currentState = STATE_RUNNING;
                return ACTION_RESP_OK;
            }
            return ACTION_RESP_ERR;

        case ACTION_SET_FREC:
            // Implementar la logica para cambiar la velocidad
            retVal = GestorSVM_SetFrecOut(value);
            if(retVal == 0) {return ACTION_RESP_OK;} 
            else {return ACTION_RESP_ERR;}

        case ACTION_SET_ACEL:
            // Implementar la logica para cambiar la aceleracion
            retVal = GestorSVM_SetAcel(value);
            if(retVal == 0) {return ACTION_RESP_OK;} 
            else {return ACTION_RESP_ERR;}           
            
        case ACTION_SET_DIR:
            // Implementar la logica para cambiar la direccion
            retVal = GestorSVM_SetDir(value);
            if(retVal == 0) {return ACTION_RESP_OK;} 
            else {return ACTION_RESP_ERR;}

        default:
            return -1; // Retornar -1 si no se reconoce la accion
    }
}
