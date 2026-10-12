/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "ili9341.h"
#include "Bitmaps.h"
#include "gh_sprites.h"   // [GH] sprites de botones y notas (en RAM)
#include "gh_notas.h"     // [GH] dibujo de botones y movimiento de notas
#include "gh_pista.h"     // [GH] pista con scroll por hardware (notas que llegan por SPI)
#include "gh_enlace.h"    // [GH] SPI2 esclavo: paquetes de MusicaGame
#include "gh_menu.h"      // [GH] lista de canciones, resultados y menú final
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* [GH] 1 = probar la pantalla SIN MusicaGame: inventa notas cada GH_PRUEBA_CADA_MS */
#define GH_PRUEBA_SIN_SPI   0
#define GH_PRUEBA_CADA_MS   350



// Variables para prueba sin maestro

//#define GH_MODO_PRUEBA_MENU    1
//#define GH_MENU_TEST_DELAY     2000 // ms para cambiar de opción automáticamente



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* [GH] Los botones ya no llegan aquí: la ESP32 se conecta a MusicaGame, que juzga y manda por SPI
   qué dibujar. USART3 queda configurada por CubeMX pero sin usarse. */

extern uint16_t fondo[];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
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
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  // Iniciar la escucha del ESP32 por interrupción


	LCD_Init();

	LCD_Clear(0x00);
	//FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
	//FillRect(0, 0, 319, 239, 0x0DFE);
	//FillRect(0, 0, 319, 239, 0x4c9d);

//	FillRect(50, 60, 20, 20, 0xF800);
//
//	FillRect(70, 60, 20, 20, 0x07E0);
//
//	FillRect(90, 60, 20, 20, 0x001F);


	//LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
	//LCD_Bitmap(100, 100, 49, 36, dkong);
	//LCD_Bitmap(100, 100, 24, 30, sonic);

	// [GH] Demo anterior en horizontal (320x240), desactivada al pasar a vertical:
	//LCD_Bitmap(0, 0, 320, 240, fondoPinguin);
	//LCD_BitmapTransparent(100, 100, 49, 36, dkong, 0x4b7e);
	//LCD_BitmapTransparent(50, 60,32,32,megaman,0x03ba);

	// [GH] Pista con scroll por hardware: fondo negro + fila de botones fija abajo
	GH_Pista_Iniciar();
	GH_Enlace_Iniciar();      // SPI2 esclavo (PB12 NSS, PB13 SCK, PB15 MOSI)
	//LCD_Bitmap(132, 100, 32, 32, megaman);



//	FillRect(0, 0, 319, 206, 0x055b);
//	LCD_Print("Hola Mundo", 20, 150, 2, 0x001F, 0x055b);
//
//
//for (int x = 0; x < 319; x++) {
//			LCD_Bitmap(x, 116, 15, 15, tile);
//			LCD_Bitmap(x, 68, 15, 15, tile);
//			LCD_Bitmap(x, 207, 15, 15, tile);
//			LCD_Bitmap(x, 223, 15, 15, tile);
//			x += 14;
//		}

	//LCD_BitmapTransparent(100, 116-36, 49, 36, dkong, 0x4b7e);
	//LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[], int columns, int index, char flip, char offset);

	//LCD_Sprite(100, 116-29, 42, 29, link, 4, 0, 0, 0);


	//  LCD_Print("Hola Mundo", 20, 100, 1, 0x001F, 0xCAB9);


