/*
 * juez.h
 * Califica cada golpe, lleva los puntos, silencia la pista del jugador que falla y maneja la pausa.
 *
 * Calificación por la distancia al momento en que la nota llega al botón:
 *   hasta JUEZ_PERFECTO_MS PERFECTO, hasta JUEZ_MEDIO_MS MEDIO, hasta JUEZ_JUSTO_MS JUSTO, si no FALLO.
 * Notas largas: suman JUEZ_PUNTOS_SOSTENIDA cada JUEZ_SOSTENIDA_CADA_MS mientras se sostienen.
 * Los puntos se escalan para que el máximo de cada jugador sea JUEZ_PUNTOS_MAXIMOS,
 * aunque uno tenga más notas que el otro.
 * Pausa: CONTINUAR / SALIR AL MENU, solo la maneja quien la puso.
 */
#ifndef INC_JUEZ_H_
#define INC_JUEZ_H_

#include <stdint.h>
#include "songs.h"

#define JUEZ_PERFECTO_MS   60
#define JUEZ_MEDIO_MS      120
#define JUEZ_JUSTO_MS      180
#define JUEZ_AJUSTE_MS     0     /* compensa el retraso del control si los golpes salen tarde */

#define JUEZ_PUNTOS_PERFECTO 100
#define JUEZ_PUNTOS_MEDIO     50
#define JUEZ_PUNTOS_JUSTO     10

#define JUEZ_PUNTOS_SOSTENIDA   5
#define JUEZ_SOSTENIDA_CADA_MS 100

#define JUEZ_PUNTOS_MAXIMOS 100000u

#define JUEZ_NOTAS_POR_CARRIL 16

#define JUEZ_PEDIDO_NADA       0
#define JUEZ_PEDIDO_SALIR      1

void Juez_Iniciar(const SongEntry *e, uint8_t nueva_cancion);
void Juez_Maximo(uint8_t jugador, uint32_t puntos_base);
void Juez_NotaNueva(uint8_t jugador, uint8_t carril, uint16_t faltan_ms, uint16_t largo_ms);
void Juez_Actualizar(void);
uint8_t Juez_EnPausa(void);
uint8_t Juez_Pedido(void);            /* se borra al leerlo */

extern volatile uint32_t juez_aciertos[2], juez_fallos[2], juez_puntos[2];
extern volatile uint8_t  juez_muteado[2];

#endif /* INC_JUEZ_H_ */
