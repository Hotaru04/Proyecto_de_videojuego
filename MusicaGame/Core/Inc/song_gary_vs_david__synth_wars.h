/*
 * song_gary_vs_david__synth_wars.h
 * Generado por midi_to_c.py desde Gary Vs David (Synth Wars).mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_GARY_VS_DAVID__SYNTH_WARS_H_
#define SONG_GARY_VS_DAVID__SYNTH_WARS_H_

#include "music.h"
#include "synth.h"

#define SONG_GARY_VS_DAVID__SYNTH_WARS_BPM 110

#include "song_gary_vs_david__synth_wars_canal1.h"
#include "song_gary_vs_david__synth_wars_canal2.h"
#include "song_gary_vs_david__synth_wars_canal3.h"

static const TrackDef gary_vs_david__synth_wars_tracks[] = {
	/* canal 1 */
	TRACK(gary_vs_david__synth_wars_pop_synth__classic, INST_LEAD, VOL(0.40)),   /* pista 0 */
	/* canal 2 */
	TRACK(gary_vs_david__synth_wars_synth_pluck, INST_SYNTH, VOL(0.30)),   /* pista 1 */
	TRACK(gary_vs_david__synth_wars_smooth_synth__classic, INST_LEAD, VOL(0.40)),   /* pista 2 */
	/* canal 3 */
	TRACK(gary_vs_david__synth_wars_s_909_drum_kit, INST_DRUMS, VOL(0.30)),   /* pista 3 */
	TRACK(gary_vs_david__synth_wars_s_8_bit_sawtooth, INST_SYNTH, VOL(0.30)),   /* pista 4 */
};

static const Song song_gary_vs_david__synth_wars = { "gary_vs_david__synth_wars", SONG_GARY_VS_DAVID__SYNTH_WARS_BPM, 5, gary_vs_david__synth_wars_tracks };

/* Canales: índices de pista (en gary_vs_david__synth_wars_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_GARY_VS_DAVID__SYNTH_WARS_CANAL1_N; i++) Music_SetTrackVolume(gary_vs_david__synth_wars_canal1[i], 0); */
#define SONG_GARY_VS_DAVID__SYNTH_WARS_NUM_CANALES 3
#define SONG_GARY_VS_DAVID__SYNTH_WARS_CANAL1_N 1
static const uint8_t gary_vs_david__synth_wars_canal1[] __attribute__((unused)) = { 0 };
#define SONG_GARY_VS_DAVID__SYNTH_WARS_CANAL2_N 2
static const uint8_t gary_vs_david__synth_wars_canal2[] __attribute__((unused)) = { 1, 2 };
#define SONG_GARY_VS_DAVID__SYNTH_WARS_CANAL3_N 2
static const uint8_t gary_vs_david__synth_wars_canal3[] __attribute__((unused)) = { 3, 4 };

#endif /* SONG_GARY_VS_DAVID__SYNTH_WARS_H_ */
