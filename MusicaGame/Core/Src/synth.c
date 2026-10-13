/*
 * synth.c
 * Voces DDS (acumulador de fase de 32 bits) con envolvente ADSR en punto fijo.
 * Solo Synth_Init usa float.
 */
#include "synth.h"

/* El render corre 32000 veces por segundo por voz: se optimiza aunque el proyecto compile en -O0 */
#pragma GCC optimize ("O2")

/* Tabla de seno de 256 muestras */
static const int16_t sine_table[256] = {
	     0,    804,   1608,   2410,   3212,   4011,   4808,   5602,   6393,   7179,   7962,   8739,   9512,  10278,  11039,  11793,
	 12539,  13279,  14010,  14732,  15446,  16151,  16846,  17530,  18204,  18868,  19519,  20159,  20787,  21403,  22005,  22594,
	 23170,  23731,  24279,  24811,  25329,  25832,  26319,  26790,  27245,  27683,  28105,  28510,  28898,  29268,  29621,  29956,
	 30273,  30571,  30852,  31113,  31356,  31580,  31785,  31971,  32137,  32285,  32412,  32521,  32609,  32678,  32728,  32757,
	 32767,  32757,  32728,  32678,  32609,  32521,  32412,  32285,  32137,  31971,  31785,  31580,  31356,  31113,  30852,  30571,
	 30273,  29956,  29621,  29268,  28898,  28510,  28105,  27683,  27245,  26790,  26319,  25832,  25329,  24811,  24279,  23731,
	 23170,  22594,  22005,  21403,  20787,  20159,  19519,  18868,  18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,
	 12539,  11793,  11039,  10278,   9512,   8739,   7962,   7179,   6393,   5602,   4808,   4011,   3212,   2410,   1608,    804,
	     0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,  -6393,  -7179,  -7962,  -8739,  -9512, -10278, -11039, -11793,
	-12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
	-23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
	-30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
	-32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
	-30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
	-23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
	-12539, -11793, -11039, -10278,  -9512,  -8739,  -7962,  -7179,  -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,   -804,};

/* Instrumentos */
const Instrument synth_instruments[INST_COUNT] = {
	/* wave           A   D    S    R   amp */
	[INST_LEAD]   = { WAVE_SINE,      5, 120, 180,  80, 255 },
	[INST_GUITAR] = { WAVE_SQUARE,    2, 180, 120, 100, 110 },
	[INST_BASS]   = { WAVE_TRIANGLE,  4, 100, 200,  60, 230 },
	[INST_SYNTH]  = { WAVE_SAW,      10, 250, 150, 150, 140 },
	[INST_DRUMS]  = { WAVE_NOISE,     1, 120,   0,  30, 170 },
};

/* Envolvente en Q24 (1.0 = 1 << 24) */
#define ENV_MAX  (1 << 24)
typedef enum { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } EnvStage;

typedef struct {
	uint32_t phase;         /* 2^32 = un ciclo */
	uint32_t inc;           /* incremento por muestra = f * 2^32 / Fs */
	int32_t  level;         /* nivel de envolvente Q24 */
	int32_t  attack_step, decay_step, release_step, sustain;
	uint16_t ticks_left;    /* ticks hasta soltar la nota */
	uint8_t  stage;
	uint8_t  wave;
	uint8_t  track;
	uint8_t  gain;          /* velocidad * amplitud del instrumento */
	uint32_t age;           /* para robar la voz más vieja */
} Voice;

static Voice    voices[SYNTH_MAX_VOICES];
static uint32_t note_inc[128];       /* incremento de fase por nota MIDI */
static uint32_t fs = 32000;
static uint32_t age_counter = 0;
static uint32_t rng = 0x12345678u;   /* generador de ruido xorshift32 */

static int32_t Steps(uint32_t ms, int32_t span) {
	uint32_t samples = (fs * ms) / 1000u;
	if (samples == 0) samples = 1;
	int32_t s = span / (int32_t)samples;
	return s > 0 ? s : 1;
}

void Synth_Init(uint32_t sample_rate) {
	fs = sample_rate;
	/* f(m) = 440 * 2^((m-69)/12), inc = f * 2^32 / fs */
	const double semitono = 1.0594630943592953;
	double f = 440.0;
	for (int m = 69; m < 128; m++) { note_inc[m] = (uint32_t)(f * 4294967296.0 / fs + 0.5); f *= semitono; }
	f = 440.0;
	for (int m = 69; m >= 0; m--) { note_inc[m] = (uint32_t)(f * 4294967296.0 / fs + 0.5); f /= semitono; }
	Synth_AllOff();
}

void Synth_AllOff(void) {
	for (int i = 0; i < SYNTH_MAX_VOICES; i++) voices[i].stage = ENV_OFF;
}

uint8_t Synth_ActiveVoices(void) {
	uint8_t n = 0;
	for (int i = 0; i < SYNTH_MAX_VOICES; i++) if (voices[i].stage != ENV_OFF) n++;
	return n;
}

