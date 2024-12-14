#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

int main(int argc, char *argv[])
{
	/* 
	This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

	/*
	Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an hex and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	*/

	bitset<8> instMem[4096];

	if (argc < 2)
	{
		// cout << "No file name entered. Exiting...";
		return -1;
	}

	// open the file
	ifstream infile(argv[1]);
	if (!(infile.is_open() && infile.good()))
	{
		cout << "error opening file\n";
		return 0;
	}
	string line;
	int i = 0;
	while (infile)
	{
		infile >> line;
		stringstream line2(line);
		int x;
		line2 >> std::hex >> x;
		instMem[i] = bitset<8>(x);
		i++;
	}
	int maxPC = i;

	CPU myCPU;

	bool done = true;
	bitset<32> curr;
	Instruction instruction = Instruction(curr);

	// processor's main loop
	// each iteration is equal to one clock cycle
	while (done == true)
	{
		// fetch
		curr = myCPU.fetch(instMem);
		instruction = Instruction(curr);

		myCPU.decode(&instruction);
		myCPU.execute();
		myCPU.memory();
		myCPU.writeback();

		if (myCPU.readPC() > maxPC)
			break;
	}

	myCPU.printRegs();

	return 0;
}