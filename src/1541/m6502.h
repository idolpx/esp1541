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

/////////////////////////////////////////////////////////////////////////////////
// Emulates a NMOS 6502
//
// To keep the hardware design simple the 6502 performs a bus read or a write on every cycle no matter what it is doing (including resetting).
// Some of these reads are discarded and not used by the CPU (even re-read later). But these bus accesses may impact other devices on the bus (eg write only memory mapped hardware registers of other devices).
// This emulator tries to emulate the CPU to a cycle accurate level including the every cycle bus accesses.
//
// The undocumented instructions and the undocumented address modes are fully emulated.
//
// Instructions in a 6502 execute over muliple cycles anywhere between 2 to 8 cycles.
// This is caused by various address modes requiring multiple cycles to process (often requiring multiple bus accesses) before the opcode can be executed.
// Typically, after all address mode functions/cycles complete the opcode function will follow.
// The branch instructions are an anomaly to this sequence in that after their opcode executes subsequent address mode cycles may follow.
// (This is more than likely the reason why the real 6502 exhibits idiosyncrasies with branch taken and IRQ/NMI assertions (Idiosyncrasies that are also emulated here)).
//
// This emulator breaks up instructions into the correct sequence of functions which are called one after another, each taking a cycle. (Just like the real hardware's state machine)
//
// To use
// You need to supply bus read and write functions. These take the form;-
//   uint8_t read(uint16_t address)
//   void write(uint16_t address, uint8_t value)
// Note: external code is responsible for mapping devices into the address space.
//		 In a 6502 system if a device is not mapped to an address range the 6502 will read back the last value placed onto the data bus (eg the high byte of the address)
//	     Your external bus read fuction will need to implement this for correct emulation.
//
// CPU can then be supplied with input signals;-
//   CLOCK - call Step().
//   RESET - can be asserted by calling Reset().
//   RDY - assertion state can be set via RDY(bool asserted) function.
//   SO - can be asserted by calling SO().
//
// IRQ and NMI interrupts can be asserted via the "Interrupt" support class and calling its Assert()/Release() functions on the emulated CPU's child objects;-
//   IRQ
//   NMI
//
// Output signals emulated;-
//   SYNC - can be polled by calling SYNC()
//
// You can also read the internal state of the processor via;-
//   GetRegs - returns a copy of all major CPU registers
//   IRQDisabled - returns the status of the I flag in the CPU's status register

#ifndef M6502_H
#define M6502_H
#include "types.h"

// Turn SUPPORT_RDY_HALTING on if you would like to support the RDY line and halting the CPU. (eg BA from the VIC-II in a C64)
//#define SUPPORT_RDY_HALTING

#ifdef  SUPPORT_RDY_HALTING
#define BUS_READ BusRead
#else
#define BUS_READ dataBusReadFn
#endif //  SUPPORT_RDY_HALTING

//#define SUPPORT_NMI		// Some devices don't use the NMI eg Commodore 1541
#define SUPPORT_IRQ		// Some devices don't use IRQ eg Atari 7800

// Visual6502 explains the XAA_MAGIC value (http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
// From taking measurements from my 1541 drives, they all use EE.
#define XAA_MAGIC 0xee
#define LXA_MAGIC 0xee

//2, 3 or 4 cycles
#define BRANCH_CONDITION(flag, condition)		\
	ra = BUS_READ(pc++);						\
	if (ra & 0x80) ra |= 0xFF00;				\
	if ((status & flag) == condition)			\
	{											\
		oldpc = pc;								\
		pc = (pc & 0xff00) | ((pc + ra) & 0xff);\
		addressModeCycleFn = &M6502::rel_5_8_T2;\
	}											\
	else addressModeCycleFn = &M6502::InstructionFetch;

typedef uint8_t(*DataBusReadFn)(uint16_t address);
typedef void(*DataBusWriteFn)(uint16_t address, const uint8_t value);

#if defined(SUPPORT_IRQ) || defined(SUPPORT_NMI)
class Interrupt
{
public:
	Interrupt() : asserted(false) { }
	inline bool IsAsserted() { return asserted; }
	inline void Assert()	{ asserted = true; }
	inline void Release() { asserted = false; }
	inline void Reset() { Release(); }
private:
	bool asserted;
};
#endif //  SUPPORT_IRQ

class M6502
{
private:
	enum
	{
		FLAG_CARRY = 0x01,
		FLAG_ZERO = 0x02,
		FLAG_INTERRUPT = 0x04,
		FLAG_DECIMAL = 0x08,
		FLAG_BREAK = 0x10,
		FLAG_CONSTANT = 0x20,
		FLAG_OVERFLOW = 0x40,
		FLAG_SIGN = 0x80
	};

