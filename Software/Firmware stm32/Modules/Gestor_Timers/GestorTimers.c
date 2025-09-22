#include "../Gestor_Timers/GestorTimers.h"

#include "../Inc/main.h"

// Este gestor va a mantener los timers. A este le vamos a pedir que nos inicie
// y detenga los timers.
// Los posibles timers ya estan establecidos


// Dos punteros a las variables de timer
TIM_HandleTypeDef* hTimerSwitch;
TIM_HandleTypeDef* hTimerCalc;

void GestorTimers_Init(TIM_HandleTypeDef* _hTimerSwitch, TIM_HandleTypeDef* _hTimerCalc) {
    hTimerSwitch = _hTimerSwitch;
    hTimerCalc = _hTimerCalc;
}


void GestorTimers_IniciarTimerSVM() {

    // Seteo de la prioridad del timer 2 a la segunda mas alta (1)
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    // Seteo de la prioridad del timer 3 a la prioridad mas alta (0)
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    // Se inician el timer 2
    HAL_TIM_IC_Start_IT(hTimerCalc, TIM_CHANNEL_1);

    // Se inician los 4 canales del timer 3
    HAL_TIM_IC_Start_IT(hTimerSwitch, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(hTimerSwitch, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(hTimerSwitch, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(hTimerSwitch, TIM_CHANNEL_4);
}

void GestorTimers_DetenerTimerSVM() {

    // Se detiene el timer de switcheo
    HAL_TIM_IC_Stop_IT(hTimerSwitch, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(hTimerSwitch, TIM_CHANNEL_2);
    HAL_TIM_IC_Stop_IT(hTimerSwitch, TIM_CHANNEL_3);
    HAL_TIM_IC_Stop_IT(hTimerSwitch, TIM_CHANNEL_4);
    
    __HAL_TIM_SET_COUNTER(hTimerSwitch, 0);
    
    
    // Se detiene el timer de calculo
    HAL_TIM_IC_Stop_IT(hTimerCalc, TIM_CHANNEL_1);
    __HAL_TIM_SET_COUNTER(hTimerCalc, 0);
    
}