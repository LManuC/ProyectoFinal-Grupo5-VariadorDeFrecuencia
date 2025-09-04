/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "../Modules/Gestor_SVM/GestorSVM.h"
//#include "../Gestor_DinamicaMotor/GestorDinamica.h"
#include <stdio.h>

#include "../Modules/Gestor_DinamicaMotor/GestorDinamica.h"
#include "../Modules/Gestor_Timers/GestorTimers.h"
#include "../Modules/Gestor_Estados/GestorEstados.h"
#include "../Modules/SoftTimers/SoftTimers.h"
#include "../Modules/SPI_Interfase/SPIModule.h"



/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Estas variables estan para prueba nomas
static inline void CallActionStart(void) { GestorEstados_Action(ACTION_START, 0);}
static inline void CallActionStop(void)  { GestorEstados_Action(ACTION_STOP, 0);}

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi2_rx;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

extern UART_HandleTypeDef huart1;

// Aca vamos a iniciar el gestor de svm
//GestorSVM gestorSVM; // Frecuencia de 1kHz para el switch y 1kHz para el temporizador

//static int contadorValores[20];
//static int index = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  // Primero inicializamos el gestor de svm
  GestorSVM_Init();
  /*
      Disposicion de transistores, los transistores
      son los Q1, Q2, Q3, Q4, Q5 y Q6

      |    |    |
      Q1   Q2   Q3
      |    |    |
      |    |    |
      Q4   Q5   Q6
      |    |    |

      Como esto esta controlado por drivers de mosfets, entonces
      vamos a poner un 1 para activar el transistor superior y un 
      cero para el transistor inferior. 

      Nosotros vamos a tener ocho vectores en que vamos a trabajar,
      v0, v1, v2, v3, v4, v5, v6 y v7. Siendo el v0 y v7 dos vectores
      nulos. Cada uno de estos cuadrantes tiene un valor que indica 
      el valor de los transistores. 
      v0 = 000, v1 = 001, v2 = 010, v3 = 011, v4 = 100, v5 = 101, 
      v6 = 110, v7 = 111.
      
      Ahora vamos a detallar por cuadrante como nos va a quedar y 
      que cambia en cada cuadrante

      Quad 1: v0, v1, v3, v7  =>  
      P3: High, P2: High, P1: High, P1: Low, P2: Low, P3: Low
      
      Quad 2: v0, v2, v3, v7  =>
      P2: High, P3: High, P1: High, P1: Low, P3: Low, P2: Low
      
      Quad 3: v0, v2, v6, v7  =>
      P2: High, P1: High, P3: High, P3: Low, P1: Low, P2: Low
      
      Quad 4: v0, v4, v6, v7  =>
      P1: High, P2: High, P3: High, P3: Low, P2: Low, P1: Low
      
      Quad 5: v0, v4, v5, v7  =>
      P1: High, P3: High, P2: High, P2: Low, P3: Low, P1: Low
      
      Quad 6: v0, v1, v5, v7  =>
      P3: High, P1: High, P2: High, P2: Low, P1: Low, P3: Low
  */

  // Puertos
  int matrizPuertos[6][3] = {
      {2, 1, 0},  // Camino: V0 -> V1 -> V2 -> V7 -> V2 -> V1 -> V0
      {1, 2, 0},  // Camino: V0 -> 
      {1, 0, 2},
      {0, 1, 2},
      {0, 2, 1},
      {2, 0, 1}
  };

  //  ------------      CONFIGURACION DEL MODULO SVM      ------------------------

  // Generamos la variable a pasar por puntero
  ConfiguracionSVM config;
  config.frecOutMin = 1;
  config.frecOutMax = 450;
  config.frec_switch = 2200; // Frecuencia de 2.2kHz para el switch
  //config.frec_output = 0; // Frecuencia de 1kHz para el temporizador
  config.frecOutputTaget = 50;
  config.direccionRotacion = 1; // Sentido horario
  config.resolucionTimer = 255; // Resolucion del timer (255 para 8 bits)

  //Configuracion dinamica
  config.acel = 5;        // En frec/seg
  config.desacel = 3;     // En frec/seg

  // Cargamos el orden de activacion de los puertos
  for(int i = 0; i < 6; i++) {
    for(int j = 0; j < 3; j++) {
      config.orden_pierna[i][j] = matrizPuertos[i][j];
    }
  }

  // Cargamos los numeros de los pines
  config.puerto_senal_pierna[0] = GPIO_PIN_1;
  config.puerto_senal_pierna[1] = GPIO_PIN_3;
  config.puerto_senal_pierna[2] = GPIO_PIN_5;
  config.puerto_encen_pierna[0] = GPIO_PIN_2;
  config.puerto_encen_pierna[1] = GPIO_PIN_4;
  config.puerto_encen_pierna[2] = GPIO_PIN_6;

  GestorSVM_SetConfiguration(&config);

  GestorDinamica_Init();
  ConfigDinamica configDin;
  configDin.acel = 2;    // 1 rps / seg ^ 2
  configDin.descel = 1; // 1 rps / seg ^ 2
  configDin.t_acel = 5;         // 5 segundos
  configDin.t_decel = 5;        // 5 segundos
  GestorDinamica_SetConfig(&configDin);


  // Inicio del gestor de timers
  GestorTimers_Init(&htim3, &htim2);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  GestorEstados_Init();
  SoftTimer_Inicio();
  SPI_Init(&hspi2);


  /*
  // Aca tenemos que iniciar los timers
  // Pero no lo hacemos, sino que esperamos a los botones de inicio


  //HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  //HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
  //HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
  //HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);
  
  //HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0); // Menor prioridad que TIM3
  //HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
  */


  // ESTO ES PARA PROBAR ACTIVAR UN BOTON -----   NO SE USA
  // Aca voy a iniciar un timmer que va a simular el toque de un boton
  //SoftTimer_Set(TIMER_10, UNICO_DISPARO, 3000, CallActionStart);


  // Informamos al gestor de estados que finalizo la inicializacion
  // Esta debe ser la ultima llama de funcion del init
  GestorEstados_Action(ACTION_INIT_DONE, 0);

  printf("Config done\n");
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    //GestorEstados_Run();
    SoftTimer_Loop();

    /*
    //SPI_Loop();
	  //printf("Holamundo\n");
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    //HAL_Delay(2000);
    //printf("AngSwitch: %d \n", GetData());
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    //HAL_Delay(500);
    */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7200-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 5;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 28;
  htim3.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED3;
  htim3.Init.Period = 256;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_1);
  sConfigOC.Pulse = 20;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_2);
  sConfigOC.Pulse = 80;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_3);
  sConfigOC.Pulse = 256;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_4);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA2 PA3 PA4
                           PA5 PA6 PA7 PA8
                           PA9 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */


int __io_putchar(int ch) {
  //HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
  return ch;
}


int _write(int file, char *ptr, int len) {
  (void)file;
  for (int i = 0; i < len; i++) {
      HAL_UART_Transmit(&huart1, (uint8_t*)&ptr[i], 1, HAL_MAX_DELAY);
  }
  return len;
}



/*

// Este se ejecuta cada vez que se termina el contador
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	UNUSED(htim);
	//HAL_TIM_IRQHandler(&htim3);

	static int contador[20];


	if(htim->Instance == TIM3) {


		uint32_t counter_value = TIM3->CNT;  // Leer el valor actual del contador

		contador[index] = (int)counter_value;
		index++;
		if(index == 20) {
			index = 0;
		}


	}


}

*/





void TIM3_CC_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim3);

    // Comparación con CCR1
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC1)) {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_CC1);

    }

    // Comparación con CCR2
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC2)) {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_CC2);

    }

    // Comparación con CCR3
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_CC3)) {
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_CC3);

    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {


	if (htim == &htim3) {
        // handle TIM1 interrupts
    	//uint32_t counter_value = TIM3->CNT;
    }
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
