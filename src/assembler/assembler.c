#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <common/file.h>
#include <common/logging.h>

#define TEST_PROGRAM_SIZE (256*256)
typedef struct
{
	const unsigned char program[TEST_PROGRAM_SIZE];
	const unsigned char output[TEST_PROGRAM_SIZE];
	int output_size;
} Test;

#define NUM_TESTS (41)
Test tests[NUM_TESTS] = {
	{{"; AASGNWFIAWNA\n"},{}, 0}, // comments
	{{"val 2 4f\n"},{0x02,0x4f}, 2},
	{{"val 1 f4\nval f aa; comment\n"},{0x01,0xf4, 0x0f,0xaa}, 4}, // multiple instructions
	{{"add 1 2\n"},{0x10,0x12},2},
	{{"sub 2 3\n"},{0x11,0x23},2},
	{{"div 3 4\n"},{0x12,0x34},2},
	{{"mul 4 5\n"},{0x13,0x45},2},
	{{"inc 5\n"},{0x14,0x50},2},
	{{"dec 6\n"},{0x15,0x60},2},
	{{"and 3 4\n"},{0x16,0x34},2},
	{{"or 4 5\n"},{0x17,0x45},2},
	{{"xor 5 6\n"},{0x18,0x56},2},
	{{"rshift 6 7\n"},{0x19,0x67},2},
	{{"lshift 7 8\n"},{0x1a,0x78},2},
	{{"mod 8 9\n"},{0x1b,0x89},2},
	{{"copy 9 a\n"},{0x1c,0x9a},2},
	{{"eq 0 1\n"},{0x20,0x01},2},
	{{"neq 0 1\n"},{0x21,0x01},2},
	{{"gt 0 1\n"},{0x22,0x01},2},
	{{"gte 0 1\n"},{0x23,0x01},2},
	{{"lt 0 1\n"},{0x24,0x01},2},
	{{"lte 0 1\n"},{0x25,0x01},2},
	{{"is0 1\n"},{0x26,0x10},2},
	{{"not0 2\n"},{0x27,0x20},2},
	{{"jmp 7 8\n"},{0x30,0x78},2},
	{{"jcn 1 2\n"},{0x31,0x12},2},
	{{"lod 5 6 7\n"},{0x65,0x67},2},
	{{"sav 6 7 8\n"},{0x76,0x78},2},
	{{"ovr 9\n"},{0xa9,0x00},2},
	{{"pnt a\n"},{0xba,0x00},2},
	{{"dsp c d\n"},{0xc0,0xcd},2},
	{{"inp c\n"},{0xdc,0x00},2},
	{{"hlt d\n"},{0xed,0x00},2},
	{{"snd e f\n"},{0xf0,0xef},2},
	{{"val 0 f0\nval 1 f2\nadd 0 1\nhlt 0\n"},{0x00,0xf0, 0x01,0xf2, 0x10,0x01, 0xe0,0x00},8}, // longer test
	{{"-label\n"},{},0}, // create a label
	{{"add 3 4\n-moo\nlabel -moo 0 1\n"},{0x10,0x34, 0x00,0x00,0x01,0x02}, 6}, // load label
	{{"label -moo 0 1\n-moo\n"},{0x00,0x00,0x01,0x04}, 4}, // label comes after usage
};

#define NUM_TOKENS (4)
#define MAX_TOKEN_LENGTH (16)
#define NUM_LABELS (64)

typedef struct
{
	unsigned char name[MAX_TOKEN_LENGTH];
	int pos;
} Label;
typedef struct
{
	Label labels[NUM_LABELS];
	int size;
} LabelStore;

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
	ARGS_LABEL=1,
	ARGS_D=2,
	ARGS_V=4,
	ARGS_A=8,
	ARGS_B=16,
	ARGS_AB=ARGS_A|ARGS_B,
	ARGS_INVALID=0,
} ArgsType;

typedef struct
{
	char instruction[MAX_TOKEN_LENGTH];
	unsigned char op;
	ArgsType type;	
} Op;

