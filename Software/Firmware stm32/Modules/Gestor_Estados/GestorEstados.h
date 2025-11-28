/*
 * GestorEstados.h
 *
 *  Created on: Aug 10, 2025
 *      Author: elian
 */

#ifndef MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_
#define MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_

/**
 * @enum SystemState
 * @brief Estados de la máquina de estados del variador.
 * @details
 * Representan el ciclo de vida del sistema y condicionan qué acciones
 * (@ref SystemAction) son válidas y qué respuestas (@ref SystemActionResponse)
 * devuelve `GestorEstados_Action()`. Las transiciones típicas son:
 * - @ref STATE_INIT → ( @ref ACTION_INIT_DONE ) → @ref STATE_IDLE
 * - @ref STATE_IDLE → ( @ref ACTION_START ) → @ref STATE_VEL_CHANGE
 * - @ref STATE_VEL_CHANGE → ( @ref ACTION_TO_CONST_RUNNING ) → @ref STATE_RUNNING
 * - @ref STATE_RUNNING → ( @ref ACTION_STOP ) → @ref STATE_BRAKING
 * - @ref STATE_BRAKING → ( @ref ACTION_MOTOR_STOPPED ) → @ref STATE_IDLE
 * - Cualquier estado → ( @ref ACTION_EMERGENCY ) → @ref STATE_EMERGENCY
 */
typedef enum {
    STATE_INIT = 0,   /** Sistema en inicialización. Solo es válida
                           @ref ACTION_INIT_DONE para pasar a @ref STATE_IDLE. */

    STATE_IDLE,       /** Reposo: motor detenido (o salida de emergencia).
                           Admite configuración (@ref ACTION_SET_FREC,
                           @ref ACTION_SET_ACEL, @ref ACTION_SET_DESACEL,
                           @ref ACTION_SET_DIR) y arranque con
                           @ref ACTION_START. */

    STATE_RUNNING,    /** Marcha a velocidad constante. Acepta cambio de
                           parámetros permitidos y @ref ACTION_STOP para
                           transicionar a @ref STATE_BRAKING. */

    STATE_VEL_CHANGE, /** Transitorio de cambio de velocidad (acel./desacel.).
                           Al alcanzar régimen: @ref ACTION_TO_CONST_RUNNING
                           → @ref STATE_RUNNING. @ref ACTION_STOP lleva a
                           @ref STATE_BRAKING. */

    STATE_BRAKING,    /** Frenado en curso. Al detenerse: 
                           @ref ACTION_MOTOR_STOPPED → @ref STATE_IDLE.
                           @ref ACTION_TO_IDLE fuerza el paso a inactivo. */

    STATE_EMERGENCY   /** Emergencia activa (salidas deshabilitadas). Solo
                           acciones de recuperación/acondicionamiento; p.ej.
                           @ref ACTION_STOP puede llevar a @ref STATE_IDLE
                           según la lógica de seguridad. */
} SystemState;


/**
 * @enum SystemAction
 * @brief Acciones externas/internas que gobiernan la máquina de estados del VFD.
 * @details
 * Cada acción puede provocar un cambio de estado (@ref SystemState) y devuelve
 * un @ref SystemActionResponse según las reglas implementadas en GestorEstados_Action().
 * Las referencias a estados (STATE_*) y respuestas (ACTION_RESP_*) indican
 * el flujo esperado y los códigos de retorno típicos.
 */
