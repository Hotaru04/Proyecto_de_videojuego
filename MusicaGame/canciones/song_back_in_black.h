/*
 * song_back_in_black.h
 * Generado por midi_to_c.py desde ACDC — Back in Black (3) [MIDIfind.com].mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_BACK_IN_BLACK_H_
#define SONG_BACK_IN_BLACK_H_

#include "music.h"
#include "synth.h"

#define SONG_BACK_IN_BLACK_BPM 96

#include "song_back_in_black_canal1.h"
#include "song_back_in_black_canal2.h"
#include "song_back_in_black_canal3.h"

static const TrackDef back_in_black_tracks[] = {
	/* canal 1 */
	TRACK(back_in_black_rhythm_guitar, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(back_in_black_drumkit, INST_DRUMS, VOL(0.30)),   /* pista 1 */
	/* canal 3 */
	TRACK(back_in_black_solos, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(back_in_black_bass, INST_BASS, VOL(0.30)),   /* pista 3 */
};

static const Song song_back_in_black = { "back_in_black", SONG_BACK_IN_BLACK_BPM, 4, back_in_black_tracks };

/* Canales: índices de pista (en back_in_black_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_BACK_IN_BLACK_CANAL1_N; i++) Music_SetTrackVolume(back_in_black_canal1[i], 0); */
#define SONG_BACK_IN_BLACK_NUM_CANALES 3
#define SONG_BACK_IN_BLACK_CANAL1_N 1
static const uint8_t back_in_black_canal1[] __attribute__((unused)) = { 0 };
#define SONG_BACK_IN_BLACK_CANAL2_N 1
static const uint8_t back_in_black_canal2[] __attribute__((unused)) = { 1 };
#define SONG_BACK_IN_BLACK_CANAL3_N 2
static const uint8_t back_in_black_canal3[] __attribute__((unused)) = { 2, 3 };

#endif /* SONG_BACK_IN_BLACK_H_ */
