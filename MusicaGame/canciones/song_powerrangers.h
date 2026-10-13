/*
 * song_powerrangers.h
 * Generado por midi_to_c.py desde Powerrangers.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_POWERRANGERS_H_
#define SONG_POWERRANGERS_H_

#include "music.h"
#include "synth.h"

#define SONG_POWERRANGERS_BPM 180

#include "song_powerrangers_canal1.h"
#include "song_powerrangers_canal2.h"
#include "song_powerrangers_canal3.h"

static const TrackDef powerrangers_tracks[] = {
	/* canal 1 */
	TRACK(powerrangers_track1, INST_SYNTH, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(powerrangers_track0, INST_DRUMS, VOL(0.30)),   /* pista 1 */
	/* canal 3 */
	TRACK(powerrangers_track2, INST_BASS, VOL(0.30)),   /* pista 2 */
	TRACK(powerrangers_track3, INST_SYNTH, VOL(0.30)),   /* pista 3 */
	TRACK(powerrangers_track4, INST_GUITAR, VOL(0.30)),   /* pista 4 */
	TRACK(powerrangers_track5, INST_SYNTH, VOL(0.30)),   /* pista 5 */
};

static const Song song_powerrangers = { "powerrangers", SONG_POWERRANGERS_BPM, 6, powerrangers_tracks };

/* Canales: índices de pista (en powerrangers_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_POWERRANGERS_CANAL1_N; i++) Music_SetTrackVolume(powerrangers_canal1[i], 0); */
#define SONG_POWERRANGERS_NUM_CANALES 3
#define SONG_POWERRANGERS_CANAL1_N 1
static const uint8_t powerrangers_canal1[] __attribute__((unused)) = { 0 };
#define SONG_POWERRANGERS_CANAL2_N 1
static const uint8_t powerrangers_canal2[] __attribute__((unused)) = { 1 };
#define SONG_POWERRANGERS_CANAL3_N 4
static const uint8_t powerrangers_canal3[] __attribute__((unused)) = { 2, 3, 4, 5 };

#endif /* SONG_POWERRANGERS_H_ */
