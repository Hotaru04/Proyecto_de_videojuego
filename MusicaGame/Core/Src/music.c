/*
 * music.c
 * Secuenciador: avanza todas las pistas con un mismo reloj de ticks y dispara las notas en synth.c.
 */
#include "music.h"
#include "synth.h"

static Track    tracks[MUSIC_MAX_TRACKS];
static uint8_t  track_vol[MUSIC_MAX_TRACKS];
static const Song *song = 0;
static uint8_t  num_tracks = 0;
static uint8_t  playing = 0;
static uint8_t  looping = 0;

static uint32_t sample_rate = 32000;
static uint32_t spt_q16 = 0;        /* muestras por tick, punto fijo 16.16 */
static uint32_t frac_q16 = 0;
static uint32_t samples_to_tick = 0;
static uint32_t song_tick = 0;

void Music_Init(uint32_t rate) {
	sample_rate = rate;
	playing = 0;
}

/* muestras por tick = Fs * 60 / (BPM * PPQ) */
static void Music_SetTempo(uint16_t bpm) {
	uint64_t num = (uint64_t)sample_rate * 60u << 16;
	spt_q16 = (uint32_t)(num / ((uint32_t)bpm * MUSIC_PPQ));
}

/* Muestras hasta el próximo tick; la fracción se acumula para no tener deriva */
static void Music_ScheduleNextTick(void) {
	frac_q16 += spt_q16;
	samples_to_tick = frac_q16 >> 16;
	frac_q16 &= 0xFFFF;
	if (samples_to_tick == 0) samples_to_tick = 1;
}

/* Dispara las notas de las pistas cuyo 'wait' llegó a 0 */
static void Music_ProcessEvents(void) {
	uint8_t alguna_sonando = 0;
	for (uint8_t t = 0; t < num_tracks; t++) {
		Track *tr = &tracks[t];
		while (tr->state == TRACK_PLAYING && tr->wait == 0) {
			if (tr->index >= tr->def->length) { tr->state = TRACK_FINISHED; break; }
			const Note *n = &tr->def->notes[tr->index++];
			if (n->note != REST) {
				uint8_t vel = n->vel ? n->vel : MUSIC_DEFAULT_VEL;
				Synth_NoteOn(t, tr->def->instrument, n->note, vel, n->dur);
			}
			tr->wait = (n->flags & NOTE_CHORD) ? 0 : n->dur;
		}
		if (tr->state == TRACK_PLAYING) alguna_sonando = 1;
	}
	if (!alguna_sonando && !Synth_ActiveVoices()) {
		if (looping) { Music_Play(song, 1); }
		else playing = 0;
	}
}

void Music_Play(const Song *s, uint8_t loop) {
	playing = 0;
	song = s;
	looping = loop;
	num_tracks = s->num_tracks > MUSIC_MAX_TRACKS ? MUSIC_MAX_TRACKS : s->num_tracks;
	for (uint8_t t = 0; t < num_tracks; t++) {
		tracks[t].def = &s->tracks[t];
		tracks[t].index = 0;
		tracks[t].wait = 0;
		tracks[t].volume = s->tracks[t].volume;
		tracks[t].state = TRACK_PLAYING;
		track_vol[t] = tracks[t].volume;
	}
	Music_SetTempo(s->bpm);
	frac_q16 = 0;
	song_tick = 0;
	Music_ScheduleNextTick();
	playing = 1;
	Music_ProcessEvents();
}

void Music_Stop(void) {
	playing = 0;
	for (uint8_t t = 0; t < num_tracks; t++) tracks[t].state = TRACK_STOPPED;
	Synth_AllOff();
}

uint8_t Music_IsPlaying(void) { return playing; }
uint32_t Music_GetTick(void) { return song_tick; }
const uint8_t *Music_TrackVolumes(void) { return track_vol; }

void Music_SetTrackVolume(uint8_t t, uint8_t v) {
	if (t < MUSIC_MAX_TRACKS) { tracks[t].volume = v; track_vol[t] = v; }
}

uint32_t Music_SamplesUntilTick(void) {
	return playing ? samples_to_tick : 0xFFFFFFFFu;
}

void Music_AdvanceSamples(uint32_t n) {
	if (!playing) return;
	samples_to_tick -= n;
	if (samples_to_tick != 0) return;

	song_tick++;
	Synth_Tick();
	for (uint8_t t = 0; t < num_tracks; t++)
		if (tracks[t].state == TRACK_PLAYING && tracks[t].wait) tracks[t].wait--;
	Music_ScheduleNextTick();
	Music_ProcessEvents();
}
