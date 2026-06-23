#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define ENCODINGS_COUNT 45


typedef struct Instruction Inst;
typedef struct InstructionEncoding InstEncoding;

enum LineType {
	CODE,
	LABEL,
	DIRECTIVE,
	END
};

enum InstructionCondition {
	EQ = 0,
	NE = 1,
	CS = 2,
	CC = 3,
	MI = 4,
	PL = 5,
	VS = 6,
	VC = 7,
	HI = 8,
	LS = 9,
	GE = 10,
	LT = 11,
	GT = 12,
	LE = 13,
	AL = 14
};


enum Oprnd2Type {
	REG = 0,
	IMM = 1
};

struct Line {
	enum LineType type;
	unsigned long line_num;
	char *start;
};

struct InstructionEncoding {
	char *mnemonic;
	uint32_t (*encode)(Inst *instruction);
};

struct Instruction {
	bool s;
	uint8_t operands_length;
	enum Oprnd2Type oprnd2_type;
	enum InstructionCondition cond;
	char **operands;
	InstEncoding *encoding;
	char *suffix;
	struct Line *line;
};


char *getTokenEnd(char *start) {
	// TODO: implement
	return NULL;
}

char getLowerChar(char c) {
	if (c >= 'A' && c <= 'Z') {
		return (c - 'A') + 'a';
	}

	return c;
}

void encodingIssue(Inst *i) {
	printf("Issue on line %lu: ", i->line->line_num);
	for (char *c = i->line->start; *c != '\n'; c++) {
		printf("%c", *c);
	}
	printf("\n");
}

uint8_t readReg(char *reg, Inst *i) {
	// TODO: implement
	return 0;
}

uint32_t readConst(char *constant, Inst *i) {
	// TODO: implement
	return 0;
}

uint16_t imm12(uint32_t val, Inst *i) {
	uint8_t imm8 = 0;
	int r;
	for (r = 0; r < 32 && (uint32_t)imm8 << r != val; r++) {
		imm8 = (val >> r) & 0xFF;
	}

	if (r == 32) {
		printf("Cannot create rotation for 12-bit immediate; ");
		encodingIssue(i);
	}

	return (r << 8) | imm8;
}

uint32_t mov(Inst *i) {
	uint32_t encoding = 0;

	switch (i->operands_length){
		case 2: // mov{cond}{s} Rd, <Oprnd2>
			encoding = 0x1A00000 | (i->cond << 28) | (i->oprnd2_type | 20) | readReg(i->operands[0], i);

			encoding |= i->oprnd2_type ? imm12(readConst(i->operands[1], i), i) : readReg(i->operands[1], i);
			break;
		case 4:
			// fall through while bit-shifts are unimplemented
		default:
			encodingIssue(i);
			break;
	}

	return encoding;
}

uint32_t unsupportedInstruction(Inst *i) {
	printf("Unsupported instruction: ");
	encodingIssue(i);

	return 0;
}

InstEncoding *createEncodings() {
	InstEncoding *encodings = malloc(ENCODINGS_COUNT * sizeof(*encodings));

	encodings[0] = (InstEncoding){"adc", unsupportedInstruction};
	encodings[1] = (InstEncoding){"add", unsupportedInstruction};
	encodings[2] = (InstEncoding){"adr", unsupportedInstruction};
	encodings[3] = (InstEncoding){"addr", unsupportedInstruction};
	encodings[4] = (InstEncoding){"adrl", unsupportedInstruction};
	encodings[5] = (InstEncoding){"and", unsupportedInstruction};
	encodings[6] = (InstEncoding){"asr", unsupportedInstruction};
	encodings[7] = (InstEncoding){"b", unsupportedInstruction};
	encodings[8] = (InstEncoding){"bic", unsupportedInstruction};
	encodings[9] = (InstEncoding){"bl", unsupportedInstruction};
	encodings[10] = (InstEncoding){"bx", unsupportedInstruction};
	encodings[11] = (InstEncoding){"cmn", unsupportedInstruction};
	encodings[12] = (InstEncoding){"cmp", unsupportedInstruction};
	encodings[13] = (InstEncoding){"eor", unsupportedInstruction};
	encodings[14] = (InstEncoding){"ldm", unsupportedInstruction};
	encodings[15] = (InstEncoding){"ldr", unsupportedInstruction};
	encodings[16] = (InstEncoding){"lsl", unsupportedInstruction};
	encodings[17] = (InstEncoding){"mla", unsupportedInstruction};
	encodings[18] = (InstEncoding){"mov", mov};
	encodings[19] = (InstEncoding){"mrs", unsupportedInstruction};
	encodings[20] = (InstEncoding){"msr", unsupportedInstruction};
	encodings[21] = (InstEncoding){"mul", unsupportedInstruction};
	encodings[22] = (InstEncoding){"mvn", unsupportedInstruction};
	encodings[23] = (InstEncoding){"neg", unsupportedInstruction};
	encodings[24] = (InstEncoding){"nop", unsupportedInstruction};
	encodings[25] = (InstEncoding){"orr", unsupportedInstruction};
	encodings[26] = (InstEncoding){"pop", unsupportedInstruction};
	encodings[27] = (InstEncoding){"push", unsupportedInstruction};
	encodings[28] = (InstEncoding){"ror", unsupportedInstruction};
	encodings[29] = (InstEncoding){"rrx", unsupportedInstruction};
	encodings[30] = (InstEncoding){"rsb", unsupportedInstruction};
	encodings[31] = (InstEncoding){"rsc", unsupportedInstruction};
	encodings[32] = (InstEncoding){"sbc", unsupportedInstruction};
	encodings[33] = (InstEncoding){"smlal", unsupportedInstruction};
	encodings[34] = (InstEncoding){"smull", unsupportedInstruction};
	encodings[35] = (InstEncoding){"stm", unsupportedInstruction};
	encodings[36] = (InstEncoding){"str", unsupportedInstruction};
	encodings[37] = (InstEncoding){"sub", unsupportedInstruction};
	encodings[38] = (InstEncoding){"swi", unsupportedInstruction};
	encodings[39] = (InstEncoding){"swp", unsupportedInstruction};
	encodings[40] = (InstEncoding){"teq", unsupportedInstruction};
	encodings[41] = (InstEncoding){"tst", unsupportedInstruction};
	encodings[42] = (InstEncoding){"und", unsupportedInstruction};
	encodings[43] = (InstEncoding){"umlal", unsupportedInstruction};
	encodings[44] = (InstEncoding){"umull", unsupportedInstruction};

	return encodings;
}


