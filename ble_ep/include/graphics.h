#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include "display.h"
#include "font.h"
unsigned char* currentBuffer;
unsigned char* lastBuffer;
unsigned char buffer[2][1536];


uint8_t displayDirty;
void gInit();
void gLine(int x0, int y0, int x1, int y1, int color);
void gSetPixel(int x, int y, int color);
void gCircle(int x0, int y0, int radius, int color);
void gRect(int x0, int y0, int w, int h, int color);
void gFillRect(int x0, int y0, int w, int h, int color);
int gDrawChar(int x, int y, unsigned char c, int color);
void gDrawString(unsigned int x, unsigned int y,  char* c, unsigned int color);
void gUpdate();

#endif /* GRAPHICS_H_ */
