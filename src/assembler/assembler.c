#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PROGRAM_SIZE (512)
typedef struct
{
	const char program[TEST_PROGRAM_SIZE];
	const char output[TEST_PROGRAM_SIZE];
} Test;

#define NUM_TESTS (1)
Test tests[NUM_TESTS] = {
	{{"    ; AASGNWFIAWNA"},{}}, // comments
};

int lex(const char *in, int in_size, const char *out, int out_size)
{
	(void)out;
	(void)out_size;
	int in_off=0;
	while(*(in+in_off) != 0)
	{
		in_off++;
		if(in_off >= in_size)
		{
			printf("\nInput exhausted without reaching null char");
			return 1;
		}
	}
	return 0;
}

int run_test(int n)
{
	char out[TEST_PROGRAM_SIZE];
	int fail = lex(tests[n].program, TEST_PROGRAM_SIZE, out, TEST_PROGRAM_SIZE);
	if(fail)
		return 1;
	printf("P");
	return 0;
}

int main( int arg, const char** argv)
{
	(void)(arg);
	(void)(argv);

	printf( "\nRunning Tests: " );
	int fail = 0;

	int i;
	for(i=0; i<NUM_TESTS&&!fail; i++)
		fail = run_test(i);
	if(fail)
		printf("\n\nSome tests failed!");
	else
		printf("\n\nAll tests passed :)\n");

	return fail;
}