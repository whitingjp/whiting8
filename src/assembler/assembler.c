#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PROGRAM_SIZE (512)
typedef struct
{
	const unsigned char program[TEST_PROGRAM_SIZE];
	const unsigned char output[TEST_PROGRAM_SIZE];
	int output_size;
} Test;

#define NUM_TESTS (2)
Test tests[NUM_TESTS] = {
	{{"; AASGNWFIAWNA\n"},{}, 0}, // comments
	{{"val 2 4f\n"},{0x02,0x4f}, 2}, // val
};

#define NUM_TOKENS (4)
#define MAX_TOKEN_LENGTH (3)

int create_instruction(unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH+1], unsigned char *c)
{
	(void)tokens;
	(void)c;

	if(strncmp((char*)tokens[0], "val", MAX_TOKEN_LENGTH)==0)
		*c = 0x00;
	return 0;
}

int tokenize(const unsigned char *in, unsigned char *out, int *out_size)
{
	int in_off=0;
	int out_off=0;
	int comment_mode = 0;
	int line = 1;
	int token_num = 0;
	int token_pos = 0;
	unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH+1];
	memset(tokens, 0, sizeof(tokens));
	for(in_off=0; in_off<TEST_PROGRAM_SIZE; in_off++)
	{
		unsigned char c = *(in+in_off);
		if(c == 0)
		{
			if(token_num || token_pos)
			{
				printf("\nMissing terminating newline on line %d", line);
			}
			break;
		}
		if(comment_mode)
		{
			if(c == '\n')
				comment_mode = 0;
		}
		if(c == '\n')
		{
			if(out_off == TEST_PROGRAM_SIZE-1)
			{
				printf("\nRun out of output space.");
				return 1;
			}
			if(token_num || token_pos)
			{
				int fail = create_instruction(tokens, &out[out_off]);
				if(fail) return 1;
				out_off+=2;
			}
			token_num = 0;
			token_pos = 0;
			memset(tokens, 0, sizeof(tokens));
			line++;
			continue;
		}
		if(comment_mode)
			continue;
		if(c == ' ')
		{
			if(token_pos > 0)
			{
				if(token_num+1 == NUM_TOKENS)
					printf("\nToo many tokens on line %d", line);
				token_num++;
				token_pos = 0;
			}
			continue;
		}
		if(c == ';')
		{
			comment_mode = 1;
			continue;
		}
		if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
		{
			if(token_pos == MAX_TOKEN_LENGTH)
			{
				printf("\nToken too long on line %d", line);
				return 1;
			}
			tokens[token_num][token_pos] = c;
			token_pos++;
			continue;			
		}
		printf("\nUnrecognised char '%c' (0x%x) on line %d", c, c, line);
		return 1;
	}
	*out_size = out_off-1;
	return 0;
}

int run_test(int n)
{
	unsigned char out[TEST_PROGRAM_SIZE];
	memset(out, 0xef, sizeof(out));
	int out_size;
	int fail = tokenize(tests[n].program, out, &out_size);
	if(fail)
	{
		printf("\nTEST %d FAILED: Assembly problem.", n);
		return 1;
	}
	int differ = 0;
	unsigned char out_comparator[TEST_PROGRAM_SIZE];
	int i=0;
	for(i=0; i<tests[n].output_size; i++)
		differ |= out[i] != out_comparator[i];
	if(differ)
	{
		printf("\nTEST %d FAILED: Output differed.", n);
		printf("\nExpected: ");
		const unsigned char *cp;
		cp = tests[n].output;
		for(i=0; i<tests[n].output_size; i++)
			printf("0x%02x,",*(cp+i));
		printf("\n  Actual: ");
		cp = out;
		for(i=0; i<out_size; i++)
			printf("0x%02x,",*(cp+i));
		return 1;
	}
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