	typedef void (M6502::*AddressModeCycleFunction)(void);	// Member function pointers for the starting cycle of the address mode functions.
	static AddressModeCycleFunction T1AddressModeFunctions[256];
	typedef void (M6502::*OpcodeCycleFunction)(void);		// Member function pointers for the opcodes.
	static OpcodeCycleFunction opcodeFunctions[256];

	union
	{
		uint16_t ea;		// Effective address
		uint16_t ra;		// Realitive address
	};
	union
	{
		uint16_t ia;		// Intermediate address
		uint16_t oldpc;	// A branch's old PC
	};

	uint16_t value;		// Intermediate data value
	uint16_t pc;			// Program Counter
	uint8_t opcode;		// The current Opcode
	uint8_t a, x, y, status, sp; // Registers

	// Idiosyncrasies of CLI and the 3 cycle Branch Taken instructions can delay interrupts if an interrupt asserts during the execution of those instructions.
	// These flags allow this behaviour to be emulated correctly.
	uint8_t CLIMaskingInterrupt : 1;
	uint8_t BranchTakenMaskingInterrupt : 1;

	// Flags for tracking interrupts
#ifdef  SUPPORT_IRQ
	uint8_t IRQPending : 1;
#endif //  SUPPORT_IRQ
#ifdef  SUPPORT_NMI
	uint8_t NMIPending : 1;
#endif //  SUPPORT_NMI

	// Flags for tracking the state of RDY
#ifdef  SUPPORT_RDY_HALTING
	uint8_t RDYCounter : 4;
	uint8_t RDYAsserted : 1;
	uint8_t RDYHalted : 1;
#endif //  SUPPORT_RDY_HALTING

	DataBusReadFn dataBusReadFn;	// A pointer to the externally supplied Data Bus read function.
	DataBusWriteFn dataBusWriteFn;	// A pointer to the externally supplied Data Bus write function.

	AddressModeCycleFunction addressModeCycleFn;	// Our pointer to the function that will process the current address mode functionality for the current cycle.
	OpcodeCycleFunction opcodeCycleFn;				// Our pointer to the function that will be called after (or during) the address mode cycle(s) that execute the actual opcode.

	inline void ExecuteOpcode(void) { (this->*M6502::opcodeCycleFn)(); addressModeCycleFn = &M6502::InstructionFetch; } // Helper function to call opcodeCycleFn and set up for the next instruction fetch. 

	// Stack manipulation helpers.
	inline void Push(uint8_t val) { dataBusWriteFn(0x100 + sp--, val); }
	inline uint8_t Pull(void) { return (dataBusReadFn(0x100 + ++sp)); }

	// Helper function to write back the results of an instruction (to memory or the A register).
	inline void WriteValue(uint8_t byte)
	{
		if (addressModeCycleFn == &M6502::sb_1_T1) a = byte;
		else dataBusWriteFn(ea, byte);
	}

	void InstructionFetch();	// T0 of every address mode (except reset).

#ifdef  SUPPORT_IRQ
	void InstructionFetchIRQ();		// Special case to correctly emulate an IRQ masking a NMI.
#endif

