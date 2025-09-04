#include "../Gestor_Timers/GestorTimers.h"

#include "../Inc/main.h"

// Este gestor va a mantener los timers. A este le vamos a pedir que nos inicie
// y detenga los timers.
// Los posibles timers ya estan establecidos


// Dos punteros a las variables de timer
TIM_HandleTypeDef* htim_SVM;
TIM_HandleTypeDef* htim_Din;

void GestorTimers_Init(TIM_HandleTypeDef* _htim_SVM, TIM_HandleTypeDef* _htim_Din) {
    htim_SVM = _htim_SVM;
    htim_Din = _htim_Din;
}

void GestorTimers_IniciarTimer(TimersEnum timerEnum) {
    switch(timerEnum) {
        case SVM_Timer:
            // Iniciar el timer SVM

            // Vamos a setear la prioridad del timer 3 a 0
            // Una menos a la del timer 3
            HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
            HAL_NVIC_EnableIRQ(TIM3_IRQn);

            // Se inician los 4 canales del timer 3
            HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_1);
            HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_2);
            HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_3);
            HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_4);
            break;
        case Dinamic_Timer:
            // Iniciar el timer Dinamic

            // Vamos a setear la prioridad del timer 2 a 2
            // Una menos a la del timer 2
            HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
            HAL_NVIC_EnableIRQ(TIM2_IRQn);

            HAL_TIM_IC_Start_IT(htim_Din, TIM_CHANNEL_1);
            break;
    }
}

void GestorTimers_DetenerTimer(TimersEnum timerEnum) {

}


void GestorTimers_IniciarTimerSVM() {

    // Vamos a setear la prioridad del timer 3 a 0
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    // Se inician los 4 canales del timer 3
    HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(htim_SVM, TIM_CHANNEL_4);
}

void GestorTimers_DetenerTimerSVM() {

    HAL_TIM_IC_Stop_IT(htim_SVM, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(htim_SVM, TIM_CHANNEL_2);
    HAL_TIM_IC_Stop_IT(htim_SVM, TIM_CHANNEL_3);
    HAL_TIM_IC_Stop_IT(htim_SVM, TIM_CHANNEL_4);

    __HAL_TIM_SET_COUNTER(htim_SVM, 0);
}