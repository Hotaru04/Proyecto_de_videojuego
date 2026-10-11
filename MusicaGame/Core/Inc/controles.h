/*
 * controles.h
 * Botones de los 2 jugadores que manda la ESP32 por UART ("p1_A", "p2_B_off", ...).
 *   USART1 RX = PA10 (D2 de la Nucleo) <--- TX (GPIO17) de la ESP32. 115200 8N1. GND común.
 *
 * Por registros, con su propia interrupción (USART1_IRQHandler está en controles.c):
 * NO actives USART1 en CubeMX.
 *
 * La interrupción guarda cada línea con la hora en que llegó (HAL_GetTick) en una cola.
 * Controles_Siguiente() las procesa y devuelve, una por una, las PRESIONES y SOLTADAS de botón.
 */
#ifndef INC_CONTROLES_H_
#define INC_CONTROLES_H_

#include <stdint.h>

#define CONTROLES_JUGADORES 2
#define CONTROLES_CARRILES  4      /* 0 verde (A), 1 rojo (B), 2 amarillo (X), 3 azul (Y) */
#define CONTROL_START       4      /* 'carril' que se reporta cuando se presiona Start (pausa) */
#define CONTROL_ARRIBA      5      /* flecha arriba (menús) */
#define CONTROL_ABAJO       6      /* flecha abajo (menús) */

typedef struct {
	uint8_t  jugador;     /* 0 = jugador 1, 1 = jugador 2 */
	uint8_t  carril;      /* 0..3, o CONTROL_START / CONTROL_ARRIBA / CONTROL_ABAJO */
	uint8_t  presionado;  /* 1 = se presionó, 0 = se soltó */
	uint32_t t_ms;        /* HAL_GetTick() cuando llegó el comando */
} Control_Evento;

void    Controles_Init(void);
uint8_t Controles_Siguiente(Control_Evento *ev);     /* 1 si había un cambio de botón */
uint8_t Controles_Estado(uint8_t jugador);           /* bits de los botones presionados ahora */

/* Diagnóstico (Live Expressions) */
extern volatile uint32_t controles_lineas, controles_perdidas, controles_errores;

#endif /* INC_CONTROLES_H_ */
