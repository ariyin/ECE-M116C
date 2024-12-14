#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

enum Operation
{
	ADD,
	LUI,
	ORI,
	XOR,
	SRAI,
	LB,
	LW,
	SB,
	SW,
	BEQ,
	JAL,
	NOP
};

class Instruction
{
public:
	bitset<32> instr;			   // instruction
	Instruction(bitset<32> fetch); // constructor
};

class CPU
{
private:
	int dmemory[4096];	   // data memory byte addressable in little endian fashion;
	unsigned long PC;	   // pc
	int32_t registers[32]; // general-purpose registers (RISC-V has 32)
	Operation operation;

	struct Decode
	{
		int32_t rs1;
		int32_t rs2;
		uint32_t rd;
		int32_t immediate;
	} decodeInstr;

	struct Execute
	{
		int32_t aluResult;
		int32_t rs2;
		uint32_t rd;
	} executeInstr;

	struct Memory
	{
		uint32_t rd;
		int32_t aluResult;
		int32_t dataMem;
	} memInstr;

public:
	CPU();
	unsigned long readPC();
	bitset<32> fetch(bitset<8> *instMem);
	void decode(Instruction *curr);
	void execute();
	void memory();
	void writeback();
	void printRegs();
};
