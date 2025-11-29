/**
 * @file GestorSVM.cpp
 * @brief Implementación del gestor SVM (Space Vector Modulation) para control de puente trifásico con TIM3.
 * @details
 *   - TIM3 (center-aligned, ARR≈255) realiza el switching (t1/t2/t3 y RESET) por canales CCR2/CCR3/CCR4 (+ CCR1 como resync).
 *   - TIM2 (no visible acá) actúa como timer de cálculo (productor) que alimenta un buffer circular que consume TIM3.
 *   - Se usan parámetros “sombra” para sincronizar cambios de consigna con el lazo de cálculo en interrupciones.
 * Debido a la lentitud del calculo, este no se puede realizar en el timer de switching, el calculo demora 46us lo que representa una parte importante del ciclo. Por ello es que se involucra un timer de calculo que va a dos veces la frecuencia del de switching e intenta adelantar calculos hasta tres muestras futuras. 
 *
 * El proceso consumidor, el timer de switching, va a ir leyendo del indice de lectura y reduciendo el la variable numDatos.
 * El proceso productor, el timer de calculo, va a ir escribiendo en el indice de escritura y aumentando la variable numDatos
 * Si numDatos llega a 3, el proceso productor se detiene hasta que el consumidor lea algun dato.
 */

#include <stdio.h>
#include "GestorSVM.h"
#include "stm32f103xb.h"
#include "../Inc/main.h"
#include "../Gestor_Estados/GestorEstados.h"
#include "../Gestor_Timers/GestorTimers.h"

/**
 * @def MAX_TICKS
 * @brief ARR efectivo del timer de switching (resolución ≈ 256 pasos).
 */
#define MAX_TICKS 256

/**
 * @def TICKS_DESHABILITAR_CANAL
 * @brief Valor mayor a ARR para forzar que un CCR nunca dispare (deshabilitar canal por “posición imposible”).
 */
#define TICKS_DESHABILITAR_CANAL 280

/**
 * @def MIN_TICKS_DIF
 * @brief Mínima separación (en ticks) entre eventos para considerar que no hay interferencia entre t1/t2/t3.
 */
#define MIN_TICKS_DIF 5

/** @name Constantes de cálculo t1/t2 por regresión lineal
 *  @brief t1 y t2 se obtienen con una regresión lineal en función del ángulo parcial y el índice de modulación.
 *  @{ */
#define CONST_CALC_T1_PROP      (float)-3.59145E-6
#define CONST_CALC_T1_ORD_ORG   (float)224.2845426
#define CONST_CALC_T2_PROP      (float)+3.59145E-6
#define CONST_CALC_T2_ORD_ORG   (float)8.797345358
/** @} */

/**
 * @enum CasoInterferenciaTimer
 * @brief Casos de interferencia temporal entre eventos t1/t2/t3 (demasiado cercanos).
 */
typedef enum {
	INTERF_NULA = 0,        /// Sin interferencias.
	INTERF_T1T2,            /// t1 y t2 demasiado cercanos.
	INTERF_T1T3,            /// t1 y t3 demasiado cercanos.
	INTERF_T2T3,            /// t2 y t3 demasiado cercanos.
	INTERF_T1T2_T2T3        /// t1~t2 y t2~t3 simultáneamente.
} CasoInterferenciaTimer;

/** @brief Frecuencia de switching [Hz] (TIM3). */
static int frecuenciaSwitching;
/** @brief Sentido de rotación: 1 horario, -1 antihorario. */
static int direccionRotacion = 1;
/** @brief Frecuencia de salida INSTANTÁNEA (escalada ×1e6) usada por el lazo de rampa. */
static int32_t frecuenciaSalida;
/** @brief Incremento angular por switching (grados×1e3). */
static uint32_t anguloSwitching;
/** @brief Índice de modulación (0..100). Controla la tensión de salida. La relación v/f debe ser constante*/
static volatile int indiceModulacion;
/** @brief acelerada configurada [Hz/s]. */
static int aceleracion;
/** @brief Desacelerada configurada [Hz/s]. */
static int desaceleracion;
/** @brief Ángulo absoluto (grados×1e3). */
static uint32_t anguloActual;
/** @brief Ángulo parcial 0..60° (grados×1e3), usado para t1/t2. Puede ser negativo durante el diente. */
static int32_t anguloParcial;
/** @brief Cuadrante SVM actual (0..5). */
static volatile int cuadranteActual;

/**
 * @brief Secuencia SVM por cuadrante (V0→V1→V2→V7→V2→V1→V0).
 * @details Cada fila representa el vector lógico {U,V,W} en 7 pasos por sector.
 */