typedef enum {
    ACTION_NONE = 0, /**< Sin uso. No debe enviarse. */

    /** 
     * @brief Fin de inicialización.
     * @details Válida **solo** si `currentState==@ref STATE_INIT`: pasa a
     * @ref STATE_IDLE y responde @ref ACTION_RESP_OK; en cualquier otro estado
     * responde @ref ACTION_RESP_ERR.
     */
    ACTION_INIT_DONE = 1,

    /**
     * @brief Forzar estado inactivo.
     * @details Válida cuando `currentState==@ref STATE_BRAKING`: pasa a
     * @ref STATE_IDLE y responde @ref ACTION_RESP_OK; si no está frenando,
     * responde @ref ACTION_RESP_ERR.
     */
    ACTION_TO_IDLE,

    /**
     * @brief Solicitud de arranque.
     * @details Si `currentState==@ref STATE_IDLE` llama a `GestorSVM_MotorStart()`,
     * pasa a @ref STATE_VEL_CHANGE y responde @ref ACTION_RESP_OK.
     * Si está en @ref STATE_RUNNING, @ref STATE_VEL_CHANGE o @ref STATE_BRAKING
     * responde @ref ACTION_RESP_MOVING. En @ref STATE_EMERGENCY responde
     * @ref ACTION_RESP_EMERGENCY_ACTIVE. En cualquier otro caso @ref ACTION_RESP_ERR.
     */
    ACTION_START,

    /**
     * @brief Solicitud de parada.
     * @details Si está en @ref STATE_RUNNING o @ref STATE_VEL_CHANGE, llama a
     * `GestorSVM_MotorStop()`, pasa a @ref STATE_BRAKING y responde @ref ACTION_RESP_OK.
     * Si está en @ref STATE_EMERGENCY, ejecuta `GestorSVM_Estop()`, pasa a
     * @ref STATE_IDLE y responde @ref ACTION_RESP_OK. Si ya está en
     * @ref STATE_IDLE responde @ref ACTION_RESP_NOT_MOVING. Otro caso: @ref ACTION_RESP_ERR.
     */
    ACTION_STOP,

    /**
     * @brief Confirmación de motor detenido.
     * @details Válida durante @ref STATE_BRAKING: pasa a @ref STATE_IDLE y
     * responde @ref ACTION_RESP_OK; si no está frenando responde @ref ACTION_RESP_ERR.
     */
    ACTION_MOTOR_STOPPED,

    /**
     * @brief Activación de emergencia.
     * @details Si no está en @ref STATE_EMERGENCY, ejecuta `GestorSVM_Estop()`,
     * pasa a @ref STATE_EMERGENCY y responde @ref ACTION_RESP_OK; si ya estaba
     * en emergencia responde @ref ACTION_RESP_ERR.
     */
    ACTION_EMERGENCY,

    /**
     * @brief Fijar frecuencia de régimen.
     * @details Permitido en @ref STATE_IDLE, @ref STATE_RUNNING o
     * @ref STATE_VEL_CHANGE. Llama a `GestorSVM_SetFrec(value)`:
     * - 0 o -2 → @ref ACTION_RESP_OK (permanece en el estado actual)
     * - 1      → pasa a @ref STATE_VEL_CHANGE y @ref ACTION_RESP_OK
     * - -1     → @ref ACTION_RESP_OUT_RANGE
     * Si está en @ref STATE_BRAKING → @ref ACTION_RESP_MOVING; otro estado → @ref ACTION_RESP_ERR.
     */
    ACTION_SET_FREC,

    /**
     * @brief Configurar aceleración.
     * @details Permitido en @ref STATE_IDLE o @ref STATE_RUNNING. Llama a
     * `GestorSVM_SetAcel(value)`:
     * - 0  → @ref ACTION_RESP_OK
     * - -1 → @ref ACTION_RESP_OUT_RANGE
     * - -2 u otro → @ref ACTION_RESP_ERR
     * En otros estados → @ref ACTION_RESP_ERR.
     */
    ACTION_SET_ACEL,

    /**
     * @brief Configurar desaceleración.
     * @details Igual a @ref ACTION_SET_ACEL pero llamando `GestorSVM_SetDecel(value)`.
     * Respuestas: @ref ACTION_RESP_OK, @ref ACTION_RESP_OUT_RANGE o @ref ACTION_RESP_ERR.
     * En otros estados → @ref ACTION_RESP_ERR.
     */
    ACTION_SET_DESACEL,

    /**
     * @brief Configurar dirección de giro.
     * @details Permitido solo en @ref STATE_IDLE. Llama a `GestorSVM_SetDir(value)`:
     * - 0  → @ref ACTION_RESP_OK
     * - -1 → @ref ACTION_RESP_OUT_RANGE
     * - -2 → @ref ACTION_RESP_MOVING
     * - otro → @ref ACTION_RESP_ERR
     * Si no está en IDLE → @ref ACTION_RESP_MOVING.
     */
    ACTION_SET_DIR,

    /**
     * @brief Transición a régimen constante.
     * @details Válida cuando `currentState==@ref STATE_VEL_CHANGE`: pasa a
     * @ref STATE_RUNNING y responde @ref ACTION_RESP_OK; caso contrario
     * @ref ACTION_RESP_ERR.
     */
    ACTION_TO_CONST_RUNNING,

    /**
     * @brief Consulta de frecuencia actual.
     * @details Acción de lectura (sin cambio de estado). La entrega del valor
     * se realiza por la capa que invoca a @ref GestorEstados_Action() (no hay
     * respuesta numérica directa en este switch).
     */
    ACTION_GET_FREC,

    /**
     * @brief Consulta de aceleración actual.
     * @details Acción de lectura (sin cambio de estado). La obtención del valor
     * queda a cargo de la capa llamadora.
     */
    ACTION_GET_ACEL,

    /**
     * @brief Consulta de desaceleración actual.
     * @details Acción de lectura (sin cambio de estado). La obtención del valor
     * queda a cargo de la capa llamadora.
     */
    ACTION_GET_DESACEL,

    /**
     * @brief Consulta de dirección actual.
     * @details Acción de lectura (sin cambio de estado). La obtención del valor
     * queda a cargo de la capa llamadora.
     */
    ACTION_GET_DIR,

    /**
     * @brief ¿El motor está detenido?
     * @details Si `currentState==@ref STATE_IDLE` o @ref STATE_EMERGENCY →
     * @ref ACTION_RESP_NOT_MOVING; en otro estado → @ref ACTION_RESP_MOVING.
     */
    ACTION_IS_MOTOR_STOP,
} SystemAction;

