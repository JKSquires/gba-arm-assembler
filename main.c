#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define ENCODINGS_COUNT 45
#define MAX_INSTRUCTION_BLOCKS 4 // FIXME: 4 is the max number of blocks a valid instruction can have excluding in multiple register operands/blocks (e.g. {r0, r3-r5, r10}). We need to figure out how we want to deal with those.


typedef struct Instruction Inst;
typedef struct InstructionEncoding InstEncoding;

enum LineType {
	DIR_B = 1,
	DIR_H = 2,
	DIR_W = 4,
	CODE,
	LABEL,
	END,
	DIR_UNK,
	DIR_A,
	DIR_I,
	DIR_T,
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
	AL = 14,
};

enum InstructionSuffix {
	COND_ONLY,
	SET_FLAGS,
	DATA_SIZE,
	ADDRESSING_MODE,
	NOP,
};

// maybe for operands, we can create a struct that stores type (register, immediate, label reference, memory operand, bit-shift, etc.) and data (union the different ones?)
//enum Oprnd2Type {
//	REG = 0,
//	IMM = 1
//};

enum BlockType {
	UNKNOWN_BLOCK,
	FIRST,
	REG,
	IMM,
	LBL,
	SHIFT_REG,
	SHIFT_IMM,
	MEM_REG,
	MEM_IMM,
	MEM_LBL,
	MEM_SHIFT_REG,
	MEM_SHIFT_IMM,
	MUL_REG,
};

enum OperandState {
	REGULAR,
	MEMORY,
	MULTIPLE,
};

enum NumType {
	BIN,
	DEC,
	HEX,
};


struct Line {
	char *start; // TODO: might be nice to get rid of this
	void *data;
	unsigned long line_num;
	enum LineType type;
};

struct Label {
	char *start;
	uint32_t offset;
	unsigned int length;
};

struct InstructionBlock {
	char *start;
	enum BlockType type;
};

struct InstructionEncoding {
	uint32_t (*encode)(uint32_t inst_offset, char *oprnd1_start, struct Label *labels, unsigned long label_tot, Inst *i);
	char *mnemonic;
	uint8_t mnemonic_length;
	enum InstructionSuffix suffix;
};

/*
struct Instruction {
	struct Line *line;
	char *suffix;
	InstEncoding *encoding;
	char **operands;
	enum InstructionCondition cond;
	enum Oprnd2Type oprnd2_type;
	uint8_t operands_length;
	bool s;
};
*/
struct Instruction {
	struct Line *line; // TODO: refactor to better whay than a loop like this where lines have instructions which store the line etc...
	struct InstructionBlock *blocks;
	uint8_t block_count;
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

char *skipWhitespace(char *c) {
	for (; *c == ' ' || *c == '\t'; c++);

	return c;
}

char *skipEmptyLines(char *c, char *buffer_end, unsigned long *file_line_num) {
	if (c < buffer_end) {
		char *next_start = skipWhitespace(c + (*c == '\n'));
		while (*next_start == '\n' || *next_start == ';') {
			//printf("\nLine %lu found empty\n", *file_line_num);
			if (next_start >= buffer_end) {
				c = next_start;
				break;
			} else {
				for (c = next_start; *c != '\n'; c++);

				(*file_line_num)++;

				next_start = skipWhitespace(c + 1);
			}
		}
	}

	return c;
}

unsigned long findLabel(char *inst_label, unsigned int inst_label_length, struct Label *labels, unsigned long label_tot) {
	unsigned long labels_i = 0;
	for (; labels_i < label_tot; labels_i++) {
		unsigned int label_length = 0;
		for (; labels[labels_i].start[label_length] != ':'; label_length++);

		bool matches = true;
		if (inst_label_length != label_length) {
			matches = false;
		} else {
			for (int i = 0; i < inst_label_length; i++) {
				if (inst_label[i] != labels[labels_i].start[i])
					matches = false;
			}
		}

		if (matches) break;
	}

	return labels_i; // error when labels_i == label_tot
}

void lineIssue(struct Line *line) {
	printf("Issue on line %lu:\n", line->line_num);
	for (char *c = line->start; *c != '\n'; c++) {
		printf("%c", *c);
	}
	printf("\n");
}

void encodingIssue(Inst *i) { // FIXME: do we really need this anymore, maybe just refactor calls
	lineIssue(i->line);
}

uint8_t readReg(char *reg, Inst *i) {
	uint8_t data = 0;

	if (*reg != 'r') {
		printf("Issue reading register: ");
		encodingIssue(i);

		return 1 << 7; // error when &(1<<7)
	}

	char *c = reg + 1;
	for (; *c >= '0' && *c <= '9'; c++) {
		data = data * 10 + (*c - '0');
	}
	data &= 0xF;

	if (*c == '!') {
		data |= 1 << 4; // writeback register when &(1<<4)
		c++;
	}

	return data; // register num is &0xF
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
		printf("Cannot create rotation for 12-bit immediate: ");
		encodingIssue(i);
	}