	// Opcode functions for documented instructions.
	void ADC(void);
	void ANC(void) { uint16_t result = a & value; EstablishNZ(result); SetC(result & 0x0080); a = (uint8_t)result; }
	void AND(void) { uint16_t result = a & value; EstablishNZ(result); a = (uint8_t)result; }
	void ASL(void) { uint16_t result = value << 1; EstablishC(result); EstablishNZ(result); WriteValue(result); }
	void BCC(void) { BRANCH_CONDITION(FLAG_CARRY, 0); }
	void BCS(void) { BRANCH_CONDITION(FLAG_CARRY, FLAG_CARRY); }
	void BEQ(void) { BRANCH_CONDITION(FLAG_ZERO, FLAG_ZERO); }
	void BIT(void) { uint16_t result = a & value; EstablishZ(result); SetV(value & 0x40); EstablishN(value); }
	void BMI(void) { BRANCH_CONDITION(FLAG_SIGN, FLAG_SIGN); }
	void BNE(void) { BRANCH_CONDITION(FLAG_ZERO, 0); }
	void BPL(void) { BRANCH_CONDITION(FLAG_SIGN, 0); }
	void BVC(void) { BRANCH_CONDITION(FLAG_OVERFLOW, 0); }
	void BVS(void) { BRANCH_CONDITION(FLAG_OVERFLOW, FLAG_OVERFLOW); }
	void BRK(void) {}
	void CLC(void) { ClearC(); }
	void CLD(void) { ClearD(); }
	void CLI(void) { ClearI(); CLIMaskingInterrupt = true; } // Like the real hardware the flag will be cleared here (incase it is read by the next instruction) but needs to delay one more cycle (and let another instruction execute) before the IRQ handling will possibly trigger (CLIMaskingInterrupt is used to track and emulate this).
	void CLV(void) { ClearV(); }
	void CMP(void) { uint16_t result = a - value; SetC(a >= (uint8_t)value); SetZ(a == (uint8_t)value); EstablishN(result); }
	void CPX(void) { uint16_t result = x - value; SetC(x >= (uint8_t)value); SetZ(x == (uint8_t)value); EstablishN(result); }
	void CPY(void) { uint16_t result = y - value; SetC(y >= (uint8_t)value); SetZ(y == (uint8_t)value); EstablishN(result); }
	void DEC(void) { uint16_t result = value - 1; EstablishNZ(result); WriteValue(result); }
	void DEX(void) { x--; EstablishNZ(x); }
	void DEY(void) { y--; EstablishNZ(y); }
	void EOR(void) { uint16_t result = a ^ value; EstablishNZ(result); a = (uint8_t)result; }
	void INC(void) { uint16_t result = value + 1; EstablishNZ(result); WriteValue(result); }
	void INX(void) { x++; EstablishNZ(x); }
	void INY(void) { y++; EstablishNZ(y); }
	void JAM(void) {}
	void JMP(void) { pc = ea; }
	void JSR(void) {}
	void LDA(void) { a = (uint8_t)value; EstablishNZ(a); }
	void LDX(void) { x = (uint8_t)value; EstablishNZ(x); }
	void LDY(void) { y = (uint8_t)value; EstablishNZ(y); }
	void LSR(void) { uint16_t result = value >> 1; SetC(value & 1); EstablishNZ(result); WriteValue(result); }
	void NOP(void) {}
	void ORA(void) { uint16_t result = a | value; EstablishNZ(result); a = (uint8_t)result; }
	void PHA(void) { Push(a);}
	void PHP(void) { Push(status | FLAG_CONSTANT | FLAG_BREAK); }		// PHP always pushes the Break (B) flag as a `1' to the stack. Needs to push SO status into V also? (ie what cycle of PHP can SO assertions take effect?)
	void PLA(void) { a = Pull(); EstablishNZ(a); }
	void PLP(void) { status = Pull() | FLAG_CONSTANT; }
	void ROL(void) { uint16_t result = (value << 1) | (status & FLAG_CARRY); EstablishC(result); EstablishNZ(result); WriteValue(result); }
	void ROR(void) { uint16_t result = (value >> 1) | ((status & FLAG_CARRY) << 7); SetC(value & 1); EstablishNZ(result); WriteValue(result); } // Post June, 1976 version.
	void RTI(void) {}
	void RTS(void) {}
	void SBC(void);
	void SEC(void) { SetC(); }
	void SED(void) { SetD(); }
	void SEI(void) { SetI(); }
	void STA(void) { WriteValue(a); }
	void STX(void) { WriteValue(x); }
	void STY(void) { WriteValue(y); }
	void TAX(void) { x = a; EstablishNZ(a); }
	void TAY(void) { y = a; EstablishNZ(y); }
	void TSX(void) { x = sp; EstablishNZ(x); }
	void TXA(void) { a = x; EstablishNZ(a); }
	void TXS(void) { sp = x; }
	void TYA(void) { a = y; EstablishNZ(a); }

