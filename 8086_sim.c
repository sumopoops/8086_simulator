/*

--================ 8086 Instruction ================--

Byte 1
OOOOOO D W
O - Opcode
D - Direction, 0 REG = Source, 1 REG = Destination
W - 0 = 8bit byte, 1 = 16bit word

Byte 2
MM RRR MMM
M - Mode
R - Register
M - Reg/Mem

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INSTRUCTION_LENGTH 2

typedef unsigned char u8;
typedef unsigned short u16;

// Parts of instruction
u8 opcode, directionFlag, wordByteFlag, mode, reg, regMem;

// Registers (Name Conflict)
// u16 AX, BX, CX, DX;
// u8 SI, DI, BP, SP;

// Register Strings
u8* regStrings[16] = {"ax\0", "cx\0", "dx\0", "bx\0", "sp\0", "bp\0", "si\0", "di\0", "al\0", "cl\0", "dl\0", "bl\0", "ah\0", "ch\0", "dh\0", "bh\0"};

// Enums
enum directions {RIGHT_TO_LEFT, LEFT_TO_RIGHT};
enum wordByte {BYTE, WORD};
enum registers {AX, CX, DX, BX, SP, BP, SI, DI};

// Globals
char ASMOutputBuffer[128];

void binPrint(u8 dataByte, int printLength) {
	for (int i=printLength-1; i>=0; i--) {
		putchar(((dataByte >> i) & 1) == 1 ? '1' : '0');
	}
}

void printDecoded() {
	printf("OPCODE: \033[32m");
	binPrint(opcode, 6);
	printf("\033[0m  DIRECTION: \033[32m");
	binPrint(directionFlag, 1);
	printf("\033[0m  WORD/BYTE: \033[32m");
	binPrint(wordByteFlag, 1);
	printf("\033[0m  MODE: \033[32m");
	binPrint(mode, 2);
	printf("\033[0m  REG: \033[32m");
	binPrint(reg, 3);
	printf("\033[0m  R/M: \033[32m");
	binPrint(regMem, 3);
	printf("\033[0m\n");
}

void decodeInstructionStream(u8 *binaryStream, int byteAddress) {
	opcode = binaryStream[byteAddress] >> 2;
	directionFlag = (binaryStream[byteAddress] & 0x02) >> 1;
	wordByteFlag = binaryStream[byteAddress] & 0x01;
	mode = (binaryStream[byteAddress+1] & 0xC0) >> 6;
	reg = (binaryStream[byteAddress+1] & 0x38) >> 3;
	regMem = (binaryStream[byteAddress+1] & 0x7);
}

void MOV(FILE* file) {
	if (directionFlag == RIGHT_TO_LEFT) {
		switch (wordByteFlag) {
			case BYTE:
				fprintf(file, "mov %s, %s\n", regStrings[regMem+8], regStrings[reg+8]);
				break;
			case WORD:
				fprintf(file, "mov %s, %s\n", regStrings[regMem], regStrings[reg]);
				break;
		}
		if (wordByteFlag == BYTE) {
		} else if (wordByteFlag == WORD) {
		}
	} else if (directionFlag == LEFT_TO_RIGHT) {
		// Reverse direction
	}
}

void generateASM(FILE* fileToWrite) {
	
	// Identify Opcode
	switch (opcode) {
		case 0x22: // 100010 MOV
			MOV(fileToWrite);
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
	for (int i=0; i < fileLength; i+=2) {
		decodeInstructionStream(fileReadBuffer, i);
		generateASM(ASMOutputFile);
	} 

	// Close output file
	fclose(ASMOutputFile);

}