const int vectorSecuenciaPorCuadrante[6][7] = {
    {0b000,0b100,0b110,0b111,0b110,0b100,0b000}, // Sector 1
    {0b000,0b010,0b110,0b111,0b110,0b010,0b000}, // Sector 2
    {0b000,0b010,0b011,0b111,0b011,0b010,0b000}, // Sector 3
    {0b000,0b001,0b011,0b111,0b011,0b001,0b000}, // Sector 4
    {0b000,0b001,0b101,0b111,0b101,0b001,0b000}, // Sector 5
    {0b000,0b100,0b101,0b111,0b101,0b100,0b000}  // Sector 6
};

/** @brief Mapa de qué pin conmuta en cada orden y cuadrante. */
int pinTogglePorCuadranteYOrden[6][3];
/** @brief Palabras BSRR precalculadas por cuadrante/estado (0/1). */
uint32_t estadoGPIOPorCuadranteYOrden[6][2];
/** @brief BSRR para apagar U/V/W simultáneo. */
uint32_t estadoGPIOOff;
/** @brief BSRR para encender U/V/W simultáneo. */
uint32_t estadoGPIOOn;

/** @brief Bandera de rampa del ángulo parcial (subida/bajada en diente). */
static volatile char flagAscensoAnguloParcial = 1;
/** @brief Estado lógico de salidas U/V/W (para depuración). */
static volatile char estadoLogicoSalida[3];
/** @brief t1/t2/t3 en ticks para el ciclo actual. */
static volatile int ticksChannel[3];
/** @brief Caso de interferencia detectado para el ciclo actual. */
static volatile CasoInterferenciaTimer casoInterferencia;

/** @brief Frecuencia objetivo (target) escalada ×1e6. */
static volatile int32_t frecObjetivo;
/** @brief Indica cambio de frecuencia en curso (rampa activa). */
static volatile int flagChangingFrecuencia;
/** @brief 1 si la rampa corresponde a una aceleración, 0 para desaceleración. */
static volatile int flagEsAcelerado;
/** @brief 1 si el motor está en movimiento. */
static volatile int flagMotorRunning;
/** @brief Frecuencia “de referencia” reportada (Hz). */
static volatile int frecuenciaReferenica;
/** @brief Delta por ciclo de switching (escalado ×1e6) para la rampa. */
static volatile uint32_t cambioFrecuenciaPorCiclo;
/** @brief Estructura auxiliar de switching usada por el ISR. */
volatile ValoresSwitching valoresSwitching;

/**
 * @brief Buffer circular (productor: timer de cálculo; consumidor: timer de switching).
 * @details Tamaño fijo 3: el productor se detiene si está lleno.
 */
#define BUFFER_CALCULO_SIZE 3

typedef struct {
	int ticksChannel1[BUFFER_CALCULO_SIZE];
	int ticksChannel2[BUFFER_CALCULO_SIZE];
	int ticksChannel3[BUFFER_CALCULO_SIZE];
	CasoInterferenciaTimer casoInterferencia[BUFFER_CALCULO_SIZE];
	int cuadranteActual[BUFFER_CALCULO_SIZE];
	int indiceEscritura;  					/// Índice de escritura del productor.
	int indiceLectura;    					/// Índice de lectura del consumidor.
	int contadorDeDatos;      				/// Elementos válidos en el buffer.
} DatoCalculado;

volatile DatoCalculado bufferCalculo;

/**
 * @brief Parámetros “sombra” para sincronización atómica con el lazo de cálculo.
 */
typedef struct {
	int32_t frecObjetivo;
	uint32_t cambioFrecuenciaPorCiclo;
	int direccionRotacion;
	int flagMotorRunning;
	int flagEsAcelerado;
	int flagChangingFrecuencia;
} Parametros;

volatile Parametros paramSombra;
volatile int flagActualizarParamSombra;

/* ================================ Prototipos privados ================================ */

/**
 * @fn static void GestorSVM_Calculoaceleracioneracion(void)
 * @brief Aplica la rampa de velocidad (aceleracion/desaceleracion) sobre @ref frecuenciaSalida y actualiza flags/estado.
 * @details
 *   - Si @ref flagActualizarParamSombra está activo, copia los parámetros de @ref paramSombra.
 *   - Ajusta @ref frecuenciaSalida en ± @ref cambioFrecuenciaPorCiclo hasta llegar a @ref frecObjetivo.
 *   - Al llegar a 0 Hz: detiene timers, limpia buffer, apaga GPIO y notifica @ref ACTION_MOTOR_STOPPED.
 * @note En esta funcion se puede llegar a dar una incoherencia debido a la sincronizacion
 * de las variables sombra. Si se esta ejecutando un cambio de frecuencia en la funcion 
 * GestorSVM_SetFrecOut, es decir que se habia escrito flagChangingFrec y antes de llegar
 * levantar el flagActualizarParamSombra, justo en ese momento llega una interrupcion 
 * de este timer de calculo y en esa iteracion se terminaba de llegar a la velocidad 
 * taget. En ese caso se va a pisar ese flagChangingFrec y se va a quedar como si nunca hubiese
 * pasado.
 * Debido a que es un caso muy extremo no se analiza 
 */
