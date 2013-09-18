#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char registers[16];
char memory[256*256];
unsigned int PC;

const char program[] = {0x00,0x01,0x0E,0x00};

int main( int arg, const char** argv)
{
	printf( "\nRunning...\n\n" );

	int running = 1;
	char exitCode = 0;
	PC = 0;
	memcpy ( memory, program, sizeof(program) );
	while(running)
	{
		char instruction = memory[PC]&0x0f;
		int d = (memory[PC]&0xf0) >> 4;
		// int a = memory[PC+1]&0x0f;
		// int b = (memory[PC+1]&0xf0) >> 4;
		int v = memory[PC+1];
		switch(instruction)
		{
			case 0x0: // VAL
				registers[d] = v;
				break;
			case 0xE: // HLT
				exitCode = registers[d];
				running = 0;
				break;
		}
		PC += 2;
	}
	exit(exitCode);
}