#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PROGRAM_SIZE (512)
typedef struct
{
	const unsigned char program[TEST_PROGRAM_SIZE];
	const unsigned char output[TEST_PROGRAM_SIZE];
} Test;

#define NUM_TESTS (2)
Test tests[NUM_TESTS] = {
	{{"; AASGNWFIAWNA"},{}}, // comments
	{{"val 2 4f"},{0x02,0x4f}}, // val
};

#define NUM_TOKENS (4)
#define MAX_TOKEN_LENGTH (3)
int lex(const unsigned char *in, int in_size, const unsigned char *out, int out_size)
{
	(void)out;
	(void)out_size;
	int in_off;
	int comment_mode = 0;
	int line = 1;
	int token_num = 0;
	int token_pos = 0;
	char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH+1];
	memset(tokens, 0, sizeof(tokens));
	for(in_off=0; in_off<in_size; in_off++)
	{
		unsigned char c = *(in+in_off);
		if(c == 0)
			break;
		if(comment_mode)
		{
			if(c == '\n')
				comment_mode = 0;
		}
		if(c == '\n')
		{
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
	return 0;
}

int run_test(int n)
{
	unsigned char out[TEST_PROGRAM_SIZE];
	memset(out, 0, sizeof(out));
	int fail = lex(tests[n].program, TEST_PROGRAM_SIZE, out, TEST_PROGRAM_SIZE);
	if(fail)
	{
		printf("\nTEST %d FAILED: Assembly problem.", n);
		return 1;
	}
	if(strncmp((char*)out, (char*)tests[n].output, TEST_PROGRAM_SIZE)!=0)
	{
		printf("\nTEST %d FAILED: Output differed.", n);
		printf("\nExpected: ");
		const unsigned char *cp;
		for(cp=tests[n].output; *cp!=0; cp++)
			printf("0x%02x,",*cp);
		printf("\n  Actual: ");
		for(cp=out; *cp!=0; cp++)
			printf("0x%02x,",*cp);
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