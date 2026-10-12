/*
 * song_la_bamba_1.h
 * Generado por midi_to_c.py desde La-Bamba-1.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_LA_BAMBA_1_H_
#define SONG_LA_BAMBA_1_H_

#include "music.h"
#include "synth.h"

#define SONG_LA_BAMBA_1_BPM 150

#include "song_la_bamba_1_canal1.h"
#include "song_la_bamba_1_canal2.h"
#include "song_la_bamba_1_canal3.h"

static const TrackDef la_bamba_1_tracks[] = {
	/* canal 1 */
	TRACK(la_bamba_1_melody, INST_LEAD, VOL(0.40)),   /* pista 0 */
	/* canal 2 */
	TRACK(la_bamba_1_drums, INST_DRUMS, VOL(0.30)),   /* pista 1 */
	/* canal 3 */
	TRACK(la_bamba_1_muted_gtr, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(la_bamba_1_acou_bass, INST_BASS, VOL(0.30)),   /* pista 3 */
	TRACK(la_bamba_1_clean_gtr, INST_GUITAR, VOL(0.30)),   /* pista 4 */
};

static const Song song_la_bamba_1 = { "la_bamba_1", SONG_LA_BAMBA_1_BPM, 5, la_bamba_1_tracks };

/* Canales: índices de pista (en la_bamba_1_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_LA_BAMBA_1_CANAL1_N; i++) Music_SetTrackVolume(la_bamba_1_canal1[i], 0); */
#define SONG_LA_BAMBA_1_NUM_CANALES 3
#define SONG_LA_BAMBA_1_CANAL1_N 1
static const uint8_t la_bamba_1_canal1[] __attribute__((unused)) = { 0 };
#define SONG_LA_BAMBA_1_CANAL2_N 1
static const uint8_t la_bamba_1_canal2[] __attribute__((unused)) = { 1 };
#define SONG_LA_BAMBA_1_CANAL3_N 3
static const uint8_t la_bamba_1_canal3[] __attribute__((unused)) = { 2, 3, 4 };

#endif /* SONG_LA_BAMBA_1_H_ */