#define NUMBER_OF_OPS (36)
Op ops[NUMBER_OF_OPS] = {
	{"val", 0x00, ARGS_D | ARGS_V},
	{"add", 0x10, ARGS_AB},
	{"sub", 0x11, ARGS_AB},
	{"div", 0x12, ARGS_AB},
	{"mul", 0x13, ARGS_AB},
	{"inc", 0x14, ARGS_A},
	{"dec", 0x15, ARGS_A},
	{"and", 0x16, ARGS_AB},
	{"or", 0x17, ARGS_AB},
	{"xor", 0x18, ARGS_AB},
	{"rshift", 0x19, ARGS_AB},
	{"lshift", 0x1a, ARGS_AB},
	{"mod", 0x1b, ARGS_AB},
	{"copy", 0x1c, ARGS_AB},
	{"eq", 0x20, ARGS_AB},
	{"neq", 0x21, ARGS_AB},
	{"gt", 0x22, ARGS_AB},
	{"gte", 0x23, ARGS_AB},
	{"lt", 0x24, ARGS_AB},
	{"lte", 0x25, ARGS_AB},
	{"is0", 0x26, ARGS_A},
	{"not0", 0x27, ARGS_A},
	{"jmp", 0x30, ARGS_AB},
	{"jcn", 0x31, ARGS_AB},
	{"lod", 0x60, ARGS_D | ARGS_AB},
	{"sav", 0x70, ARGS_D | ARGS_AB},
	{"ovr", 0xa0, ARGS_D},
	{"pnt", 0xb0, ARGS_D},
	{"dsp", 0xc0, ARGS_AB},
	{"inp", 0xd0, ARGS_D},
	{"hlt", 0xe0, ARGS_D},
	{"snd", 0xf0, ARGS_AB},
	{"label", 0xff, ARGS_LABEL},
};

int find_label(unsigned char token[MAX_TOKEN_LENGTH], LabelStore* label_store)
{
	int i=0;
	for(i=0; i<label_store->size; i++)
	{
		if(strncmp((char*)token, (char*)label_store->labels[i].name, MAX_TOKEN_LENGTH)==0)
			return label_store->labels[i].pos;
	}
	return -1;
}

int create_label(unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH], int num_tokens, LabelStore* label_store, int pos, int line)
{
	if(num_tokens > 1)
	{
		LOG("Multi-token label on line %d", line);
		return 1;
	}
	if(label_store->size == NUM_LABELS)
	{
		LOG("Label store (size %d) is full on line %d", NUM_LABELS, line);
		return 1;
	}
	if(find_label(tokens[0], label_store) != -1)
	{
		LOG("Label on line %d already exists", line);
		return 1;
	}
	Label *label = &label_store->labels[label_store->size];
	memcpy(label->name, tokens[0], MAX_TOKEN_LENGTH);
	label->pos = pos;
	label_store->size++;
	return 0;
}

int create_request(unsigned char token[MAX_TOKEN_LENGTH], LabelStore* label_requests, int pos, int line)
{
	if(label_requests->size == NUM_LABELS)
	{
		LOG("Label requests (size %d) is full on line %d", NUM_LABELS, line);
		return 1;
	}
	Label *label = &label_requests->labels[label_requests->size];
	memcpy(label->name, token, MAX_TOKEN_LENGTH);
	label->pos = pos;
	label_requests->size++;
	return 0;
}

