/*
 * GestorDinamica.h
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#ifndef GESTOR_DINAMICAMOTOR_GESTORDINAMICA_H_
#define GESTOR_DINAMICAMOTOR_GESTORDINAMICA_H_


typedef struct {
    float acel;  // En rps/seg^2
    float descel; 
    float t_acel;       // En segundos
    float t_decel;
}ConfigDinamica;

// De estos parametros no me puedo salir
typedef struct {
    float aceleracion_abs;
    float desaceleracion_abs;
    float t_acel_max;
    float t_acel_min;
    float t_deacel_max;
    float t_deacel_min;
}ConfigDinamicaAbsoluta;


//  Funciones publicas

void GestorDinamica_Init();
void GestorDinamica_SetConfig(ConfigDinamica* config);
void GestorDinamica_SetConfigAbs(ConfigDinamicaAbsoluta* configAbs);
void GestorDinamica_Interrupt();


#endif /* GESTOR_DINAMICAMOTOR_GESTORDINAMICA_H_ */
