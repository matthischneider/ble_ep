#include "graphics.h"

void gInit( char *buffer) {
	//graphicsBuffer = buffer;
}

void gShow() {
	epdBegin();
	//epdFrame(graphicsBuffer);
	epdEnd();
}

void gSetPixel(uint8_t x, uint8_t y, uint8_t color) {
	uint32_t bytePos = x / 8 + y * epd.bytesPerLine;
	uint8_t mask = 1 << (x % 8);
	if (color) {
		//graphicsBuffer[bytePos] |= mask;
	} else {
		//graphicsBuffer[bytePos] &= ~mask;
	}
}

void gLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
	uint8_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	uint8_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	uint8_t err = dx + dy, e2; // error value e_xy

	for (;;) { //* loop
		gSetPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * err;
		if (e2 > dy) {
			err += dy;
			x0 += sx;
		} //* e_xy+e_x > 0
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		} //* e_xy+e_y < 0
	}
}