int main(int argc, char **argv) {
	FILE *asm_file; // maybe for when we implement including other asm files in a file we should have an array of those files.
	FILE *gba_file;

	if (argc < 2) {
		printf("Please specify input file.\nUsage: %s <input asm file> [output gba file]\n", argv[0]);

		return -1;
	}

	asm_file = fopen(argv[1], "r");

	if (asm_file == NULL) {
		fprintf(stderr, "ERROR: Error opening %s\n", argv[1]);

		return -101;
	}


	fseek(asm_file, 0, SEEK_END);
	long asm_size = ftell(asm_file);
	rewind(asm_file);

	char *asm_buffer = malloc(asm_size + 1);
	fread(asm_buffer, 1, asm_size, asm_file);
	asm_buffer[asm_size] = '\n';

	fclose(asm_file);

	unsigned long lines_arr_size = 256;
	struct Line *lines = malloc(lines_arr_size * sizeof(*lines));

	unsigned long line_num = 0;
	unsigned long file_line_num = 1;
	lines[line_num] = (struct Line){CODE, file_line_num, asm_buffer};
	bool comment = false;
	char *asm_buffer_end = asm_buffer + asm_size;
	for (char *c = asm_buffer; c <= asm_buffer_end; c++) {
		if (*c == '\n') {
			comment = false;
			file_line_num++;

			if (!(c != asm_buffer_end && (*(c + 1) == '\n' || *(c + 1) == ';'))) {
				if ((line_num++) + 1 == lines_arr_size) {
					lines_arr_size *= 2;
					lines = realloc(lines, lines_arr_size * sizeof(*lines));
				}

				if (c != asm_buffer_end) {
					lines[line_num] = (struct Line){CODE, file_line_num, c + 1};
					if (*(c + 1) == '@') {
						lines[line_num].type = DIRECTIVE;
					}
				}
			}
		}

		if (!comment) {
			switch (*c) {
				case ':':
					lines[line_num].type = LABEL;
					break;
				case ';':
					comment = true;
					break;
				default:
					*c = getLowerChar(*c);
					break;
			}
		}
	}
	lines[line_num] = (struct Line){END, file_line_num, NULL};


	InstEncoding *encodings = createEncodings();

	printf("CODE: %d, LABEL: %d, DIRECTIVE: %d, END: %d\n---\nLine:\tType:\n", CODE, LABEL, DIRECTIVE, END);
	for (int i = 0; i < line_num; i++) {
		printf("%lu:\t%d:\t", lines[i].line_num, lines[i].type);

		for (char *c = lines[i].start; *c != '\n' && *c != ';'; c++) {
			// TODO: start processing lines: need to process directives, calculate label offsets, create instruction encodings for each instruction line, ... more most certainly
			printf("%c", *c);
		}

		printf("\n");
	}


	if (argc > 2) {
		gba_file = fopen(argv[2], "w");

		if (gba_file == NULL) {
			fprintf(stderr, "ERROR: Error creating or opening %s\n", argv[2]);

			fclose(asm_file);

			return -102;
		}
	} else {
		gba_file = fopen("a.gba", "w");

		if (gba_file == NULL) {
			fprintf(stderr, "ERROR: Error creating or opening output GBA file\n");

			fclose(asm_file);

			return -103;
		}
	}

	fclose(gba_file);

	free(encodings);
	free(lines);
	free(asm_buffer);

	return 0;
}
