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
#include "../Gestor_Estados/GestorEstados.h"
#include "../Gestor_Timers/GestorTimers.h"

// Solo para pruebas, despues borrar
//#include "stm32f1xx_hal_gpio.h"		// Para incluir a HAL_GPIO_WritePin  BORAR DE ACA

#include <stdio.h>		// Para el printf

// Definiciones privadas
#define MAX_TICKS 256	// Este es el ARR del timer
#define TICKS_DESHABILITAR_CANAL 280	// Valor para deshabilitar un canal, mayor a ARR
#define MIN_TICKS_DIF 5	// Minimos ticks para considerar este canal

// Constantes de calculo para t1 y t2

// Estas constantes se obtienen a partir de una regresion lineal en donde
// se debe multiplicar el factor proporcional por el angulo parcial multiplicado
// por 1000.000
#define CONST_CALC_T1_PROP 		(float) -3.59145E-6
#define CONST_CALC_T1_ORD_ORG 	(float) 224.2845426
#define CONST_CALC_T2_PROP		(float) +3.59145E-6
#define CONST_CALC_T2_ORD_ORG	(float) 8.797345358


// Enumeraciones privadas

enum Cuadrantes{
	CUADRANTE_1 = 0,
	CUADRANTE_2,
	CUADRANTE_3,
	CUADRANTE_4,
	CUADRANTE_5,
	CUADRANTE_6
};


typedef enum {
	INTERF_NULA = 0,
	INTERF_T1T2,
	INTERF_T1T3,
	INTERF_T2T3,
	INTERF_T1T2_T2T3
} CasoInterferenciaTimer;

// Variables privadas

// Variables de configuracion
static int frec_switch;
static int direccionRotacion; // 1 = sentido horario, -1 = antihorario
static int resolucionTimer;

static int frecOutMax;
static int frecOutMin;

// La frecuencia de salida puede ser negativa para evitar errores al momento de frenarlo
static int32_t frecSalida;	// Esta es la frecuencia actual escalada por 1000*1000
// Es es el angulo que se incrementa en cada switch, en cada conmutacion.
static uint32_t ang_switch; 		// En grados por mil

// Indice de modulacion, esta variable se sumamente importante. Nos va a 
// manejar la tension de salida. El motor necesita una relacion V/f constante
// Esta se alcanza cuando el motor gira a 50Hz
// Esta variable es entera y va de 0 a 100
static volatile int indiceModulacion;

static int acel;
static int desacel;

static int acelMin;
static int acelMax;
static int desacelMin;
static int desacelMax;


// Este va a trakear el angulo actual
static uint32_t angAct;		// Esta en grados por mil
// Este angulo va de 0 a 60*1000*1000 y puede tomar valores negativos
static int32_t angParcial;

// Con este se guarda el ultimo estado
static volatile int cuadranteActual;

// Cuadrante que se usa para los calculos, no para el switching de pines
// Esto se debe a que el calculo se realiza mientas se esta moviendo
static volatile int cuadranteActualCal;



const int vectorSecuenciaPorCuadrante[6][7] = {
    {0b000,0b100,0b110,0b111,0b110,0b100,0b000}, // Sector 1
    {0b000,0b010,0b110,0b111,0b110,0b010,0b000}, // Sector 2
    {0b000,0b010,0b011,0b111,0b011,0b010,0b000}, // Sector 3
    {0b000,0b001,0b011,0b111,0b011,0b001,0b000}, // Sector 4
    {0b000,0b001,0b101,0b111,0b101,0b001,0b000}, // Sector 5
    {0b000,0b100,0b101,0b111,0b101,0b100,0b000}  // Sector 6
};

int pinTogglePorCuadranteYOrden[6][3];
uint32_t estadoGPIOPorCuadranteYOrden[6][2];
uint32_t estadoGPIOOff;
uint32_t estadoGPIOOn;

// Vector base de los ocho estados de conmutacion
//const int vectorBase[8] = {0b000, 0b100, 0b110, 0b010, 0b011, 0b001, 0b101, 0b111};



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

// Ticks de cada channel
static volatile int ticksChannel[3];

// Flags de interferencia
// BORRAR ESTOS FLAGS, SE REMPLAZARON POR EL ENUM
//static volatile int flagInterferenciaCanal1y2;
//static volatile int flagInterferenciaCanal2y3;
//static volatile int flagInterferenciaCanal1y3;

static volatile CasoInterferenciaTimer casoInterferencia;


// Usado para un debug simple
static volatile int index = 0;
static volatile int ultsPuertos[50];
static volatile int ultsOutsLogicos[50];
static volatile int timerT3[50];
static volatile int ultsCuadrantes[50];


// Variables que guardan los datos para la gestion dinamica
// Frecuencia final de salida escalada por 1000
static volatile int32_t frecObjetivo;

// Este es un flag que indica que esta cambiando de frecuencia
static volatile int flagChangingFrec;
// Flag que indica si el movimiento es acelerado, si es 0 es desacelerado
static volatile int flagEsAcelerado;
// Frag que indica que el motor esta moviendose
static volatile int flagMotorRunning;

// Frecuencia referencia es la frecObjetivo cuando el estado esta en running
// Cuando se frena el motor la frecObjetivo difiere de esta
static volatile int frecReferencia;

