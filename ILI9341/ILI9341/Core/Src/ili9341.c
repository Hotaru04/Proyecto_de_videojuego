/*
 * ili9341.c
 *
 *  Created on: Aug 20, 2024
 *      Author: Pablo Mazariegos
 */
#include <stdlib.h> // malloc()
#include <string.h> // memset()
#include <stdbool.h>
#include "pgmspace.h"
#include "ili9341.h"
#include "main.h"

/*  RST - PC1
 *  RD  - PA0
 *  WR  - PA1
 *  RS  - PA4
 *  CS  - PB0
 *
 *  D0 - PA9
 *  D1 - PC7
 *  D2 - PA10
 *  D3 - PB3
 *  D4 - PB5
 *  D5 - PB4
 *  D6 - PB10
 *  D7 - PA8
 * */

#define LCD_WR_PORT    GPIOA
#define LCD_WR_PIN     GPIO_PIN_1
#define LCD_WR_L()     (LCD_WR_PORT->BSRR = (LCD_WR_PIN<<16))
#define LCD_WR_H()     (LCD_WR_PORT->BSRR =  LCD_WR_PIN)

#define LCD_RS_PORT    GPIOA
#define LCD_RS_PIN     GPIO_PIN_4
#define LCD_RS_L()     (LCD_RS_PORT->BSRR = (LCD_RS_PIN<<16))	//
#define LCD_RS_H()     (LCD_RS_PORT->BSRR =  LCD_RS_PIN)		//Poner el modo RS en high es para mandar dato

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_0
#define LCD_CS_L()     (LCD_CS_PORT->BSRR = (LCD_CS_PIN<<16)) 	//Pone el chip select en activo para empezar la comunicación con la pantalla
#define LCD_CS_H()     (LCD_CS_PORT->BSRR =  LCD_CS_PIN)		//

typedef struct {
	uint32_t bsrrA;  // valores para GPIOA->BSRR
	uint32_t bsrrB;  // valores para GPIOB->BSRR
	uint32_t bsrrC;  // valores para GPIOC->BSRR
} GPIO_Lookup;

GPIO_Lookup busLookup[256];

/* [GH] Escritura rápida de un byte al bus usando la tabla busLookup (3 escrituras a BSRR, sin leer ODR).
 * Antes cada byte se armaba bit por bit con lectura-modificación-escritura de 3 puertos. */
#define LCD_BUS(b)   do { const GPIO_Lookup *e_ = &busLookup[(uint8_t)(b)]; \
                          GPIOA->BSRR = e_->bsrrA; GPIOB->BSRR = e_->bsrrB; GPIOC->BSRR = e_->bsrrC; } while (0)
#define LCD_WRITE(b) do { LCD_WR_L(); LCD_BUS(b); LCD_WR_H(); } while (0)

// [GH] (las máscaras DATA_PA/PB/PC ya no se usan: las funciones Fast usan busLookup)

extern const uint8_t smallFont[1140];
extern const uint16_t bigFont[1520];

// Definir color transparente (ejemplo: magenta 0xF81F)
#define TRANSPARENT_COLOR 0xF81F

typedef struct {
	const uint16_t *bitmap;
	uint32_t *transparent_mask;  // Bitmask para píxeles transparentes
	unsigned int width;
	unsigned int height;
} TransparentBitmap;

//***************************************************************************************************************************************
// Función para inicializar tabla para pines
//***************************************************************************************************************************************
void initLookup() {
	for (int val = 0; val < 256; val++) {
		GPIO_Lookup entry = { 0, 0, 0 };

		// D0 -> PA9
		if (val & 0x01)
			entry.bsrrA |= (1 << 9);          // set
		else
			entry.bsrrA |= (1 << (9 + 16));   // reset
		// D1 -> PC7
		if (val & 0x02)
			entry.bsrrC |= (1 << 7);
		else
			entry.bsrrC |= (1 << (7 + 16));
		// D2 -> PA10
		if (val & 0x04)
			entry.bsrrA |= (1 << 10);
		else
			entry.bsrrA |= (1 << (10 + 16));
		// D3 -> PB3
		if (val & 0x08)
			entry.bsrrB |= (1 << 3);
		else
			entry.bsrrB |= (1 << (3 + 16));
		// D4 -> PB5
		if (val & 0x10)
			entry.bsrrB |= (1 << 5);
		else
			entry.bsrrB |= (1 << (5 + 16));
		// D5 -> PB4
		if (val & 0x20)
			entry.bsrrB |= (1 << 4);
		else
			entry.bsrrB |= (1 << (4 + 16));
		// D6 -> PB10
		if (val & 0x40)
			entry.bsrrB |= (1 << 10);
		else
			entry.bsrrB |= (1 << (10 + 16));
		// D7 -> PA8
		if (val & 0x80)
			entry.bsrrA |= (1 << 8);
		else
			entry.bsrrA |= (1 << (8 + 16));
		busLookup[val] = entry;
	}
}

