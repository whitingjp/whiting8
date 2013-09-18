#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_REGISTERS (16)
char registers[16];
#define MEMORY_SIZE (256*256)
char memory[MEMORY_SIZE];
unsigned int PC;

#define TEST_PROGRAM_SIZE (128)
typedef struct
{
	const char program[TEST_PROGRAM_SIZE];
	int exit_code;
} Test;

#define NUM_TESTS (2)
Test tests[NUM_TESTS] = {
	{{0x00,0x00,0x0E,0x00}, 0},
	{{0x00,0x01,0x0E,0x00}, 1},
};

unsigned char run()
{
	int running = 1;
	unsigned char exit_code = 0;
	PC = 0;	
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
				exit_code = registers[d];
				running = 0;
				break;
		}
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