// Para el cambio de frecuencia vamos a necesitar una estructura mas solida
// La variable de aceleracion la tenemos en frec/seg y hay que convertirla a un
// valor en que se incrementa/decrementa frecSalida en cada ciclo. 
// Necesita una conversion que se calcula una unica vez
// Este valor esta escalado por 1000
static volatile uint32_t cambioFrecPorCiclo;


static volatile char contadorSwitchsCiclo;


// Esta es la manejada por el handler del timer
volatile ValoresSwitching valoresSwitching;


// Debido a la lentitud del calculo, este no se puede realizar
// en el timer de switching, el calculo demora 46us lo que representa
// una parte importante del ciclo. Por ello es que se involucra un timer
// de calculo que va a dos veces la frecuencia del de switching e intenta
// adelantar calculos hasta tres muestras futuras. 

// El proceso consumidor, el timer de switching, va a ir leyendo del indice
// de lectura y reduciendo el la variable numDatos
// El proceso productor, el timer de calculo, va a ir escribiendo en el indice
// de escritura y aumentando la variable numDatos
// Si numDatos llega a 3, el proceso productor se detiene hasta que el consumidor
// lea algun dato

#define BUFFER_CALCULO_SIZE 3

typedef struct {
	int ticksChannel1[BUFFER_CALCULO_SIZE];
	int ticksChannel2[BUFFER_CALCULO_SIZE];
	int ticksChannel3[BUFFER_CALCULO_SIZE];
	
	// Si ambos canales estan muy cerca entre si se activa este flag
	//int flagInterferenciaCanal1y2[BUFFER_CALCULO_SIZE];	
	//int flagInterferenciaCanal2y3[BUFFER_CALCULO_SIZE]; 
	//int flagInterferenciaCanal1y3[BUFFER_CALCULO_SIZE];
	CasoInterferenciaTimer casoInterferencia[BUFFER_CALCULO_SIZE];

	int cuadranteActual[BUFFER_CALCULO_SIZE];
	int indEscritura;			// Indice de escritura
	int indLectura;				// Indice de lectura
	int numDatos;				// Cantidad de datos disponibles

}DatoCalculado;

volatile DatoCalculado bufferCalculo;


// Para evitar problemas de acceso a variables compartidas
// entre el timer de calculo y las funciones llamadas por el 
// gestor de estados, se recurre a una estructura con variables
// sombra que se actualizan en el timer de calculo
typedef struct {

	int32_t frecObjetivo;
	uint32_t cambioFrecPorCiclo;
	int direccionRotacion;

	// Flags
	int flagMotorRunning;
	int flagEsAcelerado;
	int flagChangingFrec;
} Parametros;	

volatile Parametros paramSombra;
volatile int flagActualizarParamSombra;





// Esta funcion se va a llamar en cada switch del timer 3 channel 4 para recalcular la 
// aceleracion/desaceleracion 
static void GestorSVM_CalculoAceleracion();

// Funcion para generar los pines de salida
static int pinMap(int x);


void GestorSVM_Init() {

	int i = 0;

	frec_switch = 0;
	frecSalida = 0;		// Motor frenado
	frecObjetivo = 0;	// La velocidad objetivo es nula
	frecReferencia = 0;
	ang_switch = 0;

	acelMin = 0;
	acelMax = 0;
	desacelMin = 0;
	desacelMax = 0;

	acel = 0;
	desacel = 0;

	angAct = 0;
	angParcial = 0;
	cuadranteActual = 0;
	flagAscensoAngParcial = 1;

	flagChangingFrec = 0;
	flagEsAcelerado = 0;
	flagMotorRunning = 0;

	contadorSwitchsCiclo = 0;

	
	direccionRotacion = 1;	// Sentido horario

	estadoLogicoSalida[0] = 0;
	estadoLogicoSalida[1] = 0;
	estadoLogicoSalida[2] = 0;


	// Se limpian los valores del buffer circular del timer de calculo
	for (i = 0; i < BUFFER_CALCULO_SIZE; i++) {

		bufferCalculo.ticksChannel1[i] = 0;
		bufferCalculo.ticksChannel2[i] = 0;
		bufferCalculo.ticksChannel3[i] = 0;
		//bufferCalculo.flagInterferenciaCanal1y2[i] = 0;
		//bufferCalculo.flagInterferenciaCanal2y3[i] = 0;
		bufferCalculo.casoInterferencia[i] = INTERF_NULA;
		bufferCalculo.cuadranteActual[i] = 0;
	}

	bufferCalculo.indEscritura = 0;
	bufferCalculo.indLectura = 0;
	bufferCalculo.numDatos = 0;
	


	// Se guardan los valores sombra
	paramSombra.frecObjetivo = frecObjetivo;
	paramSombra.cambioFrecPorCiclo = cambioFrecPorCiclo;
	paramSombra.direccionRotacion = direccionRotacion;
	paramSombra.flagMotorRunning = flagMotorRunning;
	paramSombra.flagEsAcelerado = flagEsAcelerado;
	paramSombra.flagChangingFrec = flagChangingFrec;
	flagActualizarParamSombra = 0;
}

