/*
 * canciones_sd.h
 * Canciones .ghs de la microSD (FAT32), en /CANCIONES o en la raíz, ordenadas por nombre.
 * Al elegir una se copia completa a la RAM; durante el juego no se lee la tarjeta.
 *
 * Formato .ghs (little-endian):
 *   0   "GHS1"
 *   4   u16 bpm
 *   6   u8  número de pistas
 *   7   u8  pistas del jugador 1, 8  u8[8] índices
 *   16  u8  pistas del jugador 2, 17 u8[8] índices
 *   25  u8  reservado
 *   26  u16 volumen maestro (0 = normal)
 *   28  char[32] título
 *   60  por pista: u8 instrumento, u8 volumen, u16 número de notas
 *   ... notas: u8 nota, u16 duración, u8 velocidad, u8 flags
 */
#ifndef INC_CANCIONES_SD_H_
#define INC_CANCIONES_SD_H_

#include <stdint.h>
#include "songs.h"

#define CANCIONES_SD_MAX        32
#define CANCIONES_SD_MAX_NOTAS  14000    /* 84 KB de RAM; igual a MAX_NOTAS en registrar_canciones.py */

uint8_t          CancionesSD_Iniciar(void);              /* devuelve cuántas canciones hay */
uint8_t          CancionesSD_Total(void);
uint8_t          CancionesSD_Montada(void);
const char      *CancionesSD_Titulo(uint8_t i);
const SongEntry *CancionesSD_Cargar(uint8_t i);          /* 0 si falló */

extern volatile uint32_t sd_lecturas, sd_errores;

#endif /* INC_CANCIONES_SD_H_ */
