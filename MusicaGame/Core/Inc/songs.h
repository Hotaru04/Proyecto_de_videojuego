/*
 * songs.h
 * SongEntry: una canción lista para tocar (la arma canciones_sd.c al cargarla de la microSD).
 * Las canciones ya NO están en la flash: ver Core/Inc/canciones_sd.h y tools/registrar_canciones.py.
 */
#ifndef INC_SONGS_H_
#define INC_SONGS_H_

#include <stdint.h>
#include "music.h"

typedef struct {
	const Song    *song;
	const uint8_t *p1_tracks;   /* pistas del jugador 1 (canal 1); NULL si la canción no tiene canales */
	uint8_t        p1_n;        /* cuántas pistas tiene el jugador 1 */
	uint16_t       master;      /* volumen maestro para esta canción (0 = el normal, 224) */
	const uint8_t *p2_tracks;   /* pistas del jugador 2 (canal 2); NULL si no tiene */
	uint8_t        p2_n;        /* cuántas pistas tiene el jugador 2 */
	const char    *titulo;      /* nombre que se muestra en el menú */
} SongEntry;


#endif /* INC_SONGS_H_ */
