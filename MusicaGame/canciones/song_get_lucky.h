/*
 * song_get_lucky.h
 * Generado por midi_to_c.py desde Daft Punk — Get Lucky [MIDIfind.com].mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_GET_LUCKY_H_
#define SONG_GET_LUCKY_H_

#include "music.h"
#include "synth.h"

#define SONG_GET_LUCKY_BPM 120

#include "song_get_lucky_canal1.h"
#include "song_get_lucky_canal2.h"
#include "song_get_lucky_canal3.h"

static const TrackDef get_lucky_tracks[] = {
	/* canal 1 */
	TRACK(get_lucky_guitar, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(get_lucky_melody, INST_LEAD, VOL(0.40)),   /* pista 1 */
	TRACK(get_lucky_get_lucky_1, INST_LEAD, VOL(0.40)),   /* pista 2 */
	/* canal 3 */
	TRACK(get_lucky_piano_1, INST_LEAD, VOL(0.40)),   /* pista 3 */
	TRACK(get_lucky_piano_2, INST_LEAD, VOL(0.40)),   /* pista 4 */
	TRACK(get_lucky_bass_5_str, INST_BASS, VOL(0.30)),   /* pista 5 */
	TRACK(get_lucky_get_lucky_2, INST_LEAD, VOL(0.40)),   /* pista 6 */
	TRACK(get_lucky_lead, INST_BASS, VOL(0.30)),   /* pista 7 */
	TRACK(get_lucky_hand_clap, INST_DRUMS, VOL(0.30)),   /* pista 8 */
	TRACK(get_lucky_drumkit, INST_DRUMS, VOL(0.30)),   /* pista 9 */
};

static const Song song_get_lucky = { "get_lucky", SONG_GET_LUCKY_BPM, 10, get_lucky_tracks };

/* Canales: índices de pista (en get_lucky_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_GET_LUCKY_CANAL1_N; i++) Music_SetTrackVolume(get_lucky_canal1[i], 0); */
#define SONG_GET_LUCKY_NUM_CANALES 3
#define SONG_GET_LUCKY_CANAL1_N 1
static const uint8_t get_lucky_canal1[] __attribute__((unused)) = { 0 };
#define SONG_GET_LUCKY_CANAL2_N 2
static const uint8_t get_lucky_canal2[] __attribute__((unused)) = { 1, 2 };
#define SONG_GET_LUCKY_CANAL3_N 7
static const uint8_t get_lucky_canal3[] __attribute__((unused)) = { 3, 4, 5, 6, 7, 8, 9 };

#endif /* SONG_GET_LUCKY_H_ */
