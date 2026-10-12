/*
 * song_michael_jackson___beat_it.h
 * Generado por midi_to_c.py desde Michael Jackson - Beat It.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_MICHAEL_JACKSON___BEAT_IT_H_
#define SONG_MICHAEL_JACKSON___BEAT_IT_H_

#include "music.h"
#include "synth.h"

#define SONG_MICHAEL_JACKSON___BEAT_IT_BPM 139

#include "song_michael_jackson___beat_it_canal1.h"
#include "song_michael_jackson___beat_it_canal2.h"
#include "song_michael_jackson___beat_it_canal3.h"

static const TrackDef michael_jackson___beat_it_tracks[] = {
	/* canal 1 */
	TRACK(michael_jackson___beat_it_s_5, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	TRACK(michael_jackson___beat_it_s_9, INST_GUITAR, VOL(0.30)),   /* pista 1 */
	TRACK(michael_jackson___beat_it_s_0, INST_DRUMS, VOL(0.30)),   /* pista 2 */
	/* canal 2 */
	TRACK(michael_jackson___beat_it_s_12, INST_LEAD, VOL(0.40)),   /* pista 3 */
	/* canal 3 */
	TRACK(michael_jackson___beat_it_s_1, INST_LEAD, VOL(0.40)),   /* pista 4 */
	TRACK(michael_jackson___beat_it_s_2, INST_BASS, VOL(0.30)),   /* pista 5 */
	TRACK(michael_jackson___beat_it_s_3, INST_LEAD, VOL(0.40)),   /* pista 6 */
	TRACK(michael_jackson___beat_it_s_4, INST_SYNTH, VOL(0.30)),   /* pista 7 */
	TRACK(michael_jackson___beat_it_s_6, INST_GUITAR, VOL(0.30)),   /* pista 8 */
	TRACK(michael_jackson___beat_it_s_7, INST_GUITAR, VOL(0.30)),   /* pista 9 */
	TRACK(michael_jackson___beat_it_s_8, INST_GUITAR, VOL(0.30)),   /* pista 10 */
	TRACK(michael_jackson___beat_it_s_11, INST_GUITAR, VOL(0.30)),   /* pista 11 */
	TRACK(michael_jackson___beat_it_s_13, INST_BASS, VOL(0.30)),   /* pista 12 */
	TRACK(michael_jackson___beat_it_s_14, INST_SYNTH, VOL(0.30)),   /* pista 13 */
	TRACK(michael_jackson___beat_it_s_15, INST_LEAD, VOL(0.40)),   /* pista 14 */
	TRACK(michael_jackson___beat_it_s_16, INST_LEAD, VOL(0.40)),   /* pista 15 */
};

static const Song song_michael_jackson___beat_it = { "michael_jackson___beat_it", SONG_MICHAEL_JACKSON___BEAT_IT_BPM, 16, michael_jackson___beat_it_tracks };

/* Canales: índices de pista (en michael_jackson___beat_it_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_MICHAEL_JACKSON___BEAT_IT_CANAL1_N; i++) Music_SetTrackVolume(michael_jackson___beat_it_canal1[i], 0); */
#define SONG_MICHAEL_JACKSON___BEAT_IT_NUM_CANALES 3
#define SONG_MICHAEL_JACKSON___BEAT_IT_CANAL1_N 3
static const uint8_t michael_jackson___beat_it_canal1[] __attribute__((unused)) = { 0, 1, 2 };
#define SONG_MICHAEL_JACKSON___BEAT_IT_CANAL2_N 1
static const uint8_t michael_jackson___beat_it_canal2[] __attribute__((unused)) = { 3 };
#define SONG_MICHAEL_JACKSON___BEAT_IT_CANAL3_N 12
static const uint8_t michael_jackson___beat_it_canal3[] __attribute__((unused)) = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

#endif /* SONG_MICHAEL_JACKSON___BEAT_IT_H_ */
