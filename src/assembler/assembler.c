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

#define NUM_TESTS (4)
Test tests[NUM_TESTS] = {
	{{"; AASGNWFIAWNA\n"},{}, 0}, // comments
	{{"val 2 4f\n"},{0x02,0x4f}, 2}, // val
	{{"val 1 f4\nval f aa; comment\n"},{0x01,0xf4, 0x0f,0xaa}, 4}, // multiple instructions
	{{"add 0 1 2\n"},{0x10,0x12},2}, // add
};

#define NUM_TOKENS (4)
#define MAX_TOKEN_LENGTH (3)

int get_arg(unsigned char token[MAX_TOKEN_LENGTH], int len, int line)
{
	int val=0;
	int i;
	for(i=0; i<MAX_TOKEN_LENGTH; i++)
	{
		if(token[i] == 0)
			break;
		int v = -1;
		if(token[i] >= '0' && token[i] <= '9')
			v = token[i]-'0';
		if(token[i] >= 'a' && token[i] <= 'f')
			v = token[i]-'a'+10;
		if(v==-1)
		{
			printf("\nInvalid hex char '%c' (0x%02x) in line %d", token[i], token[i], line);
			return -1;
		}
		if(i >= len)
		{
			printf("\nArg too long on %d", line);
			return -1;
		}
		val = (val<<4)+v;
	}
	return val;
}

typedef enum
{
	ARGS_D,
	ARGS_DV,
	ARGS_AB,
	ARGS_DAB,
	ARGS_INVALID,
} ArgsType;

int create_instruction(unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH], int num_tokens, unsigned char *c, int line)
{
	(void)tokens;
	(void)c;

	ArgsType type = ARGS_INVALID;
	if(strncmp((char*)tokens[0], "val", MAX_TOKEN_LENGTH)==0)
	{
		*c = 0x00;
		type = ARGS_DV;
	} else if(strncmp((char*)tokens[0], "add", MAX_TOKEN_LENGTH)==0)
	{
		*c = 0x10;
		type = ARGS_DAB;
	}
	if(type == ARGS_INVALID)
	{
		printf("\nUnsupported instruction on line %d", line);
		return 1;
	}
	int d;
	int v;
	int a;
	int b;
	switch(type)
	{
		case ARGS_DV:
		{
			if(num_tokens != 3)
			{
				printf("\nExpected 2 args, but found %d on line %d", num_tokens-1, line);
				return 1;
			}
			d = get_arg(tokens[1], 1, line);
			v = get_arg(tokens[2], 2, line);
			if(d == -1 || v == -1)
				return 1;
			*c |= d;
			*(c+1) = v;

			break;
		}
		case ARGS_DAB:
		{
			if(num_tokens != 4)
			{
				printf("\nExpected 3 args, but found %d on line %d", num_tokens-1, line);
				return 1;
			}
			d = get_arg(tokens[1], 1, line);
			a = get_arg(tokens[2], 1, line);
			b = get_arg(tokens[3], 1, line);
			*c |= d;
			*(c+1) = (a<<4)+b;
			break;
		}
		default:
			printf("\nUnsupported arg type %d", type);
			return 1;
	}
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
	unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH];
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
				int fail = create_instruction(tokens, token_num+1, &out[out_off], line);
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
	*out_size = out_off;
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
	int i=0;
	for(i=0; i<tests[n].output_size; i++)
		differ |= out[i] != tests[n].output[i];
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