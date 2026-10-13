/*
 * gh_enlace.c
 * SPI2 esclavo por registros. La interrupción guarda cada paquete válido en una cola.
 */
#include "gh_enlace.h"
#include "main.h"

#define COLA_N 32                         /* potencia de 2 */

static GH_Paquete cola[COLA_N];
static volatile uint8_t cola_w = 0, cola_r = 0;

static uint8_t rx[GH_PKT_LEN];
static uint8_t rx_i = 0;

volatile uint32_t enlace_ok = 0, enlace_malos = 0, enlace_perdidos = 0;

void GH_Enlace_Iniciar(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	(void)RCC->APB1ENR;

	/* PB12 NSS, PB13 SCK, PB15 MOSI: AF5 */
	GPIOB->MODER = (GPIOB->MODER & ~((3u << (12 * 2)) | (3u << (13 * 2)) | (3u << (15 * 2))))
	             | (2u << (12 * 2)) | (2u << (13 * 2)) | (2u << (15 * 2));
	GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((0xFu << ((12 - 8) * 4)) | (0xFu << ((13 - 8) * 4)) | (0xFu << ((15 - 8) * 4))))
	              | (5u << ((12 - 8) * 4)) | (5u << ((13 - 8) * 4)) | (5u << ((15 - 8) * 4));
	/* NSS con pull-up para no recibir basura si MusicaGame no está conectado */
	GPIOB->PUPDR = (GPIOB->PUPDR & ~(3u << (12 * 2))) | (1u << (12 * 2));

	/* esclavo, modo 0, 8 bits, NSS por hardware, solo recepción */
	SPI2->CR1 = 0;
	SPI2->CR2 = 0;
	SPI2->CR1 = SPI_CR1_RXONLY;
	SPI2->CR2 = SPI_CR2_RXNEIE | SPI_CR2_ERRIE;
	SPI2->CR1 |= SPI_CR1_SPE;

	HAL_NVIC_SetPriority(SPI2_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

void SPI2_IRQHandler(void) {
	uint32_t sr = SPI2->SR;
	if (sr & SPI_SR_OVR) {                       /* se perdió un byte */
		(void)SPI2->DR; (void)SPI2->SR;
		rx_i = 0;
		enlace_malos++;
		return;
	}
	if (!(sr & SPI_SR_RXNE)) return;

	uint8_t b = (uint8_t)SPI2->DR;
	if (rx_i == 0 && b != GH_PKT_SYNC) return;
	rx[rx_i++] = b;
	if (rx_i < GH_PKT_LEN) return;
	rx_i = 0;

	if (rx[7] != GH_Checksum(rx)) { enlace_malos++; return; }

	uint8_t sig = (uint8_t)((cola_w + 1) & (COLA_N - 1));
	if (sig == cola_r) { enlace_perdidos++; return; }
	for (uint32_t i = 0; i < GH_PKT_LEN; i++) cola[cola_w].b[i] = rx[i];
	cola[cola_w].t_ms = HAL_GetTick();
	cola_w = sig;
	enlace_ok++;
}

uint8_t GH_Enlace_Leer(GH_Paquete *p) {
	if (cola_r == cola_w) return 0;
	*p = cola[cola_r];
	cola_r = (uint8_t)((cola_r + 1) & (COLA_N - 1));
	return 1;
}
