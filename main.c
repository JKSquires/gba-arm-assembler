#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define ENCODINGS_COUNT 45
#define MAX_INSTRUCTION_BLOCKS 4 // TODO: check that 4 is the max number of blocks a valid instruction can have


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
	DIR_INC
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

// maybe for operands, we can create a struct that stores type (register, immediate, label reference, memory operand, bit-shift, etc.) and data (union the different ones?)
//enum Oprnd2Type {
//	REG = 0,
//	IMM = 1
//};

enum BlockType {
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
	MEM_SHIFT_IMM
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
};

struct InstructionBlock {
	char *start;
	enum BlockType type;
};

struct InstructionEncoding {
	uint32_t (*encode)(Inst *instruction);
	char *mnemonic;
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

/*
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

uint32_t unsupportedInstruction(Inst *i) {
	printf("Unsupported instruction: ");
	encodingIssue(i);

	return 0;
}

InstEncoding *createEncodings() {
	InstEncoding *encodings = malloc(ENCODINGS_COUNT * sizeof(*encodings));

	// maybe instead of one function for each, find patterns like (Rd, Rn, <Oprnd2>), (imm24), (Rt, [Rn, +/- Rm{, <shift>}]{!}), etc. and make functions for those?
	encodings[0] = (InstEncoding){unsupportedInstruction, "adc"};
	encodings[1] = (InstEncoding){unsupportedInstruction, "add"};
	encodings[2] = (InstEncoding){unsupportedInstruction, "adr"};
	encodings[3] = (InstEncoding){unsupportedInstruction, "addr"};
	encodings[4] = (InstEncoding){unsupportedInstruction, "adrl"};
	encodings[5] = (InstEncoding){unsupportedInstruction, "and"};
	encodings[6] = (InstEncoding){unsupportedInstruction, "asr"};
	encodings[7] = (InstEncoding){unsupportedInstruction, "b"};
	encodings[8] = (InstEncoding){unsupportedInstruction, "bic"};
	encodings[9] = (InstEncoding){unsupportedInstruction, "bl"};
	encodings[10] = (InstEncoding){unsupportedInstruction, "bx"};
	encodings[11] = (InstEncoding){unsupportedInstruction, "cmn"};
	encodings[12] = (InstEncoding){unsupportedInstruction, "cmp"};
	encodings[13] = (InstEncoding){unsupportedInstruction, "eor"};
	encodings[14] = (InstEncoding){unsupportedInstruction, "ldm"};
	encodings[15] = (InstEncoding){unsupportedInstruction, "ldr"};
	encodings[16] = (InstEncoding){unsupportedInstruction, "lsl"};
	encodings[17] = (InstEncoding){unsupportedInstruction, "mla"};
	encodings[18] = (InstEncoding){unsupportedInstruction, "mov"};
	encodings[19] = (InstEncoding){unsupportedInstruction, "mrs"};
	encodings[20] = (InstEncoding){unsupportedInstruction, "msr"};
	encodings[21] = (InstEncoding){unsupportedInstruction, "mul"};
	encodings[22] = (InstEncoding){unsupportedInstruction, "mvn"};
	encodings[23] = (InstEncoding){unsupportedInstruction, "neg"};
	encodings[24] = (InstEncoding){unsupportedInstruction, "nop"};
	encodings[25] = (InstEncoding){unsupportedInstruction, "orr"};
	encodings[26] = (InstEncoding){unsupportedInstruction, "pop"};
	encodings[27] = (InstEncoding){unsupportedInstruction, "push"};
	encodings[28] = (InstEncoding){unsupportedInstruction, "ror"};
	encodings[29] = (InstEncoding){unsupportedInstruction, "rrx"};
	encodings[30] = (InstEncoding){unsupportedInstruction, "rsb"};
	encodings[31] = (InstEncoding){unsupportedInstruction, "rsc"};
	encodings[32] = (InstEncoding){unsupportedInstruction, "sbc"};
	encodings[33] = (InstEncoding){unsupportedInstruction, "smlal"};
	encodings[34] = (InstEncoding){unsupportedInstruction, "smull"};
	encodings[35] = (InstEncoding){unsupportedInstruction, "stm"};
	encodings[36] = (InstEncoding){unsupportedInstruction, "str"};
	encodings[37] = (InstEncoding){unsupportedInstruction, "sub"};
	encodings[38] = (InstEncoding){unsupportedInstruction, "swi"};
	encodings[39] = (InstEncoding){unsupportedInstruction, "swp"};
	encodings[40] = (InstEncoding){unsupportedInstruction, "teq"};
	encodings[41] = (InstEncoding){unsupportedInstruction, "tst"};
	encodings[42] = (InstEncoding){unsupportedInstruction, "und"};
	encodings[43] = (InstEncoding){unsupportedInstruction, "umlal"};
	encodings[44] = (InstEncoding){unsupportedInstruction, "umull"};

	return encodings;
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
	struct Line *lines = malloc(lines_arr_size * sizeof(*lines));

	unsigned long line_num = 0;
	unsigned long file_line_num = 1;

	uint32_t track_rom_size = 0;
	unsigned long labels_arr_size = 64;
	struct Label *labels = malloc(labels_arr_size * sizeof(*labels)); // FIXME: should absolutely use a hash table at some point instead
	unsigned long label_tot = 0;

	bool comment = false;
	bool inside_mem_oprnd = false;
	int count_blocks = 1;
	struct InstructionBlock blocks[MAX_INSTRUCTION_BLOCKS];

	lines[0] = (struct Line){asm_buffer, NULL, 0, CODE}; // FIXME: consider the case where line 0 is not code but like a comment or something; verify this would still work
	blocks[0] = (struct InstructionBlock){asm_buffer, FIRST};

	char *asm_buffer_end = asm_buffer + asm_size;
	for (char *c = asm_buffer; c <= asm_buffer_end; c++) {
		if (*c == '\n') {
			comment = false;
			if (inside_mem_oprnd) {
				printf("Memory operand not closed properly on line %lu\n", file_line_num);
				inside_mem_oprnd = false;
			}
			file_line_num++;

			if (lines[line_num].type == CODE) {
				Inst *inst = malloc(sizeof(*inst));
				inst->line = &(lines[line_num]);
				inst->blocks = malloc(count_blocks * sizeof(struct InstructionBlock));
				for (int i = 0; i < count_blocks; i++) {
					inst->blocks[i] = blocks[i];
				}
				inst->block_count = count_blocks;

				lines[line_num].data = (void *)inst;

				track_rom_size += 4;
			}

			if (!(c != asm_buffer_end && (*(c + 1) == '\n' || *(c + 1) == ';'))) {
				if (++line_num + 1 == lines_arr_size) {
					lines_arr_size *= 2;
					lines = realloc(lines, lines_arr_size * sizeof(*lines));
				}

				if (c != asm_buffer_end) {
					lines[line_num] = (struct Line){c + 1, NULL, file_line_num, CODE};
					if (*(c + 1) == '@') {
						switch (getLowerChar(*(c + 2))) {
							case 'i':
								if (getLowerChar(*(c + 3)) == 'n'
									&& getLowerChar(*(c + 4)) == 'c'
									&& getLowerChar(*(c + 5)) == ' ') {
									printf("\nInclude directive is unsupported right now\n");
									lines[line_num].type = DIR_INC;

									c += 5;
								}
							case 'b': // fall through
							case 'h': // fall through
							case 'w':
								if (*(c + 3) == ' ') {
									char k = getLowerChar(*(c + 2));
									lines[line_num].type = k == 'b' ? DIR_B :
															k == 'h' ? DIR_H :
																		DIR_W;

									 for (c += 3; *c != '\n'; c++) {
										if (*c == '$' || *c == '%') {
											track_rom_size += lines[line_num].type;
										}
									}
								}
								break;
							default:
								lines[line_num].type = DIR_UNK;
								break;
						}
					} else {
						blocks[0] = (struct InstructionBlock){c + 1, FIRST};
						count_blocks = 1;
					}
				}
			}
		}

		if (!comment) {
			printf("%c", *c); // TODO: remove debug line
			switch (*c) {
				case ';':
					comment = true;
					break;
				case ':':
					lines[line_num].type = LABEL;

					unsigned int label_length = c - lines[line_num].start;

					bool is_dup = false;
					for (unsigned long i = 0; i < label_tot; i++) {
						unsigned int dup_char = 0;
						for (unsigned long si = 0; si < label_length; si++) {
							if (lines[line_num].start[si] == labels[i].start[si]) {
								dup_char++;
							}
						}

						if (dup_char == label_length) {
							is_dup = true;
							printf("Duplicate label on line %lu\n", file_line_num);

							break;
						}
					}

					if (!is_dup) {
						if (label_tot == labels_arr_size) {
							labels_arr_size *= 2;
							labels = realloc(labels, labels_arr_size * sizeof(*labels));
						}

						labels[label_tot] = (struct Label){lines[line_num].start, track_rom_size};
						label_tot++;
					}

					break;
				case ',':
					count_blocks++;
					// store oprnd *start but first make sure the number of operands are not too large (if too large, there must be a problem)
					if (count_blocks <= MAX_INSTRUCTION_BLOCKS) {
						blocks[count_blocks - 1] = (struct InstructionBlock){c + 1, inside_mem_oprnd ? MEM_REG : REG};
					} else {
						printf("Too many operands on line %lu\n", file_line_num);
					}

					break;
				default:
					*c = getLowerChar(*c);
					break;
			}
		}
	}
	lines[line_num] = (struct Line){NULL, NULL, file_line_num, END};


	InstEncoding *encodings = createEncodings();

	printf("DIR_B = %d, DIR_H = %d, DIR_W = %d, CODE = %d, LABEL = %d, END = %d, DIR_UNK = %d, DIR_INC = %d\n---\nLine:\tType:\n", DIR_B, DIR_H, DIR_W, CODE, LABEL, END, DIR_UNK, DIR_INC);
	for (int i = 0; i < line_num; i++) {
		printf("%lu:\t%d:\t", lines[i].line_num, lines[i].type);

		for (char *c = lines[i].start; *c != '\n' && *c != ';'; c++) {
			// TODO: start processing lines: need to process byte define directives, and create instruction encodings for each instruction line, ... more most certainly
			printf("%c", *c);
		}
		printf("\n");

		if (lines[i].type == CODE) {
			Inst *inst = (Inst *)(lines[i].data);

			printf("\t\t");
			for (int bi = 0; bi < inst->block_count; bi++) {
				if (bi != 0) {
					printf("\t,\t");
				}
				for (char *c = inst->blocks[bi].start; *c != ',' && *c != '\n' && *c != ';'; c++) {
					printf("%c", *c);
				}
			}
			printf("\n");
		}
	}

	printf("Rom size: %lu bytes\n", (unsigned long)track_rom_size);


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
