/*
 * audio.h
 * Salida de audio: TIM6 dispara el DAC1 (PA4) a AUDIO_SAMPLE_RATE y el DMA (circular) le entrega
 * las muestras desde audio_buffer. En cada mitad del buffer (callbacks del DMA) se calcula la otra mitad.
 */
#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

#include <stdint.h>
#include "music.h"

#define AUDIO_SAMPLE_RATE   32000u   /* Hz */
#define AUDIO_BUFFER_SIZE   512u     /* muestras en total (2 mitades de 256 = 8 ms cada una) */
#define AUDIO_MASTER_VOLUME 224u     /* 256 = 1.0; 224 deja algo de margen contra recorte */

void     Audio_Init(void);                                /* después de MX_DAC_Init / MX_TIM6_Init */
void     Audio_Start(void);                               /* arranca TIM6 + DAC + DMA */
void     Audio_Stop(void);
void     Audio_PlaySong(const Song *song, uint8_t loop);  /* seguro desde main (pausa la IRQ del DMA) */
void     Audio_SetMasterVolume(uint16_t vol);             /* 0..512, 256 = 1.0 */
void     Audio_Pausa(uint8_t si);                         /* 1 = silencio y la canción se congela; 0 = sigue igual */
void     Audio_Detener(void);                             /* corta la canción (reiniciar / salir al menú) */

/* Llena 'n' muestras de 12 bits (0..4095) en buf: secuenciador + voces + mezcla + recorte + offset */
void     Audio_FillBuffer(uint16_t *buf, uint32_t n);

/* Diagnóstico: % de CPU que usa el audio (medido con el contador de ciclos DWT) */
extern volatile uint8_t  audio_cpu_load;
extern volatile uint32_t audio_underruns;

#endif /* INC_AUDIO_H_ */
