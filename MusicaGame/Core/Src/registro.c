/*
 * registro.c
 * USART2 TX por interrupción con buffer circular.
 */
#include "registro.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

#define BUF_N    2048u          /* potencia de 2 */
#define BAUDIOS  115200u

static char buf[BUF_N];
static volatile uint32_t w = 0, r = 0;
volatile uint32_t registro_perdidos = 0;

void Registro_Init(void) {
#if REGISTRO_ACTIVO
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	(void)RCC->APB1ENR;
	USART2->CR1 = 0;
	USART2->BRR = (HAL_RCC_GetPCLK1Freq() + BAUDIOS / 2u) / BAUDIOS;
	USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
	HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
#endif
}

void USART2_IRQHandler(void) {
	if ((USART2->SR & USART_SR_TXE) && (USART2->CR1 & USART_CR1_TXEIE)) {
		if (r != w) {
			USART2->DR = (uint8_t)buf[r];
			r = (r + 1) & (BUF_N - 1);
		} else {
			USART2->CR1 &= ~USART_CR1_TXEIE;
		}
	}
}

static void Poner(const char *s, uint32_t n) {
	__disable_irq();
	uint32_t libre = (r - w - 1) & (BUF_N - 1);
	if (n > libre) { registro_perdidos++; __enable_irq(); return; }
	for (uint32_t i = 0; i < n; i++) { buf[w] = s[i]; w = (w + 1) & (BUF_N - 1); }
	USART2->CR1 |= USART_CR1_TXEIE;
	__enable_irq();
}

void Registro(const char *fmt, ...) {
#if REGISTRO_ACTIVO
	char linea[128];
	uint32_t t = HAL_GetTick();
	int n = snprintf(linea, sizeof(linea), "[%5lu.%03lu] ", (unsigned long)(t / 1000u), (unsigned long)(t % 1000u));
	va_list ap;
	va_start(ap, fmt);
	n += vsnprintf(linea + n, sizeof(linea) - (uint32_t)n - 2u, fmt, ap);
	va_end(ap);
	if (n > (int)sizeof(linea) - 3) n = (int)sizeof(linea) - 3;
	linea[n++] = '\r';
	linea[n++] = '\n';
	Poner(linea, (uint32_t)n);
#else
	(void)fmt;
#endif
}
