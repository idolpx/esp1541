// Pi1541 - A Commodore 1541 disk drive emulator
// Copyright(C) 2018 Stephen White
//
// This file is part of Pi1541.
// 
// Pi1541 is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Pi1541 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Pi1541. If not, see <http://www.gnu.org/licenses/>.

#ifndef SCREEN_H
#define SCREEN_H

#include "ScreenBase.h"

class Screen : public ScreenBase
{

public:
	Screen()
		: ScreenBase()
	{
	}

	void Open(uint32_t width, uint32_t height, uint32_t colourDepth);

	void DrawRectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, RGBA colour);
	void Clear(RGBA colour);

	void ScrollArea(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);

	void WriteChar(bool petscii, uint32_t x, uint32_t y, unsigned char c, RGBA colour);
	uint32_t PrintText(bool petscii, uint32_t xPos, uint32_t yPos, char *ptr, RGBA TxtColour = RGBA(0xff, 0xff, 0xff, 0xff), RGBA BkColour = RGBA(0, 0, 0, 0xFF), bool measureOnly = false, uint32_t* width = 0, uint32_t* height = 0);
	uint32_t MeasureText(bool petscii, char *ptr, uint32_t* width = 0, uint32_t* height = 0);

	void DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, RGBA colour);
	void DrawLineV(uint32_t x, uint32_t y1, uint32_t y2, RGBA colour);
	void PlotPixel(uint32_t x, uint32_t y, RGBA colour);

	void PlotImage(uint32_t* image, int x, int y, int w, int h);

	float GetScaleX() const { return scaleX; }
	float GetScaleY() const { return scaleY; }

	uint32_t ScaleX(uint32_t x) { return (uint32_t)((float)x * scaleX); }
	uint32_t ScaleY(uint32_t y) { return (uint32_t)((float)y * scaleY); }

	uint32_t GetFontHeight();
	uint32_t GetFontHeightDirectoryDisplay();

	void SwapBuffers() {}
private:

	typedef void (Screen::*PlotPixelFunction)(uint32_t pixel_offset, RGBA Colour);

	PlotPixelFunction plotPixelFn;

	void PlotPixel32(uint32_t pixel_offset, RGBA Colour);
	void PlotPixel24(uint32_t pixel_offset, RGBA Colour);
	void PlotPixel16(uint32_t pixel_offset, RGBA Colour);
	void PlotPixel8(uint32_t pixel_offset, RGBA Colour);

	float scaleX;
	float scaleY;
};

#endif
