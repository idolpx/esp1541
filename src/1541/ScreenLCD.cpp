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

#include "ScreenLCD.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "ssd_logo.h"

extern unsigned char* CBMFont;

void ScreenLCD::Open(uint32_t widthDesired, uint32_t heightDesired, uint32_t colourDepth, int BSCMaster, int LCDAddress, int LCDFlip, LCD_MODEL LCDType, bool luseCBMFont)
{
	bpp = 1;

	if (widthDesired < 128)
		widthDesired = 128;
	if (heightDesired < 32)
		heightDesired = 32;
	if (widthDesired > 128)
		widthDesired = 128;
	if (heightDesired > 64)
		heightDesired = 64;

	width = widthDesired;
	height = heightDesired;
	useCBMFont = luseCBMFont;
 
	ssd1306 = new SSD1306(BSCMaster, LCDAddress, width, height, LCDFlip, LCDType);
	ssd1306->ClearScreen();
	ssd1306->RefreshScreen();
	ssd1306->DisplayOn();

	opened = true;
}

void ScreenLCD::DrawRectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, RGBA colour)
{
	ClipRect(x1, y1, x2, y2);
}

void ScreenLCD::ScrollArea(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
}

void ScreenLCD::Clear(RGBA colour)
{
	ssd1306->ClearScreen();
}

void ScreenLCD::ClearInit(RGBA colour)
{
	ssd1306->InitHardware();
	ssd1306->ClearScreen();
	ssd1306->SetContrast(ssd1306->GetContrast());
	ssd1306->DisplayOn();
}

void ScreenLCD::SetContrast(uint8_t value)
{
	ssd1306->SetContrast(value);
}

void ScreenLCD::WriteChar(bool petscii, uint32_t x, uint32_t y, unsigned char c, RGBA colour)
{
	if (opened)
	{
	}
}

void ScreenLCD::PlotPixel(uint32_t x, uint32_t y, RGBA colour)
{
}

void ScreenLCD::PlotImage(uint32_t* image, int x, int y, int w, int h)
{
}

void ScreenLCD::PlotRawImage(const uint8_t* image, int x, int y, int w, int h)
{
	if (x==0 && y==0 && w==128 && h==64)
	{
		ssd1306->PlotImage(image);
	}
}

uint32_t ScreenLCD::PrintText(bool petscii, uint32_t x, uint32_t y, char *ptr, RGBA TxtColour, RGBA BkColour, bool measureOnly, uint32_t* width, uint32_t* height)
{
	int len = 0;
	ssd1306->PlotText(UseCBMFont(), petscii, x >> 3, y / GetFontHeight (), ptr, (BkColour & 0xffffff) != 0);
	return len;
}

uint32_t ScreenLCD::MeasureText(bool petscii, char *ptr, uint32_t* width, uint32_t* height)
{
	return PrintText(petscii, 0, 0, ptr, 0, 0, true, width, height);
}

uint32_t ScreenLCD::GetFontHeight()
{
	if (CBMFont && useCBMFont)
		return 8;
	else
		return 16;
}

void ScreenLCD::RefreshScreen()
{
	ssd1306->RefreshScreen();
}

void ScreenLCD::SwapBuffers()
{
	ssd1306->RefreshScreen();
}

void ScreenLCD::RefreshRows(uint32_t start, uint32_t amountOfRows)
{
	if (ssd1306)
	{
	if (UseCBMFont())
		ssd1306->RefreshTextRows(start, amountOfRows);
	else
		ssd1306->RefreshTextRows(start*2, amountOfRows*2);   
	}
}

bool ScreenLCD::IsLCD()
{
	return true;
}

bool ScreenLCD::UseCBMFont()
{
	return (CBMFont && useCBMFont);
}