/*
 * registro.h
 * Bitácora por USART2 -> ST-Link -> puerto COM de la PC (el mismo cable USB de la Nucleo).
 * Abrir en Termite: el COM de "STMicroelectronics STLink Virtual COM Port", 115200 8N1.
 *
 * No bloquea: el texto se guarda en un buffer y la interrupción de USART2 lo va mandando.
 * Si se escribe más rápido de lo que sale, se pierde texto (se cuenta en registro_perdidos)
 * pero el juego y el audio nunca esperan.
 *
 * Por registros: NO actives USART2 en CubeMX (los pines PA2/PA3 ya vienen configurados).
 */
#ifndef INC_REGISTRO_H_
#define INC_REGISTRO_H_

#include <stdint.h>

#define REGISTRO_ACTIVO   1     /* 0 = apagar toda la bitácora */

void Registro_Init(void);
void Registro(const char *fmt, ...) __attribute__((format(printf, 1, 2)));   /* como printf; agrega la hora y \r\n */

extern volatile uint32_t registro_perdidos;

#endif /* INC_REGISTRO_H_ */
