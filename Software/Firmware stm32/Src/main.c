/**
 * @file main.c
 * @brief Punto de entrada del firmware y configuración de periféricos.
 * @details Inicializa reloj de sistema, GPIO, DMA, SPI2 (esclavo), USART1 (debug),
 *          y TIM2/TIM3 (cálculo y switching). Integra módulos GestorSVM, GestorTimers,
 *          GestorEstados y SPI module.
 *   Disposicion de transistores, los transistores
 *   son los Q1, Q2, Q3, Q4, Q5 y Q6 
 *   |    |    |
 *   Q1   Q2   Q3
 *   |    |    |
 *   |    |    |
 *   Q4   Q5   Q6
 *   |    |    | 
 *   Como esto esta controlado por drivers de mosfets, entonces vamos a poner un 1 para activar el transistor superior y un cero para el transistor inferior.
 *   Vamos a tener ocho vectores en que vamos a trabajar: v0, v1, v2, v3, v4, v5, v6 y v7. Siendo el v0 y v7 dos vectores nulos. Cada uno de estos cuadrantes tiene un valor que indica el valor de los transistores:
 * - v0 = 000,
 * - v1 = 001,
 * - v2 = 010,
 * - v3 = 011,
 * - v4 = 100,
 * - v5 = 101, 
 * - v6 = 110,
 * - v7 = 111.
 *  Ahora vamos a detallar por cuadrante como nos va a quedar y que cambia en cada cuadrante  
 * - Quad 1: v0, v1, v3, v7  => P3: High, P2: High, P1: High, P1: Low, P2: Low, P3: Low
 * - Quad 2: v0, v2, v3, v7  => P2: High, P3: High, P1: High, P1: Low, P3: Low, P2: Low
 * - Quad 3: v0, v2, v6, v7  => P2: High, P1: High, P3: High, P3: Low, P1: Low, P2: Low
 * - Quad 4: v0, v4, v6, v7  => P1: High, P2: High, P3: High, P3: Low, P2: Low, P1: Low
 * - Quad 5: v0, v4, v5, v7  => P1: High, P3: High, P2: High, P2: Low, P3: Low, P1: Low
 * - Quad 6: v0, v1, v5, v7  => P3: High, P1: High, P2: High, P2: Low, P1: Low, P3: Low
 */

#include "main.h"
#include <stdio.h>

#include "../Modules/Gestor_SVM/GestorSVM.h"
#include "../Modules/Gestor_Timers/GestorTimers.h"
#include "../Modules/Gestor_Estados/GestorEstados.h"
#include "../Modules/SPI_Interfase/SPIModule.h"

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi2_rx;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;

/**
 * @fn static void SystemClock_Config(void)
 * @brief Configura el clock del sistema a 72 MHz desde HSE=8 MHz con PLL×9.
 * @details Activa HSE, configura PLL (source=HSE, mul=×9) y selecciona SYSCLK=PLL.
 *          AHB=72 MHz, APB2=72 MHz (div1), APB1=36 MHz (div2), FLASH_LATENCY=2.
 */
 static void SystemClock_Config(void);

/**
 * @fn static void MX_GPIO_Init(void);
 * 
 * @brief Inicializa GPIOA y GPIOB.
 *
 * @details Configura los pines de los puertos A y B como salidas push pull, sin pull up y como pines de baja frecuencia. Los pines configurados son los que controlan el modulador y salidas (leds de estado). Las entradas digitales (termoswitch y boton de stop) no son configuradas ya que serán señales controladas por el HMI.
 */
static void MX_GPIO_Init(void);

/**
 * @fn static void MX_DMA_Init(void)
 *
 * @brief Habilita el clock del DMA1 y sus interrupciones de canal 4 y 5.
 *
 * @details Habilita el reloj del **DMA1** y registra en el NVIC las IRQ de **DMA1_Channel4** y **DMA1_Channel5** con prioridad 2.
 * En F1, estos canales suelen mapearse a **SPI2_RX (Ch4)** y **SPI2_TX (Ch5)**, por lo que esta rutina deja listo el DMA para que llamadas como `HAL_SPI_TransmitReceive_DMA()` configuren y lancen las transferencias.  
 * No configura direcciones, tamaños ni modos del DMA; solo habilita el clock y las IRQ.
 * @note Anteriormente la prioridad era 0. Luego pasó a 2 ya que en el SPI se configura en 2.
 */
static void MX_DMA_Init(void);

/**
 * @fn  static void MX_TIM3_Init(void);
 * 
 * @brief Función inicialización del timer 3 (SVM)
 *
 * @details El timer 3 es utilizado para ejecutar el swtching de los pines del variador de frecuencia. Trabaja con un timer de 0 a 256 utilizando el clock interno como fuente de conteo dando un período de 398.22us (f = 2511Hz) y 4 canales de interrupción CCR1-4 que se irán corriendo conforme vaya avanzando la señal y cambiando la tensión de salida buscada de acuerdo a lo que dicte el estado del módulo SVM.
 */
static void MX_TIM3_Init(void);

/**
  * @fn static void MX_TIM2_Init(void);
  * 
  * @brief Función de inicialización del timer 2 (Pre cálculo)
  *
  * @details El timer funcionará a 10KHz tratando de adelantar los cálculos del timer 3, permitiendo agilizar la carga de sus contadores y evitando que haya problemas en la señal de salida.
  */
static void MX_TIM2_Init(void);