int pinMap(int x) {
    x &= 0x07; // limitar a 3 bits
    int r = 0;
    if (x == 1) {r = 2;} 
    else if (x == 2) {r |= 1;}
    else if (x == 4) {r |= 0;}
    return r;
}

void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion) {
	
	int i, j;
	int puerto;

	// Rangos de funcionamiento del motor
	frecOutMax = configuracion->frecOutMax;
	frecOutMin = configuracion->frecOutMin;

	frec_switch = configuracion->frec_switch;
	//frecSalida = configuracion->frecSalida; Esta es nula cuando se inicializa, el motor esta frenado
	direccionRotacion = configuracion->direccionRotacion;
	resolucionTimer = configuracion->resolucionTimer;

	// Parametros dinamicos
	acel = configuracion->acel;
	desacel = configuracion->desacel;

	acelMin = configuracion->acelMin;
	acelMax = configuracion->acelMax;
	desacelMin = configuracion->desacelMin;
	desacelMax = configuracion->desacelMax;

	// Calculamos el angulo que recorre en un switch
	//ang_switch = frecSalida * 360 / frec_switch;
	
	// La variable sombra se actualiza dentro de la funcion
	GestorSVM_SetFrec(configuracion->frecReferencia);

	// Aca debemos comprobar si es posible
	// Si el angulo es mayor a un umbral debemos descartar
	// o aumentar la frecuencia de switching

	//printf("AngSwitch: %d\n", ang_switch);

	// Aca vamos a setear los pines de salida
	for(i = 0; i < 6; i++) {
		for(j = 0; j < 3; j++) {
			puerto = (int)configuracion->orden_pierna[i][j];
			orden_pierna_pines[i][j] = configuracion->puerto_senal_pierna[puerto];
			orden_pierna_logico[i][j] = (int)configuracion->orden_pierna[i][j];
		}
	}

	for(i = 0; i < 3; i++) {
		// El de puerto senal pierna no se usa pero lo dejo igual
		puerto_senal_pierna[i] = configuracion->puerto_senal_pierna[i];
		puerto_encen_pierna[i] = configuracion->puerto_encen_pierna[i];
	}

	printf("Configuracion Seteada \n");
	int estado0, estado1, estado2, estado3;
	int pinToggle1, pinToggle2, pinToggle3;
	uint32_t myBSRR;

	estadoGPIOOff = (puerto_senal_pierna[0] | puerto_senal_pierna[1] | puerto_senal_pierna[2]) << 16; 
	estadoGPIOOn = (puerto_senal_pierna[0] | puerto_senal_pierna[1] | puerto_senal_pierna[2]); 

	for(i = 0; i < 6; i++) {
		estado0 = vectorSecuenciaPorCuadrante[i][0];
		estado1 = vectorSecuenciaPorCuadrante[i][1];
		estado2 = vectorSecuenciaPorCuadrante[i][2];
		estado3 = vectorSecuenciaPorCuadrante[i][3];

		myBSRR = 0;
		myBSRR |= ((estado1 & 0b100) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado1 & 0b010) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado1 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);

		estadoGPIOPorCuadranteYOrden[i][0] = myBSRR;

		myBSRR = 0;
		myBSRR |= ((estado2 & 0b100) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado2 & 0b010) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado2 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);

		estadoGPIOPorCuadranteYOrden[i][1] = myBSRR;





		pinToggle1 = (estado0 ^ estado1);
		pinToggle2 = (estado1 ^ estado2);
		pinToggle3 = (estado2 ^ estado3);

		pinToggle1 = pinMap(pinToggle1);
		pinToggle2 = pinMap(pinToggle2);
		pinToggle3 = pinMap(pinToggle3);

		pinTogglePorCuadranteYOrden[i][0] = puerto_senal_pierna[pinToggle1];
		pinTogglePorCuadranteYOrden[i][1] = puerto_senal_pierna[pinToggle2];
		pinTogglePorCuadranteYOrden[i][2] = puerto_senal_pierna[pinToggle3];

	}

}



