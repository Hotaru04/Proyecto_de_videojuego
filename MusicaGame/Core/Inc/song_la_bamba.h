/*
 * song_la_bamba.h
 * Generado por midi_to_c.py desde La-Bamba-1.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_LA_BAMBA_H_
#define SONG_LA_BAMBA_H_

#include "music.h"
#include "synth.h"

#define SONG_LA_BAMBA_BPM 150

#include "song_la_bamba_canal1.h"
#include "song_la_bamba_canal2.h"
#include "song_la_bamba_canal3.h"

static const TrackDef la_bamba_tracks[] = {
	/* canal 1 */
	TRACK(la_bamba_melody, INST_LEAD, VOL(0.40)),   /* pista 0 */
	TRACK(la_bamba_clean_gtr, INST_GUITAR, VOL(0.30)),   /* pista 1 */
	/* canal 2 */
	TRACK(la_bamba_muted_gtr, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	/* canal 3 */
	TRACK(la_bamba_acou_bass, INST_BASS, VOL(0.30)),   /* pista 3 */
	TRACK(la_bamba_drums, INST_DRUMS, VOL(0.15)),   /* pista 4 */
};

static const Song song_la_bamba = { "la_bamba", SONG_LA_BAMBA_BPM, 5, la_bamba_tracks };

/* Canales: índices de pista (en la_bamba_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_LA_BAMBA_CANAL1_N; i++) Music_SetTrackVolume(la_bamba_canal1[i], 0); */
#define SONG_LA_BAMBA_NUM_CANALES 3
#define SONG_LA_BAMBA_CANAL1_N 2
static const uint8_t la_bamba_canal1[] __attribute__((unused)) = { 0, 1 };
#define SONG_LA_BAMBA_CANAL2_N 1
static const uint8_t la_bamba_canal2[] __attribute__((unused)) = { 2 };
#define SONG_LA_BAMBA_CANAL3_N 2
static const uint8_t la_bamba_canal3[] __attribute__((unused)) = { 3, 4 };

#endif /* SONG_LA_BAMBA_H_ */
