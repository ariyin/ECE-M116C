#include "CPU.h"
#include <cstdint>
#include <iostream>
#include <iomanip>

Instruction::Instruction(bitset<32> fetch)
{
	instr = fetch;
}

CPU::CPU()
{
	// initialize PC to 0
	PC = 0;
	operation = NOP;

	for (int i = 0; i < 4096; i++)
	{
		// zero out data memory
		dmemory[i] = 0;
	}

	for (int i = 0; i < 32; i++)
	{
		// initialize all registers to zero
		registers[i] = 0;
	}
}

bitset<32> CPU::fetch(bitset<8> *instMem)
{
	// get 32-bit instruction
	bitset<32> instr = ((((instMem[PC + 3].to_ulong()) << 24)) + ((instMem[PC + 2].to_ulong()) << 16) + ((instMem[PC + 1].to_ulong()) << 8) + (instMem[PC].to_ulong()));
	PC += 4;
	return instr;
}

void CPU::decode(Instruction *curr)
{
	uint32_t instruction = (uint32_t)(curr->instr.to_ulong());

	uint32_t opcode = instruction & 0x7F;		  // 7 bits
	uint32_t rd = (instruction >> 7) & 0x1F;	  // 5 bits
	uint32_t funct3 = (instruction >> 12) & 0x7;  // 3 bits
	uint32_t rs1 = (instruction >> 15) & 0x1F;	  // 5 bits
	uint32_t rs2 = (instruction >> 20) & 0x1F;	  // 5 bits
	uint32_t funct7 = (instruction >> 25) & 0x7F; // 7 bits

	decodeInstr.rs1 = registers[rs1];
	decodeInstr.rs2 = registers[rs2];
	decodeInstr.rd = rd;
	decodeInstr.immediate = (int32_t)curr->instr.to_ulong() >> 20;

	switch (opcode)
	{
	// r-type
	case 0x33:
		if (funct3 == 0x0 && funct7 == 0x0)
		{
			operation = ADD;
		}
		else if (funct3 == 0x4 && funct7 == 0x0)
		{
			operation = XOR;
		}
		break;
	// i-type
	case 0x13:
		if (funct3 == 0x5)
		{
			operation = SRAI;
		}
		else if (funct3 == 0x6)
		{
			operation = ORI;
		}
		break;
	// load
	case 0x3:
		if (funct3 == 0x0)
		{
			operation = LB;
		}
		else if (funct3 == 0x2)
		{
			operation = LW;
		}
		break;
	// store
	case 0x23:
	{
		if (funct3 == 0x0)
		{
			operation = SB;
		}
		else if (funct3 == 0x2)
		{
			operation = SW;
		}

		int32_t imm11_5 = instruction & 0xFE000000;
		int32_t imm4_0 = (instruction & 0xF80) << 13;
		decodeInstr.immediate = (imm11_5 + imm4_0) >> 20;

		break;
	}
	case 0x63:
	{
		operation = BEQ;

		int32_t imm12 = (instruction & 0x80000000) >> 19;	// bit 31 (sign bit) shifted to bit 12
		int32_t imm11 = (instruction & 0x00000080) << 4;	// bit 7 shifted to bit 11
		int32_t imm10_5 = (instruction & 0x7E000000) >> 20; // bits 30-25 shifted to [10:5]
		int32_t imm4_1 = (instruction & 0x00000F00) >> 7;	// bits 11-8 shifted to [4:1]
		int32_t imm0 = 0;

		decodeInstr.immediate = imm12 + imm11 + imm10_5 + imm4_1 + imm0;

		// check if sign bit (imm12) is set
		if (imm12 > 0)
		{
			// set upper bits to maintain the sign
			decodeInstr.immediate = (int32_t)(decodeInstr.immediate | 0xFFFFF000);
		}

		break;
	}
	case 0x6F:
	{
		operation = JAL;

		int32_t imm20 = (instruction & 0x80000000) >> 11;	// bit 31 (sign bit) shifted to bit 20
		int32_t imm10_1 = (instruction & 0x7FE00000) >> 20; // bits 30-21
		int32_t imm11 = (instruction & 0x00100000) >> 9;	// bit 20 shifted to bit 11
		int32_t imm19_12 = (instruction & 0x000FF000);		// bits 19-12
		int32_t imm0 = 0;

		decodeInstr.immediate = imm20 + imm10_1 + imm11 + imm19_12 + imm0;

		// check if sign bit (imm20) is set
		if (imm20 > 0)
		{
			// set upper bits to maintain the sign
			decodeInstr.immediate = (int32_t)(decodeInstr.immediate | 0xFFF00000);
		}

		break;
	}
	case 0x37:
		operation = LUI;
		decodeInstr.immediate = instruction >> 12;
		break;
	default:
		operation = NOP;
		break;
	}
}

