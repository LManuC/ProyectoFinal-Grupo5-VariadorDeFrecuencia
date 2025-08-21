/*
 * GestorDinamica.c
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */



//
//   SACAR DE USO ESTOS ARCHIVOS, NO CUMPLEN NINGUNA FUNCION
// 
//



#include "GestorDinamica.h"
#include "../Gestor_Timers/GestorTimers.h"


ConfigDinamica config;

void GestorDinamica_Init() {

    
}


void GestorDinamica_SetConfig(ConfigDinamica* nuevaConfig) {

    config.acel = nuevaConfig->acel;
    config.descel = nuevaConfig->descel;
    config.t_acel = nuevaConfig->t_acel;
    config.t_decel = nuevaConfig->t_decel;
}

int GestorDinamica_ArrancarMotor() {

    // Se setea el gestor svm y luego se lanzan los timers
    GestorTimers_IniciarTimer(Dinamic_Timer);
    return 0;
}

void GestorDinamica_Interrupt() {

}

