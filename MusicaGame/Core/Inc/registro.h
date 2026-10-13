/*
 * registro.h
 * Bitácora por USART2 (puerto COM del ST-Link), 115200 8N1.
 * No bloquea: si se escribe más rápido de lo que sale, se pierde texto (registro_perdidos).
 * Por registros (no activar USART2 en CubeMX).
 */
#ifndef INC_REGISTRO_H_
#define INC_REGISTRO_H_

#include <stdint.h>

#define REGISTRO_ACTIVO   1

void Registro_Init(void);
void Registro(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

extern volatile uint32_t registro_perdidos;

#endif /* INC_REGISTRO_H_ */