///////////////////////////////////////////////////////////////////////////////////////////////
//
//#if GH_MODO_PRUEBA_MENU
//  // Al arrancar, desactivamos la pista de notas para que se pueda ver el menú completo.
//  GH_Pista_Desactivar();
//#endif


 ////////////////////////////////////////////////////////////////////////////


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {


//////////////////////////////////////////

//
//#if !GH_MODO_PRUEBA_MENU
//    /* 1. Recepción de paquetes reales desde el STM32 Maestro (Modo Normal) */
//    {
//      GH_Paquete pk;
//      while (GH_Enlace_Leer(&pk)) {
//        if (!GH_Menu_Paquete(pk.b)) {
//          GH_Pista_Paquete(pk.b, pk.t_ms);
//        }
//      }
//    }
//#else
//    /* PUERTA TRASERA ACTIVA: Simulación autónoma del menú */
//    {
//      static uint32_t tMenuTest = 0;
//      static uint8_t opcionMock = 0;
//      static uint8_t textoInyectado = 0;
//      static uint8_t etapaSimulacion = 0; // 0 = Pantalla Inicio, 1 = Lista Canciones
//      static uint32_t delayActual = 0;
//
//      // Inyección de nombres de canciones para la simulación
//      if (!textoInyectado) {
//
//          const char *cancionesPrueba[3] = {
//              "ANIMALS",
//              "MIGHTY MORPHIN POWER RANGERS",
//              "THE NIGHT BEGINS TO SHINE"
//          };
//
//          // Enviar los nombres en paquetes de 3 caracteres
//          for (uint8_t fila = 0; fila < 3; fila++) {
//
//              const char *nombre = cancionesPrueba[fila];
//              uint8_t longitud = (uint8_t)strlen(nombre);
//
//              for (uint8_t pos = 0; pos <= longitud; pos += 3) {
//
//                  uint8_t pkt[8] = {
//                      0xA5, 0x09, fila, pos,
//                      0, 0, 0, 0
//                  };
//
//                  // Colocar hasta 3 caracteres por paquete
//                  for (uint8_t k = 0; k < 3; k++) {
//                      if (pos + k < longitud) {
//                          pkt[4 + k] = nombre[pos + k];
//                      }
//                  }
//
//                  // Checksum del protocolo
//                  pkt[7] = pkt[1] ^ pkt[2] ^ pkt[3] ^
//                           pkt[4] ^ pkt[5] ^ pkt[6];
//
//                  GH_Menu_Paquete(pkt);
//              }
//          }
//
//          textoInyectado = 1;
//      }
//
//      // Máquina de estados de la simulación
//      if (HAL_GetTick() - tMenuTest >= delayActual) {
//        tMenuTest = HAL_GetTick();
//
//        if (etapaSimulacion == 0) {
//            // Mostrar la Pantalla de Inicio (Título)
//            uint8_t paquete_inicio[8] = { 0xA5, 0x08, 4, 0, 0, 0, 0, 0 }; // 4 = GH_PANT_INICIO
//            GH_Menu_Paquete(paquete_inicio);
//
//            etapaSimulacion = 1; // La próxima vez pasamos a canciones
//            delayActual = 3000;  // Esperamos 3 segundos viendo el título
//        }
//        else if (etapaSimulacion == 1) {
//            // Mostrar el Menú de Canciones
//            uint8_t paquete_menu[8] = { 0xA5, 0x08, 1, opcionMock, 3, 0, 0, 0 }; // 1 = GH_PANT_CANCIONES
//            GH_Menu_Paquete(paquete_menu);
//
//            opcionMock = (opcionMock + 1) % 3; // Alternar opciones
//            delayActual = 1500; // Cambiar cada 1.5 segundos
//        }
//      }
//    }
//#endif
//
//#if GH_PRUEBA_SIN_SPI && !GH_MODO_PRUEBA_MENU
//    /* Prueba de notas cayendo sin SPI */
//    {
//      static uint32_t tPrueba = 0;
//      static uint8_t  iPrueba = 0;
//      if (HAL_GetTick() - tPrueba >= GH_PRUEBA_CADA_MS) {
//        tPrueba = HAL_GetTick();
//        GH_Pista_AgregarNota(iPrueba % GH_NUM_CARRILES, iPrueba & 0x3F, tPrueba + GH_ANTICIPACION_MS,
//                             (iPrueba % 4 == 3) ? 600 : 0);
//        iPrueba++;
//      }
//    }
//#endif

////////////////////////////////////////////



//		/* 1. Paquetes de MusicaGame (notas, aciertos / fallos, botones, menús) */
//		{
//			GH_Paquete pk;
//			while (GH_Enlace_Leer(&pk))
	//			if (!GH_Menu_Paquete(pk.b)) GH_Pista_Paquete(pk.b, pk.t_ms);   /* menús primero; lo demás es de la pista */
	//	}

//#if GH_PRUEBA_SIN_SPI
		/* Prueba: notas inventadas, una por carril en orden, cada 4 una larga */
	//	{
		//	static uint32_t tPrueba = 0;
			//static uint8_t  iPrueba = 0;
			//if (HAL_GetTick() - tPrueba >= GH_PRUEBA_CADA_MS) {
				//tPrueba = HAL_GetTick();
				//GH_Pista_AgregarNota(iPrueba % GH_NUM_CARRILES, iPrueba & 0x3F, tPrueba + GH_ANTICIPACION_MS,
				          //           (iPrueba % 4 == 3) ? 600 : 0);
				//iPrueba++;
			//}
		//}
//#endif

		/* 2. Mover la pista y dibujar los botones */
		/* ====================================================
		 * RECEPCION DE PAQUETES DEL STM32 MAESTRO
		 * ==================================================== */

		{
		    GH_Paquete pk;

		    while (GH_Enlace_Leer(&pk)) {

		        /* Primero procesar los paquetes del menu */
		        if (!GH_Menu_Paquete(pk.b)) {

		            /* Si no es un paquete del menu,
		               pertenece a la pista de juego */
		            GH_Pista_Paquete(pk.b, pk.t_ms);
		        }
		    }
		}

		/* Actualizar texto desplazable del menu */
		GH_Menu_Actualizar();

		/* Actualizar pista y notas cuando corresponda */
		GH_Pista_Actualizar();


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

//
//				for (int x = 0; x < 319-42; x++) {
//					int anim = (x/10)%4;
//					// anim 0 1 2 3
//					LCD_Sprite(x, 116-29, 42, 29, link, 4, anim, 0, 0);
//					//V_line( x -1, 100, 50, 0x0DFE);
//					HAL_Delay(15);
//
//				}
//				for (int var = 319-24; var > 0;  var--) {
//					int anim = (var / 10) % 4;
//					LCD_Sprite(var, 100, 24, 30, sonics, 4, anim, 1, 0);
//					V_line(var + 24, 100, 30, 0x0DFE);
//					HAL_Delay(15);
//				}
//
//		for (int x = 0; x < 319-24; x++) {
//			int anim = (x/10)%4;
//			// anim 0 1 2 3
//			LCD_Sprite(x, 100, 24, 30, sonics, 4, anim, 0, 0);
//			V_line( x -1, 100, 30, 0x0DFE);
//			HAL_Delay(15);
//
//		}
//		for (int var = 319-24; var > 0;  var--) {
//			int anim = (var / 10) % 4;
//			LCD_Sprite(var, 100, 24, 30, sonics, 4, anim, 1, 0);
//			V_line(var + 24, 100, 30, 0x0DFE);
//			HAL_Delay(15);
//		}

//		for (int var = 0; var < 319-26;  var++) {
//			 int anim = (var / 5) % 4;
//			LCD_Sprite(var,100,26,16,link,4,anim,0,0);
//			V_line( var -1, 100, 16, 0x4d9e);
//			 HAL_Delay(15);
//		}
//		for (int var = 319-26; var > 0;  var--) {
//					 int anim = (var / 5) % 4;
//					LCD_Sprite(var,100,26,16,link,4,anim,1,0);
//					V_line( var +27, 100, 16, 0x4d9e);
//					 HAL_Delay(15);
//				}
//
//		for (int x = 0; x < 320 - 16; x++) {
//		    int anim2 = (x / 10) % 4;
//		    LCD_Sprite(x,100,16,16,kirbys,4,anim2,0,0);
//		    V_line( x -1, 100, 16, 0x74DA);
//		    HAL_Delay(15);
//		  }
//		  for (int x = 320-16; x > 0; x--) {
//		    int anim2 = (x / 10) % 4;
//		    LCD_Sprite(x,100,16,16,kirbys,4,anim2,1,0);
//		    V_line( x +16, 100, 16, 0x74DA);
//		    HAL_Delay(15);
//		  }




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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin|LCD_D1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_D1_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RD_Pin LCD_WR_Pin LCD_RS_Pin LCD_D7_Pin
                           LCD_D0_Pin LCD_D2_Pin */
  GPIO_InitStruct.Pin = LCD_RD_Pin|LCD_WR_Pin|LCD_RS_Pin|LCD_D7_Pin
                          |LCD_D0_Pin|LCD_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : bverde_Pin brojo_Pin bamarillo_Pin bazul_Pin */
  GPIO_InitStruct.Pin = bverde_Pin|brojo_Pin|bamarillo_Pin|bazul_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_D6_Pin LCD_D3_Pin LCD_D5_Pin
                           LCD_D4_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_D6_Pin|LCD_D3_Pin|LCD_D5_Pin
                          |LCD_D4_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
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
	while (1) {
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
