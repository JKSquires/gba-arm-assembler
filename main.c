#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define MAX_INSTRUCTION_BLOCKS 17
#define BLOCK_TYPE_MASK 7 // TODO: reconsider if needed
#define BLOCK_TYPE_GROUP_MASK (~BLOCK_TYPE_MASK) // "
#define SHIFT_SHIFT 5
#define BLOCK_TYPE_SHIFT_MASK (7 << SHIFT_SHIFT)


typedef struct Instruction Inst;
typedef struct InstructionEncoding InstEncoding;

enum LineType {
	DIR_B = 1,
	DIR_H = 2,
	DIR_W = 4,
	CODE,
	LABEL,
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
	BYTE_SIZE,
	NOP,
};

enum BlockType { // TODO: Use the block type building format used here to format and put together the BlockTypes later instead of hard-coding everything; in later code
	UNKNOWN_BLOCK = 0,
	FIRST = 1,
	REG = 2,
	LBL = 3,
	IMM = 4,
	PSR = 5,
	SHIFT = 1 << 3,
	MEM = 1 << 4,
	LST = 1 << 7,
	LSL = 0 << SHIFT_SHIFT,
	LSR = 1 << SHIFT_SHIFT,
	ASR = 2 << SHIFT_SHIFT,
	RRX = 3 << SHIFT_SHIFT,
	ROR = 3 << SHIFT_SHIFT | 1 << 7,
	SHIFT_REG = SHIFT | REG,
	SHIFT_IMM = SHIFT | IMM,
	MEM_REG = MEM | REG,
	MEM_LBL = MEM | LBL,
	MEM_IMM = MEM | IMM,
	MEM_SHIFT_REG = MEM | SHIFT | REG,
	MEM_SHIFT_IMM = MEM | SHIFT | IMM,
	LST_REG = LST | REG,
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
	void *data; // TODO: I don't think it needs to be a void * anymore because we never ended up using it for any other type of data
	unsigned long line_num;
	enum LineType type : 8;
};

struct Label {
	char *start;
	uint32_t offset;
	unsigned int length;
};

struct InstructionBlock {
	char *start;
	enum BlockType type : 8;
};

struct InstructionEncoding {
	uint32_t (*encode)(uint32_t, char *, bool, struct Label *, unsigned long, Inst *);
	char *mnemonic;
	uint32_t opcode;
	uint8_t mnemonic_length;
	bool encoding_variant;
	enum InstructionSuffix suffix : 8;
};

struct Instruction {
	struct Line *line;
	struct InstructionBlock *blocks;
	uint8_t block_count;
};


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
		for (; labels[labels_i].start[label_length] != ':' && labels[labels_i].start[label_length] != '\n'; label_length++);

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
	printf("\n\n");
}

#include "encoding.c"


