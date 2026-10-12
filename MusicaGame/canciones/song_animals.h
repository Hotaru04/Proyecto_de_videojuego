/*
 * song_animals.h
 * Generado por midi_to_c.py desde Martin-Garrix-Animals-iNawb-20131218110554-nonstop2k.com.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_ANIMALS_H_
#define SONG_ANIMALS_H_

#include "music.h"
#include "synth.h"

#define SONG_ANIMALS_BPM 128

#include "song_animals_canal1.h"
#include "song_animals_canal2.h"

static const TrackDef animals_tracks[] = {
	/* canal 1 */
	TRACK(animals_midi_out__4, INST_LEAD, VOL(0.40)),   /* pista 0 */
	TRACK(animals_midi_out, INST_SYNTH, VOL(0.30)),   /* pista 1 */
	/* canal 2 */
	TRACK(animals_midi_out__5, INST_SYNTH, VOL(0.30)),   /* pista 2 */
	TRACK(animals_midi_out__2, INST_BASS, VOL(0.30)),   /* pista 3 */
};

static const Song song_animals = { "animals", SONG_ANIMALS_BPM, 4, animals_tracks };

/* Canales: índices de pista (en animals_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_ANIMALS_CANAL1_N; i++) Music_SetTrackVolume(animals_canal1[i], 0); */
#define SONG_ANIMALS_NUM_CANALES 2
#define SONG_ANIMALS_CANAL1_N 2
static const uint8_t animals_canal1[] __attribute__((unused)) = { 0, 1 };
#define SONG_ANIMALS_CANAL2_N 2
static const uint8_t animals_canal2[] __attribute__((unused)) = { 2, 3 };

#endif /* SONG_ANIMALS_H_ */
