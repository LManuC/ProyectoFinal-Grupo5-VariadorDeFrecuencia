/*
 * GestorSVM.h
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
    int frec_switch;        // Frecuencia de switcheo
    float frec_output;        // Frecuencia de salida
    int direccionRotacion;  // 1 = sentido horario, -1 = antihorario
    int resolucionTimer;    // Resolucion del timer (255 para 8 bits)
    char puerto_senal_pierna[3];     // Este tiene los valores de los puertos
    char puerto_encen_pierna[3];
    char orden_pierna[6][3];     // Este tiene el orden con que se encienden los pines

} ConfiguracionSVM;

typedef enum {
    ORDEN_SWITCH_1 = 0,
    ORDEN_SWITCH_2,
    ORDEN_SWITCH_3
}OrdenSwitch ;

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
void GestorSVM_CalcularValoresSwitching(ValoresSwitching* valor);
void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado);


// Esta funcion solo es llamada por el timmer asociado
void GestorSVM_Interrupt();

#endif /* GESTOR_SVM_GESTORSVM_H_ */
