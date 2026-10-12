/*
 * gh_menu.h
 * Pantallas fuera de la pista: lista de canciones, resultados (ganador) y menú final
 * (SELECCIONAR CANCION / REVANCHA). Las decide MusicaGame con GH_PKT_PANTALLA; aquí solo se dibujan.
 */
#ifndef INC_GH_MENU_H_
#define INC_GH_MENU_H_

#include <stdint.h>


/* Devuelve 1 si el paquete era de menús (TEXTO, RESULTADO, PANTALLA) y ya se atendió */
uint8_t GH_Menu_Paquete(const uint8_t *pkt);

void GH_Menu_Actualizar(void);

#endif /* INC_GH_MENU_H_ */
