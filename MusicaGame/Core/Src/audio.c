/*
 * audio.c
 * TIM6 (TRGO) -> DAC canal 1 (PA4) <- DMA1 Stream5 Canal 7, modo circular, media palabra.
 *
 *   audio_buffer: [ mitad A (256) | mitad B (256) ]
 *   - Cuando el DMA termina la mitad A -> HAL_DAC_ConvHalfCpltCallbackCh1 -> se recalcula A
 *     (el DMA ya está leyendo B).
 *   - Cuando termina B -> HAL_DAC_ConvCpltCallbackCh1 -> se recalcula B.
 * No hay interrupción por muestra: 2 interrupciones cada 16 ms.
 */
#include "audio.h"
#include "synth.h"
#include "main.h"     /* HAL + handles generados por CubeMX */
#include <string.h>

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim6;

#define HALF_SIZE (AUDIO_BUFFER_SIZE / 2)

static uint16_t audio_buffer[AUDIO_BUFFER_SIZE];   /* lo lee el DMA */
static int32_t  mix_buffer[HALF_SIZE];             /* suma de voces antes de convertir */
static uint16_t master_volume = AUDIO_MASTER_VOLUME;
static volatile uint8_t pausa = 0;

volatile uint8_t  audio_cpu_load = 0;
volatile uint32_t audio_underruns = 0;

/* ---------------------------------------------------------------------------------------------
 * Reloj del TIM6: TIM6 cuelga de APB1. Si el prescaler de APB1 != 1, el reloj del timer es 2*PCLK1.
 * ARR = f_timer / Fs - 1 (PSC = 0). Ej.: 80 MHz / 32000 = 2500 -> ARR = 2499 (exacto).
 * ------------------------------------------------------------------------------------------- */
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

	for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++) audio_buffer[i] = 2048;  /* silencio = mitad */

	/* contador de ciclos para medir carga de CPU */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Audio_Start(void) {
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)audio_buffer, AUDIO_BUFFER_SIZE, DAC_ALIGN_12B_R);
	HAL_TIM_Base_Start(&htim6);
}

void Audio_Stop(void) {
	HAL_TIM_Base_Stop(&htim6);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
}

void Audio_PlaySong(const Song *song, uint8_t loop) {
	HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);   /* que el callback no lea pistas a medio reiniciar */
	Music_Play(song, loop);
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void Audio_SetMasterVolume(uint16_t vol) { master_volume = vol; }

/* En pausa no se avanza la canción ni las voces: al quitarla sigue exactamente donde estaba */
void Audio_Pausa(uint8_t si) { pausa = si; }

void Audio_Detener(void) {
	HAL_NVIC_DisableIRQ(DMA1_Stream5_IRQn);
	Music_Stop();
	HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	pausa = 0;
}

/* ---------------------------------------------------------------------------------------------
 * Mezcla -> DAC de 12 bits
 *   Cada voz aporta como máximo ±32767 (Q15) ya multiplicada por envolvente y volúmenes.
 *   out = mix * master / 256 / 16  -> una voz a tope = ±2047, el rango completo del DAC.
 *   Con varias voces la suma puede pasar de ±2047: se recorta (clipping) para no "dar la vuelta".
 *   Se suma 2048 porque el DAC solo da 0..3.3 V: el silencio es 1.65 V (código 2048).
 * ------------------------------------------------------------------------------------------- */
void Audio_FillBuffer(uint16_t *buf, uint32_t n) {
	uint32_t t0 = DWT->CYCCNT;

	if (pausa) {                                       /* silencio (mitad de escala) y el tiempo no corre */
		for (uint32_t i = 0; i < n; i++) buf[i] = 2048;
		audio_cpu_load = 0;
		return;
	}

	memset(mix_buffer, 0, n * sizeof(int32_t));

	/* Render por bloques entre ticks musicales: el tiempo es exacto a la muestra */
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
		int32_t s = (mix_buffer[i] * (int32_t)master_volume) >> 12;   /* /256 (master) /16 (a 12 bits) */
		if (s > 2047) s = 2047;
		else if (s < -2048) s = -2048;
		buf[i] = (uint16_t)(s + 2048);
	}

	/* carga = ciclos usados / ciclos disponibles en el tiempo de media mitad */
	uint32_t used = DWT->CYCCNT - t0;
	uint32_t budget = (SystemCoreClock / AUDIO_SAMPLE_RATE) * n;
	audio_cpu_load = (uint8_t)((used * 100u) / budget);
}

/* ---------------- Callbacks del DAC/DMA (nombres reales de HAL STM32F4) ---------------- */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	Audio_FillBuffer(&audio_buffer[0], HALF_SIZE);           /* el DMA ya va en la mitad B */
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	Audio_FillBuffer(&audio_buffer[HALF_SIZE], HALF_SIZE);   /* el DMA volvió a la mitad A */
}

void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *h) {
	(void)h;
	audio_underruns++;
}
