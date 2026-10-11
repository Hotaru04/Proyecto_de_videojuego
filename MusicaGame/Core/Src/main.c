/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audio.h"      /* motor de audio: TIM6 -> DAC1 (PA4) <- DMA1 Stream5 */
#include "songs.h"      /* tabla de canciones (los .h de canciones se incluyen en songs.c) */
#include "enlace_spi.h" /* SPI2 hacia las pantallas (una por jugador) */
#include "beats.h"      /* notas del juego que se mandan a las pantallas */
#include "controles.h"  /* botones de la ESP32 por USART1 (PA10) */
#include "juez.h"       /* acierto / fallo, mute del jugador que falla */
#include "registro.h"   /* bitacora por el USB del ST-Link (Termite, 115200) */
#include "juego.h"      /* menús, canción, resultados */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* ===== Canción inicial =====
 * Número en la tabla de canciones: ver CANCION_<NOMBRE> en Core/Inc/canciones_tabla.h
 * (ese archivo lo genera tools/registrar_canciones.py; hoy: 0 animals, 1 power rangers, 2 night begins to shine).
 * Con JUEGO_INICIAR_EN_MENU = 1 (juego.h) es solo la que aparece marcada en el menú. */
volatile uint8_t song_index = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  Audio_Init();                    /* calcula ARR de TIM6 para 32 kHz y prepara el sintetizador */
  Audio_Start();                   /* desde aquí el audio corre solo por DMA + interrupciones */
  /* Prioridades: CubeMX deja el grupo 0 (ninguna interrupción interrumpe a otra) y el audio del DMA
     tarda varios ms -> se perdían ticks de HAL_GetTick y bytes de la UART. Con el grupo 4:
     SysTick (0) > UART de los controles (1) > audio (2). */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 2, 0);
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 2, 0);

  Enlace_Init();                   /* SPI2 maestro: PB13 SCK, PB15 MOSI, PB12 CS jugador 1, PB14 CS jugador 2 */
  Controles_Init();                /* USART1 RX en PA10 <- TX de la ESP32 */
  Registro_Init();                 /* USART2 -> ST-Link -> COM de la PC */
  Registro("=== MusicaGame (maestro) ===");
  Registro("ESP32 TX -> PA10 | SPI2: PB13 SCK, PB15 MOSI, PB12 CS J1, PB14 CS J2 | audio: PA4");
  Registro("microSD (SPI3): PC10 SCK, PC11 MISO, PC12 MOSI, PD2 CS");
  Juego_Iniciar(song_index);       /* menú de canciones (o la canción directo, ver juego.h) */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Menús, canción (notas + juez), resultados */
    Juego_Actualizar();

    /* Bitácora: resumen cada 5 s */
    static uint32_t tEstado = 0;
    if (HAL_GetTick() - tEstado >= 5000) {
      tEstado = HAL_GetTick();
      Registro("estado: %s | tick %lu | audio %u%% CPU, %lu underruns | ESP32 %lu lineas, %lu errores | SPI %lu paquetes"
               " | J1 %lu/%lu %lu pts  J2 %lu/%lu %lu pts%s",
               Juego_Estado(), (unsigned long)Music_GetTick(), audio_cpu_load, (unsigned long)audio_underruns,
               (unsigned long)controles_lineas, (unsigned long)controles_errores, (unsigned long)enlace_paquetes,
               (unsigned long)juez_aciertos[0], (unsigned long)juez_fallos[0], (unsigned long)juez_puntos[0],
               (unsigned long)juez_aciertos[1], (unsigned long)juez_fallos[1], (unsigned long)juez_puntos[1],
               Juez_EnPausa() ? "  [PAUSA]" : "");
    }

    /* LD2: parpadea = el programa corre; encendido fijo = el jugador 1 está muteado (falló) */
    static uint32_t tLed = 0;
    if (juez_muteado[0]) {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    } else if (HAL_GetTick() - tLed >= 250) {
      tLed = HAL_GetTick();
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
    /* Diagnóstico (Live Expressions): audio_cpu_load, audio_underruns, controles_lineas, juez_aciertos, juez_fallos */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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