void GestorSVM_CalcularValoresSwitching() {


	// Esta funcion se ejecuta cada vez que se cumple el tiempo t1, t2 y t3
	// Estos tiempos por como esta configurado el timer tienen una resolucion de 256
	// Es decir que voy a tener que ajustar mi tiempo a un valor puntual de estos 256
	// En fswitch que dura la subida y bajada del contador voy a tener 256 pasos
	// Por lo que tengo que calcular en que paso voy a hacer el t1, el t2 y el t3
	// Para ajustar el M voy a agregar un multiplicador


	// Aca vamos a guardar el valor en que haremos la interrupcion
	int t1, t2, t0;
	int ticksC1, ticksC2, ticksC3;

	// Chequeo y calculo de aceleracion / desaceleracion
	if(flagChangingFrec) {
		GestorSVM_CalculoAceleracion();
	}
	//GestorSVM_CalculoAceleracion();
	

	// El angulo se incrementa al terminar el ciclo de switch
	angAct += ang_switch;
	if(angAct >= 360 * 1000 * 1000) {
		angAct -= 360 * 1000 * 1000;
		cuadranteActual = 0;
		flagAscensoAngParcial = 1;
		// Aca vuelvo a sincronizar el angulo parcial
		angParcial = angAct;
	}else {
		if(flagAscensoAngParcial == 1) {
			angParcial += ang_switch;
			if(angParcial >= 60 * 1000 * 1000) {
				// Si me paso le resto lo que me pase, si estaba en 61, pongo 59
				// y cuento hacia abajo ahora
				angParcial = 2 * 60 * 1000 * 1000 - angParcial;
				flagAscensoAngParcial = 0;
				cuadranteActual = (cuadranteActual + 1) % 6;
			}
		} else {
			angParcial -= ang_switch;
			if(angParcial <= 0) {
				// Si me paso de cero le resto devuelta
				// lo que me pase
				angParcial = -angParcial;
				flagAscensoAngParcial = 1;
				cuadranteActual = (cuadranteActual + 1) % 6;
			}
		}
	}

	// Chequeamos que tengamos todos los switches
	//if(contadorSwitchsCiclo != 6) {
	//	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
	//}
	// Reiniciamos el contador
	//contadorSwitchsCiclo = 0;



	//printf("AngAct: %d, AngPar: %d, Cuad: %d, angSwitch: %d\n", angAct, angParcial, cuadranteActual, ang_switch);


	// Si la resolucion es 256 justo =>
	//t1 = (int)(((-0.0018 * (float)angParcial + 112.57) * indiceModulacion) / 100);
	//t2 = (int)(((+0.0018 * (float)angParcial + 10.955) * indiceModulacion) / 100);
	t1 = (int)(((CONST_CALC_T1_PROP * (float)angParcial + CONST_CALC_T1_ORD_ORG) * indiceModulacion) / 100);
	t2 = (int)(((CONST_CALC_T2_PROP * (float)angParcial + CONST_CALC_T2_ORD_ORG) * indiceModulacion) / 100);
	// Este es t0 / 2
	t0 = (int)((float)(255 - t1 - t2) * 0.5);


	/*
	// Si la resolucion es distinta puedo dejarla parametrizada
	t1 = (-7.0E-6 * (float)currentAng + 0.4397) * resolucionTimer;
	t2 = (+7.0E-6 * (float)currentAng + 0.0428) * resolucionTimer;
	t0 = resolucionTimer - t1 - t2;
	*/
	//printf("ang: %d, t0: %d, t1: %d, t2: %d\n", angAct, t0, t1, t2);



	// Si el timer 3 es muy cercano al limite le escribo un valor que nunca se alcanza
	// Sino tengo problema con los puertos

	ticksC1 = t0;
	ticksC2 = t0 + t1;
	ticksC3 = t0 + t1 + t2;

	// Analisis de los casos de interferencia
	if(ticksC3 - ticksC1 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T3;
	}else if(ticksC2 - ticksC1 < MIN_TICKS_DIF && ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T2_T2T3;
	}else if(ticksC2 - ticksC1 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T2;
	}else if(ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T2T3;
	}else {
		casoInterferencia = INTERF_NULA;
	}

	/*
	if(ticksC2 - ticksC1 < MIN_TICKS_DIF) {
		ticksC1 = ticksC2 - MIN_TICKS_DIF;
		casoInterferencia = INTERF_NULA;
	}
	if(ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		ticksC3 = ticksC2 + MIN_TICKS_DIF;
		casoInterferencia = INTERF_NULA;
	}
	*/


	/*
	if(ticksC2 - ticksC1 < MIN_TICKS_DIF) {
		flagInterferenciaCanal1y2 = 1;
	}else {
		flagInterferenciaCanal1y2 = 0;
	}

	if(ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		flagInterferenciaCanal2y3 = 1;
	}else {
		flagInterferenciaCanal2y3 = 0;
	}
	
	if(ticksC3 - ticksC1 < MIN_TICKS_DIF) {
		flagInterferenciaCanal1y3 = 1;
	}else {
		flagInterferenciaCanal1y3 = 0;
	}
	*/

	ticksChannel[0] = ticksC1;
	ticksChannel[1] = ticksC2;
	ticksChannel[2] = ticksC3;



	//if(resolucionTimer/2 - ticksChannel[2] < 5) {
		// Esto no se ejecuta nunca ya que de todas formas el timer es muy chico
	//	ticksChannel[2] = 2 * resolucionTimer;
	//}
}







void GestorSVM_CalcInterrupt() {
	// Esta funcion se llama cada vez que se cumple el tiempo de calculo
	// Este timer va a 2 veces la frecuencia del timer de switching
	// En esta funcion se calcula y se guardan los valores en el buffer circular
	// Si el buffer esta lleno, no se hace nada

	//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);

	int indEscritura;


	if(bufferCalculo.numDatos < BUFFER_CALCULO_SIZE) {

		indEscritura = bufferCalculo.indEscritura;

		GestorSVM_CalcularValoresSwitching();
		

		// Se guardan los valores calculados
		bufferCalculo.ticksChannel1[indEscritura] = ticksChannel[0];
		bufferCalculo.ticksChannel2[indEscritura] = ticksChannel[1];
		bufferCalculo.ticksChannel3[indEscritura] = ticksChannel[2];
		//bufferCalculo.flagInterferenciaCanal1y2[indEscritura] = flagInterferenciaCanal1y2;
		//bufferCalculo.flagInterferenciaCanal2y3[indEscritura] = flagInterferenciaCanal2y3;
		//bufferCalculo.flagInterferenciaCanal1y3[indEscritura] = flagInterferenciaCanal1y3;
		bufferCalculo.casoInterferencia[indEscritura] = casoInterferencia;
		bufferCalculo.cuadranteActual[indEscritura] = cuadranteActual;

		// Se actualizan los indices y la cantidad de datos
		bufferCalculo.indEscritura = (indEscritura + 1) % BUFFER_CALCULO_SIZE;
		bufferCalculo.numDatos ++;

	}

	//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
}



