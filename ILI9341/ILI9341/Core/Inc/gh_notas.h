/*
 * gh_notas.h
 * Medidas de la pista y dibujo de los botones. Pantalla vertical 240x320.
 */
#ifndef INC_GH_NOTAS_H_
#define INC_GH_NOTAS_H_

#include <stdint.h>
#include "gh_sprites.h"

#define GH_PANTALLA_W   240
#define GH_PANTALLA_H   320
#define GH_COLOR_FONDO  0x0000

/* 4 carriles de 52 px de paso */
#define GH_CARRIL_X(c)  (18 + (c) * 52)
#define GH_BOTON_Y      262

void GH_DibujarBoton(uint8_t carril, uint8_t estado);
void GH_DibujarBotones(void);

#endif /* INC_GH_NOTAS_H_ */
