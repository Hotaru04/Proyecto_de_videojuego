/*
 * songs.h
 * Canción lista para tocar, con las pistas de cada jugador (la arma canciones_sd.c).
 */
#ifndef INC_SONGS_H_
#define INC_SONGS_H_

#include <stdint.h>
#include "music.h"

typedef struct {
	const Song    *song;
	const uint8_t *p1_tracks;   /* pistas del jugador 1 */
	uint8_t        p1_n;
	uint16_t       master;      /* volumen maestro, 0 = el normal */
	const uint8_t *p2_tracks;   /* pistas del jugador 2 */
	uint8_t        p2_n;
	const char    *titulo;
} SongEntry;

#endif /* INC_SONGS_H_ */
