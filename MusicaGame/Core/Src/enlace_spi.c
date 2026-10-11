/*
 * enlace_spi.c
 * SPI2 maestro, solo transmisión, por registros. Ver enlace_spi.h y gh_protocolo.h.
 */
#include "enlace_spi.h"
#include "gh_protocolo.h"
#include "main.h"

/* CS: PB12 = jugador 1, PB14 = jugador 2 (activos en bajo) */
#define CS_PINES(dest)  ((((dest) & ENLACE_J1) ? GPIO_BSRR_BS12 : 0u) | (((dest) & ENLACE_J2) ? GPIO_BSRR_BS14 : 0u))
#define CS_BAJO(dest)   (GPIOB->BSRR = CS_PINES(dest) << 16)
#define CS_ALTO()       (GPIOB->BSRR = GPIO_BSRR_BS12 | GPIO_BSRR_BS14)

/* Separación mínima entre paquetes: le da tiempo al esclavo de volver a armar su recepción */
#define ENLACE_PAUSA_US  40u

volatile uint32_t enlace_paquetes = 0;
static uint32_t ultimo_fin = 0;   /* DWT->CYCCNT al terminar el último paquete */

static void Esperar_us(uint32_t us) {
	uint32_t t0 = DWT->CYCCNT, ciclos = (SystemCoreClock / 1000000u) * us;
	while (DWT->CYCCNT - t0 < ciclos) { }
}

void Enlace_Init(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	(void)RCC->APB1ENR;                                   /* espera a que el reloj quede activo */

	/* PB12 y PB14: salidas push-pull, empiezan en alto (ninguna pantalla seleccionada) */
	CS_ALTO();
	GPIOB->MODER   = (GPIOB->MODER & ~((3u << (12 * 2)) | (3u << (14 * 2))))
	               | (1u << (12 * 2)) | (1u << (14 * 2));
	/* PB13 (SCK) y PB15 (MOSI): función alterna 5 = SPI2 */
	GPIOB->MODER   = (GPIOB->MODER & ~((3u << (13 * 2)) | (3u << (15 * 2))))
	               | (2u << (13 * 2)) | (2u << (15 * 2));
	GPIOB->AFR[1]  = (GPIOB->AFR[1] & ~((0xFu << ((13 - 8) * 4)) | (0xFu << ((15 - 8) * 4))))
	               | (5u << ((13 - 8) * 4)) | (5u << ((15 - 8) * 4));
	GPIOB->OSPEEDR |= (2u << (12 * 2)) | (2u << (13 * 2)) | (2u << (14 * 2)) | (2u << (15 * 2));   /* velocidad "fast" */

	/* DWT ya lo enciende Audio_Init, pero por si se llama antes */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/* SPI2: maestro, modo 0, 8 bits, MSB primero, NSS por software.
	 * APB1 = 40 MHz, BR = 100 (/32) -> 1.25 MHz */
	SPI2->CR1 = 0;
	SPI2->CR1 = SPI_CR1_MSTR | (4u << SPI_CR1_BR_Pos) | SPI_CR1_SSM | SPI_CR1_SSI;
	SPI2->CR1 |= SPI_CR1_SPE;
	ultimo_fin = DWT->CYCCNT;
}

void Enlace_Enviar(uint8_t destinos, uint8_t tipo, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) {
	uint8_t p[GH_PKT_LEN] = { GH_PKT_SYNC, tipo, d0, d1, d2, d3, d4, 0 };
	p[7] = GH_Checksum(p);

	/* respeta la pausa desde el paquete anterior */
	uint32_t pausa = (SystemCoreClock / 1000000u) * ENLACE_PAUSA_US;
	while (DWT->CYCCNT - ultimo_fin < pausa) { }

	if (!(destinos & ENLACE_AMBOS)) return;
	CS_BAJO(destinos);                                    /* con ENLACE_AMBOS las dos pantallas lo reciben */
	Esperar_us(2);                                        /* tiempo de preparación del esclavo */
	for (uint32_t i = 0; i < GH_PKT_LEN; i++) {
		while (!(SPI2->SR & SPI_SR_TXE)) { }
		*(volatile uint8_t *)&SPI2->DR = p[i];
	}
	while (!(SPI2->SR & SPI_SR_TXE)) { }
	while (SPI2->SR & SPI_SR_BSY) { }
	(void)SPI2->DR; (void)SPI2->SR;                       /* limpia OVR (no se lee MISO) */
	CS_ALTO();

	ultimo_fin = DWT->CYCCNT;
	enlace_paquetes++;
}