	return (r << 8) | imm8;
}

uint32_t imm24(uint32_t s, uint32_t d, Inst *i) {
	int32_t val = (d - (s + 8));
	val /= 4;
	uint32_t u_val = (uint32_t)val;

	if (u_val > 0x00FFFFFF && u_val < 0xFF7FFFFF) {
		printf("%lu to %lu makes 0x%X (%ld)\n", (unsigned long)s, (unsigned long)d, u_val, (long)val);
		printf("Issue creating 24-bit immediate: ");
		encodingIssue(i);
	}

	return u_val & 0x00FFFFFF;
}

uint32_t b(uint32_t inst_offset, char *oprnd1_start, struct Label *labels, unsigned long label_tot, Inst *i) {
	uint32_t encoding = 0x0A000000;

	unsigned int inst_label_length = 0;
	for (; oprnd1_start[inst_label_length] >= 'a' && oprnd1_start[inst_label_length] <= 'z' || oprnd1_start[inst_label_length] >= '0' && oprnd1_start[inst_label_length] <= '9'; inst_label_length++);

	unsigned long label_i = findLabel(oprnd1_start, inst_label_length, labels, label_tot);
	if (label_i == label_tot) { // TODO: maybe someday we can support branching to a direct address (e.g. b $8000000)
		printf("Cannot find label for branch: ");
		encodingIssue(i);

		return 0;
	}

	uint32_t label_offset = labels[label_i].offset;

	encoding |= imm24(inst_offset, label_offset, i);

	return encoding;
}

uint32_t bl(uint32_t inst_offset, char *oprnd1_start, struct Label *labels, unsigned long label_tot, Inst *i) {
	return b(inst_offset, oprnd1_start, labels, label_tot, i) | (1 << 24);
}

uint32_t bx(uint32_t inst_offset, char *oprnd1_start, struct Label *labels, unsigned long label_tot, Inst *i) {
	uint32_t encoding = 0x012FFF10;

	uint8_t reg_data = readReg(oprnd1_start, i);

	if (reg_data & (1 << 7)) {
		return 0;
	}
	if (reg_data & (1 << 4)) {
		printf("Branch and Exchange does not support writeback: ");
		encodingIssue(i);
	}

	encoding |= reg_data & 0xF;

	return encoding;
}

/* will likely need rework, it was fun to mess around earlier though
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
*/

uint32_t unsupportedInstruction(uint32_t inst_offset, char *oprnd1_start, struct Label *labels, unsigned long label_tot, Inst *i) {
	printf("Unsupported instruction: ");
	encodingIssue(i);

	return 0;
}