static void GestorSVM_Calculoaceleracioneracion(void);

/**
 * @fn static int pinMap(int x)
 * @brief Convierte el XOR de estados (bit U/V/W) al índice interno {0,1,2}.
 * @param x Máscara de pin que cambia (1bit activo entre U/V/W).
 * @return Índice 0→U, 1→V, 2→W.
 */
static int pinMap(int x);

/**
 * @fn static void GestorSVM_CalcularValoresSwitching(void)
 * @brief Calcula t1, t2 y t0 (y casos de interferencia) y los deja en @ref ticksChannel[].
 * @details
 *   - Actualiza ángulos ( @ref anguloActual y @ref anguloParcial) y @ref cuadranteActual.
 *   - t1/t2 por regresión lineal (constantes @ref CONST_CALC_T* y @ref indiceModulacion).
 *   - t0 = (255 - t1 - t2)/2. Luego: ticksC1=t0, ticksC2=t0+t1, ticksC3=t0+t1+t2.
 *   - Clasifica interferencias según @ref MIN_TICKS_DIF y setea @ref casoInterferencia.
 */
static void GestorSVM_CalcularValoresSwitching(void);

/**
 * @fn static void GestorSVM_SwitchInterrupt(SwitchInterruptType intType)
 * @brief Handler interno llamado por el ISR de TIM3 según la fuente (CH1..CH4, RESET, CLEAN).
 * @param intType Fuente de interrupción @ref SwitchInterruptType.
 * @details
 *   - RESET: consume una entrada del buffer y precarga CCR2/CCR3/CCR4.
 *   - CH1/CH2/CH3: conmuta GPIO según orden/fase y maneja interferencias habilitando/deshabilitando CCxIE.
 *   - CLEAN: limpia flags y estado interno del switching.
 */
static void GestorSVM_SwitchInterrupt(SwitchInterruptType intType);

/**
 * @fn static void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado, int numCuadrante)
 * @brief Escribe en BSRR los pines correspondientes a la orden y cuadrante.
 * @param orden Orden de conmutación @ref OrdenSwitch.
 * @param estado 0=primer estado del sector, 1=segundo estado (usa @ref estadoGPIOPorCuadranteYOrden).
 * @param numCuadrante Sector SVM (0..5).
 */
static void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado, int numCuadrante);

/* ================================ Implementación privada ================================ */

static int pinMap(int x) {
    int r = 0;
    x &= 0x07; // limitar a 3 bits (U,V,W)
    if (x == 1) {
		r = 2;   // W
	}
    else if (x == 2) {
		r = 1;   // V
	}
    else if (x == 4) {
		r = 0;   // U
	}
	return r;
}

static void GestorSVM_CalcularValoresSwitching() {
	int t1, t2, t0;
	int ticksC1, ticksC2, ticksC3;

	/* Rampa de velocidad si corresponde */
	if (flagChangingFrecuencia) {
		GestorSVM_Calculoaceleracioneracion();
	}

	/* Avance angular y gestión de diente 0..60° */
	anguloActual += anguloSwitching;
	if (anguloActual >= 360 * 1000 * 1000) {
		anguloActual -= 360 * 1000 * 1000;
		cuadranteActual = 0;
		flagAscensoAnguloParcial = 1;
		anguloParcial = anguloActual;
	} else {
		if (flagAscensoAnguloParcial == 1) {
			anguloParcial += anguloSwitching;
			if (anguloParcial >= 60 * 1000 * 1000) {
				anguloParcial = 2 * 60 * 1000 * 1000 - anguloParcial; // espejo
				flagAscensoAnguloParcial = 0;
				cuadranteActual = (cuadranteActual + 1) % 6;
			}
		} else {
			anguloParcial -= anguloSwitching;
			if (anguloParcial <= 0) {
				anguloParcial = -anguloParcial; // espejo
				flagAscensoAnguloParcial = 1;
				cuadranteActual = (cuadranteActual + 1) % 6;
			}
		}
	}

	/* t1/t2 por regresión e índice de modulación */
	t1 = (int)(((CONST_CALC_T1_PROP * (float)anguloParcial + CONST_CALC_T1_ORD_ORG) * indiceModulacion) / 100);
	t2 = (int)(((CONST_CALC_T2_PROP * (float)anguloParcial + CONST_CALC_T2_ORD_ORG) * indiceModulacion) / 100);
	t0 = (int)((float)(255 - t1 - t2) * 0.5);

	/* Ticks absolutos en el período */
	ticksC1 = t0;
	ticksC2 = t0 + t1;
	ticksC3 = t0 + t1 + t2;

	/* Clasificación de interferencias */
	if (ticksC3 - ticksC1 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T3;
	} else if (ticksC2 - ticksC1 < MIN_TICKS_DIF && ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T2_T2T3;
	} else if (ticksC2 - ticksC1 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T1T2;
	} else if (ticksC3 - ticksC2 < MIN_TICKS_DIF) {
		casoInterferencia = INTERF_T2T3;
	} else {
		casoInterferencia = INTERF_NULA;
	}

	ticksChannel[0] = ticksC1;
	ticksChannel[1] = ticksC2;
	ticksChannel[2] = ticksC3;
}

