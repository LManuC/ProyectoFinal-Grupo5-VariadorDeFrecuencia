/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "../Modules/Gestor_SVM/GestorSVM.h"



/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */






/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */

  /* USER CODE END DMA1_Channel4_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi2_rx);
  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */

  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */

  /* USER CODE END DMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi2_tx);
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */

  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
  GestorSVM_CalcInterrupt();
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */
	
  //-----------              NOTASS     ----------------------------------------


  // El timer esta seteado con un prescaler de 28 y sin division de clock

  //-----------              NOTASS     ----------------------------------------

/*
  
  // Canal 1
	if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC1) != RESET) {
		if (__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_CC1) !=RESET) {
			__HAL_TIM_CLEAR_IT(&htim3, TIM_IT_CC1);
			// Interrupción canal 1
      GestorSVM_SwitchInterrupt(SWITCH_INT_RESET);
		}
	}

  // Canal 2
  if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC2) != RESET) {
    if (__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_CC2) !=RESET) {
      __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_CC2);
      // Interrupción canal 2
      GestorSVM_SwitchInterrupt(SWITCH_INT_CH1);
    }
  }

  // Canal 3
  if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC3) != RESET) {
    if (__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_CC3) !=RESET) {
        __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_CC3);
        // Interrupción canal 3
        GestorSVM_SwitchInterrupt(SWITCH_INT_CH2);
    }
  }

  // Canal 4
  if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC4) != RESET) {
    if (__HAL_TIM_GET_IT_SOURCE(&htim3, TIM_IT_CC4) !=RESET) {
      __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_CC4);
      // Interrupción canal 4
      GestorSVM_SwitchInterrupt(SWITCH_INT_CH3);
    }
  }

  */

  // Evito el HAL_TIM_IRQHandler(&htim3);
  //return;

  /*
	// Aca realizamos el calculo 
  if (TIM3->SR & (1 << 1)) {    //Channel 1 ISR
    TIM3->SR &= ~(1 << 1);
    
    GestorSVM_CalcularValoresSwitching(&valoresSwitching);
    //HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    

    TIM3->CCR2 = valoresSwitching.ticks1[0]; // Cargo los ticks para el canal 2
    TIM3->CCR3 = valoresSwitching.ticks2[0]; // Para el canal 3
    TIM3->CCR4 = valoresSwitching.ticks3[0]; // Para el canal 4

    TIM3->EGR |= TIM_EGR_UG;
  }

  */


  /*
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

  */

  /*
  switch(valoresSwitching.contador) {
    case 0:
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_1, 1);
      valoresSwitching.contador ++;
      break;
    case 1:
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_2, 1);
      valoresSwitching.contador ++;
      break;
    case 2:
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_3, 1);
      valoresSwitching.contador ++;
      break;
    case 3:
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_3, 0);
      valoresSwitching.contador ++;
      break;
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_2, 0);
      valoresSwitching.contador ++;
    case 4:
      GestorSVM_SwitchPuertos(ORDEN_SWITCH_1, 0);
      valoresSwitching.contador ++;
      break;
    case 5:
      // Aca tendriamos un error
      //TIM3->SR &= ~(1 << 1);
    
      GestorSVM_CalcularValoresSwitching(&valoresSwitching);
      //HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
      

      TIM3->CCR2 = valoresSwitching.ticks1[0]; // Cargo los ticks para el canal 2
      TIM3->CCR3 = valoresSwitching.ticks2[0]; // Para el canal 3
      TIM3->CCR4 = valoresSwitching.ticks3[0]; // Para el canal 4

      TIM3->EGR |= TIM_EGR_UG;

      valoresSwitching.contador = 0;
      break;
  }
  */

/*

    // Esta es la salida 1, aca tenemos el T1 
	if (TIM3->SR & (1 << 2)) {    //Channel 2 ISR
		TIM3->SR &= ~(1 << 2); //Clear the interrupt flag, doesn't do anything...

    if(valoresSwitching.contador == 0) {
      valoresSwitching.contador = 1;
    } else if(valoresSwitching.contador == 5) {
      valoresSwitching.contador = 6;
    }else {
      valoresSwitching.contador = valoresSwitching.contador;
    }

		GestorSVM_SwitchPuertos(ORDEN_SWITCH_1, valoresSwitching.flagSwitch[0]);

    valoresSwitching.flagSwitch[0] = 0;
	}


    // Salida 2, T2
	if (TIM3->SR & (1 << 3)) {    //Channel 3 ISR
		TIM3->SR &= ~(1 << 3); //Clear the interrupt flag, doesn't do anything...
		
    if(valoresSwitching.contador == 1) {
      valoresSwitching.contador = 2;
    } else if(valoresSwitching.contador == 4) {
      valoresSwitching.contador = 5;
    }else {
      valoresSwitching.contador = valoresSwitching.contador;
    }


    GestorSVM_SwitchPuertos(ORDEN_SWITCH_2, valoresSwitching.flagSwitch[1]);

    valoresSwitching.flagSwitch[1] = 0;
	}

    // Salida 3, T0
	if (TIM3->SR & (1 << 4)) {    //Channel 4 ISR
    TIM3->SR &= ~(1 << 4); //Clear the interrupt flag, doesn't do anything...
    
    if(valoresSwitching.contador == 2) {
      valoresSwitching.contador = 3;
    } else if(valoresSwitching.contador == 3) {
      valoresSwitching.contador = 4;
    }else {
      valoresSwitching.contador = valoresSwitching.contador;
    }

    GestorSVM_SwitchPuertos(ORDEN_SWITCH_3, valoresSwitching.flagSwitch[2]);

    valoresSwitching.flagSwitch[2] = 0;
  }

*/


	//Determine which channel interrupted with the if statements
	/*

	if (TIM3->SR & (1 << 1))    //Channel 1 ISR
	    {
	        TIM3->SR &= ~(1 << 1); //Clear the interrupt flag, doesn't do anything...

	    }
	    if (TIM3->SR & (1 << 2))    //Channel 2 ISR
	    {
	        TIM3->SR &= ~(1 << 2); //Clear the interrupt flag, doesn't do anything...
	        //Do stuff...
	    }
	    if (TIM3->SR & (1 << 3))    //Channel 3 ISR
	    {
	        TIM3->SR &= ~(1 << 3); //Clear the interrupt flag, doesn't do anything...
	        //Do stuff...
	    }

	    */
  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */
}

/**
  * @brief This function handles SPI2 global interrupt.
  */
void SPI2_IRQHandler(void)
{
  /* USER CODE BEGIN SPI2_IRQn 0 */

  /* USER CODE END SPI2_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi2);
  /* USER CODE BEGIN SPI2_IRQn 1 */

  /* USER CODE END SPI2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