	// Opcode functions for undocumented instructions.
	void ASR(void) { uint16_t result = a & value; SetC(result & 1); result >>= 1; EstablishNZ(result); a = (uint8_t)result; }
	void LXA(void) { uint16_t result = (a | LXA_MAGIC) & value; EstablishNZ(result); x = (uint8_t)result; a = x; }
	void ARR(void);
	void LAX(void) { LDA(); LDX(); }
	void LAS(void) { sp = sp & (uint8_t)value; x = sp; a = sp; EstablishNZ(x); }
	void SAX(void) { WriteValue(a & x); }
	void SBX(void) { uint16_t result = (a & x) - value; x = result & 0xff; EstablishNZ(x); SetC(result < 0x100); }
	void SHA(void) { WriteValue(a & x & ((ea >> 8) + 1)); }
	void SHY(void) { uint16_t result = ((ea >> 8) + 1) & y; WriteValue(result); }
	void DCP(void) { uint16_t result = (value - 1) & 0xff; SetC(a >= (uint8_t)result); EstablishNZ(a - (uint8_t)result); WriteValue(result); }
	void ISB(void) { value = (value + 1) & 0xff; WriteValue(value);	SBC(); }
	void SLO(void) { uint16_t result = value << 1; EstablishC(result); a |= (uint8_t)result; EstablishNZ(a); WriteValue(result); }
	void RLA(void) { uint16_t result = (value << 1) | (status & FLAG_CARRY); EstablishC(result); a &= (uint8_t)result; EstablishNZ(a); WriteValue(result); }
	void SRE(void) { uint16_t result = value >> 1; SetC(value & 1); a ^= (uint8_t)result; EstablishNZ(a); WriteValue(result); }
	void RRA(void) { uint16_t result = (value >> 1) | ((status & FLAG_CARRY) << 7); SetC(value & 1); WriteValue(result); value = result & 0xff; ADC(); } // The ADC will use the carry we set here
	void SHS(void) { uint16_t result = (a & x); WriteValue(result & ((ea >> 8) + 1)); sp = (uint8_t)result; }
	void SHX(void) { uint16_t result = ((ea >> 8) + 1) & x; WriteValue(result); }
	void XAA(void) { uint16_t result = ((a | XAA_MAGIC) & x & ((uint8_t)(value))); EstablishNZ(result); a = (uint8_t)result; }

	// For address modes, timings and stages were taken from the "MCS6500 Family Hardware Manual"
	// Numbers in the fuction name are from apendix A section on the manual.
	// eg idy_3_6_T3 is Indirect Y Addressing Mode detailed in section 3.6 of the manual's appendix A.
	// T3 means the T3 stage explained in the manual.

	// Single byte instructions
	void sb_1_T1(void) { BUS_READ(pc); value = a; ExecuteOpcode(); } //2 cycles
	void sb_jam_T1(void) { BUS_READ(pc); ExecuteOpcode(); } //2 cycles

	void imm_2_1_T1(void) { value = BUS_READ(pc++); ExecuteOpcode(); } //2 cycles
	
	void rel_5_8_T1(void) { (this->*M6502::opcodeCycleFn)(); } // Branch instructions are the anomaly and execute their opcode in T1.
	void rel_5_8_T2(void);
	void rel_5_8_T3(void) { BUS_READ(pc); addressModeCycleFn = &M6502::InstructionFetch; } // Opcode has already been executed in T1 so just move on to the next instruction.