// Esta funcion es llamada por el handler del timer 3.
// El timer 3 tiene tres canales asociados a cada etapa de switch
// Ademas hay un cuarto canal que se usa para sincronizar el timer
// En esta ultima etapa vamos a calcular y cargar los valores de los ticks
void GestorSVM_SwitchInterrupt(SwitchInterruptType intType) {

	// Chequea si esta en cuenta arriba o abajo
	volatile static int countUpTx[3];

	// Este flag si esta en 1 significa que se esta esperando una interrupcion
	volatile static int flagTxActivo[3];
	//volatile static int flagTxPreactivacion[3];

	volatile static CasoInterferenciaTimer interferencia;

	volatile static int cuadrante = 0;
	int ticks1, ticks2, ticks3;

	
	if(bufferCalculo.numDatos <= 0) {
		return;
	}



	switch(intType) {
		case SWITCH_INT_CH1:
			if(flagTxActivo[0]) {
				flagTxActivo[0] = 0;
				if(countUpTx[0]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_1_UP, 1, cuadrante);
					countUpTx[0] = 0;
				}else {
					countUpTx[0] = 1;
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_1_DOWN, 0, cuadrante);
					// Este aca habilitamos todos los canales que habiamos bloqueado
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;

				}
			}else {
				flagTxActivo[0] = 1;
			}
			break;
		case SWITCH_INT_CH2:
			if(flagTxActivo[1]) {
				flagTxActivo[1] = 0;
				if(countUpTx[1]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_2_UP, 1, cuadrante);
					countUpTx[1] = 0;
				}else {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_2_DOWN, 0, cuadrante);
					countUpTx[1] = 1;
				}
			}else {
				if(interferencia == INTERF_T1T2_T2T3) {
					flagTxActivo[1] = 0;
					// Desactivo T2
					TIM3->DIER &= ~TIM_DIER_CC3IE;
				} else {
				flagTxActivo[1] = 1;
				}
			}
			break;

			
			case SWITCH_INT_CH3:
			if(flagTxActivo[2]) {
				flagTxActivo[2] = 0;
				if(countUpTx[2]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_3_UP, 1, cuadrante);

					// Se evaluan el caso de interferencia, el caso de INTERF_T1T3 no se puede dar aca
					switch(interferencia) {
						case INTERF_NULA:
							// Se activan todos los canales
							flagTxActivo[0] = 1;
							flagTxActivo[1] = 1;
							flagTxActivo[2] = 1;
							break;
						case INTERF_T1T2:
							// Desactivacion de T2 y Activacion de T1
							flagTxActivo[0] = 0;
							flagTxActivo[1] = 0;
							flagTxActivo[2] = 1;
							// Deshabilitacion T2
							TIM3->DIER &= ~TIM_DIER_CC3IE;
							// Activacion T1
							TIM3->DIER |= TIM_DIER_CC2IE;
							break;
						case INTERF_T2T3:
							// Desactivacion de T3 y Activacion de T2
							flagTxActivo[0] = 1;
							flagTxActivo[1] = 0;	// Queda pendiente interrupcion T2
							flagTxActivo[2] = 0;
							// Deshabilitacion T3
							TIM3->DIER &= ~TIM_DIER_CC4IE;
							// Activacion T2
							TIM3->DIER |= TIM_DIER_CC3IE;
							break;
						case INTERF_T1T3:
							// No es posible encontrear este estado aca
							break;
						case INTERF_T1T2_T2T3:
							// Se desactiva T2
							flagTxActivo[0] = 0;
							flagTxActivo[1] = 0;
							flagTxActivo[2] = 1;
							// Activacion T1 y desactivacion de T2
							TIM3->DIER &= ~TIM_DIER_CC3IE;
							TIM3->DIER |= TIM_DIER_CC2IE;
							break;
					}
					// Se establecen que las proximas interrupciones son cuenta abajo
					countUpTx[0] = 0;
					countUpTx[1] = 0;
					countUpTx[2] = 0;
				}else {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_3_DOWN, 0, cuadrante);
					countUpTx[2] = 1;
				}
			}else { 
				if(interferencia == INTERF_T1T3) {
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 0;
					flagTxActivo[2] = 0;
				}else {
					flagTxActivo[2] = 1;
				}
			}
			break;
		case SWITCH_INT_RESET:

			// RESET

			// Canales cuenta arriba
			countUpTx[0] = 1;
			countUpTx[1] = 1;
			countUpTx[2] = 1;

			// Carga de los valores calculados
			ticks1 = bufferCalculo.ticksChannel1[bufferCalculo.indLectura];
			ticks2 = bufferCalculo.ticksChannel2[bufferCalculo.indLectura];
			ticks3 = bufferCalculo.ticksChannel3[bufferCalculo.indLectura];
			interferencia = bufferCalculo.casoInterferencia[bufferCalculo.indLectura];
			cuadrante = bufferCalculo.cuadranteActual[bufferCalculo.indLectura];
			bufferCalculo.indLectura = (bufferCalculo.indLectura + 1) % BUFFER_CALCULO_SIZE;
			bufferCalculo.numDatos --;

			

			// Habilitacion de timers
			


			// Si tengo interferencia entre el 1 y 3 directamente se deshabilitan los tres canales
			// Esto significaria ir del V0 al V7, lo que no genera ningun cambio asi que nos evitamos
			// esta conmutacion inecesaria
			switch(interferencia) {
				case INTERF_NULA:
					// No hay interferencia, habilito todos los canales
					flagTxActivo[0] = 1;
					flagTxActivo[1] = 1;
					flagTxActivo[2] = 1;
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T1T2:
					// Desactivacion de T1 y Activacion de T2 y T3
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 1;
					flagTxActivo[2] = 1;
					TIM3->DIER &= ~TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T2T3:
					// Desactivacion de T2 y Activacion de T1 y T3
					flagTxActivo[0] = 1;
					flagTxActivo[1] = 0;
					flagTxActivo[2] = 1;
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER &= ~TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T1T3:
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 0;
					flagTxActivo[2] = 0;
					// Posiciones distantes ya que no genera ningun cambio		
					ticks1 = 20;
					ticks2 = 60;
					ticks3 = 100;			
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T1T2_T2T3:
					// Desactivacion de T1 y Activacion de T2 y T3
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 1;
					flagTxActivo[2] = 1;
					TIM3->DIER &= ~TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
			}


			// Carga de los timers
			TIM3->CCR2 = ticks1; // T1 en canal 2
			TIM3->CCR3 = ticks2; // T2 en canal 3
			TIM3->CCR4 = ticks3; // T3 en canal 4



			break;
		case SWITCH_INT_CLEAN:
			// En este se limpian las variales
			countUpTx[0] = 1;
			countUpTx[1] = 1;
			countUpTx[1] = 1;
			flagTxActivo[0] = 0;
			flagTxActivo[1] = 0;
			flagTxActivo[1] = 0;
			break;
			
		default:
				// En el default no hay nada
			break;
	}
	

}


