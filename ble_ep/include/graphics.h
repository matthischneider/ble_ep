#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include "display.h"

void gLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void gSetPixel(uint8_t x, uint8_t y, uint8_t color);
void gShow();
void gInit(char *buffer);

#endif /* GRAPHICS_H_ */
