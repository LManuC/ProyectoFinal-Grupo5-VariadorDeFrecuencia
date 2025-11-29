#ifndef GESTOR_SVM_GESTORSVM_H_
#define GESTOR_SVM_GESTORSVM_H_

/**
 * @struct ValoresSwitching
 * @brief Estructura de trabajo del ISR del timer de switching.
 * @details
 *   Contiene los parámetros que determinan en qué puntos (ticks) se disparan
 *   las comparaciones CCR para conmutar las fases y el estado de conmutación
 *   (encendido/apagado) de cada pierna. El campo @ref cuadrante indica el
 *   sector SVM actual (0..5).
 */
typedef struct ValoresSwitching {
    char cuadrante;          /// Sector SVM actual (0..5). Lo usa el ISR para decidir qué pierna conmutar.
    char flagSwitch[3];      /// Flags de acción por pierna {U,V,W}: 1=encender, 0=apagar (par de valores por flanco).
    int  ticks1[2];          /// Ticks CCR del evento 1 (subida/bajada).
    int  ticks2[2];          /// Ticks CCR del evento 2 (subida/bajada).
    int  ticks3[2];          /// Ticks CCR del evento 3 (subida/bajada).
    int  contador;           /// Contador auxiliar para depuración/estadística del ISR.
} ValoresSwitching; 

/**
 * @struct ConfiguracionSVM
 * @brief Parámetros de configuración del módulo SVM.
 * @details
 *   - @ref frec_switch: frecuencia del timer de switching (Hz).
 *   - @ref frecReferencia: frecuencia objetivo de marcha estable (Hz).
 *   - @ref direccionRotacion: 1 horario, -1 antihorario.
 *   - @ref acel / @ref desacel: rampas dinámicas [Hz/seg].
 */
typedef struct ConfiguracionSVM {
    int frec_switch;         /// Frecuencia de switching del PWM SVM [Hz].
    int frecReferencia;      /// Frecuencia de referencia (target) en régimen [Hz].
    int direccionRotacion;   /// 1 = sentido horario, -1 = antihorario.
    int acel;                /// Aceleración dinámica [Hz/seg].
    int desacel;             /// Desaceleración dinámica [Hz/seg].
} ConfiguracionSVM;

/**
 * @enum OrdenSwitch
 * @brief Ordenes simbólicas de conmutación por canal/flujo (UP/DOWN).
 * @details
 *   Mapear a eventos CCR (t1/t2/t3) y flancos (subida/bajada) facilita
 *   el código del ISR. El orden debe coincidir con la secuencia SVM configurada.
 */
typedef enum {
    ORDEN_SWITCH_1_UP   = 0, /** Evento t1 en flanco de subida.  */
    ORDEN_SWITCH_2_UP   = 1, /** Evento t2 en flanco de subida.  */
    ORDEN_SWITCH_3_UP   = 2, /** Evento t3 en flanco de subida.  */
    ORDEN_SWITCH_3_DOWN = 3, /** Evento t3 en flanco de bajada.  */
    ORDEN_SWITCH_2_DOWN = 4, /** Evento t2 en flanco de bajada.  */
    ORDEN_SWITCH_1_DOWN = 5, /** Evento t1 en flanco de bajada.  */
} OrdenSwitch;

/**
 * @enum SwitchInterruptType
 * @brief Fuentes de interrupción de switching usadas por el ISR.
 * @details
 *   - CH1..CH3: comparaciones CCR activas.
 *   - RESET: sincroniza y precarga nuevos ticks (cambio de sector).
 *   - CLEAN: limpia flags/estado (p.ej. al detener motor).
 */
typedef enum {
    SWITCH_INT_CH1 = 0,  /// Interrupción por CCR de t1.
    SWITCH_INT_CH2,      /// Interrupción por CCR de t2.
    SWITCH_INT_CH3,      /// Interrupción por CCR de t3.
    SWITCH_INT_RESET,    /// Re-armado de ticks/sincronización por ciclo.
    SWITCH_INT_CLEAN,    /// Limpieza de estado/flags de switching.
} SwitchInterruptType;

/* -------------------- Parámetros de operación por defecto / límites -------------------- */

#define FREC_SWITCH                     2511        /// Frecuencia por defecto del switching SVM [Hz].
#define FREC_REFERENCIA                 50          /// Frecuencia de referencia por defecto [Hz].
#define DIRECCION_ROTACION              1           /// Sentido de giro por defecto: horario.
#define RESOLUCION_TIMER                255         /// Resolución (ARR efectivo ~ 255 → 8 bits).
#define ACCELERACION_DEFAUL             5           /// Aceleración por defecto [Hz/seg].

#define FERC_OUT_MIN                    1           /// Frecuencia mínima permitida [Hz].
#define FERC_OUT_MAX                    150         /// Frecuencia máxima permitida [Hz].
#define DESACELERACION_DEFAULT          3           /// Desaceleración por defecto [Hz/seg].
#define ACCLERACION_MAXIMA              50          /// Aceleración máxima permitida [Hz/seg].
#define ACELERACION_MINIMA              1           /// Aceleración mínima permitida [Hz/seg].
#define DESACELERACION_MAXIMA           50          /// Desaceleración máxima permitida [Hz/seg].
#define DESACELERACION_MINIMA           1           /// Desaceleración mínima permitida [Hz/seg].

/**
 * @fn void GestorSVM_Init(void)
 * @brief Inicializa el gestor SVM y su estado interno.
 * @details
 *   Pone a cero variables internas, limpia buffers de cálculo y deja
 *   flags en estado consistente (motor detenido, sin cambios pendientes).
 */
