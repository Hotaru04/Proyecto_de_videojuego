/*
 * audio.h
 * Salida de audio por el DAC1 (PA4): TIM6 marca la frecuencia de muestreo y el DMA (circular)
 * envía audio_buffer. En cada mitad del buffer se calcula la otra.
 */
#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

#include <stdint.h>
#include "music.h"

#define AUDIO_SAMPLE_RATE   32000u
#define AUDIO_BUFFER_SIZE   512u     /* 2 mitades de 256 muestras (8 ms cada una) */
#define AUDIO_MASTER_VOLUME 224u     /* 256 = 1.0 */

void Audio_Init(void);
void Audio_Start(void);
void Audio_PlaySong(const Song *song, uint8_t loop);
void Audio_SetMasterVolume(uint16_t vol);
void Audio_Pausa(uint8_t si);
void Audio_Detener(void);
void Audio_FillBuffer(uint16_t *buf, uint32_t n);

extern volatile uint8_t  audio_cpu_load;     /* % de CPU que usa el audio */
extern volatile uint32_t audio_underruns;

#endif /* INC_AUDIO_H_ */
