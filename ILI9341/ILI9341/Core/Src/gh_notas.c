/*
 * gh_notas.c
 * Botones de la pista.
 */
#include "gh_notas.h"
#include "ili9341.h"

void GH_DibujarBoton(uint8_t carril, uint8_t estado) {
	LCD_SpriteFast(GH_CARRIL_X(carril), GH_BOTON_Y, GH_SPR_W, GH_SPR_H,
	               gh_btn_sheet[carril], GH_BTN_CUADROS, estado, 0, 0, 0);
}

void GH_DibujarBotones(void) {
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) GH_DibujarBoton(c, GH_BTN_REPOSO);
}
