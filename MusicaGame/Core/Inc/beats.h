/*
 * beats.h
 * Convierte las pistas de cada jugador (canal 1 = J1, canal 2 = J2) en notas del juego y las manda
 * a las pantallas GH_ANTICIPACION_MS antes de que suenen.
 */
#ifndef INC_BEATS_H_
#define INC_BEATS_H_

#include <stdint.h>
#include "songs.h"

#define BEATS_SEPARACION_MS   200   /* mínimo entre dos notas del mismo jugador */
#define BEATS_LARGA_MIN_MS    400   /* desde esta duración es nota larga */
#define BEATS_ACORDE_NOTAS    255   /* notas simultáneas que piden 2 botones (255 = nunca) */
#define BEATS_LATENCIA_MS     12    /* retraso del audio (~1.5 mitades de buffer) */
#define BEATS_MAX_PISTAS      8     /* pistas por jugador */

void Beats_Iniciar(const SongEntry *e, uint8_t indice);
void Beats_Detener(void);
void Beats_Previa(uint32_t faltan_ms);   /* antes de que arranque el audio */
void Beats_Actualizar(void);

extern volatile uint32_t beats_enviadas[2];
extern volatile uint32_t beats_tarde;

#endif /* INC_BEATS_H_ */
