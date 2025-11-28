/** 
 *  @file GestorEstados.c
 *  @brief Implementa la máquina de estados del LVFV.
 *  @details Ver @ref SystemState, @ref SystemAction y @ref SystemActionResponse.
 */

#include "GestorEstados.h"
#include "../Gestor_SVM/GestorSVM.h"

static SystemState currentState = STATE_INIT;

SystemActionResponse GestorEstados_Action(SystemAction sysAct, int value) {
    SystemActionResponse retVal = ACTION_RESP_ERR;  // default seguro
    switch(sysAct) {
        case ACTION_INIT_DONE:
            if(currentState == STATE_INIT) {
                currentState = STATE_IDLE;
                retVal = ACTION_RESP_OK;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_TO_IDLE:
            if(currentState == STATE_BRAKING) {
                currentState = STATE_IDLE;
                retVal = ACTION_RESP_OK;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_START:
            if(currentState == STATE_IDLE) {
                GestorSVM_MotorStart();
                currentState = STATE_VEL_CHANGE;
                retVal = ACTION_RESP_OK;
            }else if(currentState == STATE_RUNNING || currentState == STATE_VEL_CHANGE || currentState == STATE_BRAKING) {
                retVal = ACTION_RESP_MOVING;
            }else if(currentState == STATE_EMERGENCY) {
                retVal = ACTION_RESP_EMERGENCY_ACTIVE;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_STOP:
            if(currentState == STATE_RUNNING || currentState == STATE_VEL_CHANGE) {
                GestorSVM_MotorStop();
                /// @todo: Aca tengo que poner un timer para que calcule el tiempo aprox de frenado.
                currentState = STATE_BRAKING;
                retVal = ACTION_RESP_OK;
            } else if(currentState == STATE_EMERGENCY) {
                GestorSVM_Estop();
                currentState = STATE_IDLE;
                retVal = ACTION_RESP_OK;
            } else if (currentState == STATE_IDLE) {
                retVal = ACTION_RESP_NOT_MOVING;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_MOTOR_STOPPED:
            if(currentState == STATE_BRAKING) {
                currentState = STATE_IDLE;
                retVal = ACTION_RESP_OK;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_EMERGENCY:
            if(currentState != STATE_EMERGENCY) {
                /// @todo: Revisar si es necesario chequear esto ¿Por qué no ejecutamos el stop sin importar el estado?
                GestorSVM_Estop();
                currentState = STATE_EMERGENCY;
                retVal = ACTION_RESP_OK;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_TO_CONST_RUNNING:
            if(currentState == STATE_VEL_CHANGE) {
                currentState = STATE_RUNNING;
                retVal = ACTION_RESP_OK;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_SET_FREC:
            if(currentState == STATE_IDLE || currentState == STATE_RUNNING || currentState == STATE_VEL_CHANGE) {
                int conf_retVal = GestorSVM_SetFrec(value);
                if(conf_retVal == 0 || conf_retVal == -2) {
                    retVal = ACTION_RESP_OK;
                } else if(conf_retVal == 1) {
                    currentState = STATE_VEL_CHANGE;
                    retVal = ACTION_RESP_OK;
                } else if(conf_retVal == -1) {
                    retVal = ACTION_RESP_OUT_RANGE;
                }
            } else if( currentState == STATE_BRAKING) {
                retVal = ACTION_RESP_MOVING;
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_SET_ACEL:
            // Implementar la logica para cambiar la aceleracion
            if(currentState == STATE_IDLE || currentState == STATE_RUNNING) {
                switch( GestorSVM_SetAcel(value) ) {
                    case 0:
                        retVal = ACTION_RESP_OK;
                        break;
                    case -1:
                        retVal = ACTION_RESP_OUT_RANGE;
                        break;
                    case -2:
                    default:
                        retVal = ACTION_RESP_ERR;
                        break;
                }
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_SET_DESACEL:
            if(currentState == STATE_IDLE || currentState == STATE_RUNNING) {
                switch( GestorSVM_SetDecel(value) ) {
                    case 0:
                        retVal = ACTION_RESP_OK;
                        break;
                    case -1:
                        retVal = ACTION_RESP_OUT_RANGE;
                        break;
                    case -2:
                    default:
                        retVal = ACTION_RESP_ERR;
                        break;
                }
            } else {
                retVal = ACTION_RESP_ERR;
            }
            break;
        case ACTION_SET_DIR:
            if(currentState == STATE_IDLE) {
                switch( GestorSVM_SetDir(value)) {
                    case 0:
                        retVal = ACTION_RESP_OK;
                        break;
                    case -1:
                        retVal = ACTION_RESP_OUT_RANGE;
                        break;
                    case -2:
                        retVal = ACTION_RESP_MOVING;
                        break;
                    default:
                        retVal = ACTION_RESP_ERR;
                        break;
                }
            } else {
                retVal = ACTION_RESP_MOVING;
            }
            break;
        // case ACTION_GET_FREC:            
        // case ACTION_GET_ACEL:            
        // case ACTION_GET_DESACEL:            
        // case ACTION_GET_DIR:
        //     retVal = ACTION_RESP_OK;     // lectura válida, sin transición
        //     break;
        case ACTION_IS_MOTOR_STOP:
            if(currentState == STATE_IDLE || currentState == STATE_EMERGENCY) {
                retVal = ACTION_RESP_NOT_MOVING;
            }else {
                retVal = ACTION_RESP_MOVING;
            }
            break;
        default:
            retVal = ACTION_RESP_ERR;
            break;
    }
    return retVal;
}
