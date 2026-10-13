/*
 * music.h
 * Secuenciador: lee las pistas de una canción y dispara las notas en el sintetizador.
 * El tiempo se mide en ticks (MUSIC_PPQ por negra); audio.c mueve el reloj contando muestras.
 */
#ifndef INC_MUSIC_H_
#define INC_MUSIC_H_

#include <stdint.h>
#include "notes.h"

/* Duraciones en ticks */
#define MUSIC_PPQ      48
#define WHOLE          (MUSIC_PPQ * 4)
#define HALF           (MUSIC_PPQ * 2)
#define QUARTER        (MUSIC_PPQ)
#define EIGHTH         (MUSIC_PPQ / 2)
#define SIXTEENTH      (MUSIC_PPQ / 4)
#define THIRTYSECOND   (MUSIC_PPQ / 8)
#define DOTTED(d)      ((d) * 3 / 2)
#define TRIPLET(d)     ((d) * 2 / 3)

/* Con NOTE_CHORD la siguiente nota empieza al mismo tiempo (acordes) */
#define NOTE_CHORD           0x01
#define MUSIC_DEFAULT_VEL    100

typedef struct {
	uint8_t  note;    /* número MIDI o REST */
	uint16_t dur;     /* ticks */
	uint8_t  vel;     /* 0 = MUSIC_DEFAULT_VEL */
	uint8_t  flags;
} Note;

typedef struct {
	const Note *notes;
	uint16_t    length;
	uint8_t     instrument;  /* INST_* */
	uint8_t     volume;      /* 0..255 */
} TrackDef;

typedef struct {
	const char     *name;
	uint16_t        bpm;
	uint8_t         num_tracks;
	const TrackDef *tracks;
} Song;

#define VOL(x)               ((uint8_t)((x) * 255.0f + 0.5f))   /* 0.0..1.0 -> 0..255 */
#define NOTES_LEN(arr)       ((uint16_t)(sizeof(arr) / sizeof((arr)[0])))
#define TRACK(arr, inst, vol) { (arr), NOTES_LEN(arr), (inst), (vol) }

typedef enum { TRACK_STOPPED = 0, TRACK_PLAYING, TRACK_FINISHED } TrackState;

typedef struct {
	const TrackDef *def;
	uint16_t index;
	uint16_t wait;       /* ticks para la siguiente nota */
	uint8_t  volume;
	uint8_t  state;
} Track;

#define MUSIC_MAX_TRACKS 16

void     Music_Init(uint32_t sample_rate);
void     Music_Play(const Song *song, uint8_t loop);
void     Music_Stop(void);
uint8_t  Music_IsPlaying(void);
void     Music_SetTrackVolume(uint8_t track, uint8_t volume);
uint32_t Music_GetTick(void);
const uint8_t *Music_TrackVolumes(void);

/* Solo para audio.c */
uint32_t Music_SamplesUntilTick(void);
void     Music_AdvanceSamples(uint32_t n);

#endif /* INC_MUSIC_H_ */
