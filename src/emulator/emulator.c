#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/file.h>
#include <common/logging.h>

#define NUM_REGISTERS (16)
unsigned char registers[16];
#define MEMORY_SIZE (256*256)
unsigned char memory[MEMORY_SIZE];
unsigned int PC;

#define TEST_PROGRAM_SIZE (512)
typedef struct
{
	const char program[TEST_PROGRAM_SIZE];
	int exit_code;
} Test;

#define NUM_TESTS (50)
Test tests[NUM_TESTS] = {
	{{0x00,0x00, 0xe0,0x00}, 0x00}, // can exit
	{{0x00,0x01, 0xe0,0x00}, 0x01}, // can exit with non-zero code
	{{0x0f,0x05, 0xef,0x00}, 0x05}, // can use higher register
	{{0x00,0x04, 0x01,0x05, 0x10,0x01, 0xe0,0x00}, 0x09}, // add two values
	{{0x00,0xf0, 0x01,0xf2, 0x10,0x01, 0xe0,0x00}, 0xe2}, // add two high values
	{{0x00,0x05, 0x01,0x03, 0x11,0x01, 0xe0,0x00}, 0x02}, // sub
	{{0x00,0x05, 0x01,0x03, 0x11,0x10, 0xe1,0x00}, 0xfe}, // sub larger from smaller
	{{0x00,0x04, 0x01,0x05, 0x02,0x10, 0x10,0x01, 0xa2,0x00, 0xe2,0x00}, 0x10}, // add two low values, check overflow
	{{0x00,0xf0, 0x01,0xf2, 0x02,0x10, 0x10,0x01, 0xa2,0x00, 0xe2,0x00}, 0x11}, // add two high values, check overflow
	{{0x00,0x05, 0x01,0x03, 0x02,0x07, 0x11,0x01, 0xa2,0x00, 0xe2,0x00}, 0x07}, // sub without overflow
	{{0x00,0x05, 0x01,0x03, 0x02,0x07, 0x11,0x10, 0xa2,0x00, 0xe2,0x00}, 0x06}, // sub with overflow
	{{0x00,0x08, 0x01,0x04, 0x12,0x01, 0xe0,0x00}, 0x02}, // div
	{{0x00,0x08, 0x01,0x00, 0x12,0x01, 0xe0,0x00}, 0xff}, // div by zero
	{{0x00,0x08, 0x01,0x04, 0x13,0x01, 0xe0,0x00}, 0x20}, // mul
	{{0x00,0xff, 0x01,0xff, 0x02,0x00, 0x13,0x01, 0xa2,0x00, 0xe2,0x00}, 0xfe}, // mul overflow
	{{0x00,0x08, 0x14,0x00, 0xe0,0x00}, 0x09}, // inc
	{{0x00,0xff, 0x02,0x00, 0x14,0x00, 0xa2,0x00, 0xe2,0x00}, 0x01}, // inc overflow
	{{0x00,0x08, 0x15,0x00, 0xe0,0x00}, 0x07}, // dec
	{{0x00,0x00, 0x02,0x05, 0x15,0x00, 0xa2,0x00, 0xe2,0x00}, 0x04}, // dec overflow
	{{0x00,0x14, 0x01,0x25, 0x16,0x01, 0xe0,0x00}, 0x04}, // and
	{{0x00,0x14, 0x01,0x25, 0x17,0x01, 0xe0,0x00}, 0x35}, // or
	{{0x00,0x14, 0x01,0x25, 0x18,0x01, 0xe0,0x00}, 0x31}, // xor
	{{0x00,0x14, 0x01,0x03, 0x19,0x01, 0xe0,0x00}, 0x02}, // shiftr
	{{0x00,0xf2, 0x01,0x02, 0x1a,0x01, 0xe0,0x00}, 0xc8}, // shiftl
	{{0x00,0x4f, 0x01,0x0f, 0x1b,0x01, 0xe0,0x00}, 0x04}, // mod
	{{0x00,0x00, 0x01,0xca, 0x1c,0x01, 0xe0,0x00}, 0xca}, // copy
	{{0x00,0xaf, 0x01,0xaf, 0x20,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // eq
	{{0x00,0xaf, 0x01,0xae, 0x20,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !eq
	{{0x00,0xaf, 0x01,0xae, 0x21,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // neq
	{{0x00,0xaf, 0x01,0xaf, 0x21,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !neq
	{{0x00,0xb0, 0x01,0xaf, 0x22,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // gt
	{{0x00,0xaf, 0x01,0xb0, 0x22,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !gt
	{{0x00,0xaf, 0x01,0xaf, 0x23,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // gte eq
	{{0x00,0xb0, 0x01,0xaf, 0x23,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // gte gt
	{{0x00,0xaf, 0x01,0xb0, 0x23,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !gte
	{{0x00,0xaf, 0x01,0xb0, 0x24,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // lt
	{{0x00,0xb0, 0x01,0xaf, 0x24,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !lt
	{{0x00,0xaf, 0x01,0xaf, 0x25,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // lte eq
	{{0x00,0xaf, 0x01,0xb0, 0x25,0x01, 0x28,0x20, 0xe2,0x00}, 0x01}, // lte gt
	{{0x00,0xb0, 0x01,0xaf, 0x25,0x01, 0x28,0x20, 0xe2,0x00}, 0x00}, // !lte
	{{0x00,0x00, 0x26,0x00, 0x28,0x20, 0xe2,0x00}, 0x01}, // is0
	{{0x00,0xaf, 0x26,0x00, 0x28,0x20, 0xe2,0x00}, 0x00}, // !is0
	{{0x00,0xaf, 0x27,0x00, 0x28,0x20, 0xe2,0x00}, 0x01}, // not0
	{{0x00,0x00, 0x27,0x00, 0x28,0x20, 0xe2,0x00}, 0x00}, // !not0
	{{0x01,0x00, 0x02,0x08, 0x60,0x12, 0xe0,0x00, /*data*/ 0xfe}, 0xfe}, // lod memory
	{{0x00,0xef, 0x01,0xcc, 0x02,0xaa, 0x70,0x12, 0x63,0x12, 0xe3,0x00}, 0xef}, // sav then lod high mem page
	{{0x00,0x00, 0x01,0x08, 0x30,0x01, 0xe0,0x00, 0xe1,0x00}, 0x08}, // jmp over a halt
	{{0x00,0x00, 0x01,0x0a, 0x26,0x00, 0x31,0x01, 0xe0,0x00, 0xe1,0x00}, 0x0a}, // jcn
	{{0x00,0x00, 0x01,0x0a, 0x27,0x00, 0x31,0x01, 0xe0,0x00, 0xe1,0x00}, 0x00}, // jcn negative
	{{0x00,0x00, 0x01, 'P', 0xB1,0x00, 0xe0,0x00}, 0x00}, // pnt
	//{{0x00,0x00, 0x01,0x0f, 0x02,0x01, 0x10,0x01, 0x21,0x12, 0x03,0x00, 0x04,0x06, 0x91,0x34, 0xe0,0x00},0x78}, // calculate 15th tri number
};

inline int memory_location(int a, int b)
{
	return (a<<8)+b;
}

inline int math(unsigned char code, unsigned char *a, const unsigned char *b)
{
	int overflow = 0;
	switch(code)
	{
		case 0x0: // add
			if(0xff-*b < *a)
				overflow = 1;
			*a=*a+*b;
			break;
		case 0x1: // sub
			if(*b > *a)
				overflow = -1;
			*a=*a-*b;
			break;
		case 0x2: // div
			if(*b == 0)
				*a = 0xff;
			else
				*a=*a / *b;
			break;
		case 0x3: // mul
		{
			unsigned int av = *a;
			unsigned int bv = *b;
			unsigned int r = av*bv;
			*a=0xff&r;
			overflow=(0xff00&r)>>8;
			break;
		}
		case 0x4: // inc
			if(*a == 0xff)
				overflow = 1;
			(*a)++;
			break;
		case 0x5: // dec
			if(*a == 0x00)
				overflow = -1;
			(*a)--;
			break;
		case 0x6: *a=*a&*b; break; // and
		case 0x7: *a=*a|*b; break; // or
		case 0x8: *a=*a^*b; break; // xor
		case 0x9: *a=*a>>*b; break; // rshift
		case 0xa: *a=*a<<*b; break; // lshift
		case 0xb: *a=(*a)%(*b); break; // mod
		case 0xc: *a=*b; break; // copy
		default:
			LOG("Invalid math instruction 0x%x", code);
			*a=0xff;
			break;
	}
	return overflow;
}

inline void cond(unsigned char code, unsigned char *a, unsigned char *b, unsigned char *comparebit)
{
	switch(code)
	{
		case 0x0: *comparebit = *a==*b; break; // eq
		case 0x1: *comparebit = *a!=*b; break; // neq
		case 0x2: *comparebit = *a>*b; break; // gt
		case 0x3: *comparebit = *a>=*b; break; // gte
		case 0x4: *comparebit = *a<*b; break; // lt
		case 0x5: *comparebit = *a<=*b; break; // lte
		case 0x6: *comparebit = *a==0; break; // is0
		case 0x7: *comparebit = *a!=0; break; // not0
		case 0x8: *a = *comparebit; break; // cnd
		default:
			LOG("Invalid cond instruction 0x%x", code);
			*comparebit = 0xff;
			break;
	}
}

inline bool should_jump(unsigned char code, unsigned char comparebit)
{
	switch(code)
	{
		case 0x0: return true; break;
		case 0x1: return comparebit; break;
		default:
			LOG("Invalid jump instruction 0x%x", code);
			return false;
	}
}

unsigned char run()
{
	int running = 1;
	unsigned char exit_code = 0;
	PC = 0;
	int overflow=0;
	unsigned char comparebit=0;
	while(running)
	{
		unsigned char instruction = (memory[PC]&0xf0) >> 4;
		int d = memory[PC]&0x0f;
		int a = (memory[PC+1]&0xf0) >> 4;
		int b = memory[PC+1]&0x0f;
		int v = memory[PC+1];
		int new_overflow = 0;
		switch(instruction)
		{
			case 0x0: // val
				registers[d] = v;
				break;
			case 0x1: // math
				new_overflow = math(d, &registers[a], &registers[b]);
				break;
			case 0x2: // cond
				cond(d, &registers[a], &registers[b], &comparebit);
				break;
			case 0x3: // jmp
				if(should_jump(d, comparebit))
				{
					PC = memory_location(registers[a], registers[b]);
					PC -= 2; // because we are about to advance it
				}
				break;
			case 0x6: // lod
				registers[d] = memory[memory_location(registers[a],registers[b])];
				break;
			case 0x7: // sav
				memory[memory_location(registers[a],registers[b])] = registers[d];
				break;
			case 0xa: // ovr
				if(overflow > 0 && 0xff-overflow < registers[d])
					new_overflow = 1;
				if(overflow < 0 && -overflow > registers[d])
					new_overflow = -1;
				registers[d]+=overflow;
				break;
			case 0xb: // pnt
				putchar(registers[d]);
				break;
			case 0xe: // hlt
				exit_code = registers[d];
				running = 0;
				break;
			default:
				LOG("Invalid instruction 0x%02x", instruction);
				exit_code = 0xff;
				running = 0;
				break;
		}
		PC += 2;
		overflow = new_overflow;
	}
	return exit_code;
}

int run_test(int n)
{
	memcpy ( memory, tests[n].program, TEST_PROGRAM_SIZE );
	unsigned char exit_code = run();
	if(exit_code == tests[n].exit_code)
	{
		QLOG("P");
		return 0;
	}
	else
	{
		LOG("TEST %d FAILED: Expected exit_code 0x%02x got 0x%02x", n, tests[n].exit_code, exit_code);
		return 1;
	}
}

void corrupt_memory()
{
	int i;
	srand(0);
	for(i=0; i<MEMORY_SIZE; i++)
		memory[i] = rand();
	for(i=0; i<NUM_REGISTERS; i++)
		registers[i] = rand();
}

int run_tests()
{
	corrupt_memory();

	QLOG( "Running Tests: " );

	int i;
	int fail = 0;
	for(i=0; i<NUM_TESTS&&!fail; i++)
		fail = run_test(i);
	if(fail)
		QLOG("\nSome tests failed!\n");
	else
		QLOG("\nAll tests passed :)");
	return fail;
}

int main( int arg, const char** argv)
{
	if(arg < 2)
	{
		QLOG("Insufficent arguments.");
		return 1;
	}
	if(strncmp(argv[1], "--test", 6) == 0)
	{
		if(arg < 3)
		{
			QLOG("Insufficent arguments.");
			return 1;
		}
		set_logfile(argv[2]);
		return run_tests();
	}
	
	unsigned char input[TEST_PROGRAM_SIZE];
	int size;
	bool success = file_load(argv[1], &size, input, sizeof(input));
	if(!success)
	{
		LOG("Failed to load machine code file.");
		return 1;
	}
	memset(memory, 0xe0, sizeof(memory)); // Halt rapidly if overrun
	memset(registers, 0, sizeof(registers));
	memcpy(memory, input, size);
	return run();
}