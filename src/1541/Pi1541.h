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

#ifndef PI1541_H
#define PI1541_H

#include "Drive.h"
#include "m6502.h"
#include "iec_bus.h"

class Pi1541
{

public:
	Pi1541();

	void Initialise();

	void Update();

	void Reset();

	//void ConfigureOfExtraRAM(bool extraRAM);

	Drive drive;
	m6522 VIA[2];

	M6502 m6502;

	enum PortPins
	{
		VIAPORTPINS_DEVSEL0 = 0x20,	//pb5
		VIAPORTPINS_DEVSEL1 = 0x40,	//pb6
	};

	inline void SetDeviceID(uint8_t id)
	{
		VIA[0].GetPortB()->SetInput(VIAPORTPINS_DEVSEL0, id & 1);
		VIA[0].GetPortB()->SetInput(VIAPORTPINS_DEVSEL1, id & 2);
	}

private:
	//uint8_t Memory[0xc000];

	//static uint8_t Read6502(uint16_t address, void* data);
	//static uint8_t Read6502ExtraRAM(uint16_t address, void* data);
	//static uint8_t Peek6502(uint16_t address, void* data);
	//static void Write6502(uint16_t address, const uint8_t value, void* data);
	//static void Write6502ExtraRAM(uint16_t address, const uint8_t value, void* data);
};

#endif