	void zp_2_1_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zp_2_1_T2; } //3 cycles
	void zp_2_1_T2(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void zp_3_1_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zp_3_1_T2; } //3 cycles
	void zp_3_1_T2(void) { ExecuteOpcode(); }

	void abs_2_3_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::abs_2_3_T2; } //4 cycles
	void abs_2_3_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::abs_2_3_T3; }
	void abs_2_3_T3(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void abs_3_2_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::abs_3_2_T2; } //4 cycles
	void abs_3_2_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::abs_3_2_T3; }
	void abs_3_2_T3(void) { ExecuteOpcode(); }

	void idx_2_4_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idx_2_4_T2; } //6 cycles
	void idx_2_4_T2(void) { BUS_READ(ia); addressModeCycleFn = &M6502::idx_2_4_T3; }
	void idx_2_4_T3(void) { ia = (ia + x) & 0xff; ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idx_2_4_T4; }
	void idx_2_4_T4(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idx_2_4_T5; }
	void idx_2_4_T5(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void idx_3_3_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idx_3_3_T2; } //6 cycles
	void idx_3_3_T2(void) { BUS_READ(ia); addressModeCycleFn = &M6502::idx_3_3_T3; }
	void idx_3_3_T3(void) { ia = (ia + x) & 0xff; ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idx_3_3_T4; }
	void idx_3_3_T4(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idx_3_3_T5; }
	void idx_3_3_T5(void) { ExecuteOpcode(); }

	// idx_Undoc behaviour was determined by capturing bus activity on a real 6502 in a 1541 and confirmed by observing Visual6502.
	void idx_Undoc_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idx_Undoc_T2; } //8 cycles
	void idx_Undoc_T2(void) { BUS_READ(ia); addressModeCycleFn = &M6502::idx_Undoc_T3; }
	void idx_Undoc_T3(void) { ia = (ia + x) & 0xff; ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idx_Undoc_T4; }
	void idx_Undoc_T4(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idx_Undoc_T5; }
	void idx_Undoc_T5(void) { value = BUS_READ(ea);  addressModeCycleFn = &M6502::idx_Undoc_T6; }
	void idx_Undoc_T6(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::idx_Undoc_T7; }
	void idx_Undoc_T7(void) { ExecuteOpcode(); }

	void absx_2_5_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absx_2_5_T2; } //4/5 cycles
	void absx_2_5_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absx_2_5_T3; }
	void absx_2_5_T3(void);
	void absx_2_5_T4(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void absx_3_4_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absx_3_4_T2; } //5 cycles
	void absx_3_4_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absx_3_4_T3; }
	void absx_3_4_T3(void) { BUS_READ(ea); ea += x; addressModeCycleFn = &M6502::absx_3_4_T4; }
	void absx_3_4_T4(void) { ExecuteOpcode(); }

	void absy_2_5_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absy_2_5_T2; } //4/5 cycles
	void absy_2_5_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absy_2_5_T3; }
	void absy_2_5_T3(void);
	void absy_2_5_T4(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void absy_3_4_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absy_3_4_T2; } //5 cycles
	void absy_3_4_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absy_3_4_T3; }
	void absy_3_4_T3(void) { BUS_READ(ea); ea += y; addressModeCycleFn = &M6502::absy_3_4_T4; }
	void absy_3_4_T4(void) { ExecuteOpcode(); }

	void zpx_2_6_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zpx_2_6_T2; } //4 cycles
	void zpx_2_6_T2(void) { BUS_READ(ea); addressModeCycleFn = &M6502::zpx_2_6_T3; }
	void zpx_2_6_T3(void) { ea = (ea + x) & 0xFF; value = BUS_READ(ea); ExecuteOpcode(); }

	void zpx_3_5_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zpx_3_5_T2; } //4 cycles
	void zpx_3_5_T2(void) { BUS_READ(ea); addressModeCycleFn = &M6502::zpx_3_5_T3; }
	void zpx_3_5_T3(void) { ea = (ea + x) & 0xFF; ExecuteOpcode(); }

	void zpy_2_6_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zpy_2_6_T2; } //4 cycles
	void zpy_2_6_T2(void) { BUS_READ(ea); addressModeCycleFn = &M6502::zpy_2_6_T3; }
	void zpy_2_6_T3(void) { ea = (ea + y) & 0xFF; value = BUS_READ(ea); ExecuteOpcode(); }

	void zpy_3_5_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zpy_3_5_T2; } //4 cycles
	void zpy_3_5_T2(void) { BUS_READ(ea); addressModeCycleFn = &M6502::zpy_3_5_T3; }
	void zpy_3_5_T3(void) { ea = (ea + y) & 0xFF; ExecuteOpcode(); }

	void idy_2_7_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idy_2_7_T2; } //5/6 cycles
	void idy_2_7_T2(void) { ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idy_2_7_T3; }
	void idy_2_7_T3(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idy_2_7_T4; }
	void idy_2_7_T4(void);
	void idy_2_7_T5(void) { value = BUS_READ(ea); ExecuteOpcode(); }

	void idy_3_6_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idy_3_6_T2; } //6 cycles
	void idy_3_6_T2(void) { ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idy_3_6_T3; }
	void idy_3_6_T3(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idy_3_6_T4; }
	void idy_3_6_T4(void) { ea += y; BUS_READ(ea); addressModeCycleFn = &M6502::idy_3_6_T5; }
	void idy_3_6_T5(void) { ExecuteOpcode(); }

	// idy_Undoc behaviour was determined by capturing bus activity on a real 6502 in a 1541 and confirmed by Visual6502.
	void idy_Undoc_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::idy_Undoc_T2; } //8 cycles
	void idy_Undoc_T2(void) { ea = BUS_READ(ia++); addressModeCycleFn = &M6502::idy_Undoc_T3; }
	void idy_Undoc_T3(void) { ea |= (BUS_READ(ia & 0xff) << 8); addressModeCycleFn = &M6502::idy_Undoc_T4; }
	void idy_Undoc_T4(void) { ea += y; BUS_READ(ea); addressModeCycleFn = &M6502::idy_Undoc_T5; }
	void idy_Undoc_T5(void) { value = BUS_READ(ea);  addressModeCycleFn = &M6502::idy_Undoc_T6; }
	void idy_Undoc_T6(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::idy_Undoc_T7; }
	void idy_Undoc_T7(void) { ExecuteOpcode(); }

	void zp_4_1_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zp_4_1_T2; } //5 cycles
	void zp_4_1_T2(void) { value = BUS_READ(ea); addressModeCycleFn = &M6502::zp_4_1_T3; }
	void zp_4_1_T3(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::zp_4_1_T4; }
	void zp_4_1_T4(void) { ExecuteOpcode(); }

	void abs_4_2_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::abs_4_2_T2; } //6 cycles
	void abs_4_2_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::abs_4_2_T3; }
	void abs_4_2_T3(void) { value = BUS_READ(ea); addressModeCycleFn = &M6502::abs_4_2_T4; }
	void abs_4_2_T4(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::abs_4_2_T5; }
	void abs_4_2_T5(void) { ExecuteOpcode(); }

	void zpx_4_3_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::zpx_4_3_T2; } //6 cycles
	void zpx_4_3_T2(void) { BUS_READ(ea); addressModeCycleFn = &M6502::zpx_4_3_T3; }
	void zpx_4_3_T3(void) { ea = (ea + x) & 0xFF; value = BUS_READ(ea); addressModeCycleFn = &M6502::zpx_4_3_T4; }
	void zpx_4_3_T4(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::zpx_4_3_T5; }
	void zpx_4_3_T5(void) { ExecuteOpcode(); }

	void absx_4_4_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absx_4_4_T2; } //7 cycles
	void absx_4_4_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absx_4_4_T3; }
	void absx_4_4_T3(void) { ea += x; BUS_READ(ea); addressModeCycleFn = &M6502::absx_4_4_T4; }
	void absx_4_4_T4(void) { value = BUS_READ(ea); addressModeCycleFn = &M6502::absx_4_4_T5; }
	void absx_4_4_T5(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::absx_4_4_T6; }
	void absx_4_4_T6(void) { ExecuteOpcode(); }

	void absy_4_4_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::absy_4_4_T2; } //7 cycles
	void absy_4_4_T2(void) { ea |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::absy_4_4_T3; }
	void absy_4_4_T3(void) { ea += y; BUS_READ(ea); addressModeCycleFn = &M6502::absy_4_4_T4; }
	void absy_4_4_T4(void) { value = BUS_READ(ea); addressModeCycleFn = &M6502::absy_4_4_T5; }
	void absy_4_4_T5(void) { dataBusWriteFn(ea, (uint8_t)value); addressModeCycleFn = &M6502::absy_4_4_T6; }
	void absy_4_4_T6(void) { ExecuteOpcode(); }

	void ph_5_1_T1(void) { BUS_READ(pc); addressModeCycleFn = &M6502::ph_5_1_T2; } //3 cycles
	void ph_5_1_T2(void) { ExecuteOpcode(); }

	void pl_5_2_T1(void) { BUS_READ(pc); addressModeCycleFn = &M6502::pl_5_2_T2; } //4 cycles
	void pl_5_2_T2(void) { BUS_READ(0x100 + sp); addressModeCycleFn = &M6502::pl_5_2_T3; }
	void pl_5_2_T3(void) { ExecuteOpcode(); }

	void jsr_5_3_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::jsr_5_3_T2; } //6 cycles
	void jsr_5_3_T2(void) { BUS_READ(0x100 + sp); addressModeCycleFn = &M6502::jsr_5_3_T3; }
	void jsr_5_3_T3(void) { Push((uint8_t)((pc) >> 8)); addressModeCycleFn = &M6502::jsr_5_3_T4; }
	void jsr_5_3_T4(void) { Push(pc & 0xff); addressModeCycleFn = &M6502::jsr_5_3_T5; }
	void jsr_5_3_T5(void) { ea |= (BUS_READ(pc++) << 8); pc = ea; ExecuteOpcode(); }

	void rti_5_5_T1(void) { BUS_READ(pc++); addressModeCycleFn = &M6502::rti_5_5_T2; } //6 cycles
	void rti_5_5_T2(void) { BUS_READ(0x100 + sp); addressModeCycleFn = &M6502::rti_5_5_T3; }
	void rti_5_5_T3(void) { status = Pull(); addressModeCycleFn = &M6502::rti_5_5_T4; }
	void rti_5_5_T4(void) { pc = Pull(); addressModeCycleFn = &M6502::rti_5_5_T5; }
	void rti_5_5_T5(void) { pc |= (Pull() << 8); ExecuteOpcode(); }

	void abs5_6_1_T1(void) { ea = BUS_READ(pc++); addressModeCycleFn = &M6502::abs5_6_1_T2; } //3 cycles
	void abs5_6_1_T2(void) { ea |= (BUS_READ(pc++) << 8); ExecuteOpcode(); }

	void abs5_6_2_T1(void) { ia = BUS_READ(pc++); addressModeCycleFn = &M6502::abs5_6_2_T2; } //5 cycles
	void abs5_6_2_T2(void) { ia |= (BUS_READ(pc++) << 8); addressModeCycleFn = &M6502::abs5_6_2_T3; }
	void abs5_6_2_T3(void) { ea = BUS_READ(ia++); addressModeCycleFn = &M6502::abs5_6_2_T4; }
	void abs5_6_2_T4(void) { ea |= (BUS_READ(ia) << 8); ExecuteOpcode(); }

	void rts_5_7_T1(void) { BUS_READ(pc++); addressModeCycleFn = &M6502::rts_5_7_T2; } //6 cycles
	void rts_5_7_T2(void) { BUS_READ(0x100 + sp); addressModeCycleFn = &M6502::rts_5_7_T3; }
	void rts_5_7_T3(void) { pc = Pull(); addressModeCycleFn = &M6502::rts_5_7_T4; }
	void rts_5_7_T4(void) { pc |= (Pull() << 8); addressModeCycleFn = &M6502::rts_5_7_T5; }
	void rts_5_7_T5(void) { BUS_READ(pc); pc++; ExecuteOpcode(); }

	// The BRK, RESET, NMI and IRQ instructions are closely related.
	// At T4 BRK can morph into one of the interrupts if that interrupt condition has subsequently occurred since the instruction started.
	void brk_5_4_T1(void) { BUS_READ(pc); pc++; addressModeCycleFn = &M6502::brk_5_4_T2; } //7 cycles
	void brk_5_4_T2(void) { Push((uint8_t)(pc >> 8)); addressModeCycleFn = &M6502::brk_5_4_T3; }
	void brk_5_4_T3(void) { Push(pc & 0xff); addressModeCycleFn = &M6502::brk_5_4_T4; }
	void brk_5_4_T4(void); // We check here if we continue on executing the BRK or take the interrupt.
	void brk_5_4_T5(void) { ea = BUS_READ(0xFFFE); addressModeCycleFn = &M6502::brk_5_4_T6; } // Short burts of interrupt assertions will be correctly masked by the BRK in these 2 cycles.
	void brk_5_4_T6(void) { SetI(); pc = ea | (BUS_READ(0xFFFF) << 8); ExecuteOpcode(); }

	void Reset_T0(void) { sp = 0; BUS_READ(pc);	addressModeCycleFn = &M6502::Reset_T1; } //7 cycles
	void Reset_T1(void) { BUS_READ(pc); addressModeCycleFn = &M6502::Reset_T2; }
	void Reset_T2(void) { BUS_READ(0x100 + sp--); addressModeCycleFn = &M6502::Reset_T3; }
	void Reset_T3(void) { BUS_READ(0x100 + sp--); addressModeCycleFn = &M6502::Reset_T4; }
	void Reset_T4(void) { ClearB(); BUS_READ(0x100 + sp--); addressModeCycleFn = &M6502::Reset_T5; }
	void Reset_T5(void) { ea = BUS_READ(0xFFFC); addressModeCycleFn = &M6502::Reset_T6; }
	void Reset_T6(void) { pc = ea | (BUS_READ(0xFFFD) << 8); addressModeCycleFn = &M6502::InstructionFetch; }

