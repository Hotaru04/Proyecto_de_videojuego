/*
 * song_outkast_hey_ya.h
 * Generado por midi_to_c.py desde outkast-hey_ya.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_OUTKAST_HEY_YA_H_
#define SONG_OUTKAST_HEY_YA_H_

#include "music.h"
#include "synth.h"

#define SONG_OUTKAST_HEY_YA_BPM 155

#include "song_outkast_hey_ya_canal1.h"
#include "song_outkast_hey_ya_canal2.h"
#include "song_outkast_hey_ya_canal3.h"

static const TrackDef outkast_hey_ya_tracks[] = {
	/* canal 1 */
	TRACK(outkast_hey_ya_tabit_midi___track_1, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(outkast_hey_ya_tabit_midi___track_5, INST_LEAD, VOL(0.40)),   /* pista 1 */
	/* canal 3 */
	TRACK(outkast_hey_ya_tabit_midi___track_2, INST_DRUMS, VOL(0.30)),   /* pista 2 */
	TRACK(outkast_hey_ya_tabit_midi___track_3, INST_BASS, VOL(0.30)),   /* pista 3 */
	TRACK(outkast_hey_ya_tabit_midi___track_4, INST_BASS, VOL(0.30)),   /* pista 4 */
	TRACK(outkast_hey_ya_tabit_midi___track_6, INST_LEAD, VOL(0.40)),   /* pista 5 */
	TRACK(outkast_hey_ya_tabit_midi___track_7, INST_LEAD, VOL(0.40)),   /* pista 6 */
};

static const Song song_outkast_hey_ya = { "outkast_hey_ya", SONG_OUTKAST_HEY_YA_BPM, 7, outkast_hey_ya_tracks };

/* Canales: índices de pista (en outkast_hey_ya_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_OUTKAST_HEY_YA_CANAL1_N; i++) Music_SetTrackVolume(outkast_hey_ya_canal1[i], 0); */
#define SONG_OUTKAST_HEY_YA_NUM_CANALES 3
#define SONG_OUTKAST_HEY_YA_CANAL1_N 1
static const uint8_t outkast_hey_ya_canal1[] __attribute__((unused)) = { 0 };
#define SONG_OUTKAST_HEY_YA_CANAL2_N 1
static const uint8_t outkast_hey_ya_canal2[] __attribute__((unused)) = { 1 };
#define SONG_OUTKAST_HEY_YA_CANAL3_N 5
static const uint8_t outkast_hey_ya_canal3[] __attribute__((unused)) = { 2, 3, 4, 5, 6 };

#endif /* SONG_OUTKAST_HEY_YA_H_ */
