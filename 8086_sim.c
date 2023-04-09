#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;

// Parts of instruction
u8 opcode, directionFlag, wordByteFlag, mode, reg, regMem, dataLow, dataHigh;

// Registers (Name Conflict)
// u16 AX_var, BX_var, CX_var, DX_var;
// u8 SI_var, DI_var, BP_var, SP_var;

// Register Strings
u8* regStr[16] = {"ax\0", "cx\0", "dx\0", "bx\0", "sp\0", "bp\0", "si\0", "di\0", "al\0", "cl\0", "dl\0", "bl\0", "ah\0", "ch\0", "dh\0", "bh\0"};

// Enums
enum directions {REG_SOURCE, REG_DEST};
enum wordByte {BYTE, WORD};
enum modes {MODE_MEM_NO_DISP, MODE_MEM_8_DISP, MODE_MEM_16_DISP, MODE_REG};
enum registers {AX, CX, DX, BX, SP, BP, SI, DI};

// Globals
char ASMOutputBuffer[128];

const char* binPrint(u8 dataByte, int printLength) {
	char binaryString[printLength+1];
	for (int i=printLength-1; i>=0; i--) {
		binaryString[i] = ((dataByte >> i) & 1) == 1 ? '1' : '0';
	}
	char* returnString = (char*)binaryString;
	return returnString;
}

void printDecoded() {
	printf("OPCODE: \033[32m%d\033[0m  ", binPrint(opcode, 6));
	printf("DIRECTION: \033[32m%s\033[0m  ", binPrint(directionFlag, 1));
	printf("WORD/BYTE: \033[32m%s\033[0m  ", binPrint(wordByteFlag, 1));
	printf("MODE: \033[32m%s\033[0m  ", binPrint(mode, 2));
	printf("REG: \033[32m%s\033[0m  ", binPrint(reg, 3));
	printf("R/M: \033[32m%s\033[0m\n", binPrint(regMem, 3));
}

int decodeInstructionStream(u8 *binaryStream, int byteAddress) {

	if ((binaryStream[byteAddress] & 0b11110000) >> 4 == 0b1011) {
	
		// Immediate to register move
		opcode = (binaryStream[byteAddress] & 0b11110000) >> 4;
		wordByteFlag = (binaryStream[byteAddress] & 0b00001000) >> 3;
		reg = binaryStream[byteAddress] & 0b00000111;
		dataLow = binaryStream[byteAddress+1];
		if (wordByteFlag == BYTE) {
			return 2;
		} else {
			dataHigh = binaryStream[byteAddress+2];
			return 3;
		}

	}

	if ((binaryStream[byteAddress] & 0b11111100) >> 2 == 0b100010) {

		// Register to register move
		opcode = binaryStream[byteAddress] >> 2;
		directionFlag = (binaryStream[byteAddress] & 0b00000010) >> 1;
		wordByteFlag = binaryStream[byteAddress] & 0b00000001;
		mode = (binaryStream[byteAddress+1] & 0b11000000) >> 6;
		reg = (binaryStream[byteAddress+1] & 0b00111000) >> 3;
		regMem = (binaryStream[byteAddress+1] & 0b00000111);
		return 2;

	}

	return 1;
}

void moveImmediateToReg(FILE* file) {
	u16 data16 = (dataHigh << 8) + dataLow;
	fprintf(file, "mov %s, %d\n", wordByteFlag ? regStr[reg] : regStr[reg+8], data16);
}

void moveRegToReg(FILE* file) {
	switch (mode) {
		case MODE_REG:
			int strShift = wordByteFlag ? 0 : 8;
			if (directionFlag == REG_SOURCE) fprintf(file, "mov %s, %s\n", regStr[regMem+strShift], regStr[reg+strShift]);
			if (directionFlag == REG_DEST) fprintf(file, "mov %s, %s\n", regStr[reg+strShift], regStr[regMem+strShift]);
			break;
	}
}

int main(int argc, char* argv[]) {

	// Check input filename existence and length
	if (argc < 2) {
		printf("No input file specified.");
		exit(1);
	}
	if (strlen(argv[1]) > 250) {
		printf("Path name too long.");
		exit(1);
	}

	// Create variables
	FILE *binaryInputFile;
	u8 fileReadBuffer[64];
	memset(fileReadBuffer, 0, sizeof(fileReadBuffer));

	// Open file
	if ((binaryInputFile = fopen(argv[1], "rb")) == NULL) {
		printf("Error opening file!");
		exit(1);
	}

	// Get length of file
	int fileLength = 0;
	fseek(binaryInputFile, 0, SEEK_END);
	fileLength = ftell(binaryInputFile);
	fseek(binaryInputFile, 0, SEEK_SET);

	// Read data from file and close
	fread(&fileReadBuffer, 64, 1, binaryInputFile);	
	fclose(binaryInputFile);

	// Open file for writing (text mode)
	FILE* ASMOutputFile;
	char outputFilename[256];
	sprintf(outputFilename, "%s_disassembled.asm", argv[1]);
	ASMOutputFile = fopen(outputFilename, "w");
	if (ASMOutputFile == NULL) {
		printf("Error creating output file!");
		exit(1);
	}
	fprintf(ASMOutputFile, "bits 16\n\n");

	// Decode instructions and write to output file
	int fileBytePointer = 0;
	while (fileBytePointer < fileLength) {
		fileBytePointer += decodeInstructionStream(fileReadBuffer, fileBytePointer);
		switch (opcode) {
			case 0b100010: moveRegToReg(ASMOutputFile); break;
			case 0b1011: moveImmediateToReg(ASMOutputFile); break;
		}
	}

	// Close output file
	fclose(ASMOutputFile);

}

