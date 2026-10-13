/*
 * controles.h
 * Botones de los 2 jugadores que manda la ESP32 por UART ("p1_A", "p2_B_off", ...).
 * USART1 RX = PA10, 115200 8N1, por registros (no activar USART1 en CubeMX).
 */
#ifndef INC_CONTROLES_H_
#define INC_CONTROLES_H_

#include <stdint.h>

#define CONTROLES_JUGADORES 2
#define CONTROLES_CARRILES  4      /* 0 verde (A), 1 rojo (B), 2 amarillo (X), 3 azul (Y) */
#define CONTROL_START       4
#define CONTROL_ARRIBA      5
#define CONTROL_ABAJO       6

typedef struct {
	uint8_t  jugador;     /* 0 o 1 */
	uint8_t  carril;      /* 0..3 o CONTROL_* */
	uint8_t  presionado;
	uint32_t t_ms;        /* cuándo llegó el comando */
} Control_Evento;

void    Controles_Init(void);
uint8_t Controles_Siguiente(Control_Evento *ev);     /* 1 si hubo un cambio de botón */
uint8_t Controles_Estado(uint8_t jugador);           /* carriles presionados ahora */

extern volatile uint32_t controles_lineas, controles_perdidas, controles_errores;

#endif /* INC_CONTROLES_H_ */