// Esta funcion utiliza el caudranteActual para setear los pines de salida
void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado, int numCuadrante) {

	switch(orden) {
		case ORDEN_SWITCH_1_UP:
			GPIOA->BSRR = estadoGPIOPorCuadranteYOrden[numCuadrante][0];
			break;
		case ORDEN_SWITCH_2_UP:
			GPIOA->BSRR = estadoGPIOPorCuadranteYOrden[numCuadrante][1];
			break;
		case ORDEN_SWITCH_3_UP:
			GPIOA->BSRR = estadoGPIOOn;
			break;
		case ORDEN_SWITCH_3_DOWN:
			GPIOA->BSRR = estadoGPIOPorCuadranteYOrden[numCuadrante][1];
			break;
		case ORDEN_SWITCH_2_DOWN:
			GPIOA->BSRR = estadoGPIOPorCuadranteYOrden[numCuadrante][0];
			break;
		case ORDEN_SWITCH_1_DOWN:
			GPIOA->BSRR = estadoGPIOOff;
			break;
	}

}



// Funcion ejecutada por el calculo, no necesita parametros sombra
void GestorSVM_CalculoAceleracion() {

	// En esta funcion se puede llegar a dar una incoherencia debido a la sincronizacion
	// de las variables sombra. Si se esta ejecutando un cambio de frecuencia en la funcion 
	// GestorSVM_SetFrecOut, es decir que se habia escrito flagChangingFrec y antes de llegar
	// levantar el flagActualizarParamSombra, justo en ese momento llega una interrupcion 
	// de este timer de calculo y en esa iteracion se terminaba de llegar a la velocidad 
	// taget. En ese caso se va a pisar ese flagChangingFrec y se va a quedar como si nunca hubiese
	// pasado.
	// Debido a que es un caso muy extremo no se analiza 

	// Chequeamos si hay que actualizar los parametros sombra
	if(flagActualizarParamSombra) {
		frecObjetivo = paramSombra.frecObjetivo;
		cambioFrecPorCiclo = paramSombra.cambioFrecPorCiclo;
		direccionRotacion = paramSombra.direccionRotacion;

		flagMotorRunning = paramSombra.flagMotorRunning;
		flagEsAcelerado = paramSombra.flagEsAcelerado;
		flagChangingFrec = paramSombra.flagChangingFrec;

		flagActualizarParamSombra = 0;
	}

	// La frecOutput y la frecObjetivo esta escalada por 1000
	// Tenemos que agregarle un prescaler para las frec bajas


	if(flagEsAcelerado) {
		frecSalida += cambioFrecPorCiclo;
		if(frecSalida >= frecObjetivo) {
			frecSalida = frecObjetivo;
			// Se manda la señal, la indicacion al gestor de estados
			GestorEstados_Action(ACTION_TO_CONST_RUNNING, 0);
			// Se acomodan los flags
			flagChangingFrec = 0;
		}
	}else {
		frecSalida -= cambioFrecPorCiclo;
		if(frecSalida <= frecObjetivo) {
			frecSalida = frecObjetivo;
			
			if(frecObjetivo == 0) {
				flagMotorRunning = 0;
				
				// Se detiene el timer de interrupcion
				GestorTimers_DetenerTimerSVM();
				
				// Se limpian la funcion de interrpuciones de switch
				GestorSVM_SwitchInterrupt(SWITCH_INT_CLEAN);
				
				// Limpieza del buffer de calculo
				bufferCalculo.indEscritura = 0;
				bufferCalculo.indLectura = 0;
				bufferCalculo.numDatos = 0;
				
				// Apagamos los pines de la senal
				HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[0], GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[1], GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[2], GPIO_PIN_RESET);
				
				// Apagamos los pines de encendido del IR2104
				HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[0], GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[1], GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[2], GPIO_PIN_RESET);
				
				// Se envia la señal de que el motor se detuvo
				GestorEstados_Action(ACTION_MOTOR_STOPPED, 0);

			}else {
				// Se manda la señal, la indicacion al gestor de estados
				GestorEstados_Action(ACTION_TO_CONST_RUNNING, 0);
			}			
			
			// Se acomodan los flags
			flagChangingFrec = 0;
		}
	}

	if(frecSalida < 50 * 1000 * 1000) {
		indiceModulacion = frecSalida / (500 * 1000);
	}else {
		indiceModulacion = 100;
	}

	// No vamos a permitir un indice de modulacion menor a 2
	if(indiceModulacion < 1) {
		indiceModulacion = 1;
	}

	// Calculamos el angulo que recorre en un switch
	// El orden de la operacion es importante, de manera contraria
	// se podria generar un overflow
	ang_switch = (frecSalida / frec_switch) * 360 ;


	//static int indx = 0;

	//if(frecSalida > indx * 1000 * 1000) {
	//	printf("angParcial: %d, angSwitch: %d, frecOut: %d, indiceMod: %d\n", angParcial, ang_switch, frecSalida, indiceModulacion);
	//	indx ++;
	//}
}