#ifdef  SUPPORT_NMI
	void NMI_T1(void) { BUS_READ(pc); addressModeCycleFn = &M6502::NMI_T2; } //7 cycles
	void NMI_T2(void) { Push((uint8_t)(pc >> 8)); addressModeCycleFn = &M6502::NMI_T3; }
	void NMI_T3(void) { Push(pc & 0xff); addressModeCycleFn = &M6502::NMI_T4; }
	void NMI_T4(void) { ClearB(); Push(status); status |= FLAG_INTERRUPT; addressModeCycleFn = &M6502::NMI_T5; }
	void NMI_T5(void) { ea = BUS_READ(0xFFFA); addressModeCycleFn = &M6502::NMI_T6; }
	void NMI_T6(void) { SetI(); pc = ea | (BUS_READ(0xFFFB) << 8); NMIPending = false; addressModeCycleFn = &M6502::InstructionFetch; }
#endif //  SUPPORT_NMI

#ifdef  SUPPORT_IRQ
	void IRQ_T1(void) { BUS_READ(pc); addressModeCycleFn = &M6502::IRQ_T2; } //7 cycles
	void IRQ_T2(void) { Push((uint8_t)(pc >> 8)); addressModeCycleFn = &M6502::IRQ_T3; }
	void IRQ_T3(void) { Push(pc & 0xff); addressModeCycleFn = &M6502::IRQ_T4; }
	void IRQ_T4(void);  // We check here if we continue on executing as IRQ or morph into NMI
	void IRQ_T5(void) { ea = BUS_READ(0xFFFE); addressModeCycleFn = &M6502::IRQ_T6; } // Short burts of NMI assertions will be correctly masked by the IRQ in these 2 cycles
	void IRQ_T6(void) { SetI();	pc = ea | (BUS_READ(0xFFFF) << 8); addressModeCycleFn = &M6502::InstructionFetchIRQ; }
