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

#ifndef SSD1306_H
#define SSD1306_H

#include <stdlib.h>
#include "types.h"
extern "C"
{
#include "rpi-i2c.h"
}

// 8 pages * (128 columns * 8 bits)
//0                      127 0                         127
//0                      127 0                         127
//0                      127 0                         127
//________________________________________________________
//7777777					|    						 7
//6666666					|							 6
//5555555					|							 5
//4444444	PG(ROW)0		|			PG(ROW)1		 4
//3333333					|							 3
//2222222					|							 2
//1111111					|							 1
//0000000					|							 0
//________________________________________________________
//7777777					|    						 7
//6666666					|							 6
//5555555					|							 5
//4444444	PG(ROW)2		|			PG(ROW)3		 4
//3333333					|							 3
//2222222					|							 2
//1111111					|							 1
//0000000					|							 0
//________________________________________________________
//7777777					|    						 7
//6666666					|							 6
//5555555					|							 5
//4444444	PG(ROW)4		|			PG(ROW)5		 4
//3333333					|							 3
//2222222					|							 2
//1111111					|							 1
//0000000					|							 0
//________________________________________________________
//7777777					|    						 7
//6666666					|							 6
//5555555					|							 5
//4444444	PG(ROW)6		|			PG(ROW)7		 4
//3333333					|							 3
//2222222					|							 2
//1111111					|							 1
//0000000					|							 0
//________________________________________________________

#define SSD1306_128x64_BYTES ((128 * 64) / 8)

class SSD1306
{
public:
	// 128x32 0x3C
	// 128x64 0x3D or 0x3C (if SA0 is grounded)
	SSD1306(int BSCMaster = 1, uint8_t address = 0x3C, unsigned width = 128, unsigned height = 64, int flip = 0, LCD_MODEL type=LCD_UNKNOWN);

	void PlotCharacter(bool useCBMFont, bool petscii, int x, int y, char ascii, bool inverse);
	void PlotText(bool useCBMFont, bool petscii, int x, int y, char* str, bool inverse);

	void InitHardware();
	void DisplayOn();
	void DisplayOff();
	void SetContrast(uint8_t value);
	uint8_t GetContrast() { return contrast; }
	void SetVCOMDeselect(uint8_t value);

	void ClearScreen();
	void RefreshScreen();
	void RefreshPage(uint32_t page);
	void RefreshTextRows(uint32_t start, uint32_t amountOfRows);
	void SetDisplayWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
	void PlotPixel(int x, int y, int c);
	void PlotImage(const unsigned char * source);

protected:
	void SendCommand(uint8_t command);
	void SendData(uint8_t data);
	void SendDataLong(void* data, uint8_t length);

	void Home();
	void SetDataPointer(uint8_t row, uint8_t col);

//	unsigned char frame[SSD1306_128x64_BYTES];
//	unsigned char oldFrame[SSD1306_128x64_BYTES];
	unsigned char * frame;
	unsigned char * oldFrame;
	unsigned sizeof_frame;

	int BSCMaster;
	uint8_t address;
	int type;
	int flip;
	int contrast;
	unsigned width;
	unsigned height;
};
#endif


#define SSD1306_CMD_SET_COLUMN_LOW 0x00
#define SSD1306_CMD_SET_COLUMN_HIGH 0x10
#define SSD1306_CMD_SET_PAGE 0xB0
#define SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE 0x20
#define SSD1306_CMD_SET_COLUMN_ADDRESS 0x21
#define SSD1306_CMD_SET_PAGE_ADDRESS 0x22
#define SSD1306_CMD_DEACTIVATE_SCROLL 0x2E
#define SSD1306_CMD_ACTIVATE_SCROLL 0x2F
#define SSD1306_CMD_SET_CONTRAST_CONTROL 0x81	//  Set Contrast Control for BANK0 
#define SSD1306_ENABLE_CHARGE_PUMP 0x8D
#define SSD1306_CMD_TEST_DISPLAY_OFF 0xA4
#define SSD1306_CMD_TEST_DISPLAY_ON 0xA5
#define SSD1306_CMD_NORMAL_DISPLAY 0xA6	// 1 = on pixel
#define SSD1306_CMD_INVERT_DISPLAY 0xA7	// 0 = on pixel
#define SSD1306_CMD_DISPLAY_OFF 0xAE
#define SSD1306_CMD_DISPLAY_ON 0xAF
#define SSD1306_CMD_MULTIPLEX_RATIO 0xA8
#define SSD1306_CMD_SET_START_LINE 0x40
#define SSD1306_CMD_SET_DISPLAY_OFFSET 0xD3
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO 0xD5
#define SSD1306_CMD_SET_PRE_CHARGE_PERIOD 0xD9
#define SSD1306_CMD_SET_COM_PINS 0xDA
#define SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL 0xDB
#define SSD1306_CONTROL_REG 0x00
#define SSD1306_DATA_REG 0x40
