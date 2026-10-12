/*
 * song_mighty_morphin_power_rangers.h
 * Generado por midi_to_c.py desde Powerrangers.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_MIGHTY_MORPHIN_POWER_RANGERS_H_
#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_H_

#include "music.h"
#include "synth.h"

#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_BPM 180

#include "song_mighty_morphin_power_rangers_canal1.h"
#include "song_mighty_morphin_power_rangers_canal2.h"
#include "song_mighty_morphin_power_rangers_canal3.h"

static const TrackDef mighty_morphin_power_rangers_tracks[] = {
	/* canal 1 */
	TRACK(mighty_morphin_power_rangers_track4, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(mighty_morphin_power_rangers_track0, INST_DRUMS, VOL(0.30)),   /* pista 1 */
	/* canal 3 */
	TRACK(mighty_morphin_power_rangers_track1, INST_SYNTH, VOL(0.30)),   /* pista 2 */
	TRACK(mighty_morphin_power_rangers_track2, INST_BASS, VOL(0.30)),   /* pista 3 */
	TRACK(mighty_morphin_power_rangers_track3, INST_SYNTH, VOL(0.30)),   /* pista 4 */
	TRACK(mighty_morphin_power_rangers_track5, INST_SYNTH, VOL(0.30)),   /* pista 5 */
};

static const Song song_mighty_morphin_power_rangers = { "mighty_morphin_power_rangers", SONG_MIGHTY_MORPHIN_POWER_RANGERS_BPM, 6, mighty_morphin_power_rangers_tracks };

/* Canales: índices de pista (en mighty_morphin_power_rangers_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL1_N; i++) Music_SetTrackVolume(mighty_morphin_power_rangers_canal1[i], 0); */
#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_NUM_CANALES 3
#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL1_N 1
static const uint8_t mighty_morphin_power_rangers_canal1[] __attribute__((unused)) = { 0 };
#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL2_N 1
static const uint8_t mighty_morphin_power_rangers_canal2[] __attribute__((unused)) = { 1 };
#define SONG_MIGHTY_MORPHIN_POWER_RANGERS_CANAL3_N 4
static const uint8_t mighty_morphin_power_rangers_canal3[] __attribute__((unused)) = { 2, 3, 4, 5 };

#endif /* SONG_MIGHTY_MORPHIN_POWER_RANGERS_H_ */
