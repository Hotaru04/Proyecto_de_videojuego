/*
 * sd_spi.c
 * microSD (módulo HW-203) por SPI3 para FatFs, solo lectura.
 * SCK PC10, MISO PC11, MOSI PC12, CS PD2. Por registros (no activar SPI3 en CubeMX).
 * Inicializa a ~312 kHz y luego lee a 10 MHz. Tarjetas SDSC y SDHC en FAT32.
 */
#include "ff.h"
#include "diskio.h"
#include "main.h"

#define CS_BAJO()  (GPIOD->BSRR = GPIO_BSRR_BR2)
#define CS_ALTO()  (GPIOD->BSRR = GPIO_BSRR_BS2)

#define SPI_LENTO  (6u << SPI_CR1_BR_Pos)    /* 40 MHz / 128 = 312 kHz */
#define SPI_RAPIDO (1u << SPI_CR1_BR_Pos)    /* 40 MHz / 4   = 10 MHz  */

static volatile DSTATUS estado = STA_NOINIT;
static uint8_t es_sdhc = 0;                  /* 1 = se direcciona por bloque, 0 = por byte */

volatile uint32_t sd_lecturas = 0, sd_errores = 0;

static uint8_t Byte(uint8_t b) {
	while (!(SPI3->SR & SPI_SR_TXE)) { }
	*(volatile uint8_t *)&SPI3->DR = b;
	while (!(SPI3->SR & SPI_SR_RXNE)) { }
	return *(volatile uint8_t *)&SPI3->DR;
}

static void Velocidad(uint32_t br) {
	SPI3->CR1 &= ~SPI_CR1_SPE;
	SPI3->CR1 = (SPI3->CR1 & ~SPI_CR1_BR) | br;
	SPI3->CR1 |= SPI_CR1_SPE;
}

static void Pines(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
	(void)RCC->APB1ENR;
	/* PC10 SCK, PC11 MISO, PC12 MOSI: AF6 */
	GPIOC->MODER = (GPIOC->MODER & ~((3u << 20) | (3u << 22) | (3u << 24))) | (2u << 20) | (2u << 22) | (2u << 24);
	GPIOC->AFR[1] = (GPIOC->AFR[1] & ~((0xFu << 8) | (0xFu << 12) | (0xFu << 16))) | (6u << 8) | (6u << 12) | (6u << 16);
	GPIOC->OSPEEDR |= (2u << 20) | (2u << 24);
	GPIOC->PUPDR = (GPIOC->PUPDR & ~(3u << 22)) | (1u << 22);          /* MISO con pull-up */
	/* PD2 CS: salida, en alto */
	CS_ALTO();
	GPIOD->MODER = (GPIOD->MODER & ~(3u << 4)) | (1u << 4);
	GPIOD->OSPEEDR |= (2u << 4);
	/* SPI3 maestro, modo 0, 8 bits, NSS por software */
	SPI3->CR1 = 0;
	SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_LENTO;
	SPI3->CR1 |= SPI_CR1_SPE;
}

/* Espera a que la tarjeta suelte la línea (0xFF). 1 = lista */
static uint8_t Esperar_Lista(uint32_t ms) {
	uint32_t t0 = HAL_GetTick();
	do { if (Byte(0xFF) == 0xFF) return 1; } while (HAL_GetTick() - t0 < ms);
	return 0;
}

static void Soltar(void) { CS_ALTO(); Byte(0xFF); }

static uint8_t Seleccionar(void) {
	CS_BAJO(); Byte(0xFF);
	if (Esperar_Lista(500)) return 1;
	Soltar();
	return 0;
}

/* Manda un comando y devuelve la respuesta R1 */
static uint8_t Comando(uint8_t cmd, uint32_t arg) {
	if (cmd & 0x80) {                                   /* ACMDn = CMD55 + CMDn */
		cmd &= 0x7F;
		uint8_t r = Comando(55, 0);
		if (r > 1) return r;
	}
	if (cmd != 12) { Soltar(); if (!Seleccionar()) return 0xFF; }
	Byte(0x40 | cmd);
	Byte((uint8_t)(arg >> 24)); Byte((uint8_t)(arg >> 16)); Byte((uint8_t)(arg >> 8)); Byte((uint8_t)arg);
	Byte(cmd == 0 ? 0x95 : cmd == 8 ? 0x87 : 0x01);     /* CRC válido solo hace falta en CMD0 y CMD8 */
	if (cmd == 12) Byte(0xFF);
	uint8_t r, n = 10;
	do { r = Byte(0xFF); } while ((r & 0x80) && --n);
	return r;
}

static uint8_t Leer_Bloque(uint8_t *buf, uint32_t n) {
	uint32_t t0 = HAL_GetTick();
	uint8_t tok;
	do { tok = Byte(0xFF); } while (tok == 0xFF && HAL_GetTick() - t0 < 200);
	if (tok != 0xFE) return 0;                          /* 0xFE = empieza el bloque de datos */
	while (n--) *buf++ = Byte(0xFF);
	Byte(0xFF); Byte(0xFF);                             /* CRC (no se revisa) */
	return 1;
}

/* Interfaz de FatFs */
DSTATUS disk_status(BYTE pdrv) { return pdrv ? STA_NOINIT : estado; }

DSTATUS disk_initialize(BYTE pdrv) {
	if (pdrv) return STA_NOINIT;
	Pines();
	Velocidad(SPI_LENTO);
	CS_ALTO();
	for (int i = 0; i < 10; i++) Byte(0xFF);            /* 80 ciclos de reloj con CS en alto */

	estado = STA_NOINIT;
	es_sdhc = 0;
	if (Comando(0, 0) == 1) {                           /* a modo SPI */
		uint32_t t0 = HAL_GetTick();
		if (Comando(8, 0x1AA) == 1) {                   /* SD v2 */
			uint8_t ocr[4];
			for (int i = 0; i < 4; i++) ocr[i] = Byte(0xFF);
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
				while (HAL_GetTick() - t0 < 1000 && Comando(0x80 | 41, 1UL << 30)) { }
				if (HAL_GetTick() - t0 < 1000 && Comando(58, 0) == 0) {
					for (int i = 0; i < 4; i++) ocr[i] = Byte(0xFF);
					es_sdhc = (ocr[0] & 0x40) ? 1 : 0;
					estado = 0;
				}
			}
		} else {                                         /* SD v1 */
			while (HAL_GetTick() - t0 < 1000 && Comando(0x80 | 41, 0)) { }
			if (HAL_GetTick() - t0 < 1000 && Comando(16, 512) == 0) estado = 0;
		}
	}
	Soltar();
	if (estado == 0) Velocidad(SPI_RAPIDO);
	else sd_errores++;
	return estado;
}

DRESULT disk_read(BYTE pdrv, BYTE *buf, LBA_t sector, UINT count) {
	if (pdrv || !count) return RES_PARERR;
	if (estado & STA_NOINIT) return RES_NOTRDY;
	if (!es_sdhc) sector *= 512;
	DRESULT res = RES_OK;
	while (count--) {
		if (Comando(17, (uint32_t)sector) != 0 || !Leer_Bloque(buf, 512)) { res = RES_ERROR; sd_errores++; break; }
		buf += 512;
		sector += es_sdhc ? 1 : 512;
		sd_lecturas++;
	}
	Soltar();
	return res;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
	(void)buff;
	if (pdrv) return RES_PARERR;
	if (estado & STA_NOINIT) return RES_NOTRDY;
	if (cmd == CTRL_SYNC) return RES_OK;
	return RES_PARERR;
}