//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {
	initLookup();
	//****************************************
	// Secuencia de Inicialización
	//****************************************
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LCD_RD_Pin | LCD_WR_Pin | LCD_RS_Pin,
			GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(GPIOC, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(150);
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);

	//****************************************
	LCD_CMD(0xE9);  // SETPANELRELATED
	LCD_DATA(0x20);
	//****************************************
	LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
	HAL_Delay(100);
	//****************************************
	LCD_CMD(0xD1);    // (SETVCOM)
	LCD_DATA(0x00);
	LCD_DATA(0x71);
	LCD_DATA(0x19);
	//****************************************
	LCD_CMD(0xD0);   // (SETPOWER)
	LCD_DATA(0x07);
	LCD_DATA(0x01);
	LCD_DATA(0x08);
	//****************************************
	LCD_CMD(0x36);  // (MEMORYACCESS)
	LCD_DATA(LCD_MADCTL_VERTICAL); // [GH vertical] antes: 0x40 | 0x80 | 0x20 | 0x08 (horizontal)
	//****************************************
	LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
	LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
	//****************************************
	LCD_CMD(0xC1);    // (POWERCONTROL2)
	LCD_DATA(0x10);
	LCD_DATA(0x10);
	LCD_DATA(0x02);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
	LCD_DATA(0x00);
	LCD_DATA(0x35);
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA(0x01);
	LCD_DATA(0x02);
	//****************************************
	LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
	LCD_DATA(0x04); // 72Hz
	//****************************************
	LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
	LCD_DATA(0x01);
	LCD_DATA(0x44);
	//****************************************
	LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
	LCD_DATA(0x04);
	LCD_DATA(0x67);
	LCD_DATA(0x35);
	LCD_DATA(0x04);
	LCD_DATA(0x08);
	LCD_DATA(0x06);
	LCD_DATA(0x24);
	LCD_DATA(0x01);
	LCD_DATA(0x37);
	LCD_DATA(0x40);
	LCD_DATA(0x03);
	LCD_DATA(0x10);
	LCD_DATA(0x08);
	LCD_DATA(0x80);
	LCD_DATA(0x00);
	//****************************************
	LCD_CMD(0x2A); // Set_column_address 240px (CASET) [GH vertical] antes 0..319
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA((ILI9341_TFTWIDTH - 1) >> 8);
	LCD_DATA((ILI9341_TFTWIDTH - 1) & 0xFF);
	//****************************************
	LCD_CMD(0x2B); // Set_page_address 320px (PASET) [GH vertical] antes 0..240
	LCD_DATA(0x00);
	LCD_DATA(0x00);
	LCD_DATA((ILI9341_TFTHEIGHT - 1) >> 8);
	LCD_DATA((ILI9341_TFTHEIGHT - 1) & 0xFF);
	//LCD_DATA(0x01);
	//LCD_DATA(0xE0);
	//  LCD_DATA(0x8F);
	LCD_CMD(0x29); //display on
	LCD_CMD(0x2C); //display on

	LCD_CMD(ILI9341_INVOFF); //Invert Off
	HAL_Delay(120);
	LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
	HAL_Delay(120);
	LCD_CMD(ILI9341_DISPON);    //Display on
	HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
	LCD_RS_L();
	LCD_WR_L();
	GPIOA->BSRR |= busLookup[cmd].bsrrA;
	GPIOB->BSRR |= busLookup[cmd].bsrrB;
	GPIOC->BSRR |= busLookup[cmd].bsrrC;
	LCD_WR_H();
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
	LCD_RS_H();
	LCD_WR_L();
	GPIOA->BSRR |= busLookup[data].bsrrA;
	GPIOB->BSRR |= busLookup[data].bsrrB;
	GPIOC->BSRR |= busLookup[data].bsrrC;
	LCD_WR_H();
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2,
		unsigned int y2) {
	LCD_CMD(0x2a); // Set_column_address 4 parameters
	LCD_DATA(x1 >> 8);
	LCD_DATA(x1);
	LCD_DATA(x2 >> 8);
	LCD_DATA(x2);
	LCD_CMD(0x2b); // Set_page_address 4 parameters
	LCD_DATA(y1 >> 8);
	LCD_DATA(y1);
	LCD_DATA(y2 >> 8);
	LCD_DATA(y2);
	LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
	unsigned int x, y;
	LCD_CMD(0x02c); // write_memory_start
	//HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_RS_H();
	LCD_CS_L();
	SetWindows(0, 0, ILI9341_TFTWIDTH - 1, ILI9341_TFTHEIGHT - 1); // [GH vertical] antes (0, 0, 319, 239)
	for (x = 0; x < ILI9341_TFTWIDTH; x++)
		for (y = 0; y < ILI9341_TFTHEIGHT; y++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);
		}
	LCD_CS_H();
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	//HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_RS_H();
	LCD_CS_L();

	//l = l + x;
	SetWindows(x, y, l + x, y);
	//j = l; // * 2;
	for (i = 0; i < l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	//HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_RS_H();
	LCD_CS_L();
	//l = l + y;
	SetWindows(x, y, x, y + l);
	//j = l; //* 2;
	for (i = 1; i <= l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	H_line(x, y, w, c);
	H_line(x, y + h, w, c);
	V_line(x, y, h, c);
	V_line(x + w, y, h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	LCD_CMD(0x02c); // write_memory_start
	//HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_CS_L();
	SetWindows(x, y, x + w - 1, y + h - 1);
	LCD_RS_H();
	unsigned int k = w * h * 2 - 1;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);

			//LCD_DATA(bitmap[k]);
			k = k - 2;
		}
	}
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRectFast(unsigned int x, unsigned int y, unsigned int w,
		unsigned int h, unsigned int c) {
	/* [GH] Versión con busLookup: el color se separa en byte alto y bajo una sola vez */
	const GPIO_Lookup hi = busLookup[(c >> 8) & 0xFF];
	const GPIO_Lookup lo = busLookup[c & 0xFF];

	LCD_CS_L();
	SetWindows(x, y, x + w - 1, y + h - 1);
	LCD_RS_H();
	for (uint32_t n = (uint32_t)w * h; n; n--) {
		LCD_WR_L(); GPIOA->BSRR = hi.bsrrA; GPIOB->BSRR = hi.bsrrB; GPIOC->BSRR = hi.bsrrC; LCD_WR_H();
		LCD_WR_L(); GPIOA->BSRR = lo.bsrrA; GPIOB->BSRR = lo.bsrrB; GPIOC->BSRR = lo.bsrrC; LCD_WR_H();
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//***************************************************************************************************************************************
void LCD_Print(char *text, int x, int y, int fontSize, int color,
		int background) {

	int fontXSize;
	int fontYSize;

	if (fontSize == 1) {
		fontXSize = fontXSizeSmal;
		fontYSize = fontYSizeSmal;
	}
	if (fontSize == 2) {
		fontXSize = fontXSizeBig;
		fontYSize = fontYSizeBig;
	}
	if (fontSize == 3) {
		fontXSize = fontXSizeNum;
		fontYSize = fontYSizeNum;
	}

	char charInput;
	int cLength = strlen(text);
	//Serial.println(cLength, DEC);
	int charDec;
	int c;
	//int charHex;
	char char_array[cLength + 1];
	for (int i = 0; text[i] != '\0'; i++) {
		char_array[i] = text[i];
	}

	//text.toCharArray(char_array, cLength + 1);

	for (int i = 0; i < cLength; i++) {
		charInput = char_array[i];
		//Serial.println(char_array[i]);
		charDec = (int) charInput;
		LCD_CS_L();  // CS = 0
		SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1,
				y + fontYSize);
		long charHex1;
		for (int n = 0; n < fontYSize; n++) {
			if (fontSize == 1) {
				charHex1 = pgm_read_word_near(
						smallFont + ((charDec - 32) * fontYSize) + n);
			}
			if (fontSize == 2) {
				charHex1 = pgm_read_word_near(
						bigFont + ((charDec - 32) * fontYSize) + n);
			}
			for (int t = 1; t < fontXSize + 1; t++) {
				if ((charHex1 & (1 << (fontXSize - t))) > 0) {
					c = color;
				} else {
					c = background;
				}
				LCD_DATA(c >> 8);
				LCD_DATA(c);
			}
		}
		LCD_CS_H();  // CS = 1
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height, const uint16_t *bitmap) {
	LCD_CMD(0x02c); // write_memory_start
	LCD_CS_L();
	SetWindows(x, y, x + width - 1, y + height - 1);
	LCD_RS_H();
	unsigned int total_pixels = width * height;

	// Enviar datos en bloques si es posible
	for (unsigned int i = 0; i < total_pixels; i++) {
		uint16_t pixel = bitmap[i];
		LCD_DATA(pixel >> 8);
		LCD_DATA(pixel & 0xFF);
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits) con color transparente
//***************************************************************************************************************************************
void LCD_BitmapTransparent(uint16_t x, uint16_t y, uint16_t width,
		uint16_t height, const uint16_t *bitmap, uint16_t transparentColor) {
	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			unsigned int index = j * width + i;
			uint16_t pixel = bitmap[index];

			if (pixel != transparentColor) {
				// Comandos mínimos para un solo píxel
				LCD_CS_L();
				SetWindows(x + i, y + j, x + i, y + j);
				LCD_CMD(0x02c);
				LCD_RS_H();
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				LCD_CS_H();
			}
		}
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_BitmapFast(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height, const uint8_t *bitmap) {
	/* [GH] Versión con busLookup. El bitmap viene en RGB565, byte alto primero */
	LCD_CS_L();
	SetWindows(x, y, x + width - 1, y + height - 1);
	LCD_RS_H();
	const uint8_t *p = bitmap;
	for (uint32_t n = (uint32_t)width * height * 2u; n; n--) {
		LCD_WRITE(*p++);
	}
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, const uint16_t *bitmap,
                int columns, int index, char flip, char offset) {
    LCD_CMD(0x02c); // write_memory_start
    LCD_RS_H();
    LCD_CS_L();

    SetWindows(x, y,  x + width - 1, y + height - 1);

    int ancho = width * columns;
    int k = 0;

    if (flip) {
        for (int j = 0; j < height; j++) {
            k = j * ancho + index * width - 1 - offset;
            k = k + width;
            for (int i = 0; i < width; i++) {
                uint16_t pixel = bitmap[k];
                LCD_DATA(pixel >> 8);
                LCD_DATA(pixel & 0xFF);
                k--;
            }
        }
    } else {
        for (int j = 0; j < height; j++) {
            k = j * ancho + index * width + 1 + offset;
            for (int i = 0; i < width; i++) {
                uint16_t pixel = bitmap[k];
                LCD_DATA(pixel >> 8);
                LCD_DATA(pixel & 0xFF);
                k++;
            }
        }
    }
    LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_SpriteFast(int x, int y, int width, int height, const uint8_t *bitmap,
		int columns, int index, bool flipX, bool flipY, int offset) {
	/* [GH] Versión con busLookup; misma lógica de columnas / índice / volteo que antes */
	LCD_CS_L();
	SetWindows(x, y, x + width - 1, y + height - 1);
	LCD_RS_H();

	const int stride = width * columns * 2;                         /* bytes de una fila de la hoja */
	const uint8_t *basePtr = bitmap + (index * width + offset) * 2; /* primer byte del cuadro */

	for (int row = 0; row < height; row++) {
		int ry = flipY ? (height - 1 - row) : row;
		const uint8_t *fila = basePtr + ry * stride;
		for (int col = 0; col < width; col++) {
			int cx = flipX ? (width - 1 - col) : col;
			const uint8_t *q = fila + cx * 2;
			LCD_WRITE(q[0]);   /* byte alto */
			LCD_WRITE(q[1]);   /* byte bajo */
		}
	}
	LCD_CS_H();
}

void LCD_Sprite_Transparent(int x, int y, int width, int height,
		const uint16_t *bitmap, uint8_t columns, uint8_t index, uint8_t flip,
		uint8_t offset, uint16_t transparent_color) {
	LCD_RS_H();

	int ancho = width * columns;
	int k = 0;

	if (flip) {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width - 1 - offset;
			k = k + width;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				// Dibujar solo si no es transparente
				if (pixel != transparent_color) {
					LCD_CS_L();
					SetWindows(x + (width - 1 - i), y + j, x + (width - 1 - i),
							y + j);
					LCD_CMD(0x02c);
					LCD_DATA(pixel >> 8);
					LCD_DATA(pixel & 0xFF);
					LCD_CS_H();
				}
				k--;
			}
		}
	} else {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width + 1 + offset;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				// Dibujar solo si no es transparente
				if (pixel != transparent_color) {
					LCD_CS_L();
					SetWindows(x + i, y + j, x + i, y + j);
					LCD_CMD(0x02c);
					LCD_DATA(pixel >> 8);
					LCD_DATA(pixel & 0xFF);
					LCD_CS_H();
				}
				k++;
			}
		}
	}
}

//***************************************************************************************************************************************
// [GH] Scroll vertical por hardware (comandos MIPI 0x33 y 0x37)
//   La pantalla se divide en: zona fija arriba (tfa) + zona que se desplaza (vsa) + zona fija abajo (bfa).
//   tfa + vsa + bfa = 320. LCD_ScrollStart(vsp) elige qué fila de memoria se ve arriba de la zona que se desplaza.
//***************************************************************************************************************************************
void LCD_ScrollArea(uint16_t tfa, uint16_t vsa, uint16_t bfa) {
	LCD_CS_L();
	LCD_CMD(0x33);
	LCD_DATA(tfa >> 8); LCD_DATA(tfa & 0xFF);
	LCD_DATA(vsa >> 8); LCD_DATA(vsa & 0xFF);
	LCD_DATA(bfa >> 8); LCD_DATA(bfa & 0xFF);
	LCD_CS_H();
}

void LCD_ScrollStart(uint16_t vsp) {
	LCD_CS_L();
	LCD_CMD(0x37);
	LCD_DATA(vsp >> 8); LCD_DATA(vsp & 0xFF);
	LCD_CS_H();
}
