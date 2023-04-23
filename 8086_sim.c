#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;

// Parts of instruction
u8 opcode, directionFlag, wordByteFlag, mode, reg, regMem, dataLow, dataHigh, dispLo, dispHi, SWbits;

// Registers
u16 AX, BX, CX, DX;
u8 SI, DI, BP, SP;

// Register Strings
u8* regStr[16] = {"ax\0", "cx\0", "dx\0", "bx\0", "sp\0", "bp\0", "si\0", "di\0", "al\0", "cl\0", "dl\0", "bl\0", "ah\0", "ch\0", "dh\0", "bh\0"};

u8* effAddrStr[8] = {"bx + si\0", "bx + di\0", "bp + si\0", "bp + di\0", "si\0", "di\0", "bp\0", "bx"};

// Enums
enum directions {REG_SOURCE, REG_DEST};
enum wordByte {BYTE, WORD};
enum modes {MODE_MEM_NO_DISP, MODE_MEM_8_DISP, MODE_MEM_16_DISP, MODE_REG};
typedef enum bool {false, true} bool;

// Globals
char ASMOutputBuffer[128];

// Settings
int debug = false;

int getFileLength(FILE* file) {
	int fileLength = 0;
	fseek(file, 0, SEEK_END);
	fileLength = ftell(file);
	fseek(file, 0, SEEK_SET);
	return fileLength;
}

long long binPrint(u8 byte) {
	long long bin = 0;
	int rem, i = 1;

	while (byte != 0) {
    	rem = byte % 2;
    	byte /= 2;
    	bin += rem * i;
    	i *= 10;
 	}

	return bin;
}

