/*
 * gh_sprites.h
 * Sprites del Beat Hero en RAM.
 */
#ifndef INC_GH_SPRITES_H_
#define INC_GH_SPRITES_H_

#include <stdint.h>

#define GH_NUM_CARRILES   4
#define GH_SPR_W          48
#define GH_SPR_H          48
#define GH_BTN_CUADROS    3
#define GH_COLA_TILE_H    8

/* Índices de cuadro en la hoja de botones (parámetro index de LCD_SpriteFast) */
#define GH_BTN_REPOSO     0
#define GH_BTN_PRESIONADO 1
#define GH_BTN_SOSTENIDO  2

/* Orden de carriles: 0 verde, 1 rojo, 2 amarillo, 3 azul */
extern uint8_t btn_green[], btn_red[], btn_yellow[], btn_blue[];
extern uint8_t note_green[], note_red[], note_yellow[], note_blue[];

extern uint8_t * const gh_btn_sheet[GH_NUM_CARRILES];
extern uint8_t * const gh_note[GH_NUM_CARRILES];

/* Notas largas: cabeza 48x48 y cola en mosaico 48x8 (apagada / encendida) */
extern uint8_t hold_head_green[], hold_head_red[], hold_head_yellow[], hold_head_blue[];
extern uint8_t hold_tail_green[], hold_tail_red[], hold_tail_yellow[], hold_tail_blue[];
extern uint8_t hold_tail_green_on[], hold_tail_red_on[], hold_tail_yellow_on[], hold_tail_blue_on[];
extern uint8_t * const gh_hold_head[GH_NUM_CARRILES];
extern uint8_t * const gh_hold_tail[GH_NUM_CARRILES];
extern uint8_t * const gh_hold_tail_on[GH_NUM_CARRILES];

#endif /* INC_GH_SPRITES_H_ */
