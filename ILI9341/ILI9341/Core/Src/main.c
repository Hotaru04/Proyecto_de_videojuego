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

// Para el uart3
#include <stdio.h>
#include <string.h>
#include <stdbool.h>      // Necesario para usar 'bool'
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* [GH] 1 = probar la pantalla SIN MusicaGame: inventa notas cada GH_PRUEBA_CADA_MS */
#define GH_PRUEBA_SIN_SPI   0
#define GH_PRUEBA_CADA_MS   350

/* [UART] Cola de comandos: si llegan varios mientras se dibuja, no se pisan */
#define UART_COLA_N         16    /* potencia de 2 */
#define UART_LINEA_MAX      30

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

volatile uint8_t rx_byte;
char rx_buffer[UART_LINEA_MAX];
volatile uint8_t rx_index = 0;

/* [UART] Comandos completos esperando a ser procesados (los llena la interrupción) */
static char     uart_cola[UART_COLA_N][UART_LINEA_MAX];
static uint32_t uart_cola_t[UART_COLA_N];          /* HAL_GetTick() cuando llegó el '\n' */
static volatile uint8_t uart_w = 0, uart_r = 0;
volatile uint32_t uart_perdidos = 0, uart_errores = 0;   /* diagnóstico */

/* Botones virtuales del Jugador 1 controlados desde el ESP32 */
volatile uint8_t p1_btn_verde = 0;    // Asignado al botón A
volatile uint8_t p1_btn_rojo = 0;     // Asignado al botón B
volatile uint8_t p1_btn_amarillo = 0; // Asignado al botón X
volatile uint8_t p1_btn_azul = 0;     // Asignado al botón Y

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
// Lógica de traducción de comandos UART a Botones Virtuales
void ProcesarComandoESP32(char* comando) {
    bool presionado = true;

    /* Detectar si se soltó el botón */
    if (strstr(comando, "_off") != NULL) {
        presionado = false;
    }

    /* Asignar a variables si el comando es para el Jugador 1 (p1_) */
    if (strncmp(comando, "p1_", 3) == 0) {
        if (strstr(comando, "A") != NULL && !strstr(comando, "Down")) {
            p1_btn_verde = presionado;
        }
        else if (strstr(comando, "B") != NULL) {
            p1_btn_rojo = presionado;
        }
        else if (strstr(comando, "X") != NULL) {
            p1_btn_amarillo = presionado;
        }
        else if (strstr(comando, "Y") != NULL) {
            p1_btn_azul = presionado;
        }
    }
}

/* [UART] Saca un comando de la cola. Devuelve 1 si había. */
static uint8_t UART_Leer(char *linea, uint32_t *t) {
	if (uart_r == uart_w) return 0;
	strcpy(linea, uart_cola[uart_r]);
	*t = uart_cola_t[uart_r];
	uart_r = (uint8_t)((uart_r + 1) & (UART_COLA_N - 1));
	return 1;
}

/* [GH] Estado de los 4 botones del jugador 1 (lo que deja ProcesarComandoESP32) */
static void Botones_Leer(uint8_t b[GH_NUM_CARRILES]) {
	b[0] = p1_btn_verde; b[1] = p1_btn_rojo; b[2] = p1_btn_amarillo; b[3] = p1_btn_azul;
}

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
    HAL_UART_Receive_IT(&huart3, (uint8_t*)&rx_byte, 1);


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

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		/* 1. Comandos del ESP32 (todos los que haya en la cola) */
		{
			char linea[UART_LINEA_MAX];
			uint32_t t;
			uint8_t antes[GH_NUM_CARRILES], ahora[GH_NUM_CARRILES];
			while (UART_Leer(linea, &t)) {
				Botones_Leer(antes);
				ProcesarComandoESP32(linea);
				Botones_Leer(ahora);
				/* flanco: solo cuenta el momento de presionar (dejarlo presionado no atrapa notas) */
				for (uint8_t c = 0; c < GH_NUM_CARRILES; c++)
					if (ahora[c] && !antes[c]) GH_Pista_Presionar(c, t);
			}
		}

		/* 2. Notas que manda MusicaGame por SPI */
		{
			GH_Paquete pk;
			while (GH_Enlace_Leer(&pk)) GH_Pista_Paquete(pk.b, pk.t_ms);
		}

#if GH_PRUEBA_SIN_SPI
		/* Prueba: notas inventadas, una por carril en orden, cada 4 una larga */
		{
			static uint32_t tPrueba = 0;
			static uint8_t  iPrueba = 0;
			if (HAL_GetTick() - tPrueba >= GH_PRUEBA_CADA_MS) {
				tPrueba = HAL_GetTick();
				GH_Pista_AgregarNota(iPrueba % GH_NUM_CARRILES, tPrueba + GH_ANTICIPACION_MS,
				                     (iPrueba % 4 == 3) ? 600 : 0);
				iPrueba++;
			}
		}
#endif

		/* 3. Mover la pista, aciertos / fallos y botones */
		{
			uint8_t b[GH_NUM_CARRILES];
			Botones_Leer(b);
			GH_Pista_Actualizar(b);
		}


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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        if (rx_byte == '\n') {
            rx_buffer[rx_index] = '\0'; // Terminar la cadena
            /* [UART] Copiar el comando a la cola (el while(1) lo procesa) */
            uint8_t sig = (uint8_t)((uart_w + 1) & (UART_COLA_N - 1));
            if (sig != uart_r) {
                memcpy(uart_cola[uart_w], rx_buffer, rx_index + 1);
                uart_cola_t[uart_w] = HAL_GetTick();
                uart_w = sig;
            } else {
                uart_perdidos++;        // cola llena
            }
            rx_index = 0;               // Reiniciar el índice
        }
        else if (rx_byte != '\r' && rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = rx_byte;
        }
        // Re-armar la interrupción para escuchar el próximo byte
        HAL_UART_Receive_IT(&huart3, (uint8_t*)&rx_byte, 1);
    }
}

/* [UART] Si hay un error (ruido, overrun...) HAL deja de recibir: volver a armar la recepción */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        uart_errores++;
        rx_index = 0;
        HAL_UART_Receive_IT(&huart3, (uint8_t*)&rx_byte, 1);
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
