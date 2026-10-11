/*
 * juez.h
 * El juez del juego: califica cada golpe con el reloj de MusicaGame, lleva los puntos, mutea la pista
 * del jugador que falla (hasta que vuelva a atinar), maneja la pausa y le avisa a su pantalla qué dibujar.
 *
 *   beats.c  -> Juez_NotaNueva()   cada nota que se manda a una pantalla
 *   main.c   -> Juez_Actualizar()  en el while(1)
 *
 * Calificación según qué tan lejos del momento en que la nota llega al botón se presionó (antes o después):
 *   hasta JUEZ_PERFECTO_MS  -> PERFECTO, 100 puntos
 *   hasta JUEZ_MEDIO_MS     -> MEDIO,     50 puntos
 *   hasta JUEZ_JUSTO_MS     -> JUSTO,     10 puntos
 *   más lejos o nunca       -> FALLO,      0 puntos y se mutea la pista del jugador
 * Cualquier acierto (aunque sea JUSTO) hace que la pista vuelva a sonar.
 *
 * Pausa: Start de cualquier jugador. Solo el que la puso la puede quitar.
 */
#ifndef INC_JUEZ_H_
#define INC_JUEZ_H_

#include <stdint.h>
#include "songs.h"

#define JUEZ_PERFECTO_MS   60
#define JUEZ_MEDIO_MS      120
#define JUEZ_JUSTO_MS      180
#define JUEZ_AJUSTE_MS     0     /* si en la bitácora casi todos los golpes salen "+40 ms" (tarde), pon 40 aquí:
                                    compensa el retraso del Bluetooth del control */

#define JUEZ_PUNTOS_PERFECTO 100
#define JUEZ_PUNTOS_MEDIO     50
#define JUEZ_PUNTOS_JUSTO     10

#define JUEZ_NOTAS_POR_CARRIL 16

void Juez_Iniciar(const SongEntry *e, uint8_t nueva_cancion);   /* e = 0: nada. Al repetir (0) se conservan los puntos */
void Juez_NotaNueva(uint8_t jugador, uint8_t carril, uint16_t faltan_ms, uint16_t largo_ms);
void Juez_Actualizar(void);
uint8_t Juez_EnPausa(void);

/* Diagnóstico (Live Expressions): [0] jugador 1, [1] jugador 2 */
extern volatile uint32_t juez_aciertos[2], juez_fallos[2], juez_puntos[2];
extern volatile uint8_t  juez_muteado[2];

#endif /* INC_JUEZ_H_ */