/**
 * @enum SystemActionResponse
 * @brief Códigos de respuesta del gestor de estados ante una acción recibida.
 * @details
 * Los valores indican el resultado de `GestorEstados_Action()` según la
 * acción solicitada (@ref SystemAction) y el estado actual (@ref SystemState).
 * Se utilizan también como payload de respuesta en el enlace SPI con el ESP32.
 */
typedef enum {
    ACTION_RESP_OK = 0,              /** La acción fue válida y se ejecutó correctamente.
                                           Si correspondía, la transición de estado se realizó. */

    ACTION_RESP_ERR = 1,             /** Error genérico: secuencia inválida para el estado
                                           actual o fallo al aplicar la configuración. No hay
                                           garantía de cambio de estado. */

    ACTION_RESP_MOVING = 2,          /** El motor está en movimiento (p. ej., en
                                           @ref STATE_RUNNING, @ref STATE_VEL_CHANGE o
                                           @ref STATE_BRAKING). Puede usarse como respuesta a
                                           una consulta o para rechazar acciones no permitidas
                                           mientras hay movimiento. */

    ACTION_RESP_NOT_MOVING = 3,      /** El motor está detenido (p. ej., en
                                           @ref STATE_IDLE o @ref STATE_EMERGENCY). Puede ser
                                           respuesta a consulta o a un STOP redundante. */

    ACTION_RESP_OUT_RANGE = 4,       /** El parámetro de configuración recibido (frecuencia,
                                           aceleración, etc.) está fuera de los límites
                                           admitidos por la lógica (p. ej., `GestorSVM_*`). */

    ACTION_RESP_EMERGENCY_ACTIVE = 5 /** Se solicitó una acción no permitida (p. ej., START)
                                           mientras el sistema estaba en @ref STATE_EMERGENCY. */
} SystemActionResponse;


/**
 * @fn SystemActionResponse GestorEstados_Action(SystemAction sysAct, int value)
 * @brief Ejecuta una acción de la máquina de estados y devuelve el resultado.
 * @details
 * Evalúa la acción @p sysAct contra el estado actual (@ref SystemState) y,
 * según corresponda, invoca rutinas del SVM (p.ej. `GestorSVM_MotorStart()`,
 * `GestorSVM_MotorStop()`, `GestorSVM_Estop()`, `GestorSVM_SetFrec/SetAcel/SetDecel/SetDir`)
 * y actualiza la variable interna `currentState`.  
 * Las reglas de transición/retorno (según el código mostrado) incluyen, entre otras:
 * - @ref ACTION_INIT_DONE : solo desde @ref STATE_INIT → @ref STATE_IDLE.
 * - @ref ACTION_START     : desde @ref STATE_IDLE → @ref STATE_VEL_CHANGE; si hay movimiento → @ref ACTION_RESP_MOVING; en emergencia → @ref ACTION_RESP_EMERGENCY_ACTIVE.
 * - @ref ACTION_STOP      : desde RUNNING/VEL_CHANGE → @ref STATE_BRAKING; en EMERGENCY → @ref STATE_IDLE; en IDLE → @ref ACTION_RESP_NOT_MOVING.
 * - @ref ACTION_MOTOR_STOPPED : en BRAKING → @ref STATE_IDLE.
 * - @ref ACTION_EMERGENCY : a @ref STATE_EMERGENCY (si no lo estaba).
 * - @ref ACTION_TO_CONST_RUNNING : en VEL_CHANGE → @ref STATE_RUNNING.
 * - @ref ACTION_SET_FREC/SET_ACEL/SET_DESACEL/SET_DIR : validan rango/estado y pueden
 *   dejar el estado o forzar @ref STATE_VEL_CHANGE (según retorno de `GestorSVM_*`).
 *
 * @param[in] sysAct  Acción solicitada (@ref SystemAction).
 * @param[in] value   Parámetro asociado a la acción (p.ej., frecuencia, acel./desacel., dirección).
 *
 * @return SystemActionResponse
 *         - @ref ACTION_RESP_OK              : La acción fue válida y se ejecutó (posible cambio de estado).
 *         - @ref ACTION_RESP_ERR             : Secuencia inválida para el estado actual o fallo de configuración.
 *         - @ref ACTION_RESP_MOVING          : Rechazo/indicación de que el motor está en movimiento.
 *         - @ref ACTION_RESP_NOT_MOVING      : Rechazo/indicación de que el motor está detenido.
 *         - @ref ACTION_RESP_OUT_RANGE       : Parámetro @p value fuera de límites admitidos.
 *         - @ref ACTION_RESP_EMERGENCY_ACTIVE: Acción no permitida bajo @ref STATE_EMERGENCY.
 *
 * @note Las acciones de lectura (@ref ACTION_GET_FREC, @ref ACTION_GET_ACEL,
 *       @ref ACTION_GET_DESACEL, @ref ACTION_GET_DIR) no cambian de estado en este switch.
 * @warning Verificar que cada `case` finalice con `break` (en el código provisto
 *          faltaba `break` tras @ref ACTION_EMERGENCY para evitar fall-through).
 */
SystemActionResponse GestorEstados_Action(SystemAction sysAct, int value);


#endif /* MODULES_GESTOR_ESTADOS_GESTORESTADOS_H_ */
