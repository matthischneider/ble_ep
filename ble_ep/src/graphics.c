#include "graphics.h"

void gSetPixel(int x, int y, int color) {
	if (x < 128 && y < 96 && x >= 0 && y >= 0) {

		int bytePos = x / 8 + y * epd.bytesPerLine;
		int mask = 1 << (x % 8);
		if (color) {
			graphicsBuffer[bytePos] |= mask;
		} else {
			graphicsBuffer[bytePos] &= ~mask;
		}
	}
	displayDirty333=1;
}

void gLine(int x0, int y0, int x1, int y1, int color) {
	int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; /* error value e_xy */

	for (;;) { /* loop */
		gSetPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * err;
		if (e2 > dy) {
			err += dy;
			x0 += sx;
		} /* e_xy+e_x > 0 */
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		} /* e_xy+e_y < 0 */
	}
}

void gCircle(int x0, int y0, int radius, int color) {
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	gSetPixel(x0, y0 + radius, color);
	gSetPixel(x0, y0 - radius, color);
	gSetPixel(x0 + radius, y0, color);
	gSetPixel(x0 - radius, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		gSetPixel(x0 + x, y0 + y, color);
		gSetPixel(x0 - x, y0 + y, color);
		gSetPixel(x0 + x, y0 - y, color);
		gSetPixel(x0 - x, y0 - y, color);
		gSetPixel(x0 + y, y0 + x, color);
		gSetPixel(x0 - y, y0 + x, color);
		gSetPixel(x0 + y, y0 - x, color);
		gSetPixel(x0 - y, y0 - x, color);
	}
}

void gRect(int x0, int y0, int w, int h, int color) {
	gLine(x0, y0, x0, y0 + h, color);
	gLine(x0 + w, y0, x0 + w, y0 + h, color);
	gLine(x0, y0, x0 + w, y0, color);
	gLine(x0, y0 + h, x0 + w, y0 + h, color);
}

void gFillRect(int x0, int y0, int w, int h, int color) {
	for (int i = x0; i <= (x0 + w); i++) {
		for (int j = y0; j <= y0 + h; j++) {
			gSetPixel(i, j, color);
		}
	}
}

int getFontPixel(unsigned int x,unsigned  int y, unsigned char c) {
	if (x < font_width && y < font_height) {
		int bytePos = x / 8 + y * font_width/8;
		int shift = 7-(x % 8);
		int mask = 1<<shift ;
		return (font_bits[bytePos] & mask)>>shift;
	}else{
		return -1;
	}
}

int gDrawChar(int x, int y, unsigned char c, int color) {
	int startX = char_pos[c];
	int endX = char_pos[c + 1];
	int width = endX- startX;
	for(int i=0; i< width; i++){
		for(uint32_t j=0; j< font_height; j++){
			gSetPixel(x+i,y+j,getFontPixel(startX+i,j,c));
		}
	}
	return width;
}

void gDrawString(unsigned int x, unsigned int y, char* c, unsigned int color){
	int pos=0;
	for(int i=0;c[i]!='\0'&&i<30;i++){
		if(c[i]==' '){
			pos+=3;
		}else{
			pos += gDrawChar(x+pos,y,c[i],color) +1;
		}
	}
}
