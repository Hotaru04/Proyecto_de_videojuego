/*
 * song_under_pressure.h
 * Generado por midi_to_c.py desde queen-under_pressure_feat_david_bowie.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_UNDER_PRESSURE_H_
#define SONG_UNDER_PRESSURE_H_

#include "music.h"
#include "synth.h"

#define SONG_UNDER_PRESSURE_BPM 113

#include "song_under_pressure_canal1.h"
#include "song_under_pressure_canal2.h"
#include "song_under_pressure_canal3.h"

static const TrackDef under_pressure_tracks[] = {
	/* canal 1 */
	TRACK(under_pressure_track8, INST_LEAD, VOL(0.40)),   /* pista 0 */
	TRACK(under_pressure_track10, INST_BASS, VOL(0.30)),   /* pista 1 */
	/* canal 2 */
	TRACK(under_pressure_track3, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(under_pressure_track4, INST_GUITAR, VOL(0.30)),   /* pista 3 */
	/* canal 3 */
	TRACK(under_pressure_track1, INST_BASS, VOL(0.30)),   /* pista 4 */
	TRACK(under_pressure_track2, INST_GUITAR, VOL(0.30)),   /* pista 5 */
	TRACK(under_pressure_track5, INST_LEAD, VOL(0.40)),   /* pista 6 */
	TRACK(under_pressure_track6, INST_LEAD, VOL(0.40)),   /* pista 7 */
	TRACK(under_pressure_track7, INST_LEAD, VOL(0.40)),   /* pista 8 */
	TRACK(under_pressure_track9, INST_BASS, VOL(0.30)),   /* pista 9 */
	TRACK(under_pressure_track11, INST_DRUMS, VOL(0.30)),   /* pista 10 */
	TRACK(under_pressure_track12, INST_DRUMS, VOL(0.30)),   /* pista 11 */
	TRACK(under_pressure_track13, INST_DRUMS, VOL(0.30)),   /* pista 12 */
	TRACK(under_pressure_track14, INST_DRUMS, VOL(0.30)),   /* pista 13 */
	TRACK(under_pressure_track15, INST_DRUMS, VOL(0.30)),   /* pista 14 */
};

static const Song song_under_pressure = { "under_pressure", SONG_UNDER_PRESSURE_BPM, 15, under_pressure_tracks };

/* Canales: índices de pista (en under_pressure_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_UNDER_PRESSURE_CANAL1_N; i++) Music_SetTrackVolume(under_pressure_canal1[i], 0); */
#define SONG_UNDER_PRESSURE_NUM_CANALES 3
#define SONG_UNDER_PRESSURE_CANAL1_N 2
static const uint8_t under_pressure_canal1[] __attribute__((unused)) = { 0, 1 };
#define SONG_UNDER_PRESSURE_CANAL2_N 2
static const uint8_t under_pressure_canal2[] __attribute__((unused)) = { 2, 3 };
#define SONG_UNDER_PRESSURE_CANAL3_N 11
static const uint8_t under_pressure_canal3[] __attribute__((unused)) = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

#endif /* SONG_UNDER_PRESSURE_H_ */
