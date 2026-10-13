/*
 * song_balck_betty.h
 * Generado por midi_to_c.py desde Ram Jam — Black Betty (2) [MIDIfind.com].mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_BALCK_BETTY_H_
#define SONG_BALCK_BETTY_H_

#include "music.h"
#include "synth.h"

#define SONG_BALCK_BETTY_BPM 120

#include "song_balck_betty_canal1.h"
#include "song_balck_betty_canal2.h"

static const TrackDef balck_betty_tracks[] = {
	/* canal 1 */
	TRACK(balck_betty_solo_guitar, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	/* canal 2 */
	TRACK(balck_betty_rythm_guitar, INST_GUITAR, VOL(0.30)),   /* pista 1 */
};

static const Song song_balck_betty = { "balck_betty", SONG_BALCK_BETTY_BPM, 2, balck_betty_tracks };

/* Canales: índices de pista (en balck_betty_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_BALCK_BETTY_CANAL1_N; i++) Music_SetTrackVolume(balck_betty_canal1[i], 0); */
#define SONG_BALCK_BETTY_NUM_CANALES 2
#define SONG_BALCK_BETTY_CANAL1_N 1
static const uint8_t balck_betty_canal1[] __attribute__((unused)) = { 0 };
#define SONG_BALCK_BETTY_CANAL2_N 1
static const uint8_t balck_betty_canal2[] __attribute__((unused)) = { 1 };

#endif /* SONG_BALCK_BETTY_H_ */