#endif //  SUPPORT_IRQ

	inline void ClearB() { status &= (~FLAG_BREAK); }
	inline void SetB() { status |= FLAG_BREAK; }
	inline void ClearC() { status &= (~FLAG_CARRY); }
	inline void SetC() { status |= FLAG_CARRY; }
	inline void SetC(uint16_t test) { test != 0 ? SetC() : ClearC(); }
	inline void ClearZ() { status &= (~FLAG_ZERO); }
	inline void SetZ() { status |= FLAG_ZERO; }
	inline void SetZ(uint16_t test) { test != 0 ? SetZ() : ClearZ(); }
	inline void ClearI() {status &= (~FLAG_INTERRUPT); }
	inline void SetI() { status |= FLAG_INTERRUPT; }
	inline void ClearD() {status &= (~FLAG_DECIMAL); }
	inline void SetD() { status |= FLAG_DECIMAL; }
	inline void ClearV() {status &= (~FLAG_OVERFLOW); }
	inline void SetV() { status |= FLAG_OVERFLOW; }
	inline void SetV(uint16_t test) { test != 0 ? SetV() : ClearV(); }
	inline void ClearN() {status &= (~FLAG_SIGN); }
	inline void SetN() { status |= FLAG_SIGN; }
	inline void SetN(uint16_t test) { test != 0 ? SetN() : ClearN(); }

	inline void EstablishZ(uint16_t val)	{ SetZ((val & 0x00FF) == 0); }
	inline void EstablishN(uint16_t val)	{ SetN(val & 0x0080); }
	inline void EstablishC(uint16_t val) { SetC(val & 0xFF00); }
	inline void EstablishV(uint16_t result, uint8_t val) { SetV((result ^ a) & (result ^ val) & 0x0080); }
	inline void EstablishNZ(uint16_t val) { EstablishZ(val); EstablishN(val); }