static void GestorSVM_SwitchInterrupt(SwitchInterruptType intType) {
	volatile static int countUpTx[3];               // 1=subiendo, 0=bajando
	volatile static int flagTxActivo[3];            // espera de evento por canal
	volatile static CasoInterferenciaTimer interferencia;
	volatile static int cuadrante = 0;
	int ticks1, ticks2, ticks3;

	/* Si no hay datos precargados, no hacer nada */
	if (bufferCalculo.contadorDeDatos <= 0) {
		return;
	}

	switch (intType) {
		case SWITCH_INT_CH1:
			if (flagTxActivo[0]) {
				flagTxActivo[0] = 0;
				if (countUpTx[0]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_1_UP, 1, cuadrante);
					countUpTx[0] = 0;
				} else {
					countUpTx[0] = 1;
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_1_DOWN, 0, cuadrante);
					/* Rehabilita interrupciones de los otros canales */
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
				}
			} else {
				flagTxActivo[0] = 1;
			}
			break;

		case SWITCH_INT_CH2:
			if (flagTxActivo[1]) {
				flagTxActivo[1] = 0;
				if (countUpTx[1]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_2_UP, 1, cuadrante);
					countUpTx[1] = 0;
				} else {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_2_DOWN, 0, cuadrante);
					countUpTx[1] = 1;
				}
			} else {
				if (interferencia == INTERF_T1T2_T2T3) {
					flagTxActivo[1] = 0;
					TIM3->DIER &= ~TIM_DIER_CC3IE; // desactiva T2
				} else {
					flagTxActivo[1] = 1;
				}
			}
			break;

		case SWITCH_INT_CH3:
			if (flagTxActivo[2]) {
				flagTxActivo[2] = 0;
				if (countUpTx[2]) {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_3_UP, 1, cuadrante);

					/* Manejo de interferencias */
					switch (interferencia) {
						case INTERF_NULA:
							flagTxActivo[0] = 1;
							flagTxActivo[1] = 1;
							flagTxActivo[2] = 1;
							break;
						case INTERF_T1T2:
							flagTxActivo[0] = 0;  // T1 pendiente
							flagTxActivo[1] = 0;  // desactiva T2
							flagTxActivo[2] = 1;
							// Deshabilitacion T2
							TIM3->DIER &= ~TIM_DIER_CC3IE;
							// Activacion T1
							TIM3->DIER |= TIM_DIER_CC2IE;
							break;
						case INTERF_T2T3:
							flagTxActivo[0] = 1;
							flagTxActivo[1] = 0;  // T2 pendiente
							flagTxActivo[2] = 0;  // T3 off
							TIM3->DIER &= ~TIM_DIER_CC4IE; // CC4 off
							TIM3->DIER |=  TIM_DIER_CC3IE; // CC3 on
							break;
						case INTERF_T1T3:
							/* No debe ocurrir aquí */
							break;
						case INTERF_T1T2_T2T3:
							flagTxActivo[0] = 0;
							flagTxActivo[1] = 0;
							flagTxActivo[2] = 1;
							TIM3->DIER &= ~TIM_DIER_CC3IE; // CC3 off
							TIM3->DIER |=  TIM_DIER_CC2IE; // CC2 on
							break;
					}
					/* Próximas interrupciones: cuenta abajo */
					countUpTx[0] = 0;
					countUpTx[1] = 0;
					countUpTx[2] = 0;
				} else {
					GestorSVM_SwitchPuertos(ORDEN_SWITCH_3_DOWN, 0, cuadrante);
					countUpTx[2] = 1;
				}
			} else {
				if (interferencia == INTERF_T1T3) {
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 0;
					flagTxActivo[2] = 0;
				} else {
					flagTxActivo[2] = 1;
				}
			}
			break;

		case SWITCH_INT_RESET:
			/* Re-sync / precarga */
			countUpTx[0] = 1;
			countUpTx[1] = 1;
			countUpTx[2] = 1;

			/* Consume entrada del buffer */
			ticks1 = bufferCalculo.ticksChannel1[bufferCalculo.indiceLectura];
			ticks2 = bufferCalculo.ticksChannel2[bufferCalculo.indiceLectura];
			ticks3 = bufferCalculo.ticksChannel3[bufferCalculo.indiceLectura];
			interferencia = bufferCalculo.casoInterferencia[bufferCalculo.indiceLectura];
			cuadrante = bufferCalculo.cuadranteActual[bufferCalculo.indiceLectura];
			bufferCalculo.indiceLectura = (bufferCalculo.indiceLectura + 1) % BUFFER_CALCULO_SIZE;
			bufferCalculo.contadorDeDatos--;

			/* Habilitación según interferencias */
			switch (interferencia) {
				case INTERF_NULA:
					flagTxActivo[0] = 1;
					flagTxActivo[1] = 1;
					flagTxActivo[2] = 1;
					TIM3->DIER |= TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T1T2:
					flagTxActivo[0] = 0;
					flagTxActivo[1] = 1;
					flagTxActivo[2] = 1;
					TIM3->DIER &= ~TIM_DIER_CC2IE;
					TIM3->DIER |= TIM_DIER_CC3IE;
					TIM3->DIER |= TIM_DIER_CC4IE;
					break;
				case INTERF_T2T3:
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
					/* Carga posiciones distantes (nunca disparan simultáneo) */
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

			/* Precarga CCRs (t1→CCR2, t2→CCR3, t3→CCR4) */
			TIM3->CCR2 = ticks1;
			TIM3->CCR3 = ticks2;
			TIM3->CCR4 = ticks3;
			break;

		case SWITCH_INT_CLEAN:
			/* Limpieza de estado */
			countUpTx[0] = 1;
			countUpTx[1] = 1;
			countUpTx[2] = 1;
			flagTxActivo[0] = 0;
			flagTxActivo[1] = 0;
			flagTxActivo[2] = 0;
			break;

		default:
			break;
	}
}