int decodeInstructionBytes(u8 *buffer, int byte, FILE* file) {

	int instructionLength = 0;

	// Immediate to register move
	if ((buffer[byte] & 0b11110000) >> 4 == 0b1011) {
	
		opcode = (buffer[byte] & 0b11110000) >> 4;
		wordByteFlag = (buffer[byte] & 0b00001000) >> 3;
		reg = buffer[byte] & 0b00000111;
		dataLow = buffer[byte+1];

		if (wordByteFlag == BYTE) {
			instructionLength = 2;
		} else {
			dataHigh = buffer[byte+2];
			instructionLength = 3;
		}

		u16 data16 = (dataHigh << 8) + dataLow;
		fprintf(file, "mov %s, %d\n", wordByteFlag ? regStr[reg] : regStr[reg+8], data16);

	}



	// Immediate to accumulator add
	if ((buffer[byte]) >> 1 == 0b10) {
		wordByteFlag = buffer[byte] & 0b00000001;
		int strShift;
		strShift = wordByteFlag ? 0 : 8;
		u16 data16;
		if (wordByteFlag == BYTE) {
			instructionLength = 2;
			data16 = buffer[byte+1];
		} else if (wordByteFlag == WORD) {
			instructionLength = 3;
			data16 = buffer[byte+1] + (buffer[byte+2] << 8);
		}
		fprintf(file, "add %s, %d\n", regStr[0+strShift], data16);
	}



	// Immediate to register/memory add
	if ((buffer[byte] & 0b11111100) >> 2 == 0b100000) {

		/* 	SW Table
			00 - 8 bit data
			01 - 16 Bit data
			10 - 8 bit data
			11 - 8 bit converted to 16bits
		*/

		SWbits = (buffer[byte] & 0b00000011);
		mode = (buffer[byte+1] & 0b11000000) >> 6;
		reg = (buffer[byte+1] & 0b00111000) >> 3;
		regMem = (buffer[byte+1] & 0b00000111);
		wordByteFlag = (buffer[byte] & 0b00000001);

		switch (mode) {
			case MODE_MEM_NO_DISP:
				instructionLength = 3;
				dataLow = buffer[byte+2];
				fprintf(file, "add %s [%s], %d\n", wordByteFlag ? "word" : "byte", effAddrStr[regMem], dataLow);
				break;
			case MODE_MEM_8_DISP:
				dispLo = buffer[byte+2];
				dataLow = buffer[byte+3];
				fprintf(file, "; 8 BIT DISPLACEMENT\n");
				instructionLength = 4;
				break;
			case MODE_MEM_16_DISP:
				dispLo = buffer[byte+2];
				dispHi = buffer[byte+3];
				dataLow = buffer[byte+4];
				instructionLength = 5;
				u16 disp = (dispHi << 8) + dispLo;
				fprintf(file, "add %s [%s + %d], %d\n", wordByteFlag ? "word" : "byte", effAddrStr[regMem], disp, dataLow);
				break;
			case MODE_REG:
				instructionLength = 3;
				dataLow = buffer[byte+2];
				u16 data16 = (dataHigh << 8) + dataLow;
				fprintf(file, "add %s, %d\n", wordByteFlag ? regStr[regMem] : regStr[regMem+8], data16);
				break;
		}
	}



	// Register/memory to register/memory move/add
	if ((buffer[byte] & 0b11111100) >> 2 == 0b100010 || (buffer[byte] & 0b11111100) >> 2 == 0b000000) {

		opcode = buffer[byte] >> 2;
		directionFlag = (buffer[byte] & 0b00000010) >> 1;
		wordByteFlag = buffer[byte] & 0b00000001;
		mode = (buffer[byte+1] & 0b11000000) >> 6;
		reg = (buffer[byte+1] & 0b00111000) >> 3;
		regMem = (buffer[byte+1] & 0b00000111);
		switch (mode) {
			case MODE_MEM_NO_DISP:
				instructionLength = 2;
				break;
			case MODE_MEM_8_DISP:
				dispLo = buffer[byte+2];
				instructionLength = 3;
				break;
			case MODE_MEM_16_DISP:
				dispLo = buffer[byte+2];
				dispHi = buffer[byte+3];
				instructionLength = 4;
				break;
			case MODE_REG:
				instructionLength = 2;
				break;
		}

		char* operation;
		if (opcode == 0b100010) operation = "mov"; 
		if (opcode == 0b000000) operation = "add"; 
		int strShift = wordByteFlag ? 0 : 8;
		switch (mode) {
			case MODE_MEM_NO_DISP:
				switch (directionFlag) {
					case REG_SOURCE:
						fprintf(file, "%s [%s], %s\n", operation, effAddrStr[regMem], regStr[reg+strShift]);
						break;
					case REG_DEST:
						fprintf(file, "%s %s, [%s]\n", operation, regStr[reg+strShift], effAddrStr[regMem]);
						break;
				}
				break;
			case MODE_MEM_8_DISP:
			case MODE_MEM_16_DISP:
				u16 disp;
				if (dispLo && dispHi) {
					disp = (dispHi << 8) + dispLo;
				} else {
					disp = dispLo;
				}
				switch (directionFlag) {
					case REG_SOURCE:
						if (disp) {
							fprintf(file, "%s %s, [%s + %d]\n", operation, regStr[reg+strShift], effAddrStr[regMem], disp);
						} else {
							fprintf(file, "%s [%s], %s\n", operation, effAddrStr[regMem], regStr[reg+strShift]);
						}
						break;
					case REG_DEST:
						if (disp) {
							fprintf(file, "%s %s, [%s + %d]\n", operation, regStr[reg+strShift], effAddrStr[regMem], disp);
						} else {
							fprintf(file, "%s %s, [%s]\n", operation, regStr[reg+strShift], effAddrStr[regMem]);
						}
						break;
				}
				break;
			case MODE_REG:
				if (directionFlag == REG_SOURCE) {
					fprintf(file, "%s %s, %s\n", operation, regStr[regMem+strShift], regStr[reg+strShift]);
				}
				if (directionFlag == REG_DEST) {
					fprintf(file, "%s %s, %s\n", operation, regStr[reg+strShift], regStr[regMem+strShift]);
				} 
				break;
		}

	}

	// Debug
	if (debug) {
		if (instructionLength > 0) fprintf(file, "; ");
		for (int i = 0; i < instructionLength; i++) {
			fprintf(file, "%08lld ", binPrint(buffer[byte+i]));
		}
		if (instructionLength > 0) {
			fprintf(file, "\n\n");
		}
	}

	return instructionLength;
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
	u8 fileReadBuffer[512];
	memset(fileReadBuffer, 0, sizeof(fileReadBuffer));

	// Open file
	if ((binaryInputFile = fopen(argv[1], "rb")) == NULL) {
		printf("Error opening file!");
		exit(1);
	}

	// Get length of file
	int fileLength = getFileLength(binaryInputFile);

	// Read data from file and close
	fread(&fileReadBuffer, 512, 1, binaryInputFile);	
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

	// Debug print all bytes
	if (debug) {
		printf("File length: \033[33m%d\033[0m\n", fileLength);
		for (int i=0; i<fileLength; i++) {
			if (i % 6 == 0) printf("\n");
			printf("%08lld ", binPrint(fileReadBuffer[i]));
		}
	}

	// Decode instructions and write to output file
	int fileBytePointer = 0;
	int increment = 0;
	while (fileBytePointer < fileLength) {
		increment = decodeInstructionBytes(fileReadBuffer, fileBytePointer, ASMOutputFile);
		if (increment > 0) {
			fileBytePointer += increment;
			increment = 0;
		} else {
			printf("Cannot decode bytes\nFinal bytes: \033[31m");
			for (int i=0; i<6; i++) printf("%08lld ", binPrint(fileReadBuffer[fileBytePointer+i]));
			printf("\033[0m\n");
			exit(1);
		}
	}

	// Close output file
	fclose(ASMOutputFile);

}