int create_instruction(unsigned char tokens[NUM_TOKENS][MAX_TOKEN_LENGTH], int num_tokens, LabelStore* label_requests, unsigned char *c, int* size, int line)
{
	(void)label_requests;
	*size = 2;
	*c = 0;
	*(c+1) = 0;
	ArgsType type = ARGS_INVALID;
	int i;
	for(i=0; i<NUMBER_OF_OPS; i++)
	{
		if(strncmp((char*)tokens[0], ops[i].instruction, MAX_TOKEN_LENGTH)==0)
		{
			*c = ops[i].op;
			type = ops[i].type;
		}
	}

	if(type == ARGS_INVALID)
	{
		LOG("Unsupported instruction on line %d", line);
		return 1;
	}
	int required_num_args = 0;
	if(type & ARGS_LABEL) required_num_args+=3;
	if(type & ARGS_D) required_num_args++;
	if(type & ARGS_V) required_num_args++;
	if(type & ARGS_A) required_num_args++;
	if(type & ARGS_B) required_num_args++;
	if(num_tokens-1 != required_num_args)
	{
		LOG("Expected %d args, but found %d on line %d", required_num_args, num_tokens-1, line);
		return 1;
	}
	int current_token=1;
	if(type & ARGS_LABEL)
	{
		if(create_request(tokens[current_token++], label_requests, (int)c, line))
			return 1;
		int a = get_arg(tokens[current_token++], 1, line);
		int b = get_arg(tokens[current_token++], 1, line);
		*c = a;
		*(c+1) = 0;
		*(c+2) = b;
		*(c+3) = 0;
		*size = 4;
	}
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
	if(type & ARGS_A)
	{
		int a = get_arg(tokens[current_token++], 1, line);
		if(a == -1) return 1;
		*(c+1) = (a<<4);
	}
	if(type & ARGS_B)
	{		
		int b = get_arg(tokens[current_token++], 1, line);
		if(b == -1) return 1;
		*(c+1) |= b;
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
	LabelStore label_store;
	LabelStore label_requests;
	memset(&label_store, 0, sizeof(label_store));
	memset(&label_requests, 0, sizeof(label_requests));
	for(in_off=0; in_off<TEST_PROGRAM_SIZE; in_off++)
	{
		unsigned char c = *(in+in_off);
		if(c == '\r')
			continue;
		if(c == 0)
		{
			if(token_num || token_pos)
			{
				LOG("Missing terminating newline on line %d", line);
				return 1;
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
			if(out_off == TEST_PROGRAM_SIZE-2)
			{
				LOG("Run out of output space.");
				return 1;
			}
			if(token_num || token_pos)
			{
				if(tokens[0][0] == '-')
				{
					int fail = create_label(tokens, token_num+1, &label_store, out_off, line);
					if(fail) return 1;
				}
				else
				{
					int size;
					int fail = create_instruction(tokens, token_num+1, &label_requests, &out[out_off], &size, line);
					if(fail) return 1;
					out_off += size;
				}
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
		if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')
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
	int i;
	for(i=0; i<label_requests.size; i++)
	{
		Label *request = &label_requests.labels[i];
		int pos = find_label(request->name, &label_store);
		if(pos == -1)
		{
			char buffer[MAX_TOKEN_LENGTH];
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, request->name, MAX_TOKEN_LENGTH-2);
			LOG("Unable to find label %s", buffer);
		}
		unsigned char* c = (unsigned char*)request->pos;
		*(c+1) = (pos&0xff00)>>8;
		*(c+3) = (pos&0xff);
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
int run_tests()
{
	QLOG( "Running Tests: " );
	int fail = 0;

	int i;
	for(i=0; i<NUM_TESTS&&!fail; i++)
		fail = run_test(i);
	if(fail)
		QLOG("\nSome tests failed!\n");
	else
		QLOG("\nAll tests passed :)\n");
	return fail;
}

int main( int arg, const char** argv)
{
	if(arg < 3)
	{
		QLOG("\nInsufficent arguments.");
		return 1;
	}
	if(strncmp(argv[1], "--test", 6) == 0)
	{
		set_logfile(argv[2]);
		return run_tests();
	}

	unsigned char input[TEST_PROGRAM_SIZE];
	memset(input, 0x00, sizeof(input));
	int size;
	bool success = file_load(argv[1], &size, input, sizeof(input));
	if(!success)
	{
		LOG("Failed to load source file.");
		return 1;
	}

	unsigned char output[TEST_PROGRAM_SIZE];
	int out_size;
	int fail = tokenize(input, output, &out_size);
	if(fail)
	{
		LOG("Failed to assemble source.");
		return 1;
	}

	success = file_save(argv[2], out_size, output);
	if(!success)
	{
		LOG("Failed to save output file.");
		return 1;
	}
	return 0;
}