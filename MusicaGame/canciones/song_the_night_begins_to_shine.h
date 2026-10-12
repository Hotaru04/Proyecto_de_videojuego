/*
 * song_the_night_begins_to_shine.h
 * Generado por midi_to_c.py desde B.E.R - The Night Begins to Shine (WIP).mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_THE_NIGHT_BEGINS_TO_SHINE_H_
#define SONG_THE_NIGHT_BEGINS_TO_SHINE_H_

#include "music.h"
#include "synth.h"

#define SONG_THE_NIGHT_BEGINS_TO_SHINE_BPM 130

#include "song_the_night_begins_to_shine_canal1.h"
#include "song_the_night_begins_to_shine_canal2.h"
#include "song_the_night_begins_to_shine_canal3.h"

static const TrackDef the_night_begins_to_shine_tracks[] = {
	/* canal 1 */
	TRACK(the_night_begins_to_shine_distortion_guitar, INST_GUITAR, VOL(0.62)),   /* pista 0 */
	/* canal 2 */
	TRACK(the_night_begins_to_shine_bass_guitar, INST_BASS, VOL(0.60)),   /* pista 1 */
	/* canal 3 */
	TRACK(the_night_begins_to_shine_electric_guitar, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(the_night_begins_to_shine_synth_pluck, INST_SYNTH, VOL(0.30)),   /* pista 3 */
	TRACK(the_night_begins_to_shine_electric_drum_kit, INST_DRUMS, VOL(0.30)),   /* pista 4 */
};

static const Song song_the_night_begins_to_shine = { "the_night_begins_to_shine", SONG_THE_NIGHT_BEGINS_TO_SHINE_BPM, 5, the_night_begins_to_shine_tracks };

/* Canales: índices de pista (en the_night_begins_to_shine_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL1_N; i++) Music_SetTrackVolume(the_night_begins_to_shine_canal1[i], 0); */
#define SONG_THE_NIGHT_BEGINS_TO_SHINE_NUM_CANALES 3
#define SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL1_N 1
static const uint8_t the_night_begins_to_shine_canal1[] __attribute__((unused)) = { 0 };
#define SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL2_N 1
static const uint8_t the_night_begins_to_shine_canal2[] __attribute__((unused)) = { 1 };
#define SONG_THE_NIGHT_BEGINS_TO_SHINE_CANAL3_N 3
static const uint8_t the_night_begins_to_shine_canal3[] __attribute__((unused)) = { 2, 3, 4 };

#endif /* SONG_THE_NIGHT_BEGINS_TO_SHINE_H_ */
