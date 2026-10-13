/*
 * song_i_like_to_move_it.h
 * Generado por midi_to_c.py desde I-Like-To-Move-It.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_I_LIKE_TO_MOVE_IT_H_
#define SONG_I_LIKE_TO_MOVE_IT_H_

#include "music.h"
#include "synth.h"

#define SONG_I_LIKE_TO_MOVE_IT_BPM 122

#include "song_i_like_to_move_it_canal1.h"
#include "song_i_like_to_move_it_canal2.h"
#include "song_i_like_to_move_it_canal3.h"

static const TrackDef i_like_to_move_it_tracks[] = {
	/* canal 1 */
	TRACK(i_like_to_move_it_fretless, INST_BASS, VOL(0.30)),   /* pista 0 */
	TRACK(i_like_to_move_it_squarewave, INST_SYNTH, VOL(0.30)),   /* pista 1 */
	/* canal 2 */
	TRACK(i_like_to_move_it_melody, INST_SYNTH, VOL(0.30)),   /* pista 2 */
	TRACK(i_like_to_move_it_calliope, INST_SYNTH, VOL(0.30)),   /* pista 3 */
	/* canal 3 */
	TRACK(i_like_to_move_it_bass_lead, INST_SYNTH, VOL(0.30)),   /* pista 4 */
	TRACK(i_like_to_move_it_bag_pipe, INST_LEAD, VOL(0.40)),   /* pista 5 */
	TRACK(i_like_to_move_it_drums, INST_DRUMS, VOL(0.30)),   /* pista 6 */
};

static const Song song_i_like_to_move_it = { "i_like_to_move_it", SONG_I_LIKE_TO_MOVE_IT_BPM, 7, i_like_to_move_it_tracks };

/* Canales: índices de pista (en i_like_to_move_it_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_I_LIKE_TO_MOVE_IT_CANAL1_N; i++) Music_SetTrackVolume(i_like_to_move_it_canal1[i], 0); */
#define SONG_I_LIKE_TO_MOVE_IT_NUM_CANALES 3
#define SONG_I_LIKE_TO_MOVE_IT_CANAL1_N 2
static const uint8_t i_like_to_move_it_canal1[] __attribute__((unused)) = { 0, 1 };
#define SONG_I_LIKE_TO_MOVE_IT_CANAL2_N 2
static const uint8_t i_like_to_move_it_canal2[] __attribute__((unused)) = { 2, 3 };
#define SONG_I_LIKE_TO_MOVE_IT_CANAL3_N 3
static const uint8_t i_like_to_move_it_canal3[] __attribute__((unused)) = { 4, 5, 6 };

#endif /* SONG_I_LIKE_TO_MOVE_IT_H_ */