static void GestorSVM_SwitchPuertos(OrdenSwitch orden, char estado, int numCuadrante) {
	switch (orden) {
		case ORDEN_SWITCH_1_UP:
		case ORDEN_SWITCH_2_UP:
		case ORDEN_SWITCH_3_DOWN:
		case ORDEN_SWITCH_2_DOWN:
			GPIOA->BSRR = estadoGPIOPorCuadranteYOrden[numCuadrante][estado];
			break;
		case ORDEN_SWITCH_3_UP:
			GPIOA->BSRR = estadoGPIOOn;
			break;
		case ORDEN_SWITCH_1_DOWN:
			GPIOA->BSRR = estadoGPIOOff;
			break;
	}
}

static void GestorSVM_Calculoaceleracioneracion() {
	/* Sincronización con parámetros sombra, si corresponde */
	if (flagActualizarParamSombra) {
		frecObjetivo       = paramSombra.frecObjetivo;
		cambioFrecuenciaPorCiclo = paramSombra.cambioFrecuenciaPorCiclo;
		direccionRotacion  = paramSombra.direccionRotacion;
		flagMotorRunning   = paramSombra.flagMotorRunning;
		flagEsAcelerado    = paramSombra.flagEsAcelerado;
		flagChangingFrecuencia   = paramSombra.flagChangingFrecuencia;
		flagActualizarParamSombra = 0;
	}

	/* Aplicación de rampa */
	if (flagEsAcelerado) {
		frecuenciaSalida += cambioFrecuenciaPorCiclo;
		if (frecuenciaSalida >= frecObjetivo) {
			frecuenciaSalida = frecObjetivo;
			GestorEstados_Action(ACTION_TO_CONST_RUNNING, 0);
			flagChangingFrecuencia = 0;
		}
	} else {
		frecuenciaSalida -= cambioFrecuenciaPorCiclo;
		if (frecuenciaSalida <= frecObjetivo) {
			frecuenciaSalida = frecObjetivo;

			if (frecObjetivo == 0) {
				flagMotorRunning = 0;
				GestorTimers_DetenerTimerSVM();
				GestorSVM_SwitchInterrupt(SWITCH_INT_CLEAN);

				/* Limpia buffer productor/consumidor */
				bufferCalculo.indiceEscritura = 0;
				bufferCalculo.indiceLectura   = 0;
				bufferCalculo.contadorDeDatos     = 0;

				/* Apaga salidas y drivers */
				HAL_GPIO_WritePin(GPIOA, GPIO_U_IN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_V_IN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_W_IN, GPIO_PIN_RESET);

				HAL_GPIO_WritePin(GPIOA, GPIO_U_SD, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_V_SD, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, GPIO_W_SD, GPIO_PIN_RESET);

				/* Notifica detención */
				GestorEstados_Action(ACTION_MOTOR_STOPPED, 0);
			} else {
				GestorEstados_Action(ACTION_TO_CONST_RUNNING, 0);
			}
			flagChangingFrecuencia = 0;
		}
	}

	/* Índice de modulación (V/f) */
	if (frecuenciaSalida < 50 * 1000 * 1000) {
		indiceModulacion = frecuenciaSalida / (500 * 1000); // lineal hasta 50 Hz
	} else {
		indiceModulacion = 100;
	}
	if (indiceModulacion < 1) {
		indiceModulacion = 1;
	}

	/* Incremento angular por switching (cuidar orden para evitar overflow) */
	anguloSwitching = (frecuenciaSalida / frecuenciaSwitching) * 360;
}