void CPU::execute()
{
	executeInstr.rs2 = decodeInstr.rs2;
	executeInstr.rd = decodeInstr.rd;

	switch (operation)
	{
	case ADD:
		executeInstr.aluResult = decodeInstr.rs1 + decodeInstr.rs2;
		break;
	case LUI:
		executeInstr.aluResult = decodeInstr.immediate << 12;
		break;
	case ORI:
		executeInstr.aluResult = decodeInstr.rs1 | decodeInstr.immediate;
		break;
	case XOR:
		executeInstr.aluResult = decodeInstr.rs1 ^ decodeInstr.rs2;
		break;
	case SRAI:
		executeInstr.aluResult = decodeInstr.rs1 >> (decodeInstr.immediate & (unsigned)0x1F);
		break;
	case LB:
	case SB:
	case LW:
	case SW:
		executeInstr.aluResult = decodeInstr.rs1 + decodeInstr.immediate;
		break;
	case BEQ:
		if (decodeInstr.rs1 == decodeInstr.rs2)
		{
			PC += (int32_t)(decodeInstr.immediate & ~1) - 4;
		}
		break;
	case JAL:
		executeInstr.aluResult = PC;
		PC += (int32_t)(decodeInstr.immediate & ~1) - 4;
		break;
	case NOP:
		break;
	}
}

void CPU::memory()
{
	memInstr.rd = executeInstr.rd;
	memInstr.aluResult = executeInstr.aluResult;

	switch (operation)
	{
	// loads from memory to register
	case LB:
		// 8-bit
		memInstr.dataMem = dmemory[executeInstr.aluResult];
		break;
	case LW:
		// 32-bit
		memInstr.dataMem = ((((dmemory[executeInstr.aluResult + 3]) << 24)) | ((dmemory[executeInstr.aluResult + 2]) << 16) | ((dmemory[executeInstr.aluResult + 1]) << 8) | (dmemory[executeInstr.aluResult]));
		break;
	// stores from register to memory
	case SB:
		// 8-bit
		dmemory[executeInstr.aluResult] = executeInstr.rs2 & 0xFF;
		break;
	case SW:
		// 32-bit
		dmemory[executeInstr.aluResult] = executeInstr.rs2 & 0xFF;
		dmemory[executeInstr.aluResult + 1] = ((executeInstr.rs2 & 0xFF00) >> 8);
		dmemory[executeInstr.aluResult + 2] = ((executeInstr.rs2 & 0xFF0000) >> 16);
		dmemory[executeInstr.aluResult + 3] = ((executeInstr.rs2 & 0xFF000000) >> 24);
		break;
	default:
		break;
	}
}

void CPU::writeback()
{
	if (operation == SB || operation == SW || operation == NOP)
	{
		return;
	}
	else if (operation == LB || operation == LW)
	{
		registers[memInstr.rd] = memInstr.dataMem;
	}
	else
	{
		registers[memInstr.rd] = memInstr.aluResult;
	}
}

// read the current PC
unsigned long CPU::readPC()
{
	return PC;
}

// print registers
void CPU::printRegs()
{
	int a0 = registers[10]; // x10
	int a1 = registers[11]; // x11
	cout << "(" << a0 << "," << a1 << ")" << endl;
}