/**
  * @fn static void MX_USART1_UART_Init(void);
  *
  * @brief Inicialización de la UART 1 (DEBUG)
  *
  * @details El periférico se utilizará como puerto de debug. Solo tendrá pines de transmisión y recepción sin control de flujo; con configuración 115200,8,N,1
  */
static void MX_USART1_UART_Init(void);

/**
  * @fn static void MX_SPI2_Init(void);
  * 
  * @brief SPI2 Initialization Function (HMI)
  *
  * @details Configura al SPI2 como esclavo, dependiendo de la comuincación con el maestro, el ESP32 para poder transaccionar información. Inicializa un tamaño de 8 bits de datos, con dos líneas de comunicación (MISO y MOSI) comenzando el byte por bit más gignificativo. No utiliza el módulo CRC, el latch del dato se toma en el flanco decreciente del clock y la línea en reposo queda en estado alto. El pin de selección de esclavo es utilizado por el maestro para iniciar la comunicación.
  */
static void MX_SPI2_Init(void);

/**
 * @brief 
 * 
 * @brief  Función de entrada del sistema
 *
 * @details
 * La función main inicializa la estructura de configuración, inicializa los periféricos y el gestor de estados. Al terminar su trabajo queda en un while(1) con la sentencia WFI para reducir el consumo de CPU.
 *   1) HAL_Init()
 *   2) SystemClock_Config()
 *   3) Carga configuración SVM (frecuencia de switching, ref, etc.).
 *   4) Inicializa manejador de timers (GestorTimers_Init).
 *   5) Inicializa periféricos (GPIO, DMA, TIM3, USART1, TIM2, SPI2) y driver SPI.
 *   6) Notifica fin de init al Gestor de Estados (ACTION_INIT_DONE).
 *   7) Entra en lazo con WFI para ahorrar CPU, atendiendo a interrupciones.
 *
  * @retval int
 *     Sin uso
  */
int main(void) {

  HAL_Init();                                   /// Configuración del MCU. Reset of all peripherals, Initializes the Flash interface and the Systick.
  SystemClock_Config();
 
  
  ConfiguracionSVM config = {                   /// Cofiguración del módulo SVM
    .frec_switch = 2511,                        /// Frecuencia de 2.511kHz para el switch del timer 3 entre subida y bajada del timer 3
    .frecReferencia = 50,                       /// Frecuencia de salida
    .direccionRotacion = 1,                     /// Sentido horario
    .acel = 5,                                  /// Aceleracón del sistema por default [Hz/seg]
    .desacel = 3,                               /// Desaceleracón del sistema por default [Hz/seg]
  };
  
  // Cargamos los numeros de los pines
  config.puerto_senal_pierna[0] = GPIO_PIN_1;
  config.puerto_senal_pierna[1] = GPIO_PIN_3;
  config.puerto_senal_pierna[2] = GPIO_PIN_5;
  config.puerto_encen_pierna[0] = GPIO_PIN_2;
  config.puerto_encen_pierna[1] = GPIO_PIN_4;
  config.puerto_encen_pierna[2] = GPIO_PIN_6;
  GestorSVM_SetConfiguration(&config);

  // Initialize all configured peripherals
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();
  SPI_Init(&hspi2);

  // Inicio del gestor de timers
  GestorTimers_Init(&htim3, &htim2);

  // Informamos al gestor de estados que finalizo la inicializacion
  // Esta debe ser la ultima llama de funcion del init
  GestorEstados_Action(ACTION_INIT_DONE, 0);
  printf("Config done\n");

  __HAL_DBGMCU_FREEZE_TIM3();
  __HAL_DBGMCU_FREEZE_TIM2();

  while (1) {
    __WFI();
  }
}

static void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  // Initializes the CPU, AHB and APB buses clocks
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_SPI2_Init(void) {
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
  if (HAL_SPI_Init(&hspi2) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_TIM2_Init(void) {

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 7199;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 5;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_TIM3_Init(void) {
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 55;
  htim3.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED3;
  htim3.Init.Period = 256;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
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
  sConfigOC.Pulse = 240;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_ENABLE_OCxPRELOAD(&htim3, TIM_CHANNEL_4);

}

static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_DMA_Init(void) {
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_U_IN|GPIO_U_SD|GPIO_V_IN|GPIO_V_SD|GPIO_W_IN|GPIO_W_SD, GPIO_PIN_RESET);//|GPIO_TERMO_SWITCH|GPIO_STOP_BUTTON, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_LED_STATE|GPIO_LED_ERROR, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_U_IN|GPIO_U_SD|GPIO_V_IN|GPIO_V_SD|GPIO_W_IN|GPIO_W_SD|GPIO_TERMO_SWITCH|GPIO_STOP_BUTTON;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_LED_STATE|GPIO_LED_ERROR;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @fn void Error_Handler(void);
 * @fn void Error_Handler(void)
 * @brief Manejador genérico de errores.
 * @details Deshabilita IRQs y entra en bucle infinito para provocar reset por IWDG si está activo.
 */
void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @fn  void assert_failed(uint8_t *file, uint32_t line);
  * @brief  Reporta el nombre del archivo fuente y su línea donde un assert_param falla.
  * @param  file: Puntero al nombre del archivo fuente.
  * @param  line: Número de línea del assert_param que falló
  */
void assert_failed(uint8_t *file, uint32_t line);
void assert_failed(uint8_t *file, uint32_t line) {
  printf("Wrong parameters value: file %s on line %d\r\n", file, line)
}
#endif /* USE_FULL_ASSERT */
