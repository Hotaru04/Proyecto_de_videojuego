/*
 * enlace_spi.h
 * SPI2 maestro hacia las pantallas: PB13 SCK, PB15 MOSI, PB12 CS J1, PB14 CS J2.
 * Por registros (no activar SPI2 en CubeMX).
 */
#ifndef INC_ENLACE_SPI_H_
#define INC_ENLACE_SPI_H_

#include <stdint.h>

#define ENLACE_J1     0x01
#define ENLACE_J2     0x02
#define ENLACE_AMBOS  (ENLACE_J1 | ENLACE_J2)
#define ENLACE_JUGADOR(j)  ((uint8_t)(1u << (j)))

void Enlace_Init(void);
void Enlace_Enviar(uint8_t destinos, uint8_t tipo, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);

extern volatile uint32_t enlace_paquetes;

#endif /* INC_ENLACE_SPI_H_ */