/*

	Funciones llamadas por el Gestor de Estados
	La mayoria derivan de llamadas del SPI, se ejecutan en el vector de interrupcion
	del DMA

*/


	
int GestorSVM_SetFrec(int frec) {

	int flagEsAcelerado_local;
	int32_t cambioFrecPorCiclo_local;
	int32_t fregTarget_local;
	int32_t nuevaFrec;

	if(frec < frecOutMin || frec > frecOutMax) {return -1;}	// Fuera de los rangos de operacion
	
	// Lo escalamos
	nuevaFrec = (int32_t)frec * 1000 * 1000;

	// Se cambio la frecuencia a la misma, error
	if(frecSalida == nuevaFrec) {return -2;}

	// Si el motor esta en movimiento tengo que configurar para realizar un cambio de velocidad
	if(flagMotorRunning) {

		// Seteo del flagEsAcelerado usando la frec actual
		if(frecSalida < nuevaFrec) {
			flagEsAcelerado_local = 1;
			cambioFrecPorCiclo_local = (acel * 1000 * 1000) / (frec_switch);
		}else {
			flagEsAcelerado_local = 0;
			cambioFrecPorCiclo_local = (desacel * 1000 * 1000) / (frec_switch);
		}
		
		fregTarget_local = nuevaFrec;
		frecReferencia = frec;
	
		// Seteo de los parametros sombra y el flag de actualizacion
		// No se modifica paramSombra.direccionRotacion
		paramSombra.flagMotorRunning = 1;
		paramSombra.flagChangingFrec = 1;
		paramSombra.flagEsAcelerado = flagEsAcelerado_local;
		paramSombra.cambioFrecPorCiclo = cambioFrecPorCiclo_local;
		paramSombra.frecObjetivo = fregTarget_local;
		flagActualizarParamSombra = 1;

		// Habilitamos el cambio de frecuencia
		flagChangingFrec = 1;

		return 1;
	}else {
		frecReferencia = frec;
		return 0;
	}


	return 0;
}

int GestorSVM_SetDir(int dir) {

	int estado1, estado2;
	int i;
	uint32_t myBSRR;

	// Si estamos cambiando la frecuencia, no podemos cambiar la direccion
	if(flagMotorRunning) {return -2;}
	if(flagChangingFrec) {return -2;}
	if(dir != 0 && dir != 1) {return -1;}

	if(dir == direccionRotacion) {return 0;}
	
	// Se recalcula el estadoGPIOPorCuadranteYOrden
	// Se intercambian el orden de los pines, el pin 1 por el 2

	for(i = 0; i < 6; i++) {
		estado1 = vectorSecuenciaPorCuadrante[i][1];
		estado2 = vectorSecuenciaPorCuadrante[i][2];

		myBSRR = 0;
		myBSRR |= ((estado1 & 0b100) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado1 & 0b010) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado1 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);

		estadoGPIOPorCuadranteYOrden[i][0] = myBSRR;

		myBSRR = 0;
		myBSRR |= ((estado2 & 0b100) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado2 & 0b010) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado2 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);

		estadoGPIOPorCuadranteYOrden[i][1] = myBSRR;
	}

	direccionRotacion = dir;

	return 0;
}

