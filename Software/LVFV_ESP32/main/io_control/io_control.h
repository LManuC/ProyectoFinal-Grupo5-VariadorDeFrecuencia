/**
 * @file io_control.h
 * @author Andrenacci - Carra
 * @brief Declaración de funciones de control del relay, buzzer y tarea del uso del expansor de entradas y salidas y entradas y salidas aisladas directas al ESP32
 * @version 2.0
 * @date 2025-11-06
 * 
 */
#ifndef __IO_CONTROL_H__

#define __IO_CONTROL_H__

/**    
 * @fn void GPIO_interrupt_attendance_task(void* arg);
 *
 * @brief Tarea que en función de las interrupciones recibidas permite obtener cual de los pulsadores del teclado, entradas aisladas, termo switch o entradas del MCP2307 fue presionado
 *
 * @details Trabaja con un antirrebote de 200ms. Cuando una entrada cabia su estado, la tarea vuelve a leer las entradas luego de 200ms para asegurarse el valor leído y recién ahí envía la señal correspondiente
 *
 * @todo Revisar correctamente el funcionamiento del filtro antirebote.
 *
 */
void GPIO_interrupt_attendance_task(void* arg);

/**    
 * @fn esp_err_t set_freq_output(uint16_t freq);
 *
 * @brief Convierte la frecuencia en Hz que está girando el motor en el duty necesario para poder operar el PWM de la salida 0-10V.
 *
 * @details Permite configurar el duty del PWM desde 0 a 450Hz siendo que, para 0Hz la salida será 0V y para 450HZ serán 10V.
 *
 * @param[in] freq
 *      Es la frecuencia que está girando el motor.
 *    
 * @return
 *  - ESP_OK PWM configurado correctamente
 *  - ESP_ERR_INVALID_ARG Error al configurar el duty del PWM
 */
esp_err_t set_freq_output(uint16_t freq);

/**    
 * @fn esp_err_t RelayEvantPost( uint8_t relay_event );
 *
 * @brief Crea un evento para que el relay se activo o desactive de acuerdo a la variable @p relay_event
 *
 * @details Con @p relay_event en 1 activa el ralay, con 0 desactiva el relay. Cualquier otro valor retorna ESP_FAIL.
 *
 * @param[in] relay_event
 *      Estado del relay
 *    
 * @return
 *  - ESP_OK Evento creado exitosamente
 *  - ESP_FAIL Error al crear el evento
 */
esp_err_t RelayEvantPost( uint8_t relay_event );

/**    
 * @fn esp_err_t BuzzerEvantPost( uint32_t buzzer_time_on );
 *
 * @brief Envía una señal para hacer sonar el buzzer durante @p buzzer_time_on mili segundos.
 *
 * @param[in] buzzer_time_on
 *      Tiempo en mili segundos durante el que el buzzer sonará.
 *    
 * @return
 *  - ESP_OK Evento creado exitosamente
 *  - ESP_FAIL Error al crear el evento
 */
esp_err_t BuzzerEvantPost( uint32_t buzzer_time_on );

#endif