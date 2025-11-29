/**
  * @file    main.h
  * @brief   Header de la aplicación principal.
  * @details Contiene definiciones comunes, asignación de pines y prototipos
  *          globales compartidos por todo el proyecto.
  */

#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"

/** \defgroup GPIO_SVM Pines del inversor (puerto A)
  * @brief Pines de control del módulo SVM en GPIOA.
  * @{
  */

/** @brief Entrada lógica de la pierna U (GPIOA, PIN_1). */
#define GPIO_U_IN           GPIO_PIN_1
/** @brief Shutdown/Habilitación driver pierna U (GPIOA, PIN_2). */
#define GPIO_U_SD           GPIO_PIN_2
/** @brief Entrada lógica de la pierna V (GPIOA, PIN_3). */
#define GPIO_V_IN           GPIO_PIN_3
/** @brief Shutdown/Habilitación driver pierna V (GPIOA, PIN_4). */
#define GPIO_V_SD           GPIO_PIN_4
/** @brief Entrada lógica de la pierna W (GPIOA, PIN_5). */
#define GPIO_W_IN           GPIO_PIN_5
/** @brief Shutdown/Habilitación driver pierna W (GPIOA, PIN_6). */
#define GPIO_W_SD           GPIO_PIN_6

/** @brief Entrada digital: Termo–switch (GPIOA, PIN_11). */
#define GPIO_TERMO_SWITCH   GPIO_PIN_11
/** @brief Entrada digital: Botón de parada (GPIOA, PIN_12). */
#define GPIO_STOP_BUTTON    GPIO_PIN_12
/** @} */

/** \defgroup GPIO_LEDS Pines de señalización (puerto B)
  * @brief LEDs de estado en GPIOB.
  * @{
  */

/** @brief LED de estado (GPIOB, PIN_0). */
#define GPIO_LED_STATE      GPIO_PIN_0
/** @brief LED de error (GPIOB, PIN_2). */
#define GPIO_LED_ERROR      GPIO_PIN_2
/** @} */

/**
  * @fn void Error_Handler(void)
  * @brief Manejador global de errores fatales.
  * @details Detiene la ejecución en un bucle y puede ser utilizado para
  *          reportar/diagnosticar condiciones críticas. Implementado en @ref main.c.
  */
void Error_Handler(void);

#endif /* __MAIN_H */