InstEncoding *createEncodings() {
	InstEncoding *encodings = malloc(ENCODINGS_COUNT * sizeof *encodings);

	// maybe instead of one function for each, find patterns like (Rd, Rn, <Oprnd2>), (imm24), (Rt, [Rn, +/- Rm{, <shift>}]{!}), etc. and make functions for those?
	encodings[0] =	(InstEncoding){unsupportedInstruction,	"adc",		3,	SET_FLAGS};
	encodings[1] =	(InstEncoding){unsupportedInstruction,	"add",		3,	SET_FLAGS};
	encodings[2] =	(InstEncoding){unsupportedInstruction,	"addr",		4,	COND_ONLY};
	encodings[3] =	(InstEncoding){unsupportedInstruction,	"adr",		3,	COND_ONLY};
	encodings[4] =	(InstEncoding){unsupportedInstruction,	"adrl",		4,	COND_ONLY};
	encodings[5] =	(InstEncoding){unsupportedInstruction,	"and",		3,	SET_FLAGS};
	encodings[6] =	(InstEncoding){unsupportedInstruction,	"asr",		3,	COND_ONLY};
	encodings[7] =	(InstEncoding){b,						"b",		1,	COND_ONLY};
	encodings[8] =	(InstEncoding){unsupportedInstruction,	"bic",		3,	SET_FLAGS};
	encodings[9] =	(InstEncoding){bl,						"bl",		2,	COND_ONLY};
	encodings[10] =	(InstEncoding){bx,						"bx",		2,	COND_ONLY};
	encodings[11] =	(InstEncoding){unsupportedInstruction,	"cmn",		3,	COND_ONLY};
	encodings[12] =	(InstEncoding){unsupportedInstruction,	"cmp",		3,	COND_ONLY};
	encodings[13] =	(InstEncoding){unsupportedInstruction,	"eor",		3,	SET_FLAGS};
	encodings[14] =	(InstEncoding){unsupportedInstruction,	"ldm",		3,	ADDRESSING_MODE};
	encodings[15] =	(InstEncoding){unsupportedInstruction,	"ldr",		3,	DATA_SIZE};
	encodings[16] =	(InstEncoding){unsupportedInstruction,	"lsl",		3,	COND_ONLY};
	encodings[17] =	(InstEncoding){unsupportedInstruction,	"mla",		3,	SET_FLAGS};
	encodings[18] =	(InstEncoding){unsupportedInstruction,	"mov",		3,	SET_FLAGS};
	encodings[19] =	(InstEncoding){unsupportedInstruction,	"mrs",		3,	COND_ONLY};
	encodings[20] =	(InstEncoding){unsupportedInstruction,	"msr",		3,	COND_ONLY};
	encodings[21] =	(InstEncoding){unsupportedInstruction,	"mul",		3,	SET_FLAGS};
	encodings[22] =	(InstEncoding){unsupportedInstruction,	"mvn",		3,	SET_FLAGS};
	encodings[23] =	(InstEncoding){unsupportedInstruction,	"neg",		3,	COND_ONLY};
	encodings[24] =	(InstEncoding){unsupportedInstruction,	"nop",		3,	NOP};
	encodings[25] =	(InstEncoding){unsupportedInstruction,	"orr",		3,	SET_FLAGS};
	encodings[26] =	(InstEncoding){unsupportedInstruction,	"pop",		3,	COND_ONLY};
	encodings[27] =	(InstEncoding){unsupportedInstruction,	"push",		4,	COND_ONLY};
	encodings[28] =	(InstEncoding){unsupportedInstruction,	"ror",		3,	COND_ONLY};
	encodings[29] =	(InstEncoding){unsupportedInstruction,	"rrx",		3,	COND_ONLY};
	encodings[30] =	(InstEncoding){unsupportedInstruction,	"rsb",		3,	SET_FLAGS};
	encodings[31] =	(InstEncoding){unsupportedInstruction,	"rsc",		3,	SET_FLAGS};
	encodings[32] =	(InstEncoding){unsupportedInstruction,	"sbc",		3,	SET_FLAGS};
	encodings[33] =	(InstEncoding){unsupportedInstruction,	"smlal",	5,	SET_FLAGS};
	encodings[34] =	(InstEncoding){unsupportedInstruction,	"smull",	5,	SET_FLAGS};
	encodings[35] =	(InstEncoding){unsupportedInstruction,	"stm",		3,	ADDRESSING_MODE};
	encodings[36] =	(InstEncoding){unsupportedInstruction,	"str",		3,	DATA_SIZE};
	encodings[37] =	(InstEncoding){unsupportedInstruction,	"sub",		3,	SET_FLAGS};
	encodings[38] =	(InstEncoding){unsupportedInstruction,	"swi",		3,	COND_ONLY};
	encodings[39] =	(InstEncoding){unsupportedInstruction,	"swp",		3,	COND_ONLY};
	encodings[40] =	(InstEncoding){unsupportedInstruction,	"teq",		3,	COND_ONLY};
	encodings[41] =	(InstEncoding){unsupportedInstruction,	"tst",		3,	COND_ONLY};
	encodings[42] =	(InstEncoding){unsupportedInstruction,	"und",		3,	COND_ONLY};
	encodings[43] =	(InstEncoding){unsupportedInstruction,	"umlal",	5,	SET_FLAGS};
	encodings[44] =	(InstEncoding){unsupportedInstruction,	"umull",	5,	SET_FLAGS};

	return encodings;
}

enum InstructionCondition getInstCond(char *cond_start) {
	char cond[2];
	cond[0] = getLowerChar(cond_start[0]);
	if (cond[0] < 'a' || cond[0] > 'z') return AL;
	cond[1] = getLowerChar(cond_start[1]);
	if (cond[1] < 'a' || cond[1] > 'z') return AL;