/**
 * @fn void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion)
 * @brief Carga la configuración base de SVM y prepara tablas de BSRR por cuadrante.
 * @param configuracion Puntero a @ref ConfiguracionSVM.
 */
void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion) {
	int i;
	/* Dinámicos */
	frecuenciaSwitching       = configuracion->frecuenciaSwitching;
	direccionRotacion = configuracion->direccionRotacion;
	aceleracion              = configuracion->aceleracion;
	desaceleracion           = configuracion->desaceleracion;

	/* Referencia (inicia como target) */
	GestorSVM_SetFrec(configuracion->frecuenciaReferenica);

	printf("Configuracion Seteada \n");
	int estado0, estado1, estado2, estado3;
	int pinToggle1, pinToggle2, pinToggle3;
	uint32_t myBSRR;

	estadoGPIOOff = (GPIO_U_IN | GPIO_V_IN | GPIO_W_IN) << 16;
	estadoGPIOOn  = (GPIO_U_IN | GPIO_V_IN | GPIO_W_IN);

	for (i = 0; i < 6; i++) {
		estado0 = vectorSecuenciaPorCuadrante[i][0];
		estado1 = vectorSecuenciaPorCuadrante[i][1];
		estado2 = vectorSecuenciaPorCuadrante[i][2];
		estado3 = vectorSecuenciaPorCuadrante[i][3];

		/* Estado 0→1 */
		myBSRR = 0;
		myBSRR |= ((estado1 & 0b100) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado1 & 0b010) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado1 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);
		estadoGPIOPorCuadranteYOrden[i][0] = myBSRR;

		/* Estado 1→2 */
		myBSRR = 0;
		myBSRR |= ((estado2 & 0b100) > 0) ? puerto_senal_pierna[0] : (puerto_senal_pierna[0] << 16);
		myBSRR |= ((estado2 & 0b010) > 0) ? puerto_senal_pierna[1] : (puerto_senal_pierna[1] << 16);
		myBSRR |= ((estado2 & 0b001) > 0) ? puerto_senal_pierna[2] : (puerto_senal_pierna[2] << 16);
		estadoGPIOPorCuadranteYOrden[i][1] = myBSRR;

		/* Pines que conmutan entre estados (XOR) */
		pinToggle1 = pinMap(estado0 ^ estado1);
		pinToggle2 = pinMap(estado1 ^ estado2);
		pinToggle3 = pinMap(estado2 ^ estado3);

		/* Mapa a U/V/W con desplazamiento */
		pinTogglePorCuadranteYOrden[i][0] = GPIO_U_IN << (pinToggle1 * 2);
		pinTogglePorCuadranteYOrden[i][1] = GPIO_U_IN << (pinToggle2 * 2);
		pinTogglePorCuadranteYOrden[i][2] = GPIO_U_IN << (pinToggle3 * 2);
	}
}

/**
 * @fn void GestorSVM_CalcInterrupt(void)
 * @brief Handler llamado por el timer de cálculo (productor). Encola t1/t2/t3 si hay espacio.
 */
void GestorSVM_CalcInterrupt() {
	int indiceEscritura;

	if (bufferCalculo.contadorDeDatos < BUFFER_CALCULO_SIZE) {
		indiceEscritura = bufferCalculo.indiceEscritura;

		GestorSVM_CalcularValoresSwitching();

		/* Encolar resultados */
		bufferCalculo.ticksChannel1[indiceEscritura] = ticksChannel[0];
		bufferCalculo.ticksChannel2[indiceEscritura] = ticksChannel[1];
		bufferCalculo.ticksChannel3[indiceEscritura] = ticksChannel[2];
		bufferCalculo.casoInterferencia[indiceEscritura] = casoInterferencia;
		bufferCalculo.cuadranteActual[indiceEscritura]   = cuadranteActual;

		bufferCalculo.indiceEscritura = (indiceEscritura + 1) % BUFFER_CALCULO_SIZE;
		bufferCalculo.contadorDeDatos++;
	}
}

/* ------------------------ API invocada por el Gestor de Estados (SPI) ------------------------ */

/**
 * @fn int GestorSVM_SetFrec(int frec)
 * @brief Solicita nueva frecuencia objetivo (Hz). Inicia rampa si el motor está en marcha.
 * @param frec Frecuencia objetivo [Hz].
 * @return 0 si aceptada (motor detenido), 1 si aceptada con rampa en curso; -1 fuera de rango; -2 misma que la actual.
 */