void GestorSVM_Init();

/**
 * @fn void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion)
 * @brief Carga la configuración base del SVM.
 * @param configuracion Puntero a @ref ConfiguracionSVM con @ref frec_switch, @ref frecReferencia,
 *        @ref direccionRotacion, @ref acel y @ref desacel.
 * @details
 *   Aplica @ref frec_switch, rampas y sentido; y propaga @ref frecReferencia al
 *   pipeline de cambio de velocidad. Internamente recalcula lookups/BSRR por cuadrante.
 * @warning Validar rangos antes de invocar si la config proviene de UI/externo.
 */
void GestorSVM_SetConfiguration(ConfiguracionSVM* configuracion);

/* -------------------- API de control en tiempo de ejecución -------------------- */

/**
 * @fn int GestorSVM_MotorStart(void)
 * @brief Inicia la marcha: arranca rampa de aceleración hacia @ref frecReferencia.
 * @return 0 si inicia correctamente, 1 si ya estaba en marcha.
 * @details
 *   Habilita drivers, inicia timers (cálculo y switching), precarga ticks,
 *   y pone flags para rampa ascendente. La frecuencia objetivo proviene de
 *   @ref GestorSVM_SetFrec o @ref GestorSVM_SetConfiguration.
 */
int GestorSVM_MotorStart();

/**
 * @fn int GestorSVM_MotorStop(void)
 * @brief Ordena frenado controlado (rampa de @ref desacel hasta 0 Hz).
 * @return 0 si se acepta la orden, 1 si ya estaba detenido.
 * @details
 *   No corta drivers instantáneamente: mantiene el switching hasta llegar a 0 Hz,
 *   luego apaga salidas y notifica al gestor de estados.
 */
int GestorSVM_MotorStop();

/**
 * @fn int GestorSVM_Estop(void)
 * @brief Parada de emergencia inmediata.
 * @return Siempre 0.
 * @details
 *   Deshabilita timers/interrupts, apaga drivers y salidas sin rampa.
 * @warning Uso ante condiciones de falla. Requiere nueva orden de arranque.
 */
int GestorSVM_Estop();

/**
 * @fn int GestorSVM_SetFrec(int frec)
 * @brief Solicita una nueva frecuencia objetivo.
 * @param frec Frecuencia objetivo [Hz].
 * @return
 *   -  0: Aceptada con motor detenido (queda como referencia).
 *   -  1: Aceptada con motor en marcha (inicia rampa).
 *   - -1: Fuera de rango (@ref FERC_OUT_MIN..@ref FERC_OUT_MAX).
 *   - -2: Igual a la frecuencia actual (sin cambios).
 * @details
 *   Calcula delta por ciclo a partir de @ref acel/@ref desacel y @ref frec_switch,
 *   y actualiza parámetros sombra para el lazo de cálculo.
 */
int GestorSVM_SetFrec(int frec);

/**
 * @fn int GestorSVM_SetDir(int dir)
 * @brief Cambia el sentido de giro (solo con motor detenido).
 * @param dir 0 o 1 (mapea a antihorario/horario según tu implementación).
 * @return
 *   -  0: Modificado.
 *   - -1: Fuera de rango.
 *   - -2: Rechazado (motor en marcha o cambio en curso).
 * @details
 *   Recalcula la tabla BSRR por cuadrante para invertir U/V.
 */
int GestorSVM_SetDir(int dir);

/**
 * @fn int GestorSVM_SetAcel(int acel)
 * @brief Actualiza la aceleración dinámica [Hz/seg].
 * @param acel Nueva aceleración.
 * @return 0 OK; -1 fuera de rango; -2 si hay cambio de velocidad en curso.
 */
int GestorSVM_SetAcel(int acel);

/**
 * @fn int GestorSVM_SetDecel(int decel)
 * @brief Actualiza la desaceleración dinámica [Hz/seg].
 * @param decel Nueva desaceleración.
 * @return 0 OK; -1 fuera de rango; -2 si hay cambio de velocidad en curso.
 */
int GestorSVM_SetDecel(int decel);

/**
 * @fn int GestorSVM_GetFrec(void)
 * @brief Lee la frecuencia objetivo de referencia (no escalada).
 * @return Frecuencia de referencia [Hz].
 * @note Si querés la frecuencia *instantánea* en rampa, podrías exponer otra API.
 */
int GestorSVM_GetFrec();

/**
 * @fn int GestorSVM_GetAcel();
 * 
 * @brief Obtiene la aceleración actual [Hz/seg].
 */
int GestorSVM_GetAcel();

/** 
 * @fn int GestorSVM_GetDesacel();
 * @brief Obtiene la desaceleración actual [Hz/seg]. 
 */
int GestorSVM_GetDesacel();

/**
 * @fn int GestorSVM_GetDir();
 * 
 * @brief Obtiene el sentido de giro actual (1 horario, -1 antihorario). 
 */
int GestorSVM_GetDir();

/**
 * @fn void GestorSVM_CalcInterrupt(void)
 * @brief ISR (o handler llamado por ISR) del timer de cálculo.
 * @details
 *   Corre a 2× @ref FREC_SWITCH (según tu diseño) para anticipar cómputos:
 *   si el buffer circular tiene espacio, calcula t1/t2/t3, cuadrante y
 *   posibles interferencias, y los encola para que el timer de switching
 *   (TIM3) los consuma. No bloquea si el buffer está lleno.
 * @note Tamaño de buffer: 3 entradas (pipeline productor/consumidor).
 */
void GestorSVM_CalcInterrupt();

#endif /* GESTOR_SVM_GESTORSVM_H_ */