static Voice *Synth_AllocVoice(void) {
	Voice *best = &voices[0];
	for (int i = 0; i < SYNTH_MAX_VOICES; i++) {
		Voice *v = &voices[i];
		if (v->stage == ENV_OFF) return v;
		/* sin voces libres: la que está en release y más baja, si no la más vieja */
		int vr = (v->stage == ENV_RELEASE), br = (best->stage == ENV_RELEASE);
		if (vr > br || (vr == br && (vr ? v->level < best->level : v->age < best->age))) best = v;
	}
	return best;
}

void Synth_NoteOn(uint8_t track, uint8_t inst, uint8_t note, uint8_t vel, uint16_t dur_ticks) {
	if (inst >= INST_COUNT || note > 127) return;
	const Instrument *in = &synth_instruments[inst];
	Voice *v = Synth_AllocVoice();

	uint8_t  wave = in->wave;
	uint16_t decay_ms = in->decay_ms;
	uint32_t inc = note_inc[note];

	if (inst == INST_DRUMS) {                 /* batería General MIDI simplificada */
		if (note <= 36)                        { wave = WAVE_TRIANGLE; inc = note_inc[31]; decay_ms = 160; } /* bombo ~49 Hz */
		else if (note == 42 || note == 44)     { decay_ms = 35; }    /* hi-hat cerrado */
		else if (note == 46)                   { decay_ms = 150; }   /* hi-hat abierto */
		else if (note == 38 || note == 40)     { decay_ms = 110; }   /* caja */
		else                                   { decay_ms = 350; }   /* platillos / toms */
	}

	v->stage = ENV_OFF;                       /* que el render no la use a medio configurar */
	v->phase = 0;
	v->inc = inc;
	v->wave = wave;
	v->track = track;
	v->gain = (uint8_t)(((uint32_t)vel * 2u * in->amplitude) >> 8);
	v->sustain = (int32_t)((uint32_t)in->sustain << 16);           /* 0..255 -> Q24 */
	v->attack_step = Steps(in->attack_ms, ENV_MAX);
	v->decay_step = Steps(decay_ms, ENV_MAX - v->sustain > 0 ? ENV_MAX - v->sustain : ENV_MAX);
	v->release_step = Steps(in->release_ms, ENV_MAX);
	v->ticks_left = dur_ticks ? dur_ticks : 1;
	v->level = 0;
	v->age = ++age_counter;
	v->stage = ENV_ATTACK;
}

void Synth_Tick(void) {
	for (int i = 0; i < SYNTH_MAX_VOICES; i++) {
		Voice *v = &voices[i];
		if (v->stage == ENV_OFF || v->stage == ENV_RELEASE) continue;
		if (v->ticks_left && --v->ticks_left == 0) v->stage = ENV_RELEASE;
	}
}

void Synth_Render(int32_t *mix, uint32_t n, const uint8_t *track_vol) {
	for (int k = 0; k < SYNTH_MAX_VOICES; k++) {
		Voice *v = &voices[k];
		if (v->stage == ENV_OFF) continue;
		/* ganancia = velocidad * amplitud * volumen de pista */
		int32_t gain = ((int32_t)v->gain * track_vol[v->track]) >> 8;

		for (uint32_t i = 0; i < n; i++) {
			/* envolvente ADSR */
			switch (v->stage) {
			case ENV_ATTACK:
				v->level += v->attack_step;
				if (v->level >= ENV_MAX) { v->level = ENV_MAX; v->stage = ENV_DECAY; }
				break;
			case ENV_DECAY:
				v->level -= v->decay_step;
				if (v->level <= v->sustain) {
					v->level = v->sustain;
					v->stage = (v->sustain > 0) ? ENV_SUSTAIN : ENV_OFF;  /* percusivo: termina solo */
				}
				break;
			case ENV_RELEASE:
				v->level -= v->release_step;
				if (v->level <= 0) { v->level = 0; v->stage = ENV_OFF; }
				break;
			default: break;
			}
			if (v->stage == ENV_OFF) break;

			/* oscilador, muestra en Q15 */
			int32_t s;
			uint32_t p = v->phase;
			switch (v->wave) {
			case WAVE_SINE: {                  /* tabla de 256 + interpolación lineal (8 bits de fracción) */
				uint32_t idx = p >> 24, frac = (p >> 16) & 0xFF;
				int32_t a = sine_table[idx], b = sine_table[(idx + 1) & 0xFF];
				s = a + (((b - a) * (int32_t)frac) >> 8);
				break; }
			case WAVE_SQUARE:   s = (p & 0x80000000u) ? -32767 : 32767; break;
			case WAVE_SAW:      s = (int32_t)(p >> 16) - 32768; break;
			case WAVE_TRIANGLE: {
				int32_t q = (int32_t)(p >> 15);            /* 0..131071 */
				s = (q < 65536) ? q - 32768 : 98303 - q;   /* sube y baja */
				break; }
			default: /* WAVE_NOISE */
				rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
				s = (int16_t)(rng >> 16);
				break;
			}
			v->phase = p + v->inc;

			/* envolvente y volumen */
			s = (s * (v->level >> 9)) >> 15;       /* level Q24 -> Q15 */
			mix[i] += (s * gain) >> 8;
		}
	}
}
