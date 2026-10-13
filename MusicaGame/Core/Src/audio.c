/*
 * audio.c
 * TIM6 (TRGO) -> DAC canal 1 (PA4) <- DMA1 Stream5, modo circular.
 * Al terminar cada mitad del buffer se recalcula esa mitad mientras el DMA lee la otra.
 */
#include "audio.h"
#include "synth.h"
#include "main.h"
#include <string.h>

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim6;

#define HALF_SIZE (AUDIO_BUFFER_SIZE / 2)

static uint16_t audio_buffer[AUDIO_BUFFER_SIZE];
static int32_t  mix_buffer[HALF_SIZE];
static uint16_t master_volume = AUDIO_MASTER_VOLUME;
static volatile uint8_t pausa = 0;

volatile uint8_t  audio_cpu_load = 0;
volatile uint32_t audio_underruns = 0;

/* TIM6 está en APB1: si su prescaler no es 1, el timer corre a 2 * PCLK1 */
static uint32_t Audio_TimerClock(void) {
	uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
	return ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) ? pclk1 : 2u * pclk1;
}

void Audio_Init(void) {
	uint32_t arr = Audio_TimerClock() / AUDIO_SAMPLE_RATE;
	__HAL_TIM_SET_PRESCALER(&htim6, 0);
	__HAL_TIM_SET_AUTORELOAD(&htim6, arr - 1u);

	Synth_Init(AUDIO_SAMPLE_RATE);
	Music_Init(AUDIO_SAMPLE_RATE);

	for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++) audio_buffer[i] = 2048;   /* silencio */

	/* contador de ciclos para medir la carga de CPU */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Audio_Start(void) {
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)audio_buffer, AUDIO_BUFFER_SIZE, DAC_ALIGN_12B_R);
	HAL_TIM_Base_Start(&htim6);
}

void Audio_PlaySong(const Song *song, uint8_t loop) {
	HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);
	Music_Play(song, loop);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void Audio_SetMasterVolume(uint16_t vol) { master_volume = vol; }

/* En pausa la canción no avanza: al quitarla sigue donde estaba */
void Audio_Pausa(uint8_t si) { pausa = si; }

void Audio_Detener(void) {
	HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);
	Music_Stop();
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	pausa = 0;
}

/* Mezcla las voces y la pasa a 12 bits: out = mix * master / 256 / 16, recortada a ±2047 y centrada en 2048 */
void Audio_FillBuffer(uint16_t *buf, uint32_t n) {
	uint32_t t0 = DWT->CYCCNT;

	if (pausa) {
		for (uint32_t i = 0; i < n; i++) buf[i] = 2048;
		audio_cpu_load = 0;
		return;
	}

	memset(mix_buffer, 0, n * sizeof(int32_t));

	/* por bloques entre ticks musicales para que el tiempo sea exacto */
	uint32_t done = 0;
	while (done < n) {
		uint32_t chunk = n - done;
		uint32_t until = Music_SamplesUntilTick();
		if (until < chunk) chunk = until;
		Synth_Render(&mix_buffer[done], chunk, Music_TrackVolumes());
		Music_AdvanceSamples(chunk);
		done += chunk;
	}

	for (uint32_t i = 0; i < n; i++) {
		int32_t s = (mix_buffer[i] * (int32_t)master_volume) >> 12;
		if (s > 2047) s = 2047;
		else if (s < -2048) s = -2048;
		buf[i] = (uint16_t)(s + 2048);
	}

	uint32_t used = DWT->CYCCNT - t0;
	uint32_t budget = (SystemCoreClock / AUDIO_SAMPLE_RATE) * n;
	audio_cpu_load = (uint8_t)((used * 100u) / budget);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	Audio_FillBuffer(&audio_buffer[0], HALF_SIZE);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	Audio_FillBuffer(&audio_buffer[HALF_SIZE], HALF_SIZE);
}

void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	audio_underruns++;
}
