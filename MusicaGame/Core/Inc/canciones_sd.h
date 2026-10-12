/*
 * canciones_sd.h
 * Canciones guardadas en la microSD (archivos .ghs que genera tools/registrar_canciones.py --sd).
 *
 * En la tarjeta (FAT32):   /CANCIONES/animals.ghs, /CANCIONES/the_night_begins_to_shine.ghs, ...
 * (si no existe la carpeta CANCIONES, se buscan en la raíz). El menú las ordena por nombre de archivo.
 * Es la ÚNICA fuente de canciones: en la flash no hay ninguna.
 *
 * Al elegir una canción se copia COMPLETA de la SD a la RAM (las notas ocupan 6 bytes cada una) y se
 * toca desde ahí: durante el juego no se lee la tarjeta.
 *
 * Formato .ghs (little-endian):
 *   0   "GHS1"
 *   4   u16 bpm
 *   6   u8  pistas (máx. 16)
 *   7   u8  pistas del jugador 1 (n1)      8   u8[8] índices
 *   16  u8  pistas del jugador 2 (n2)      17  u8[8] índices
 *   25  u8  reservado                      26  u16 volumen maestro (0 = normal)
 *   28  char[32] título (terminado en 0)
 *   60  por pista: u8 instrumento, u8 volumen, u16 cantidad de notas
 *   ... notas de todas las pistas en orden: u8 nota, u16 duración, u8 velocidad, u8 flags (5 bytes c/u)
 */
#ifndef INC_CANCIONES_SD_H_
#define INC_CANCIONES_SD_H_

#include <stdint.h>
#include "songs.h"

#define CANCIONES_SD_MAX        32       /* canciones en el menú */
#define CANCIONES_SD_MAX_NOTAS  14000    /* notas de la canción cargada (84 KB de RAM); igual a MAX_NOTAS del .py */

uint8_t          CancionesSD_Iniciar(void);              /* monta la tarjeta y lista las canciones; devuelve cuántas */
uint8_t          CancionesSD_Total(void);
uint8_t          CancionesSD_Montada(void);              /* 1 = la tarjeta se leyó (aunque no tenga canciones) */
const char      *CancionesSD_Titulo(uint8_t i);
const SongEntry *CancionesSD_Cargar(uint8_t i);          /* la copia a RAM; 0 si falló (ver la bitácora) */

extern volatile uint32_t sd_lecturas, sd_errores;

#endif /* INC_CANCIONES_SD_H_ */