	if (cond[0] == 'e' && cond[1] == 'q') return EQ;
	else if (cond[0] == 'n' && cond[1] == 'e') return NE;
	else if (cond[0] == 'c' && cond[1] == 's') return CS;
	else if (cond[0] == 'c' && cond[1] == 'c') return CC;
	else if (cond[0] == 'm' && cond[1] == 'i') return MI;
	else if (cond[0] == 'p' && cond[1] == 'l') return PL;
	else if (cond[0] == 'v' && cond[1] == 's') return VS;
	else if (cond[0] == 'v' && cond[1] == 'c') return VC;
	else if (cond[0] == 'h' && cond[1] == 'i') return HI;
	else if (cond[0] == 'l' && cond[1] == 's') return LS;
	else if (cond[0] == 'g' && cond[1] == 'e') return GE;
	else if (cond[0] == 'l' && cond[1] == 't') return LT;
	else if (cond[0] == 'g' && cond[1] == 't') return GT;
	else if (cond[0] == 'l' && cond[1] == 'e') return LE;
	else return AL;
}


// FIXME: should probably not store so much on the stack...
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
	struct Line *lines = malloc(lines_arr_size * sizeof *lines);

	unsigned long line_num = 0;
	unsigned long file_line_num = 1;

	uint32_t track_rom_size = 0; // FIXME: right now, this relies on the first line being code or a directive. Need to account for empty start line or commented line. Maybe we can create a line type "SKIP" as a quick fix, but there should be a better way
	unsigned long labels_arr_size = 64;
	struct Label *labels = malloc(labels_arr_size * sizeof *labels); // FIXME: should absolutely use a hash table at some point instead
	unsigned long label_tot = 0;

	bool comment = false;
	enum OperandState track_operand_state = REGULAR;
	int count_blocks = 1;
	struct InstructionBlock blocks[MAX_INSTRUCTION_BLOCKS];

	char *asm_buffer_end = asm_buffer + asm_size;
	char *asm_buffer_start = skipEmptyLines(asm_buffer, asm_buffer_end, &file_line_num);
	if ((*asm_buffer_start == '\n' || *asm_buffer_start == ';') && asm_buffer_start < asm_buffer_end) {
		asm_buffer_start = skipWhitespace(asm_buffer_start + 1);
	}

	lines[0] = (struct Line){asm_buffer_start, NULL, file_line_num, CODE};
	blocks[0] = (struct InstructionBlock){asm_buffer_start, FIRST};

	for (char *c = asm_buffer_start; c <= asm_buffer_end; c++) {
		if (*c == '\n') {
			comment = false;
			if (track_operand_state != REGULAR) {
				printf("Memory/multiple register operand not closed properly: ");
				lineIssue(&lines[line_num]);
				track_operand_state = REGULAR;
			}
			file_line_num++;

			if (lines[line_num].type == CODE) {
				Inst *inst = malloc(sizeof *inst);
				inst->line = &(lines[line_num]);
				inst->blocks = malloc(count_blocks * sizeof (struct InstructionBlock));
				for (int i = 0; i < count_blocks; i++) {
					inst->blocks[i] = blocks[i];
				}
				inst->block_count = count_blocks;

				lines[line_num].data = (void *)inst;

				track_rom_size += 4;
				//printf("\nROM size: %lu bytes\n", (unsigned long)track_rom_size);
			}

			c = skipEmptyLines(c, asm_buffer_end, &file_line_num);

			if (c != asm_buffer_end) {
				if (++line_num + 1 == lines_arr_size) {
					lines_arr_size *= 2;
					lines = realloc(lines, lines_arr_size * sizeof *lines);
				}

				c = skipWhitespace(c + 1) - 1;
				lines[line_num] = (struct Line){c + 1, NULL, file_line_num, CODE};
				//printf("\nWrite line %lu starting with '%c'\n", file_line_num, *(c + 1));

				blocks[0] = (struct InstructionBlock){c + 1, FIRST};
				count_blocks = 1;
			} else {
				break;
			}
		}

		if (!comment) {
			//printf("<%c>", *c); // TODO: remove debug line

			switch (*c) {
				case '@':
					switch (getLowerChar(*(c + 1))) {
						case 'b': // fall through
						case 'h': // fall through
						case 'w':
							if (*(c + 2) == ' ') {
								char k = getLowerChar(*(c + 1));
								lines[line_num].type = k == 'b' ? DIR_B :
														k == 'h' ? DIR_H :
																	DIR_W;

								for (c += 2; *c != '\n'; c++) {
									if (*c == '$' || *c == '#' || *c == '%') {
										track_rom_size += lines[line_num].type;
									}
								}
								c--;
							}
							break;
						case 'a':
							if (*(c + 2) == ' ') {
								printf("\nAlign bytes directive is unsupported right now: "); // TODO: Implement; maybe something like `@a <num; e.g. 4 or 2>`
								lineIssue(&lines[line_num]);
								lines[line_num].type = DIR_A;

								c += 0; // TODO: count
								// TODO: calculate what is needed to align.
							}
							break;
						case 'i':
							if (*(c + 2) == ' ') {
								printf("\nInclude directive is unsupported right now: "); // TODO: Implement; maybe something like `@i "<file name>"`
								lineIssue(&lines[line_num]);
								lines[line_num].type = DIR_I;

								c += 4;
							}
							break;
						case 't':
							if (*(c + 2) == ' ') {
								printf("\nText directive is unsupported right now: "); // TODO: Implement; maybe something like `@t "<text>"`
								lineIssue(&lines[line_num]);
								lines[line_num].type = DIR_T;

								c += 0; // TODO: count characters for c and track_rom_size
							}
							break;
						default:
							lines[line_num].type = DIR_UNK;
							break;
					}

					continue;
				case ';':
					comment = true;
					break;
				case ':':
					lines[line_num].type = LABEL;

					unsigned int label_length = c - lines[line_num].start;

					unsigned long label_i = findLabel(lines[line_num].start, label_length, labels, label_tot);

					if (label_i == label_tot) {
						if (label_tot == labels_arr_size) {
							labels_arr_size *= 2;
							labels = realloc(labels, labels_arr_size * sizeof *labels);
						}

						labels[label_tot] = (struct Label){lines[line_num].start, track_rom_size, label_length};
						label_tot++;
					} else {
						printf("Duplicate label: ");
						lineIssue(&lines[line_num]);
					}

					break;
				case ',':
					count_blocks++;

					if (count_blocks <= MAX_INSTRUCTION_BLOCKS) {
						c = skipWhitespace(c + 1) - 1;
						blocks[count_blocks - 1] = (struct InstructionBlock){c + 1, track_operand_state != MULTIPLE ? UNKNOWN_BLOCK : MUL_REG};
					} else {
						printf("Too many operands: ");
						lineIssue(&lines[line_num]);
					}

					break;
				case '[':
					if (track_operand_state != REGULAR) {
						printf("Broken memory operand: ");
						lineIssue(&lines[line_num]);
					}

					track_operand_state = MEMORY;

					break;
				case ']':
					if (track_operand_state != MEMORY) {
						printf("Broken memory operand: ");
						lineIssue(&lines[line_num]);
					}

					track_operand_state = REGULAR;

					break;
				case '{':
					if (blocks[count_blocks - 1].type != UNKNOWN_BLOCK) {
						printf("Broken multiple register operand: ");
						lineIssue(&lines[line_num]);
					}

					track_operand_state = MULTIPLE;
					blocks[count_blocks - 1].type = MUL_REG;

					break;
				case '}':
					if (blocks[count_blocks - 1].type != MUL_REG) {
						printf("Broken multiple register operand: ");
						lineIssue(&lines[line_num]);
					}

					track_operand_state = REGULAR;

					break;
				default:
					*c = getLowerChar(*c); // TODO: the goal here is to have every character lowercase so that we don't have to do getLowerChar on characters again in the future, so we need to check to make sure this is setting all to lowercase (except things like labels and label calls unless we want to make it so that something like "ABC" == "abc"), and then we should remove all getLowerChar in later code

					enum BlockType block_type = blocks[count_blocks - 1].type;
					if (block_type == UNKNOWN_BLOCK) {
						switch (*c) {
							case '$': // fall through
							case '#': // fall through
							case '%':
								blocks[count_blocks - 1].type = track_operand_state == REGULAR ? IMM : MEM_IMM;
								break;
							case 'r':
								if (*(c + 1) >= '0' && *(c + 1) <= '9') {
									blocks[count_blocks - 1].type = track_operand_state == REGULAR ? REG : MEM_REG;
								}
								break;
							default:
								blocks[count_blocks - 1].type = LBL;

								char c3[] = {0, 0, 0};
								c3[0] = getLowerChar(*c);
								if (c3[0] != '\n') c3[1] = getLowerChar(*(c + 1));
								if (c3[1] != '\n') c3[2] = getLowerChar(*(c + 2));

								if (c3[0] == 'l' && c3[1] == 's' && c3[2] == 'l'
									|| c3[0] == 'l' && c3[1] == 's' && c3[2] == 'r'
									|| c3[0] == 'a' && c3[1] == 's' && c3[2] == 'r') {

									if (c3[0] == 'a' && c3[1] == 's' && c3[2] == 'l') { // handle arithmetic left shift
										*c = 'l';
									}

									char *skip_shift_whitespace = skipWhitespace(c + 3);
									if (skip_shift_whitespace == c + 3) break;

									if (*skip_shift_whitespace == '$' || *skip_shift_whitespace == '#' || *skip_shift_whitespace == '%') {
										blocks[count_blocks - 1].type = track_operand_state == REGULAR ? SHIFT_IMM : MEM_SHIFT_IMM;
									} else {
										blocks[count_blocks - 1].type = track_operand_state == REGULAR ? SHIFT_REG : MEM_SHIFT_REG;
									}
								}

								break;
						}
					}

					break;
			}
		}
	}
	//lines[line_num] = (struct Line){NULL, NULL, file_line_num, END};


	InstEncoding *encodings = createEncodings();
	unsigned char *rom = calloc(track_rom_size, 1);
	uint32_t rom_offset = 0;

	printf("DIR_B = %d, DIR_H = %d, DIR_W = %d, CODE = %d, LABEL = %d, END = %d, DIR_UNK = %d, DIR_A = %d, DIR_I = %d, DIR_T = %d\n", DIR_B, DIR_H, DIR_W, CODE, LABEL, END, DIR_UNK, DIR_A, DIR_I, DIR_T);
	printf("UNKNOWN_BLOCK = %d, FIRST = %d, REG = %d, IMM = %d, LBL = %d, SHIFT_REG = %d, SHIFT_IMM = %d, MEM_REG = %d, MEM_IMM = %d, MEM_LBL = %d, MEM_SHIFT_REG = %d, MEM_SHIFT_IMM = %d, MUL_REG = %d\n", UNKNOWN_BLOCK, FIRST, REG, IMM, LBL, SHIFT_REG, SHIFT_IMM, MEM_REG, MEM_IMM, MEM_LBL, MEM_SHIFT_REG, MEM_SHIFT_IMM, MUL_REG);
	printf("---\nLine:\tType:\n");

	for (int i = 0; i <= line_num; i++) {
		printf("\n%lu:\t%d:\t", lines[i].line_num, lines[i].type);

		switch (lines[i].type) {
			case CODE:
				Inst *inst = (Inst *)(lines[i].data);

				printf("\t\t");
				for (int bi = 0; bi < inst->block_count; bi++) {
					if (bi != 0) {
						printf("\t,\t");
					}
					printf("(%d)", inst->blocks[bi].type);
					for (char *c = inst->blocks[bi].start; *c != ',' && *c != '\n' && *c != ';'; c++) {
						printf("%c", *c);
					}
				}
				printf("\n");

				uint32_t encoding = 0x00000000; // TODO: encode CODE lines and write into the ROM buffer

				if (inst->block_count >= 1) {
					char *mnemonic_start = inst->blocks[0].start;
					int encode_i = 0;
					int last_encode_success = -1;
					int mi = 0;
					for (; encode_i < ENCODINGS_COUNT && mnemonic_start[mi] >= 'a' && mnemonic_start[mi] <= 'z'; mi++) {
						while (encode_i < ENCODINGS_COUNT && encodings[encode_i].mnemonic[mi] != getLowerChar(mnemonic_start[mi])) {
							encode_i++;

							while (encode_i < ENCODINGS_COUNT && mi >= encodings[encode_i].mnemonic_length)
								encode_i++;
						}

						if (encode_i < ENCODINGS_COUNT) {

							bool correct_prev = true;
							for (int check_c = 0; check_c <= mi && check_c < encodings[encode_i].mnemonic_length; check_c++) {
								if (encodings[encode_i].mnemonic[check_c] != getLowerChar(mnemonic_start[check_c]))
									correct_prev = false;
							}

							if (correct_prev) {
								last_encode_success = encode_i;
							}
						}
					}

					bool starts_with_last_success = true;
					if (encode_i == ENCODINGS_COUNT || mi != encodings[encode_i].mnemonic_length) {
						if (last_encode_success != -1 && mi > encodings[last_encode_success].mnemonic_length) {
							for (int check_c = 0; check_c < encodings[last_encode_success].mnemonic_length; check_c++) {
								if (encodings[last_encode_success].mnemonic[check_c] != getLowerChar(mnemonic_start[check_c]))
									starts_with_last_success = false;
							}
						} else {
							starts_with_last_success = false;
						}

						if (starts_with_last_success) {
							printf("Last encoding success at #%d\n", last_encode_success);
						} else {
							printf("Unknown instruction: ");
							lineIssue(&lines[i]);
						}
					}

					if (starts_with_last_success && encodings[last_encode_success].suffix != NOP) {
						InstEncoding *success_encoding;

						// TODO: make sure all instructions read properly, like `blt`, used to read as `bl` + `t`, not `b` + `lt`
						if (last_encode_success == 9 && mi == 3) { // make sure bl isn't actually something like blt
							printf("BL should be B\n");
							success_encoding = &(encodings[7]);
						} else {
							success_encoding = &(encodings[last_encode_success]);
						}

						printf("Probable mnemonic #%d (%s)\n", last_encode_success, success_encoding->mnemonic);

						// TODO: encode as much of the mnemonic as we can at this stage, later, we'll read the rest of the instruction to determine other values we can OR in.
						char *oprnd1_start = mnemonic_start + mi;
						for (; *oprnd1_start != ' ' && *oprnd1_start != '\t' && *oprnd1_start != '\n' && *oprnd1_start != ';'; *oprnd1_start++);
						oprnd1_start = skipWhitespace(oprnd1_start);
						encoding = success_encoding->encode(rom_offset, oprnd1_start, labels, label_tot, inst);

						enum InstructionCondition cond = mi == success_encoding->mnemonic_length ? AL : getInstCond(mnemonic_start + success_encoding->mnemonic_length); // TODO: when AL comes through, need to check to make sure it is proper.
						printf("Probable instruction condition: 0x%x\n", cond);

						encoding |= cond << 28;

						//printf("MI=%d->'%c'", mi, mnemonic_start[mi]);
						for (; mnemonic_start[mi] != ' ' && mnemonic_start[mi] != '\t' && mnemonic_start[mi] != '\n'; mi++); // TODO: do we really need this, we kind of check this earlier--review
						if (mnemonic_start + mi > asm_buffer_start + 2) {
							char last_two[] = {getLowerChar(mnemonic_start[mi - 2]), getLowerChar(mnemonic_start[mi - 1])};

							switch (success_encoding->suffix) {
								case SET_FLAGS:
									printf("Instruction detected to set flags\n");

									encoding |= (last_two[1] == 's') << 20;

									break;
								case DATA_SIZE: // FIXME: when encoding ldr and sdr in previous section, note that ldr has bit 20 set and str has bit 20 cleared. this means that we should probably not mask that bit in this section--review
									// TODO: bits 21, 23, and 24 will need to be set later accordingly
									printf("Instruction detected with data size ");

									switch (last_two[1]) {
										case 'b':
											if (last_two[0] == 's') { // ___sb
												printf("sb");

												encoding = (encoding & 0xF1FFFF9F) | (0xD << 4);
											} else { // ___b
												printf("b");

												encoding = (encoding & 0xF7FFFFFF) | (1 << 26) | (1 << 22);
											}
											break;
										case 't':
											if (last_two[0] == 'b') { // ___bt
												printf("bt");

												encoding = (encoding & 0xF6FFFFFF) | (1 << 26) | (0x3 << 21);
											} else { // ___t
												printf("t");

												encoding = (encoding & 0xF6BFFFFF) | (1 << 26) | (1 << 21);
											}
											break;
										case 'h':
											encoding &= encoding & 0xF1FFFF9F;
											if (last_two[0] == 's') { // ___sh
												printf("sh");

												encoding |= 0xF << 4;
											} else { // ___h
												printf("h");

												encoding |= 0xB << 4;
											}
											break;
									}

									printf("\n");

									break;
								case ADDRESSING_MODE: // FIXME: when encoding ldm and sdm in previous section, note that ldm has bit 20 set and stm has bit 20 cleared. this means that we should probably not mask that bit in this section--review
									// TODO: bit 21 is wback, will need to set later if needed for the specific instruction based on operands
									encoding = (encoding & 0xF02FFFFF) | (1 << 27);

									switch (last_two[0]) {
										case 'd':
											if (last_two[1] == 'b') {
												encoding |= 1 << 24;
											}
											break;
										case 'i':
											switch (last_two[1]) {
												case 'a': // TODO: ldmia = ldm and stmia = stm; might want to just remove this
													encoding |= 1 << 23;
													break;
												case 'b':
													encoding |= 0x3 << 23;
													break;
											}
											break;
										default:
											if (last_encode_success == 14) { // ldm__
												switch (last_two[0]) {
													case 'e':
														switch (last_two[1]) { // TODO: are we using too many switch-cases?
															case 'a':
																encoding |= 1 << 24;
																break;
															case 'd':
																encoding |= 0x3 << 23;
																break;
														}
														break;
													case 'f':
														if (last_two[1] == 'd') {
															encoding |= 1 << 23; // TODO: ldmfd = ldm; might want to just remove this
														}
														break;
												}
											} else { // stm__
												switch (last_two[0]) {
													case 'e':
														if (last_two[1] == 'a') {
															encoding |= 1 << 23; // TODO: stmea = stm; might want to just remove this
														}
														break;
													case 'f':
														switch (last_two[1]) {
															case 'a':
																encoding |= 0x3 << 23;
																break;
															case 'd':
																encoding |= 1 << 24;
																break;
														}
														break;
												}
											}
											break;
									}
									break;
							}
						}
						// TODO: we might want to say somewhere that this assembler will only accept divided syntax for ARM assembly rather than unified syntax / UAL, but then we would have to change it to make sure that all immediate values are required to start with '#'
					} else {
						if (starts_with_last_success) {
							printf("Probable nop\n");
						}

						encoding = 0x00000000; // TODO: We might not need this here because the default value is 0x00000000, but keeping this might be a good barrier for if anything touches encoding that isn't planned rn
					}
				}

				rom[rom_offset] = (uint8_t)encoding;
				rom[rom_offset + 1] = (uint8_t)(encoding >> 8);
				rom[rom_offset + 2] = (uint8_t)(encoding >> 16);
				rom[rom_offset + 3] = (uint8_t)(encoding >> 24);
				rom_offset += 4;

				break;
			case DIR_B:
			case DIR_H:
			case DIR_W:
				printf("\t\t");

				for (char *c = lines[i].start; *c != '\n' && *c != ';'; c++) {
					printf("%c", *c);

					if (*c == '$' || *c == '#' || *c == '%') {
						enum NumType num_type = *c == '$' ? HEX :
												*c == '#' ? DEC :
															BIN;

						uint32_t val = 0;
						for (c++; (*c >= '0' && *c <= '9') || (getLowerChar(*c) >= 'a' && getLowerChar(*c) <= 'f'); c++) {
							printf("%c", *c);
							switch (num_type) {
								case HEX:
									val <<= 4;
									if (*c >= '0' && *c <= '9') {
										val += *c - '0';
									} else {
										val += 10 + (getLowerChar(*c) - 'a');
									}
									break;
								case DEC:
									if (*c >= '0' && *c <= '9') {
										val = val * 10 + (*c - '0');
									} else {
										printf("Broken decimal literal: ");
										lineIssue(&lines[i]);
									}
									break;
								case BIN:
									if (*c == '0' || *c || '1') {
										val = (val << 1) + (*c - '0');
									} else {
										printf("Broken binary literal: ");
										lineIssue(&lines[i]);
									}
									break;
							}
						}
						c--;

						switch (lines[i].type) {
							case DIR_B:
								rom[rom_offset] = (uint8_t)val;
								rom_offset++;
								break;
							case DIR_H:
								rom[rom_offset] = (uint8_t)val;
								rom[rom_offset + 1] = (uint8_t)(val >> 8);
								rom_offset += 2;
								break;
							case DIR_W:
								rom[rom_offset] = (uint8_t)val;
								rom[rom_offset + 1] = (uint8_t)(val >> 8);
								rom[rom_offset + 2] = (uint8_t)(val >> 16);
								rom[rom_offset + 3] = (uint8_t)(val >> 24);
								rom_offset += 4;
								break;
						}
					}
				}
				printf("\n");

				break;
			default:
				printf("\n");

				break;
		}
	}


	printf("\n\nLabel:\t\tOffset:\n");
	for (int i = 0; i < label_tot; i++) {
		for (char *c = labels[i].start; *c != ':'; c++) {
			printf("%c", *c);
		}

		printf("\t\t%lu\n", (unsigned long)(labels[i].offset));
	}

	printf("\n\nROM size: %lu bytes\n", (unsigned long)track_rom_size);


	if (argc > 2) {
		gba_file = fopen(argv[2], "w");

		if (gba_file == NULL) {
			fprintf(stderr, "ERROR: Error creating or opening %s\n", argv[2]);

			fclose(asm_file);

			return -102;
		}
	} else {
		gba_file = fopen("a.gba", "wb");

		if (gba_file == NULL) {
			fprintf(stderr, "ERROR: Error creating or opening output GBA file\n");

			fclose(asm_file);

			return -103;
		}
	}

	fwrite(rom, track_rom_size, 1, gba_file);

	fclose(gba_file);

	free(rom);
	free(encodings);
	for (int i = 0; i < line_num; i++) {
		void *line_data = lines[i].data;
		if (line_data != NULL) {
			if (lines[i].type == CODE) {
				free(((struct Instruction *)line_data)->blocks);
			}
			free(line_data);
		}
	}
	free(labels);
	free(lines);
	free(asm_buffer);

	return 0;
}
