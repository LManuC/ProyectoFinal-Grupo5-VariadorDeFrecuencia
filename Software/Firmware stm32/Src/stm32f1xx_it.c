/* USER CODE BEGIN Header */
/**
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

#include "main.h"
#include "stm32f1xx_it.h"

#include "../Modules/Gestor_SVM/GestorSVM.h"

extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {

  while (1) {

  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void) {

  while (1) {

  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void) {

  while (1) {

  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void) {

  while (1) {

  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void) {

  while (1) {

  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void) {

}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void) {

}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void) {

}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void) {
  HAL_IncTick();
}

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void) {
  GestorSVM_CalcInterrupt();
  HAL_TIM_IRQHandler(&htim2);
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void) {
  HAL_TIM_IRQHandler(&htim3);
}

/**
  * @brief This function handles SPI2 global interrupt.
  */
void SPI2_IRQHandler(void) {
  HAL_SPI_IRQHandler(&hspi2);
}