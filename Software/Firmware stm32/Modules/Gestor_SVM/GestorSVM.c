/*
 * GestorSVM.cpp
 *
 *  Created on: Apr 4, 2025
 *      Author: Elian
 */

// Includes privados
#include "GestorSVM.h"

#include "stm32f103xb.h"			// Para incluir la definicion de GPIOA

#include "../Inc/main.h"

// Solo para pruebas, despues borrar
//#include "stm32f1xx_hal_gpio.h"		// Para incluir a HAL_GPIO_WritePin  BORAR DE ACA

#include <stdio.h>		// Para el printf

// Definiciones privadas
#define FREC_SWITCH_DEF 1000
#define FREC_OUTPUT_DEF 50

// Enumeraciones privadas

enum Cuadrantes{
	CUADRANTE_1 = 0,
	CUADRANTE_2,
	CUADRANTE_3,
	CUADRANTE_4,
	CUADRANTE_5,
	CUADRANTE_6
};

// Variables privadas
static int frec_switch;
static float frec_output;
static int ang_switch; // En grados por mil

static int direccionRotacion; // 1 = sentido horario, -1 = antihorario

static int resolucionTimer;

// Este va a trakear el angulo actual
static int angAct;		// Esta en grados por mil
static int angParcial;

// Con este se guarda el ultimo estado
static volatile int cuadranteActual;


// Aca se guardan los pines de salida, estos se obtiene cruzando los datos 
// de la configuracion, del orden y de los pines.
static char orden_pierna_pines[6][3];

// Aca guardamos el valor logico de las salidas sin tener en cuenta el numero
// de pin
static char orden_pierna_logico[6][3];


// Aca guardamos los pines de la pierna
static int puerto_encen_pierna[3];
// Vector de los numero de pin de salida
static int puerto_senal_pierna[3];


// Esta variable nos sirve para saber si el angulo parcial hay que incrementarlo o decrementar
static volatile char flagAscensoAngParcial;

// Aca guardamos el estado logico de las salidas
static volatile char estadoLogicoSalida[3];


// Usado para un debug simple
static volatile int index = 0;
static volatile int ultsPuertos[50];
static volatile int ultsOutsLogicos[50];
static volatile int timerT3[50];
static volatile int ultsCuadrantes[50];
static int maxIndex = 50;


// Variables que guardan los datos para la gestion dinamica
// Frecuencia final de salida
static volatile int frecOutputTaget;
// Diferencia de la frec de salida entre una conmutacion y otra, 
// Este va a ser un valor bajo.
static volatile int frecOutputDif;
// Este es un flag que indica que esta cambiando de frecuencia
static volatile int flagChangingFrec;

static volatile char contadorSwitchsCiclo;


// Esta es la manejada por el handler del timer
volatile ValoresSwitching valoresSwitching;


void GestorSVM_Init() {

	frec_switch = FREC_SWITCH_DEF;
	frec_output = FREC_OUTPUT_DEF;
	ang_switch = 0;

	angAct = 0;
	angParcial = 0;
	cuadranteActual = 0;
	flagAscensoAngParcial = 1;

	contadorSwitchsCiclo = 0;

	
	direccionRotacion = 1;	// Sentido horario

	estadoLogicoSalida[0] = 0;
	estadoLogicoSalida[1] = 0;
	estadoLogicoSalida[2] = 0;
}

void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion) {
	
	int i, j;
	int puerto;

	frec_switch = configuracion->frec_switch;
	frec_output = configuracion->frec_output;
	direccionRotacion = configuracion->direccionRotacion;
	resolucionTimer = configuracion->resolucionTimer;

	// Calculamos el angulo que recorre en un switch
	ang_switch = frec_output * 360000 / frec_switch;
	

	// Aca debemos comprobar si es posible
	// Si el angulo es mayor a un umbral debemos descartar
	// o aumentar la frecuencia de switching

	printf("AngSwitch: %d\n", ang_switch);

	// Aca vamos a setear los pines de salida
	for(i = 0; i < 6; i++) {
		for(j = 0; j < 3; j++) {
			puerto = (int)configuracion->orden_pierna[i][j];
			orden_pierna_pines[i][j] = configuracion->puerto_senal_pierna[puerto];
			orden_pierna_logico[i][j] = (int)configuracion->orden_pierna[i][j];
		}
	}

	for(i = 0; i < 3; i++) {
		puerto_senal_pierna[i] = configuracion->puerto_senal_pierna[i];
		puerto_encen_pierna[i] = configuracion->puerto_encen_pierna[i];
	}

	printf("Configuracion Seteada \n");
	
}



