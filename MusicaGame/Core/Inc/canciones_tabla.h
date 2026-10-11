/*
 * canciones_tabla.h
 * GENERADO por tools/registrar_canciones.py  ->  NO editar a mano (se sobrescribe).
 * Para agregar una canción: expórtala con midi_to_c_gui.py (botón "Agregar al juego")
 * o copia sus .h a Core/Inc y corre:  python tools/registrar_canciones.py
 */
#ifndef INC_CANCIONES_TABLA_H_
#define INC_CANCIONES_TABLA_H_

#include "song_animals.h"
#include "song_michael_jackson___beat_it.h"
#include "song_mighty_morphin_power_rangers.h"
#include "song_seven_nation_army.h"
#include "song_the_night_begins_to_shine.h"

/* Número de cada canción (para song_index en main.c) */
#define CANCION_ANIMALS                                  0
#define CANCION_MICHAEL_JACKSON___BEAT_IT                1
#define CANCION_MIGHTY_MORPHIN_POWER_RANGERS             2
#define CANCION_SEVEN_NATION_ARMY                        3
#define CANCION_THE_NIGHT_BEGINS_TO_SHINE                4
#define CANCIONES_TOTAL 5

/* FILA(cancion, titulo, pistas_j1, n_j1, pistas_j2, n_j2, volumen_maestro) -> la define songs.c */
#define CANCIONES_FILAS \
	FILA(song_animals, "ANIMALS", animals_canal1, SONG_ANIMALS_CANAL1_N, animals_canal2, SONG_ANIMALS_CANAL2_N, 0), \
	FILA(song_michael_jackson___beat_it, "MICHAEL JACKSON   BEAT IT", michael_jackson___beat_it_canal1, SONG_MICHAEL_JACKSON___BEAT_IT_CANAL1_N, michael_jackson___beat_it_canal2, SONG_MICHAEL_JACKSON___BEAT_IT_CANAL2_N, 0), \
	FILA(song_mighty_morphin_power_rangers, "MIGHTY MORPHIN POWER RANGERS", mighty_morphin_power_rangers_canal1, SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL1_N, mighty_morphin_power_rangers_canal2, SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL2_N, 0), \
	FILA(song_seven_nation_army, "SEVEN NATION ARMY", seven_nation_army_canal1, SONG_SEVEN_NATION_ARMY_CANAL1_N, seven_nation_army_canal2, SONG_SEVEN_NATION_ARMY_CANAL2_N, 0), \
	FILA(song_the_night_begins_to_shine, "THE NIGHT BEGINS TO SHINE", the_night_begins_to_shine_canal1, SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL1_N, the_night_begins_to_shine_canal2, SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL2_N, 0), \

#endif /* INC_CANCIONES_TABLA_H_ */