// TODO: go through and make sure not too much gets stored on the stack
int main(int argc, char **argv) {
	FILE *asm_file; // TODO: maybe for when we implement including other asm files in a file we should have an array of those files.
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
	fread(asm_buffer, 1, asm_size, asm_file); // TODO: once executed, check to make sure it worked
	asm_buffer[asm_size] = '\n';

	fclose(asm_file);

	unsigned long lines_arr_size = 256;
	struct Line *lines = malloc(lines_arr_size * sizeof *lines);

	unsigned long line_num = 0;
	unsigned long file_line_num = 1;

	uint32_t track_rom_size = 0;
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

					for (unsigned long i = 0; i < line_num; i++) {
						if (lines[i].data != NULL) {
							((Inst *)(lines[i].data))->line = &(lines[i]);
						}
					}
				}

				c = skipWhitespace(c + 1) - 1;
				lines[line_num] = (struct Line){c + 1, NULL, file_line_num, CODE};

				blocks[0] = (struct InstructionBlock){c + 1, FIRST};
				count_blocks = 1;
			} else {
				break;
			}
		}

		if (!comment) {
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

								for (c += 2; *c != '\n' && *c != ';'; c++) {
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
						blocks[count_blocks - 1] = (struct InstructionBlock){c + 1, track_operand_state != MULTIPLE ? UNKNOWN_BLOCK : LST_REG};
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
					blocks[count_blocks - 1].type = LST_REG;

					break;
				case '}':
					if (blocks[count_blocks - 1].type != LST_REG) {
						printf("Broken multiple register operand: ");
						lineIssue(&lines[line_num]);
					}

					track_operand_state = REGULAR;

					break;
				default:
					*c = getLowerChar(*c); // TODO: the goal here is to have every character lowercase so that we don't have to do getLowerChar on characters again in the future, so we need to check to make sure this is setting all to lowercase (except things like labels and label calls unless we want to make it so that something like "ABC" == "abc"), and then we should remove all getLowerChar in later code

					enum BlockType block_type = blocks[count_blocks - 1].type;
					if (block_type == UNKNOWN_BLOCK) {
						if (*c == '$' || *c == '#' || *c == '%') {
							blocks[count_blocks - 1].type = track_operand_state == REGULAR ? IMM : MEM_IMM;
						} else {
							if (*c == 'r' || (*c == '-' && getLowerChar(*(c + 1)) == 'r')) {
								int reg_offset = 1 + (*c == '-');
								if (*(c + reg_offset) >= '0' && *(c + reg_offset) <= '9') {
									blocks[count_blocks - 1].type = track_operand_state == REGULAR ? REG : MEM_REG;
									goto endDetermineBlock;
								}
							}

							if ((*c == 'c' || *c == 's') && (getLowerChar(*(c + 1)) == 'p' && getLowerChar(*(c + 2)) == 's' && getLowerChar(*(c + 3)) == 'r')) {
								char after_psr = *(c + 4);
								if (after_psr == '_' || after_psr == '\n' || after_psr == ' ' || after_psr == '\t' || after_psr == ';') {
									blocks[count_blocks - 1].type = PSR;
									goto endDetermineBlock;
								}
							}

							blocks[count_blocks - 1].type = LBL;

							char c3[] = {0, 0, 0};
							c3[0] = getLowerChar(*c);
							if (c3[0] != '\n') c3[1] = getLowerChar(*(c + 1));
							if (c3[1] != '\n') c3[2] = getLowerChar(*(c + 2));

							if (c3[0] == 'l' && c3[1] == 's' && c3[2] == 'l'
								|| c3[0] == 'l' && c3[1] == 's' && c3[2] == 'r'
								|| c3[0] == 'a' && c3[1] == 's' && c3[2] == 'r'
								|| c3[0] == 'r' && c3[1] == 'o' && c3[2] == 'r'
								|| c3[0] == 'r' && c3[1] == 'r' && c3[2] == 'x') {

								if (c3[0] == 'a' && c3[1] == 's' && c3[2] == 'l') { // handle arithmetic left shift -> logical shift left
									*c = 'l';
									blocks[count_blocks - 1].type = LSL;
								} else if (c3[0] == 'a' && c3[1] == 's' && c3[2] == 'r') {
									blocks[count_blocks - 1].type = ASR;
								} else if (c3[0] == 'l' && c3[1] == 's' && c3[2] == 'l') {
									blocks[count_blocks - 1].type = LSL;
								} else if (c3[0] == 'l' && c3[1] == 's' && c3[2] == 'r') {
									blocks[count_blocks - 1].type = LSR;
								} else if (c3[0] == 'r' && c3[1] == 'o' && c3[2] == 'r') {
									blocks[count_blocks - 1].type = ROR;
								} else if (c3[0] == 'r' && c3[1] == 'r' && c3[2] == 'x') {
									blocks[count_blocks - 1].type = RRX | SHIFT;
								}

								char *skip_shift_whitespace = skipWhitespace(c + 3);
								if (skip_shift_whitespace == c + 3) goto endDetermineBlock;

								if (*skip_shift_whitespace == '$' || *skip_shift_whitespace == '#' || *skip_shift_whitespace == '%') {
									blocks[count_blocks - 1].type |= track_operand_state == REGULAR ? SHIFT_IMM : MEM_SHIFT_IMM;
								} else {
									blocks[count_blocks - 1].type |= track_operand_state == REGULAR ? SHIFT_REG : MEM_SHIFT_REG;
								}
							}
						}
						endDetermineBlock:
					}

					break;
			}
		}
	}


	InstEncoding *encodings = createEncodings();
	unsigned char *rom = calloc(track_rom_size, 1);
	uint32_t rom_offset = 0;

	printf("\nLine:\tType:\n");

	for (int i = 0; i <= line_num; i++) {
		printf("%lu:\t%d:\t", lines[i].line_num, lines[i].type);

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

				uint32_t encoding = 0x00000000;

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

						if (!starts_with_last_success) {
							printf("Unrecognized instruction: ");
							lineIssue(&lines[i]);
						}// else {
						//	printf("Last encoding success at #%d\n", last_encode_success);
						//}
					}

					if (starts_with_last_success && encodings[last_encode_success].suffix != NOP) {
						InstEncoding *success_encoding;

						if (last_encode_success == 9 && mi == 3) { // make sure bl isn't actually something like blt
							success_encoding = &(encodings[7]);
						} else {
							success_encoding = &(encodings[last_encode_success]);
						}

						enum InstructionCondition cond = mi == success_encoding->mnemonic_length ? AL : getInstCond(mnemonic_start + success_encoding->mnemonic_length);
						//printf("Probable instruction condition: 0x%x\n", cond);
						//printf("Probable mnemonic #%d (%s)\n", last_encode_success, success_encoding->mnemonic);

						for (; mnemonic_start[mi] != ' ' && mnemonic_start[mi] != '\t' && mnemonic_start[mi] != '\n'; mi++);

						uint8_t encoding_variant = success_encoding->encoding_variant;
						if (success_encoding->suffix == DATA_SIZE) { // determine load/store register encoding variations (non-zero for _h, _sh, and _sb). Implicit mi > 2 as all encodings with DATA_SIZE have mnemonic_length > 2
								encoding_variant = mnemonic_start[mi - 1] == 'h' || mnemonic_start[mi - 2] == 's';
						}

						char *oprnd1_start = skipWhitespace(mnemonic_start + mi);

						encoding |= cond << 28 | success_encoding->opcode | success_encoding->encode(rom_offset, oprnd1_start, encoding_variant, labels, label_tot, inst);

						//printf("MI=%d->'%c'", mi, mnemonic_start[mi]);
						if (mi > 2) {
							char last_two[] = {mnemonic_start[mi - 2], mnemonic_start[mi - 1]};

							switch (success_encoding->suffix) {
								case SET_FLAGS:
									//printf("Instruction detected with possible set flags\n");

									encoding |= (last_two[1] == 's') << 20;

									break;
								case DATA_SIZE:
									//printf("Instruction detected with possible data size ");

									switch (last_two[1]) {
										case 'b':
											if (last_two[0] == 's') { // ___sb
												//printf("sb");

												encoding = (encoding & 0xF1FFFF9F) | (0xD << 4);
											} else { // ___b
												//printf("b");

												encoding = (encoding & 0xF7FFFFFF) | (1 << 26) | (1 << 22);
											}
											break;
										case 't':
											if (last_two[0] == 'b') { // ___bt
												//printf("bt");

												encoding = (encoding & 0xF6FFFFFF) | (1 << 26) | (0x3 << 21);
											} else { // ___t
												//printf("t");

												encoding = (encoding & 0xF6BFFFFF) | (1 << 26) | (1 << 21);
											}
											break;
										case 'h':
											encoding &= encoding & 0xF1FFFF9F;
											if (last_two[0] == 's') { // ___sh
												//printf("sh");

												encoding |= 0xF << 4;
											} else { // ___h
												//printf("h");

												encoding |= 0xB << 4;
											}
											break;
									}

									//printf("\n");

									break;
								case ADDRESSING_MODE: // FIXME: when encoding ldm and sdm in previous section, note that ldm has bit 20 set and stm has bit 20 cleared. this means that we should probably not mask that bit in this section--review
									//printf("Instruction detected with possible addressing mode\n");

									encoding = (encoding & 0xF03FFFFF) | (1 << 27);

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
								case BYTE_SIZE:
									//printf("Instruction detected with possible byte data size\n");

									encoding |= (last_two[1] == 'b') << 22;

									break;
							}
						}
						// TODO: we might want to say somewhere that this assembler will only accept divided syntax for ARM assembly rather than unified syntax / UAL, but then we would have to change it to make sure that all immediate values are required to start with '#'
					} else {
						//if (starts_with_last_success) {
						//	printf("Probable nop\n");
						//}

						encoding = encodings[NOP_ENCODING_INDEX].encode(rom_offset, mnemonic_start, false, labels, label_tot, inst);
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
									if (*c == '0' || *c == '1') {
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
		gba_file = fopen(argv[2], "wb");

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
	for (int i = 0; i <= line_num; i++) {
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