void GestorSVM_CalcularValoresSwitching(ValoresSwitching* valorSwitch) {

	// Cuando un cambio de cuadrante se activa este flag
	char cambioCuadrante = 0;
	// Aca vamos a guardar el valor en que haremos la interrupcion
	int t1, t2, t0;

	// El angulo se incrementa al terminar el ciclo de switch
	angAct += ang_switch;
	if(angAct >= 360000) {
		angAct -= 360000;
		cuadranteActual = 0;
		cambioCuadrante = 1;
		flagAscensoAngParcial = 1;
		// Aca vuelvo a sincronizar el angulo parcial
		angParcial = angAct;
	}else {
		if(flagAscensoAngParcial == 1) {
			angParcial += ang_switch;
			if(angParcial >= 60000) {
				// Si me paso le resto lo que me pase
				angParcial -= angParcial - 60000;
				cuadranteActual ++;
				cambioCuadrante = 1;
				flagAscensoAngParcial = 0;
			}
		} else {
			angParcial -= ang_switch;
			if(angParcial <= 0) {
				// Si me paso de cero le resto devuelta
				// lo que me pase
				angParcial += -angParcial;
				cambioCuadrante = 1;
				cuadranteActual ++;
				flagAscensoAngParcial = 1;
			}
		}
	}

	// Chequeamos que tengamos todos los switches
	if(contadorSwitchsCiclo != 6) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
	}
	// Reiniciamos el contador
	contadorSwitchsCiclo = 0;

	if(cuadranteActual == 6) {
		cuadranteActual = 0;
	}

	if(cambioCuadrante == 1) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	}
	/*
	angParcial += ang_switch;
	if(angParcial >= 60000) {
		angParcial -= 60000;
		cambioCuadrante = 1;
		cuadranteActual ++;
	}
	*/

	//printf("AngAct: %d, AngPar: %d, Cuad: %d, angSwitch: %d\n", angAct, angParcial, cuadranteActual, ang_switch);


	// Si la resolucion es 256 justo =>

	t1 = (int)(-0.0018 * (float)angParcial + 112.57);
	t2 = (int)(+0.0018 * (float)angParcial + 10.955);
	// Este es t0 / 2
	t0 = (int)((float)(255 - t1 - t2) * 0.5);

	/*
	if(t1 < 0) {
		t1 = 5;
	}
	timerT1[index] = t1;
	timerT2[index] = t2;
	timerT3[index] = t0;
	cuadrante[index] = cuadranteActual;
	index ++;

	if(index == 50) {
		index = 0;
	}
	*/

	/*
	// Si la resolucion es distinta puedo dejarla parametrizada
	t1 = (-7.0E-6 * (float)currentAng + 0.4397) * resolucionTimer;
	t2 = (+7.0E-6 * (float)currentAng + 0.0428) * resolucionTimer;
	t0 = resolucionTimer - t1 - t2;
	*/
	//printf("ang: %d, t0: %d, t1: %d, t2: %d\n", angAct, t0, t1, t2);

	valorSwitch->ticks1[0] = t0;
	//valorSwitch->ticks1[1] = 0;
	valorSwitch->ticks2[0] = t0+t1;
	//valorSwitch->ticks2[1] = 0;
	valorSwitch->ticks3[0] = t0+t1+t2;
	//valorSwitch->ticks0[1] = 0;

	// Le indicamos a la interrupcion que la proxima tiene que encender 
	// los puertos
	valorSwitch->flagSwitch[0] = 1;
	valorSwitch->flagSwitch[1] = 1;
	valorSwitch->flagSwitch[2] = 1;

	valorSwitch->contador = 0;

	//printf("Calculo realizado\n");
}


void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado) {

	GPIO_PinState estadoPin;
	int puerto;
	int puertoPin;

	contadorSwitchsCiclo ++;

	
	estadoPin = (estado == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
	
	ultsPuertos[index] = estado;
	ultsOutsLogicos[index] = orden_pierna_logico[cuadranteActual][orden];
	ultsCuadrantes[index] = cuadranteActual;
	
	puerto = orden_pierna_logico[cuadranteActual][orden];
	if(estadoLogicoSalida[puerto] == estado) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	}else {
		estadoLogicoSalida[puerto] = estado;
	}
	
	index ++;
	if(index == maxIndex) {
		index = 0;
	}


	//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	
	// Original - funcionaba bien (capaz que con glitches)
	//HAL_GPIO_WritePin(GPIOA, orden_pierna_pines[cuadranteActual][orden], estadoPin);

	// Ultimo 
	//HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[0], estadoLogicoSalida[0]);
	//HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[1], estadoLogicoSalida[1]);
	//HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[2], estadoLogicoSalida[2]);
	puertoPin = orden_pierna_pines[cuadranteActual][orden];
	if(puertoPin != GPIO_PIN_3 && puertoPin != GPIO_PIN_4 && puertoPin != GPIO_PIN_5) {
	
		puertoPin = 0;
	}

	HAL_GPIO_WritePin(GPIOA, orden_pierna_pines[cuadranteActual][orden], estadoPin);

	//printf("SP Q: %d, Odr: %d, Pin: %d, est: %d\n", cuadranteActual, orden, orden_pierna_pines[cuadranteActual][orden], estadoPin);
}

// Esta funcion es llamada por el handler del timer 3.
// El timer 3 tiene tres canales asociados a cada etapa de switch
// Ademas hay un cuarto canal que se usa para sincronizar el timer
// En esta ultima etapa vamos a calcular y cargar los valores de los ticks
void GestorSVM_Interrupt() {

	static int contador = 0;


	switch(contador) {
		case 0:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_1, 1);
			contador ++;
		break;
		case 1:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_2, 1);
			contador ++;
		break;
		case 2:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_3, 1);
			contador ++;
		break;
		case 3:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_3, 0);
			contador ++;
		break;
		case 4:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_2, 0);
			contador ++;
		case 5:
			GestorSVM_SwitchPuertos(ORDEN_SWITCH_1, 0);
			contador ++;
		break;
		case 6:
			// Aca tendriamos un error
			//TIM3->SR &= ~(1 << 1);
			
			GestorSVM_CalcularValoresSwitching(&valoresSwitching);
			//HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
			

			TIM3->CCR2 = valoresSwitching.ticks1[0]; // Cargo los ticks para el canal 2
			TIM3->CCR3 = valoresSwitching.ticks2[0]; // Para el canal 3
			TIM3->CCR4 = valoresSwitching.ticks3[0]; // Para el canal 4

			TIM3->EGR |= TIM_EGR_UG;

			contador = 0;
		break;
	}

}