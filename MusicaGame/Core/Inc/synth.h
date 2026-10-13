/*
 * synth.h
 * Sintetizador: voces DDS con envolvente ADSR que se suman en un buffer de mezcla.
 */
#ifndef INC_SYNTH_H_
#define INC_SYNTH_H_

#include <stdint.h>

#define SYNTH_MAX_VOICES  12

typedef enum { WAVE_SINE = 0, WAVE_SQUARE, WAVE_TRIANGLE, WAVE_SAW, WAVE_NOISE } Waveform;

enum {
	INST_LEAD = 0,   /* seno */
	INST_GUITAR,     /* cuadrada */
	INST_BASS,       /* triangular */
	INST_SYNTH,      /* diente de sierra */
	INST_DRUMS,      /* ruido; la nota MIDI elige el tambor */
	INST_COUNT
};

typedef struct {
	Waveform wave;
	uint16_t attack_ms;
	uint16_t decay_ms;
	uint8_t  sustain;      /* 0..255 */
	uint16_t release_ms;
	uint8_t  amplitude;    /* 0..255, iguala el volumen entre formas de onda */
} Instrument;

extern const Instrument synth_instruments[INST_COUNT];

void    Synth_Init(uint32_t sample_rate);
void    Synth_NoteOn(uint8_t track, uint8_t inst, uint8_t midi_note, uint8_t vel, uint16_t dur_ticks);
void    Synth_Tick(void);
void    Synth_AllOff(void);
uint8_t Synth_ActiveVoices(void);
void    Synth_Render(int32_t *mix, uint32_t n, const uint8_t *track_vol);   /* suma sobre mix[] */

#endif /* INC_SYNTH_H_ */
