/*
 * songs.h
 * Tabla de canciones. El índice de la tabla elige qué canción suena.
 *
 * Para agregar una canción: en tools/midi_to_c_gui.py, botón "Agregar al juego"
 * (o copia sus .h a Core/Inc y corre  python tools/registrar_canciones.py).
 * Eso regenera canciones_tabla.h: los números quedan como CANCION_<NOMBRE>.
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

extern const SongEntry song_table[];
extern const uint8_t   SONG_COUNT;

#endif /* INC_SONGS_H_ */