// Devuelve 0 si se modifica exitosamente
// Devuelve -1 si falla por fuera de rango
// Devuelve -2 si falla ya que esta cambiando de velocidad
int GestorSVM_SetAcel(int nuevaAcel) {
	
	// Si estamos cambiando la frecuencia, no podemos cambiar la aceleracion
	if(flagChangingFrec) {return -2;}

	if(nuevaAcel < acelMin || nuevaAcel > acelMax) {return -1;}	// Fuera de los rangos de operacion
	
	acel = nuevaAcel;
	
	return 0;
}

int GestorSVM_SetDecel(int nuevaDecel) {
	// Si estamos cambiando la frecuencia, no podemos cambiar la desaceleracion
	if(flagChangingFrec) {return -2;}

	if(nuevaDecel < desacelMin || nuevaDecel > desacelMax) {return -1;}	// Fuera de los rangos de operacion
	
	desacel = nuevaDecel;

	return 0;
}

int GestorSVM_MotorStart() {

	// En esta funcion no se utilizan parametros sombra ya que no se esta ejecutando el timer
	// de calculo ni el de switcheo
	if(!flagMotorRunning) {

		// El motor se acelera, se levanta el flag de running
		// Es un movimiento acelerado, aumenta las rpm
		// Se calcula la aceleracion, se levanta el flag de cambio de movimiento
		flagMotorRunning = 1;
		flagEsAcelerado = 1;
		flagChangingFrec = 1;

		// Calculo de la aceleracion
		cambioFrecPorCiclo = (acel * 1000 * 1000) / (frec_switch);

		paramSombra.frecObjetivo = (uint32_t)frecReferencia * 1000 * 1000;
		paramSombra.flagMotorRunning = 1;
		paramSombra.flagEsAcelerado = 1;
		paramSombra.flagChangingFrec = 1;
		paramSombra.cambioFrecPorCiclo = cambioFrecPorCiclo;
		flagActualizarParamSombra = 1;


		//GestorSVM_CalcularValoresSwitching();

		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[0], GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[1], GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[2], GPIO_PIN_SET);

		// Se calcula un valor
		GestorSVM_CalcInterrupt();

		// Se consume y se precargan los timers
		GestorSVM_SwitchInterrupt(SWITCH_INT_RESET);

		// Se inician las interrupciones
		GestorTimers_IniciarTimerSVM();

		return 0;
	}

	return 1;
}

int GestorSVM_MotorStop() {

	if(flagMotorRunning) {
		
		flagEsAcelerado = 0;
		flagChangingFrec = 1;

		// Calculo de la desaceleracion
		cambioFrecPorCiclo = (desacel * 1000 * 1000) / (frec_switch);

		frecObjetivo = 0;

		// Se actualizan los parametros sombra
		paramSombra.flagMotorRunning = 1;
		paramSombra.flagEsAcelerado = 0;
		paramSombra.flagChangingFrec = 1;
		paramSombra.cambioFrecPorCiclo = cambioFrecPorCiclo;
		paramSombra.frecObjetivo = 0;
		flagActualizarParamSombra = 1;

		return 0;
	}
	return 1;
}

int GestorSVM_Estop() {

	if(flagMotorRunning) {

		// Se detiene el timer de interrupcion
		GestorTimers_DetenerTimerSVM();
		
		// Apagamos los pines de encendido del IR2104
		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[0], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[1], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, puerto_encen_pierna[2], GPIO_PIN_RESET);
		
		// Se limpian la funcion de interrpuciones de switch
		GestorSVM_SwitchInterrupt(SWITCH_INT_CLEAN);
		
		// Limpieza del buffer de calculo
		bufferCalculo.indEscritura = 0;
		bufferCalculo.indLectura = 0;
		bufferCalculo.numDatos = 0;

		// Apagamos los pines de la senal
		HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[0], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[1], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, puerto_senal_pierna[2], GPIO_PIN_RESET);

		// Limpio los flags y establezco la frecuencia en cero
		flagChangingFrec = 0;
		flagMotorRunning = 0;
		frecSalida = 0;
	}

	return 0;	// Siempre devuelve cero
}

int GestorSVM_GetFrec() {
	/*if(flagMotorRunning) {
		return frecSalida / (1000 * 1000);
	} else {
		return 0;
	}*/

	return frecReferencia;
}

int GestorSVM_GetAcel() {
	return acel;
}

int GestorSVM_GetDesacel() {
	return desacel;
}

int GestorSVM_GetDir() {
	return direccionRotacion;
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM3) return;

    switch (htim->Channel)
    {
        case HAL_TIM_ACTIVE_CHANNEL_1:
            // Canal 1 → normalmente RESET o sincronización
            GestorSVM_SwitchInterrupt(SWITCH_INT_RESET);
            break;

        case HAL_TIM_ACTIVE_CHANNEL_2:
            GestorSVM_SwitchInterrupt(SWITCH_INT_CH1);
            break;

        case HAL_TIM_ACTIVE_CHANNEL_3:
            GestorSVM_SwitchInterrupt(SWITCH_INT_CH2);
            break;

        case HAL_TIM_ACTIVE_CHANNEL_4:
            GestorSVM_SwitchInterrupt(SWITCH_INT_CH3);
            break;

        default:
            break;
    }
}
