/*
 * song_its_be_so_long.h
 * Generado por midi_to_c.py desde The_Living_Tombstone_-_fnaf_its__be_so_long.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_ITS_BE_SO_LONG_H_
#define SONG_ITS_BE_SO_LONG_H_

#include "music.h"
#include "synth.h"

#define SONG_ITS_BE_SO_LONG_BPM 96

#include "song_its_be_so_long_canal1.h"
#include "song_its_be_so_long_canal2.h"

static const TrackDef its_be_so_long_tracks[] = {
	/* canal 1 */
	TRACK(its_be_so_long_track2, INST_BASS, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(its_be_so_long_track1, INST_LEAD, VOL(0.40)),   /* pista 1 */
};

static const Song song_its_be_so_long = { "its_be_so_long", SONG_ITS_BE_SO_LONG_BPM, 2, its_be_so_long_tracks };

/* Canales: índices de pista (en its_be_so_long_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_ITS_BE_SO_LONG_CANAL1_N; i++) Music_SetTrackVolume(its_be_so_long_canal1[i], 0); */
#define SONG_ITS_BE_SO_LONG_NUM_CANALES 2
#define SONG_ITS_BE_SO_LONG_CANAL1_N 1
static const uint8_t its_be_so_long_canal1[] __attribute__((unused)) = { 0 };
#define SONG_ITS_BE_SO_LONG_CANAL2_N 1
static const uint8_t its_be_so_long_canal2[] __attribute__((unused)) = { 1 };

#endif /* SONG_ITS_BE_SO_LONG_H_ */
