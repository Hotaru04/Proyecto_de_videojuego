/*
 * enlace_spi.h
 * SPI2 maestro hacia las pantallas (un STM por jugador). Solo transmite. Ver gh_protocolo.h.
 *   PB13 SCK · PB15 MOSI · PB12 CS jugador 1 · PB14 CS jugador 2 (activos en bajo)
 *
 * Por registros a propósito: no depende de CubeMX. NO actives SPI2 en CubeMX.
 */
#ifndef INC_ENLACE_SPI_H_
#define INC_ENLACE_SPI_H_

#include <stdint.h>

#define ENLACE_J1     0x01
#define ENLACE_J2     0x02
#define ENLACE_AMBOS  (ENLACE_J1 | ENLACE_J2)
#define ENLACE_JUGADOR(j)  ((uint8_t)(1u << (j)))   /* j = 0 jugador 1, j = 1 jugador 2 */

void Enlace_Init(void);
/* Manda un paquete de GH_PKT_LEN bytes a las pantallas indicadas (agrega SYNC y checksum). ~60 us. */
void Enlace_Enviar(uint8_t destinos, uint8_t tipo, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4);

extern volatile uint32_t enlace_paquetes;   /* diagnóstico */

#endif /* INC_ENLACE_SPI_H_ */
