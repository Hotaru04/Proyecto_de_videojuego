/*
 * song_seven_nation_army.h
 * Generado por midi_to_c.py desde Seven_Nation_Army.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_SEVEN_NATION_ARMY_H_
#define SONG_SEVEN_NATION_ARMY_H_

#include "music.h"
#include "synth.h"

#define SONG_SEVEN_NATION_ARMY_BPM 120

#include "song_seven_nation_army_canal1.h"
#include "song_seven_nation_army_canal2.h"
#include "song_seven_nation_army_canal3.h"

static const TrackDef seven_nation_army_tracks[] = {
	/* canal 1 */
	TRACK(seven_nation_army_guitar_w__octave_bass, INST_BASS, VOL(0.51)),   /* pista 0 */
	/* canal 2 */
	TRACK(seven_nation_army_drums__meg_white, INST_DRUMS, VOL(0.53)),   /* pista 1 */
	/* canal 3 */
	TRACK(seven_nation_army_guitar_1__jack_white__open_a_tuning, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(seven_nation_army_solo_guitar___jack_white_standard_tuning, INST_GUITAR, VOL(0.30)),   /* pista 3 */
};

static const Song song_seven_nation_army = { "seven_nation_army", SONG_SEVEN_NATION_ARMY_BPM, 4, seven_nation_army_tracks };

/* Canales: índices de pista (en seven_nation_army_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_SEVEN_NATION_ARMY_CANAL1_N; i++) Music_SetTrackVolume(seven_nation_army_canal1[i], 0); */
#define SONG_SEVEN_NATION_ARMY_NUM_CANALES 3
#define SONG_SEVEN_NATION_ARMY_CANAL1_N 1
static const uint8_t seven_nation_army_canal1[] __attribute__((unused)) = { 0 };
#define SONG_SEVEN_NATION_ARMY_CANAL2_N 1
static const uint8_t seven_nation_army_canal2[] __attribute__((unused)) = { 1 };
#define SONG_SEVEN_NATION_ARMY_CANAL3_N 2
static const uint8_t seven_nation_army_canal3[] __attribute__((unused)) = { 2, 3 };

#endif /* SONG_SEVEN_NATION_ARMY_H_ */