int GestorSVM_SetFrec(int frec) {
	int flagEsAcelerado_local;
	int32_t cambioFrecuenciaPorCiclo_local;
	int32_t frecTarget_local;
	int32_t nuevaFrec;

	if (frec < FERC_OUT_MIN || frec > FERC_OUT_MAX) {
		return -1;
	}
	nuevaFrec = (int32_t)frec * 1000 * 1000;
	if (frecuenciaSalida == nuevaFrec) {
		return -2;
	}

	if (flagMotorRunning) {
		/* Determinar sentido de la rampa */
		if (frecuenciaSalida < nuevaFrec) {
			flagEsAcelerado_local = 1;
			cambioFrecuenciaPorCiclo_local = (aceleracion * 1000 * 1000) / (frecuenciaSwitching);
		} else {
			flagEsAcelerado_local = 0;
			cambioFrecuenciaPorCiclo_local = (desaceleracion * 1000 * 1000) / (frecuenciaSwitching);
		}

		frecTarget_local = nuevaFrec;
		frecuenciaReferenica = frec;

		/* Parámetros sombra */
		paramSombra.flagMotorRunning   = 1;
		paramSombra.flagChangingFrecuencia   = 1;
		paramSombra.flagEsAcelerado    = flagEsAcelerado_local;
		paramSombra.cambioFrecuenciaPorCiclo = cambioFrecuenciaPorCiclo_local;
		paramSombra.frecObjetivo       = frecTarget_local;
		flagActualizarParamSombra = 1;

		flagChangingFrecuencia = 1;
		return 1;
	} else {
		frecuenciaReferenica = frec;
		return 0;
	}
}

/**
 * @fn int GestorSVM_SetDir(int dir)
 * @brief Cambia el sentido de giro si el motor está detenido.
 * @param dir 0 o 1 (mapeo a antihorario/horario).
 * @return 0 OK; -1 fuera de rango; -2 si motor en marcha o rampa activa.
 */
int GestorSVM_SetDir(int dir) {
	int estado1, estado2;
	int i;
	uint32_t myBSRR;

	if (flagMotorRunning) {
		return -2;
	}
	if (flagChangingFrecuencia) {
		return -2;
	}
	if (dir != 0 && dir != 1) {
		return -1;
	}
	if (dir == direccionRotacion) {
		return 0;
	}

	/* Recalcular tablas BSRR intercambiando U↔V */
	for (i = 0; i < 6; i++) {
		estado1 = vectorSecuenciaPorCuadrante[i][1];
		estado2 = vectorSecuenciaPorCuadrante[i][2];

		myBSRR = 0;
		myBSRR |= ((estado1 & 0b100) ? GPIO_V_IN : (GPIO_V_IN << 16));
		myBSRR |= ((estado1 & 0b010) ? GPIO_U_IN : (GPIO_U_IN << 16));
		myBSRR |= ((estado1 & 0b001) ? GPIO_W_IN : (GPIO_W_IN << 16));
		estadoGPIOPorCuadranteYOrden[i][0] = myBSRR;

		myBSRR = 0;
		myBSRR |= ((estado2 & 0b100) ? GPIO_V_IN : (GPIO_V_IN << 16));
		myBSRR |= ((estado2 & 0b010) ? GPIO_U_IN : (GPIO_U_IN << 16));
		myBSRR |= ((estado2 & 0b001) ? GPIO_W_IN : (GPIO_W_IN << 16));
		estadoGPIOPorCuadranteYOrden[i][1] = myBSRR;
	}

	direccionRotacion = dir;
	return 0;
}

/**
 * @fn int GestorSVM_Setaceleracion(int nuevaaceleracion)
 * @brief Actualiza acelerada [Hz/s].
 * @return 0 OK; -1 fuera de rango; -2 si hay rampa activa.
 */
int GestorSVM_Setaceleracion(int nuevaaceleracion) {
	if (flagChangingFrecuencia) {
		return -2;
	}
	if (nuevaaceleracion < ACELERACION_MINIMA || nuevaaceleracion > ACCLERACION_MAXIMA) {
		return -1;
	}
	aceleracion = nuevaaceleracion;
	return 0;
}

/**
 * @fn int GestorSVM_SetDecel(int nuevaDecel)
 * @brief Actualiza desacelerada [Hz/s].
 * @return 0 OK; -1 fuera de rango; -2 si hay rampa activa.
 */
int GestorSVM_SetDecel(int nuevaDecel) {
	if (flagChangingFrecuencia) {
		return -2;
	}
	if (nuevaDecel < DESACELERACION_MINIMA || nuevaDecel > DESACELERACION_MAXIMA) {
		return -1;
	}
	desaceleracion = nuevaDecel;
	return 0;
}

