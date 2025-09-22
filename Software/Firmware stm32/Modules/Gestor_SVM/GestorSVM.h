/*
 *  GestorSVM.h
 *
 *  Created on: Apr 4, 2025
 *      Author: Elian
 */

#ifndef GESTOR_SVM_GESTORSVM_H_
#define GESTOR_SVM_GESTORSVM_H_

// Esta es la esctructura que va a utilizar el handler IRQ del timer
typedef struct ValoresSwitching {

    // Aca se lleva la cuenta del cuadrante actual
    // Le sirve al ISR para saber que pin debe activar
    char cuadrante;

    // Este es el flag que indica si tiene que encender o 
    // apagar el pin
    char flagSwitch[3];

    // Este es el valor de los ticks para la interruccion
    int ticks1[2];
    int ticks2[2];
    int ticks3[2];

    int contador;
} ValoresSwitching; 


typedef struct ConfiguracionSVM {
    
    // Configuracion basica
    int frecOutMin;
    int frecOutMax;
    int frec_switch;        // Frecuencia de switcheo
    int frecReferencia;     // Frecuencia de referencia
    int direccionRotacion;  // 1 = sentido horario, -1 = antihorario
    int resolucionTimer;    // Resolucion del timer (255 para 8 bits)

    // Configuracion dinamica en Hz/seg
    int acel;
    int desacel;
    // Rango
    int acelMax;
    int acelMin;
    int desacelMax;
    int desacelMin;

    // Configuracion de puertos
    char puerto_senal_pierna[3];    // Puertos In de los IR2104
    char puerto_encen_pierna[3];    // Puertos Shutdown de los IR2104
    char orden_pierna[6][3];     // Este tiene el orden con que se encienden los pines

} ConfiguracionSVM;

typedef enum {
    ORDEN_SWITCH_1_UP = 0,
    ORDEN_SWITCH_2_UP = 1,
    ORDEN_SWITCH_3_UP = 2,
    ORDEN_SWITCH_3_DOWN = 3,
    ORDEN_SWITCH_2_DOWN = 4,
    ORDEN_SWITCH_1_DOWN = 5,
}OrdenSwitch ;


typedef enum {
    SWITCH_INT_CH1 = 0,
    SWITCH_INT_CH2,
    SWITCH_INT_CH3,
    SWITCH_INT_RESET,
    SWITCH_INT_CLEAN,
} SwitchInterruptType;

#ifdef __cplusplus

class GestorSVM {
public:
	// Constructor
	GestorSVM();

	// Destructor
	virtual ~GestorSVM();

	// Metodos
    void SetConfiguration(int fs, int fo);
    void setValue(int val);
    int getValue();
    void CalcularValoresSwitching(ValoresSwitching& valorSwitch);
    

private:
    int value;
    int frec_switch;
    int frec_output;
    int ang_switch; // En grados por mil


};

extern GestorSVM gestorSVM; // Instancia del gestor de SVM

#endif

void GestorSVM_Init();
void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion);
void GestorSVM_CalcularValoresSwitching(/*ValoresSwitching* valor*/);
void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado, int numCuadrante);


// Las siguientes funciones devuelven 0 si todo salio bien y 1 si no se ejecuto

// El motor se arranca con la funcion start, este acelera hasta la frec
// que tiene cargada.
int GestorSVM_MotorStart();
// Si el motor esta girando lo recomendado es frenarlo con esta funcion.
// No usar setFrecOut(0)
int GestorSVM_MotorStop();
// Ante el caso de emergencia se desactiva el inversor y se apagan las
// interruciones, para volver a encender se debe usar motorStart
int GestorSVM_Estop();
// Seteo de una frecuencia objetivo, si el motor esta girando
// va a reaccionar con una aceleracion o desaceleracion dependiendo
// de la frecuencia actual.
// Devuelve 0 si se modifica exitosamente la configuracion pero el motor 
// no esta girando
// Devuelve 1 si se modifica la config y el motor esta en movimiento
// Devuelve -1 si falla la config por fuera de rango
// Devuelve -2 si falla la config por ser la misma velocidad a la seteada
int GestorSVM_SetFrec(int frec);
// Cambio de sentido de giro, esta solo se puede dar si el motor esta frenado
// Devuelve 0 si se modifica la configuracion
// Devuelve -1 si esta fuera del rango
// Devuelve -2 si no se modifica
int GestorSVM_SetDir(int dir);
// Cambio de la aceleracion
// Devuelve 0 si se modifica exitosamente
// Devuelve -1 si falla por fuera de rango
// Devuelve -2 si falla ya que esta cambiando de velocidad
int GestorSVM_SetAcel(int acel);
// Cambio de la desaceleracion
// Devuelve 0 si se modifica exitosamente
// Devuelve -1 si falla por fuera de rango
// Devuelve -2 si falla ya que esta cambiando de velocidad
int GestorSVM_SetDecel(int decel);

// Devuelve la velocidad actual del motor
int GestorSVM_GetFrec();
int GestorSVM_GetAcel();
int GestorSVM_GetDesacel();
int GestorSVM_GetDir();

// Funcion llamada a la frecSwitch por el timmer asociado al switcheo
void GestorSVM_SwitchInterrupt(SwitchInterruptType intType);
// Funcion llamada a la frecCalculo por el timmer asociado al calculo
void GestorSVM_CalcInterrupt();

#endif /* GESTOR_SVM_GESTORSVM_H_ */
