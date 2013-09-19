#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_REGISTERS (16)
unsigned char registers[16];
#define MEMORY_SIZE (256*256)
unsigned char memory[MEMORY_SIZE];
unsigned int PC;

#define TEST_PROGRAM_SIZE (128)
typedef struct
{
	const char program[TEST_PROGRAM_SIZE];
	int exit_code;
} Test;

#define NUM_TESTS (17)
Test tests[NUM_TESTS] = {
	{{0x00,0x00, 0x0e,0x00}, 0x00}, // can exit
	{{0x00,0x01, 0x0e,0x00}, 0x01}, // can exit with non-zero code
	{{0xf0,0x05, 0xfe,0x00}, 0x05}, // can use higher register
	{{0x00,0x04, 0x10,0x05, 0x01,0x10, 0x0e,0x00}, 0x09}, // add two values
	{{0x00,0xf0, 0x10,0xf2, 0x01,0x10, 0x0e,0x00}, 0xe2}, // add two high values
	{{0x00,0x05, 0x10,0x03, 0x02,0x10, 0x0e,0x00}, 0x02}, // sub
	{{0x00,0x05, 0x10,0x03, 0x02,0x01, 0x0e,0x00}, 0xfe}, // sub larger from smaller
	{{0x00,0x04, 0x10,0x05, 0x20,0x10, 0x01,0x10, 0x2a,0x00, 0x2e,0x00}, 0x10}, // add two low values, check overflow
	{{0x00,0xf0, 0x10,0xf2, 0x20,0x10, 0x01,0x10, 0x2a,0x00, 0x2e,0x00}, 0x11}, // add two high values, check overflow
	{{0x00,0x05, 0x10,0x03, 0x20,0x07, 0x02,0x10, 0x2a,0x00, 0x2e,0x00}, 0x07}, // sub without overflow
	{{0x00,0x05, 0x10,0x03, 0x20,0x07, 0x02,0x01, 0x2a,0x00, 0x2e,0x00}, 0x06}, // sub with overflow
	{{0x00,0x14, 0x10,0x25, 0x03,0x10, 0x0e,0x00}, 0x04}, // and
	{{0x00,0x14, 0x10,0x25, 0x04,0x10, 0x0e,0x00}, 0x35}, // or
	{{0x00,0x15, 0x10,0x14, 0x05,0x10, 0x0e,0x00}, 0x01}, // is greater
	{{0x00,0x15, 0x10,0x15, 0x05,0x10, 0x0e,0x00}, 0x00}, // isn't greater
	{{0x10,0x00, 0x20,0x08, 0x06,0x21, 0x0e,0x00, /*data*/ 0xfe}, 0xfe}, // lod memory
	{{0x00,0xef, 0x10,0xcc, 0x20,0xaa, 0x07,0x21, 0x36,0x21, 0x3e,0x00}, 0xef}, // sav then lod high mem page
};

inline int memory_location(int a, int b)
{
	return (a<<8)+b;
}

unsigned char run()
{
	int running = 1;
	unsigned char exit_code = 0;
	PC = 0;
	int overflow=0;
	while(running)
	{
		unsigned char instruction = memory[PC]&0x0f;
		int d = (memory[PC]&0xf0) >> 4;
		int a = memory[PC+1]&0x0f;
		int b = (memory[PC+1]&0xf0) >> 4;
		int v = memory[PC+1];
		int new_overflow = 0;
		switch(instruction)
		{
			case 0x0: // VAL
				registers[d] = v;
				break;
			case 0x1: // ADD
				if(0xff-registers[b] < registers[a])
					new_overflow = 1;
				registers[d] = registers[a] + registers[b];
				break;
			case 0x2: // SUB
				if(registers[b] > registers[a])
					new_overflow = -1;
				registers[d] = registers[a] - registers[b];
				break;
			case 0x3: // AND
				registers[d] = registers[a] & registers[b];
				break;
			case 0x4: // OR
				registers[d] = registers[a] | registers[b];
				break;
			case 0x5: // GTE
				registers[d] = registers[a] > registers[b];
				break;
			case 0x6: // LOD
				registers[d] = memory[memory_location(registers[a],registers[b])];
				break;
			case 0x7: // SAV
				memory[memory_location(registers[a],registers[b])] = registers[d];
			case 0xa: // OVR
				registers[d]+=overflow;
				break;
			case 0xe: // HLT
				exit_code = registers[d];
				running = 0;
				break;
			default:
				printf("\nInvalid instruction 0x%x", instruction);
				exit_code = 0xff;
				running = 0;
				break;
		}
		overflow = new_overflow;
		PC += 2;
	}
	return exit_code;
}

int run_test(int n)
{
	memcpy ( memory, tests[n].program, TEST_PROGRAM_SIZE );
	unsigned char exit_code = run();
	if(exit_code == tests[n].exit_code)
	{
		printf("P");
		return 0;
	}
	else
	{
		printf("\nTEST %d FAILED: Expected exit_code 0x%x got 0x%x", n, tests[n].exit_code, exit_code);
		return 1;
	}
}

void corrupt_memory()
{
	printf("\nFilling memory with values.");
	int i;
	srand(0);
	for(i=0; i<MEMORY_SIZE; i++)
		memory[i] = rand();
	for(i=0; i<NUM_REGISTERS; i++)
		registers[i] = rand();
}

int main( int arg, const char** argv)
{
	corrupt_memory();

	printf( "\nRunning Tests: " );

	int i;
	int fail = 0;
	for(i=0; i<NUM_TESTS&&!fail; i++)
		fail = run_test(i);
	if(fail)
		printf("\n\nSome tests failed!");
	else
		printf("\n\nAll tests passed :)\n");
	return fail;
}