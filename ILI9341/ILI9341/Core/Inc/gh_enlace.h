/*
 * gh_enlace.h
 * configuracion de SPI2 ESCLAVO
 */
#ifndef INC_GH_ENLACE_H_
#define INC_GH_ENLACE_H_

#include <stdint.h>
#include "gh_protocolo.h"

typedef struct {
	uint8_t  b[GH_PKT_LEN];   /* paquete completo y ya verificado */
	uint32_t t_ms;            /* HAL_GetTick() cuando terminó de llegar */
} GH_Paquete;

void    GH_Enlace_Iniciar(void);
uint8_t GH_Enlace_Leer(GH_Paquete *p);   /* 1 si había un paquete en la cola */

extern volatile uint32_t enlace_ok, enlace_malos, enlace_perdidos;

#endif /* INC_GH_ENLACE_H_ */
