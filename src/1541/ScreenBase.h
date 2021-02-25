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

#ifndef SCREENBASE_H
#define SCREENBASE_H

#include "types.h"

typedef uint32_t RGBA;

#define RED(colour)   ( (uint8_t)(((uint32_t)colour) & 0xFF) )
#define GREEN(colour)  ( (uint8_t)(((uint32_t)colour >> 8) & 0xFF) )
#define BLUE(colour)    ( (uint8_t)(((uint32_t)colour >> 16) & 0xFF) )
#define ALPHA(colour)  ( (uint8_t)(((uint32_t)colour >> 24) & 0xFF) )

#define RGBA(r, g, b, a)  ( ((uint32_t)((uint8_t)(r))) | ((uint32_t)((uint8_t)(g)) << 8) | ((uint32_t)((uint8_t)(b)) << 16) | ((uint32_t)((uint8_t)(a)) << 24) )

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

class ScreenBase
{

public:
	ScreenBase()
		: opened(false)
		, width(0)
		, height(0)
		, bpp(0)
		, pitch(0)
		, framebuffer(0)
	{
	}

	virtual void DrawRectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, RGBA colour) = 0;
	virtual void Clear(RGBA colour) = 0;

	virtual void ScrollArea(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) = 0;

	virtual void WriteChar(bool petscii, uint32_t x, uint32_t y, unsigned char c, RGBA colour) = 0;
	virtual uint32_t PrintText(bool petscii, uint32_t xPos, uint32_t yPos, char *ptr, RGBA TxtColour = RGBA(0xff, 0xff, 0xff, 0xff), RGBA BkColour = RGBA(0, 0, 0, 0xFF), bool measureOnly = false, uint32_t* width = 0, uint32_t* height = 0) = 0;
	virtual uint32_t MeasureText(bool petscii, char *ptr, uint32_t* width = 0, uint32_t* height = 0) = 0;

	virtual void PlotPixel(uint32_t x, uint32_t y, RGBA colour) = 0;

	virtual void PlotImage(uint32_t* image, int x, int y, int w, int h) = 0;

	uint32_t Width() const { return width; }
	uint32_t Height() const { return height; }

	virtual float GetScaleX() const { return 1; }
	virtual float GetScaleY() const { return 1; }

	virtual uint32_t ScaleX(uint32_t x) { return x; }
	virtual uint32_t ScaleY(uint32_t y) { return y; }

	virtual uint32_t GetFontWidth() { return 8; }
	virtual uint32_t GetFontHeight() = 0;
	virtual uint32_t GetFontHeightDirectoryDisplay() { return 16; }

	virtual void SwapBuffers() = 0;
	virtual void RefreshRows(uint32_t start, uint32_t amountOfRows) {}

	virtual bool IsLCD() { return false; };
	virtual bool UseCBMFont() { return false; };

	bool IsMonocrome() const { return bpp == 1; }

protected:

	//typedef void (ScreenBase::*PlotPixelFunction)(uint32_t pixel_offset, RGBA Colour);

	//PlotPixelFunction plotPixelFn;

	void ClipRect(uint32_t& x1, uint32_t& y1, uint32_t& x2, uint32_t& y2)
	{
		if (x1 > width) x1 = width;
		if (y1 > height) y1 = height;
		if (x2 > width) x2 = width;
		if (y2 > height) y2 = height;
	}

	bool opened;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t pitch;
	uint8_t* framebuffer;
};

#endif
