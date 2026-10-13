/*
 * song_sweet_child.h
 * Generado por midi_to_c.py desde Guns n Roses - Sweet Child O Mine.mid
 * Incluir en UN solo .c. Los arreglos son static const (flash).
 */
#ifndef SONG_SWEET_CHILD_H_
#define SONG_SWEET_CHILD_H_

#include "music.h"
#include "synth.h"

#define SONG_SWEET_CHILD_BPM 128

#include "song_sweet_child_canal1.h"
#include "song_sweet_child_canal2.h"
#include "song_sweet_child_canal3.h"

static const TrackDef sweet_child_tracks[] = {
	/* canal 1 */
	TRACK(sweet_child_lead_guitar, INST_GUITAR, VOL(0.30)),   /* pista 0 */
	TRACK(sweet_child_s, INST_GUITAR, VOL(0.30)),   /* pista 1 */
	/* canal 2 */
	TRACK(sweet_child_rhthym_guitar_clean___dist, INST_GUITAR, VOL(0.30)),   /* pista 2 */
	TRACK(sweet_child_drums, INST_DRUMS, VOL(0.30)),   /* pista 3 */
	/* canal 3 */
	TRACK(sweet_child_rhthym_guitar_dist, INST_GUITAR, VOL(0.30)),   /* pista 4 */
	TRACK(sweet_child_acoustic_guitar, INST_GUITAR, VOL(0.30)),   /* pista 5 */
	TRACK(sweet_child_lead_guitar_fill, INST_GUITAR, VOL(0.30)),   /* pista 6 */
	TRACK(sweet_child_guitar_harmonics, INST_GUITAR, VOL(0.30)),   /* pista 7 */
	TRACK(sweet_child_bass, INST_BASS, VOL(0.30)),   /* pista 8 */
};

static const Song song_sweet_child = { "sweet_child", SONG_SWEET_CHILD_BPM, 9, sweet_child_tracks };

/* Canales: índices de pista (en sweet_child_tracks[]) que pertenecen a cada canal.
 * Ej.: for (i = 0; i < SONG_SWEET_CHILD_CANAL1_N; i++) Music_SetTrackVolume(sweet_child_canal1[i], 0); */
#define SONG_SWEET_CHILD_NUM_CANALES 3
#define SONG_SWEET_CHILD_CANAL1_N 2
static const uint8_t sweet_child_canal1[] __attribute__((unused)) = { 0, 1 };
#define SONG_SWEET_CHILD_CANAL2_N 2
static const uint8_t sweet_child_canal2[] __attribute__((unused)) = { 2, 3 };
#define SONG_SWEET_CHILD_CANAL3_N 5
static const uint8_t sweet_child_canal3[] __attribute__((unused)) = { 4, 5, 6, 7, 8 };

#endif /* SONG_SWEET_CHILD_H_ */