#ifdef  SUPPORT_RDY_HALTING
	void CheckForHalt();
#endif //  SUPPORT_RDY_HALTING

public:
	M6502() : status(FLAG_CONSTANT), dataBusReadFn(0), dataBusWriteFn(0) {}
	M6502(void* data, DataBusReadFn dataBusReadFn, DataBusWriteFn dataBusWriteFn) { SetBusFunctions(dataBusReadFn, dataBusWriteFn); }
	void SetBusFunctions(DataBusReadFn dataBusReadFn, DataBusWriteFn dataBusWriteFn) {this->dataBusReadFn = dataBusReadFn; this->dataBusWriteFn = dataBusWriteFn; status = FLAG_CONSTANT; Reset(); }
	void Reset(void);
	void Step(void);
#ifdef  SUPPORT_RDY_HALTING
	void RDY(bool asserted);
	bool Halted() { return RDYHalted != 0; }
	uint8_t BusRead(uint16_t address);
#endif //  SUPPORT_RDY_HALTING
	inline void SO(void) { SetV(); }
	inline bool IRQDisabled(void) const { return (status & FLAG_INTERRUPT) != 0; }

	void GetRegs(uint16_t& PC, uint8_t& SP, uint8_t& A, uint8_t& X, uint8_t& Y, uint8_t& Status) { PC = pc; SP = sp; A = a; X = x; Y = y; Status = status; }
	uint16_t GetPC() const { return pc; }
	uint8_t GetA() const { return a; }
	uint8_t GetX() const { return x;	}
	uint8_t GetY() const { return y; }
	uint8_t GetStatus() const { return status; }
	// Emulate the 6502's SYNC signal and pin
	bool SYNC(void) const { return addressModeCycleFn == &M6502::InstructionFetch; }

#ifdef  SUPPORT_IRQ
	Interrupt IRQ;
#endif //  SUPPORT_IRQ
#ifdef  SUPPORT_NMI
	Interrupt NMI;
#endif //  SUPPORT_NMI
};
#endif
