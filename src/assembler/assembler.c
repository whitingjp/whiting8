//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/logging.h>

#define TEST_PROGRAM_SIZE (512)
typedef struct
{
	const unsigned char program[TEST_PROGRAM_SIZE];
	const unsigned char output[TEST_PROGRAM_SIZE];
	int output_size;
} Test;

#define NUM_TESTS (19)
Test tests[NUM_TESTS] = {
	{{"; AASGNWFIAWNA\n"},{}, 0}, // comments
	{{"val 2 4f\n"},{0x02,0x4f}, 2}, // val
	{{"val 1 f4\nval f aa; comment\n"},{0x01,0xf4, 0x0f,0xaa}, 4}, // multiple instructions
	{{"add 0 1 2\n"},{0x10,0x12},2}, // add
	{{"sub 1 2 3\n"},{0x21,0x23},2}, // sub
	{{"and 2 3 4\n"},{0x32,0x34},2}, // and
	{{"or 3 4 5\n"},{0x43,0x45},2}, // or
	{{"gte 4 5 6\n"},{0x54,0x56},2}, // gte
	{{"lod 5 6 7\n"},{0x65,0x67},2}, // lod
	{{"sav 6 7 8\n"},{0x76,0x78},2}, // sav
	{{"jmp 7 8\n"},{0x80,0x78},2}, // jmp
	{{"cnd 8\n"},{0x98,0x00},2}, // cnd
	{{"ovr 9\n"},{0xa9,0x00},2}, // ovr
	{{"pnt a\n"},{0xba,0x00},2}, // pnt
	{{"dsp b c d\n"},{0xcb,0xcd},2}, // dsp
	{{"inp c\n"},{0xdc,0x00},2}, // inp
	{{"hlt d\n"},{0xed,0x00},2}, // hlt
	{{"snd e f\n"},{0xf0,0xef},2}, // snd
	{{"val 0 f0\nval 1 f2\nadd 0 0 1\nhlt 0\n"},{0x00,0xf0, 0x01,0xf2, 0x10,0x01, 0xe0,0x00},8}, // longer test
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
			LOG("Invalid hex char '%c' (0x%02x) in line %d", token[i], token[i], line);
			return -1;
		}
		if(i >= len)
		{
			LOG("Arg too long on %d", line);
			return -1;
		}
		val = (val<<4)+v;
	}
	return val;
}

typedef enum
{
	ARGS_D=1,
	ARGS_V=2,
	ARGS_AB=4,
	ARGS_INVALID=0,
} ArgsType;

typedef struct
{
	char instruction[MAX_TOKEN_LENGTH];
	ArgsType type;
} Op;

#define NUMBER_OF_OPS (16)
Op ops[NUMBER_OF_OPS] = {
	{"val", ARGS_D | ARGS_V},
	{"add", ARGS_D | ARGS_AB},
	{"sub", ARGS_D | ARGS_AB},
	{"and", ARGS_D | ARGS_AB},
	{"or", ARGS_D | ARGS_AB},
	{"gte", ARGS_D | ARGS_AB},
	{"lod", ARGS_D | ARGS_AB},
	{"sav", ARGS_D | ARGS_AB},
	{"jmp", ARGS_AB},
	{"cnd", ARGS_D},
	{"ovr", ARGS_D},
	{"pnt", ARGS_D},
	{"dsp", ARGS_D | ARGS_AB},
	{"inp", ARGS_D},
	{"hlt", ARGS_D},
	{"snd", ARGS_AB},
};

int create_instruction(unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH], int num_tokens, unsigned char *c, int line)
{
	*c = 0;
	*(c+1) = 0;
	ArgsType type = ARGS_INVALID;
	int i;
	for(i=0; i<NUMBER_OF_OPS; i++)
	{
		if(strncmp((char*)tokens[0], ops[i].instruction, MAX_TOKEN_LENGTH)==0)
		{
			*c = i<<4;
			type = ops[i].type;
		}
	}

	if(type == ARGS_INVALID)
	{
		LOG("Unsupported instruction on line %d", line);
		return 1;
	}
	int required_num_args = 0;
	if(type & ARGS_D) required_num_args++;
	if(type & ARGS_V) required_num_args++;
	if(type & ARGS_AB) required_num_args+=2;
	if(num_tokens-1 != required_num_args)
	{
		LOG("Expected %d args, but found %d on line %d", required_num_args, num_tokens-1, line);
		return 1;
	}
	int current_token=1;
	if(type & ARGS_D)
	{
		int d = get_arg(tokens[current_token++], 1, line);
		if(d == -1) return 1;
		*c |= d;
	}
	if(type & ARGS_V)
	{
		int v = get_arg(tokens[current_token++], 2, line);
		if(v == -1) return 1;
		*(c+1) = v;
	}
	if(type & ARGS_AB)
	{
		int a = get_arg(tokens[current_token++], 1, line);
		int b = get_arg(tokens[current_token++], 1, line);
		if(a == -1 || b == -1) return 1;
		*(c+1) = (a<<4)+b;
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
				LOG("Missing terminating newline on line %d", line);
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
				LOG("Run out of output space.");
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
					LOG("Too many tokens on line %d", line);
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
				LOG("Token too long on line %d", line);
				return 1;
			}
			tokens[token_num][token_pos] = c;
			token_pos++;
			continue;			
		}
		LOG("Unrecognised char '%c' (0x%x) on line %d", c, c, line);
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
		QLOG("\nTEST %d FAILED: Assembly problem.", n);
		return 1;
	}
	int differ = 0;
	int i=0;
	for(i=0; i<tests[n].output_size; i++)
		differ |= out[i] != tests[n].output[i];
	if(differ)
	{
		QLOG("\nTEST %d FAILED: Output differed.", n);
		QLOG("\nExpected: ");
		const unsigned char *cp;
		cp = tests[n].output;
		for(i=0; i<tests[n].output_size; i++)
			QLOG("0x%02x,",*(cp+i));
		QLOG("\n  Actual: ");
		cp = out;
		for(i=0; i<out_size; i++)
			QLOG("0x%02x,",*(cp+i));
		return 1;
	}
	QLOG("P");
	return 0;
}

int main( int arg, const char** argv)
{
	(void)(arg);
	(void)(argv);

	QLOG( "Running Tests: " );
	int fail = 0;

	int i;
	for(i=0; i<NUM_TESTS&&!fail; i++)
		fail = run_test(i);
	if(fail)
		QLOG("\n\nSome tests failed!\n");
	else
		QLOG("\nAll tests passed :)\n");

	return fail;
}