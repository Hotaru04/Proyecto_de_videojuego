/*
 * controles.c
 * USART1 RX por registros, cola de líneas y traducción de comandos a botones.
 */
#include "controles.h"
#include "main.h"
#include "registro.h"
#include <string.h>
#include <stdbool.h>

#define COLA_N      16          /* potencia de 2 */
#define LINEA_MAX   30
#define BAUDIOS     115200u

static char     cola[COLA_N][LINEA_MAX];
static uint32_t cola_t[COLA_N];
static volatile uint8_t cola_w = 0, cola_r = 0;

static char    rx_buffer[LINEA_MAX];
static uint8_t rx_index = 0;

static uint8_t botones[CONTROLES_JUGADORES];          /* bits 0-3 carriles, 4 Start, 5 arriba, 6 abajo */

/* Cambios de la última línea que faltan por reportar */
static uint8_t  pend_jugador, pend_antes, pend_ahora;
static uint32_t pend_t;
static uint8_t  pend_c = CONTROL_ABAJO + 1;            /* nada pendiente */

volatile uint32_t controles_lineas = 0, controles_perdidas = 0, controles_errores = 0;

void Controles_Init(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	(void)RCC->APB2ENR;

	/* PA10: USART1_RX (AF7) con pull-up */
	GPIOA->MODER = (GPIOA->MODER & ~(3u << (10 * 2))) | (2u << (10 * 2));
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(0xFu << ((10 - 8) * 4))) | (7u << ((10 - 8) * 4));
	GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << (10 * 2))) | (1u << (10 * 2));

	/* USART1 está en APB2 (80 MHz) */
	USART1->CR1 = 0;
	USART1->BRR = (HAL_RCC_GetPCLK2Freq() + BAUDIOS / 2u) / BAUDIOS;
	USART1->CR1 = USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

	HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void) {
	uint32_t sr = USART1->SR;
	uint8_t  b  = (uint8_t)USART1->DR;                     /* leer DR limpia ORE/NE/FE */
	if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE)) {
		controles_errores++;
		rx_index = 0;
		return;
	}
	if (!(sr & USART_SR_RXNE)) return;

	if (b == '\n') {
		rx_buffer[rx_index] = '\0';
		uint8_t sig = (uint8_t)((cola_w + 1) & (COLA_N - 1));
		if (sig != cola_r) {
			memcpy(cola[cola_w], rx_buffer, rx_index + 1u);
			cola_t[cola_w] = HAL_GetTick();
			cola_w = sig;
			controles_lineas++;
		} else {
			controles_perdidas++;
		}
		rx_index = 0;
	} else if (b != '\r' && rx_index < LINEA_MAX - 1) {
		rx_buffer[rx_index++] = (char)b;
	}
}

/* Comandos: pN_A, pN_B, pN_X, pN_Y, pN_Start, pN_Up, pN_Down (con _off al soltar) */
static void ProcesarComandoESP32(const char *comando) {
	bool presionado = true;
	if (strstr(comando, "_off") != NULL) presionado = false;

	uint8_t j;
	if      (strncmp(comando, "p1_", 3) == 0) j = 0;
	else if (strncmp(comando, "p2_", 3) == 0) j = 1;
	else return;

	int8_t c = -1;
	if (strstr(comando, "Start") != NULL)                              c = CONTROL_START;
	else if (strstr(comando, "_Up") != NULL)                           c = CONTROL_ARRIBA;
	else if (strstr(comando, "_Down") != NULL)                         c = CONTROL_ABAJO;
	else if (strstr(comando, "A") != NULL && !strstr(comando, "Down")) c = 0;   /* verde */
	else if (strstr(comando, "B") != NULL)                             c = 1;   /* rojo */
	else if (strstr(comando, "X") != NULL)                             c = 2;   /* amarillo */
	else if (strstr(comando, "Y") != NULL)                             c = 3;   /* azul */
	if (c < 0) return;

	if (presionado) botones[j] |= (uint8_t)(1u << c);
	else            botones[j] &= (uint8_t)~(1u << c);
}

uint8_t Controles_Siguiente(Control_Evento *ev) {
	for (;;) {
		/* cambios pendientes de la última línea */
		while (pend_c <= CONTROL_ABAJO) {
			uint8_t c = pend_c++;
			uint8_t m = (uint8_t)(1u << c);
			if ((pend_antes ^ pend_ahora) & m) {
				ev->jugador = pend_jugador;
				ev->carril = c;
				ev->presionado = (pend_ahora & m) ? 1 : 0;
				ev->t_ms = pend_t;
				return 1;
			}
		}
		/* siguiente línea de la cola */
		if (cola_r == cola_w) return 0;
		char linea[LINEA_MAX];
		strcpy(linea, cola[cola_r]);
		pend_t = cola_t[cola_r];
		cola_r = (uint8_t)((cola_r + 1) & (COLA_N - 1));

		uint8_t antes[CONTROLES_JUGADORES] = { botones[0], botones[1] };
		ProcesarComandoESP32(linea);
		Registro("ESP32: %s%s", linea,
		         (botones[0] == antes[0] && botones[1] == antes[1]) ? "   (no cambia ningun boton del juego)" : "");
		pend_jugador = (botones[0] != antes[0]) ? 0 : 1;
		pend_antes = antes[pend_jugador];
		pend_ahora = botones[pend_jugador];
		pend_c = 0;
	}
}

uint8_t Controles_Estado(uint8_t jugador) {
	return (jugador < CONTROLES_JUGADORES) ? (uint8_t)(botones[jugador] & 0x0F) : 0;
}
