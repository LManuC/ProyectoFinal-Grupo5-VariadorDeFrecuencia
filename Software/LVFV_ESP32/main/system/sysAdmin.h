/**
 * @file SysAdmin.h
 * @author Andrenacci - Carra
 * @brief Header con las funciones de funcionamiento del sistema
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __SYS_ADMIN_H__

#define __SYS_ADMIN_H__

#include "../LVFV_system.h"

/**
 * @fn esp_err_t get_status(system_status_t *s_e);
 *
 * @brief Obtiene una réplica del status del sistema
 *
 * @param[out] s_e
 *      Puntero a la variable donde se devolverá la información de status
 *
 * @return
 *      - ESP_ERR_NOT_ALLOWED: En caso de que s_e sea un puntero a NULL
 *      - ESP_OK: Si la copia fue exitosa
 */
esp_err_t get_status(system_status_t *s_e);

/**
 * @fn uint16_t engine_start();
 *
 * @brief Función que setea la frecuencia de régimen de acuerdo a lo configurado por el usuario y ejecuta la tarea accelerating.
 *
 * @details En caso de que desaccelerating esté corriendo, antes de ejecutar accelerating, la cierra para no generar un efecto de subida y bajada de frecuencia. El status pasará a SYSTEM_ACCLE_DESACCEL
 * 
 * @return Frecuencia de destino configurada
 */
uint16_t engine_start();

/**
 * @fn void engine_stop();
 *
 * @brief Función que setea la frecuencia de régimen de acuerdo a lo configurado por el usuario y ejecuta la tarea desaccelerating.
 *
 * @details En caso de que accelerating esté corriendo, antes de ejecutar desaccelerating, la cierra para no generar un efecto de subida y bajada de frecuencia. El status pasará a SYSTEM_ACCLE_DESACCEL o SYSTEM_BREAKING de  a cuerdo a la acción que haya ocurrido
 * 
 * @return Frecuencia de destino configurada
 */
uint16_t engine_stop();

/**
 * @fn void engine_emergency_stop();
 *
 * @brief Pasa la frecuencia de regimen a 0Hz y pasa a estado SYSTEM_EMERGENCY
 *
 * @details Termina las tareas accelerating y desaccelerating
 */
void engine_emergency_stop();

/**
 * @fn uint16_t change_frequency(uint8_t speed_slector);
 *
 * @brief Cambia la frecuencias de régimen del motor de acuerdo a la entrada aislada que haya ingresado. Ejecuta accelerating o desaccelerating, de acuerdo a lo necesario.
 *
 * @details De acuerdo a la frecuencia de régimen elegida y el tipo de variación, la frecuencia de destión tendrá diferentes valores, siempre menores a las de régimen.
 * 
 * @param[in] speed_slector
 *      Índice dentro del array con frecuencias de trabajo
 *
 * @return 0Hz si @p speed_selector fue mal pasada como argumento, sino la frecuencia de destino
 */
uint16_t change_frequency(uint8_t speed_slector);

/**
 * @fn system_status_e update_meas(uint16_t vbus_meas, uint16_t ibus_meas);
 *
 * @brief Actualiza las mediciones de tensión y corriente del bus de contínua.
 *
 * @details En caso que superar los límites configurados, entra en SYSTEM_EMERGENCY
 * 
 * @param[in] vbus_meas
 *      - Tensión de bus de contínua leído por el ADC
 *
 * @param[in] ibus_meas
 *      - Corriente de bus de contínua leído por el ADC
 *
 * @return
 *      - SYSTEM_IDLE: El sistema se encuentra en estado de reposo
 *      - SYSTEM_ACCLE_DESACCEL: El sistema se encuentra acelerando o desacelerando
 *      - SYSTEM_REGIME: El sistema está con el motor girando a régimen
 *      - SYSTEM_BREAKING: El sistema está frenando
 *      - SYSTEM_EMERGENCY: El sistema entra por primera vez en estado de emergencia
 *      - SYSTEM_EMERGENCY_OK: El sistema continúa en estado de emergencia
 */
system_status_e update_meas(uint16_t vbus_meas, uint16_t ibus_meas);

/**
 * @fn void set_frequency_table( uint16_t input_variable, uint16_t freq_regime );
 *
 * @brief Carga en el vector de velocidades configurables por entradas, las diferentes frecuencias de trabajo
 *
 * @details La posición 0 del buffer es la frecuencia de regimen, la posición 7 es la frecuencia más baja distinta de cero
 * 
 * @param[in] input_variable
 *      Tipo de variación entre las diferentes entradas. 1 para variación lineal; 2 para variación cuadrática.
 *
 * @param[in] freq_regime
 *      Frecuencia de regimen ingresada por el usuario
 */
void set_frequency_table( uint16_t input_variable, uint16_t freq_regime );

/**
 * @fn void set_system_settings( frequency_settings_t *f_s, seccurity_settings_t *s_s );
 *
 * @brief Actualiza las variables de seguridad del sistema
 *
 * @param[in] f_s
 *      Puntero a la estructura con las variables de frecuencia a actualizar
 *
 * @param[in] s_s
 *      Puntero a la estructura con las variables de seguridad a actualizar
 */
void set_system_settings( frequency_settings_t *f_s, seccurity_settings_t *s_s );

#endif
