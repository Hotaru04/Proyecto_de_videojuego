/*
 * songs.c
 * Tabla de canciones. Las filas las genera tools/registrar_canciones.py en canciones_tabla.h
 * (cada canción existe una sola vez en flash porque sus .h solo se incluyen aquí).
 */
#include "songs.h"
#include "synth.h"

/* Una fila: canción, título del menú, pistas del jugador 1 (canal 1), pistas del jugador 2 (canal 2), volumen maestro */
#define FILA(song, titulo, c1, n1, c2, n2, master)  { &(song), (c1), (n1), (master), (c2), (n2), (titulo) }

#include "canciones_tabla.h"

const SongEntry song_table[] = {
	CANCIONES_FILAS
};

const uint8_t SONG_COUNT = sizeof(song_table) / sizeof(song_table[0]);