/**
 * @fn int GestorSVM_MotorStart(void)
 * @brief Arranca el motor: habilita drivers, inicia timers y rampa hacia @ref frecuenciaReferenica.
 * @return 0 si arranca; 1 si ya estaba en marcha.
 */
int GestorSVM_MotorStart() {
	if (!flagMotorRunning) {
		flagMotorRunning = 1;
		flagEsAcelerado = 1;
		flagChangingFrecuencia = 1;

		cambioFrecuenciaPorCiclo = (aceleracion * 1000 * 1000) / (frecuenciaSwitching);

		/* Parámetros sombra de arranque */
		paramSombra.frecObjetivo = (uint32_t)frecuenciaReferenica * 1000 * 1000;
		paramSombra.flagMotorRunning = 1;
		paramSombra.flagEsAcelerado = 1;
		paramSombra.flagChangingFrecuencia = 1;
		paramSombra.cambioFrecuenciaPorCiclo = cambioFrecuenciaPorCiclo;
		flagActualizarParamSombra = 1;

		/* Habilitar drivers */
		HAL_GPIO_WritePin(GPIOA, GPIO_U_SD, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_V_SD, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_W_SD, GPIO_PIN_SET);

		/* Precargar una muestra y sincronizar switching */
		GestorSVM_CalcInterrupt();
		GestorSVM_SwitchInterrupt(SWITCH_INT_RESET);

		/* Iniciar timer de switching */
		GestorTimers_IniciarTimerSVM();
		return 0;
	}
	return 1;
}

/**
 * @fn int GestorSVM_MotorStop(void)
 * @brief Ordena frenado con rampa de @ref desaceleracion hasta 0 Hz.
 * @return 0 si acepta; 1 si ya estaba detenido.
 */
int GestorSVM_MotorStop() {
	if (flagMotorRunning) {
		flagEsAcelerado = 0;
		flagChangingFrecuencia = 1;

		cambioFrecuenciaPorCiclo = (desaceleracion * 1000 * 1000) / (frecuenciaSwitching);
		frecObjetivo = 0;

		/* Parámetros sombra */
		paramSombra.flagMotorRunning = 1;
		paramSombra.flagEsAcelerado = 0;
		paramSombra.flagChangingFrecuencia = 1;
		paramSombra.cambioFrecuenciaPorCiclo = cambioFrecuenciaPorCiclo;
		paramSombra.frecObjetivo = 0;
		flagActualizarParamSombra = 1;

		return 0;
	}
	return 1;
}

/**
 * @fn int GestorSVM_Estop(void)
 * @brief Parada de emergencia inmediata: desactiva timers, apaga drivers y salidas.
 * @return Siempre 0.
 */
int GestorSVM_Estop() {
	if (flagMotorRunning) {
		GestorTimers_DetenerTimerSVM();

		HAL_GPIO_WritePin(GPIOA, GPIO_U_SD, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_V_SD, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_W_SD, GPIO_PIN_RESET);

		GestorSVM_SwitchInterrupt(SWITCH_INT_CLEAN);

		bufferCalculo.indiceEscritura = 0;
		bufferCalculo.indiceLectura   = 0;
		bufferCalculo.contadorDeDatos     = 0;

		HAL_GPIO_WritePin(GPIOA, GPIO_U_IN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_V_IN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_W_IN, GPIO_PIN_RESET);

		flagChangingFrecuencia = 0;
		flagMotorRunning = 0;
		frecuenciaSalida = 0;
	}
	return 0;
}

/**
 * @fn int GestorSVM_GetFrec(void)
 * @brief Devuelve la frecuencia de referencia configurada (Hz).
 * @return @ref frecuenciaReferenica.
 */
int GestorSVM_GetFrec() {
	return frecuenciaReferenica;
}

/** @brief Devuelve acelerada actual [Hz/s]. */
int GestorSVM_Getaceleracion() {
	return aceleracion; 
}

/** @brief Devuelve desacelerada actual [Hz/s]. */
int GestorSVM_GetDesaceleracion() {
	return desaceleracion; 
}

/** @brief Devuelve sentido de giro (1 horario, -1 antihorario). */
int GestorSVM_GetDir() {
	return direccionRotacion; 
}

/* ================================ ISR de TIM3 (HAL) ================================ */

/**
 * @fn void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
 * @brief Callback HAL de Output Compare: enruta eventos CCR de TIM3 a @ref GestorSVM_SwitchInterrupt.
 * @param htim Puntero al handle del timer que disparó el evento.
 * @details
 *   - CH1 → @ref SWITCH_INT_RESET
 *   - CH2 → @ref SWITCH_INT_CH1
 *   - CH3 → @ref SWITCH_INT_CH2
 *   - CH4 → @ref SWITCH_INT_CH3
 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance != TIM3) {
		return;
	}

	switch (htim->Channel) {
		case HAL_TIM_ACTIVE_CHANNEL_1:
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
