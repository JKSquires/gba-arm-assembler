#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define ENCODINGS_COUNT 46
#define NOP_ENCODING_INDEX 25
#define MAX_INSTRUCTION_BLOCKS 4 // FIXME: 4 is the max number of blocks a valid instruction can have excluding in multiple register operands/blocks (e.g. {r0, r3-r5, r10}). We need to figure out how we want to deal with those. Might be easiest to just count the max number including these
#define REG_READ_ERR (1 << 7)
#define REG_READ_WRITEBACK (1 << 4)
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
	SHIFT = 1 << 3,
	MEM = 1 << 4,
	MUL = 1 << 7,
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
	MUL_REG = MUL | REG,
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
	uint32_t (*encode)(uint32_t, char *, bool, struct Label *, unsigned long, Inst *);
	char *mnemonic;
	uint32_t opcode;
	uint8_t mnemonic_length;
	bool secondary_encoding;
	enum InstructionSuffix suffix;
};

struct Instruction {
	struct Line *line; // TODO: refactor to better whay than a loop like this where lines have instructions which store the line etc...
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
	printf("\n");
}

uint8_t readReg(char *reg, uint8_t oprnd_num, bool support_writeback, Inst *i) {
	uint8_t data = 0;

	if (*reg != 'r') {
		printf("Issue reading register: ");
		lineIssue(i->line);

		return REG_READ_ERR;
	}

	char *c = reg + 1;
	for (; *c >= '0' && *c <= '9'; c++) {
		data = data * 10 + (*c - '0');
	}
	data &= 0xF;

	if (*c == '!') {
		data |= REG_READ_WRITEBACK;
		c++;

		if (!support_writeback) {
			printf("Instruction does not support writeback in operand %d: ", oprnd_num);
			lineIssue(i->line);
		}
	}

	return data; // register num is &0xF
}

uint32_t readConst(char *constant, Inst *i) {
	uint32_t data = 0;

	if (*constant == '$' || *constant == '#' || *constant == '%') {
		enum NumType num_type = *constant == '$' ? HEX :
								*constant == '#' ? DEC :
											BIN;

		char *c = constant + 1;
		for (; (*c >= '0' && *c <= '9') || (getLowerChar(*c) >= 'a' && getLowerChar(*c) <= 'f'); c++) {
			switch (num_type) {
				case HEX:
					data <<= 4;
					if (*c >= '0' && *c <= '9') {
						data += *c - '0';
					} else {
						data += 10 + (getLowerChar(*c) - 'a');
					}
					break;
				case DEC:
					if (*c >= '0' && *c <= '9') {
						data = data * 10 + (*c - '0');
					} else {
						printf("Broken decimal literal: ");
						lineIssue(i->line);
					}
					break;
				case BIN:
					if (*c == '0' || *c == '1') {
						data = (data << 1) + (*c - '0');
					} else {
						printf("Broken binary literal: ");
						lineIssue(i->line);
					}
					break;
			}
		}
	} else {
		printf("Issue reading constant/immediate value: ");
		lineIssue(i->line);
	}

	return data;
}

uint16_t imm12(uint32_t val, Inst *i) {
	for (int r = 0; r < 32; r += 2) {
		uint32_t imm8 = (val << r) | (uint32_t)((uint64_t)val >> (32 - r));

		if ((imm8 & 0xFFFFFF00) == 0) {
			return (r << 7) | imm8;
		}
	}

	printf("Cannot create rotation for 12-bit immediate: ");
	lineIssue(i->line);
	return 0;
}

uint32_t unsupportedInstruction(uint32_t, char *, bool, struct Label *, unsigned long, Inst *i) {
	printf("Unsupported instruction: ");
	lineIssue(i->line);

	return 0;
}

uint32_t handleShift(uint8_t oprnd_i, Inst *i) {
	uint32_t encoding = 0;

	if (i->blocks[oprnd_i].type & SHIFT) {
		encoding |= (i->blocks[oprnd_i].type & BLOCK_TYPE_SHIFT_MASK) & 0x7F;

		if ((i->blocks[oprnd_i].type & BLOCK_TYPE_SHIFT_MASK) != RRX) {
			char *val_start = i->blocks[oprnd_i].start;
			for (; val_start < i->blocks[oprnd_i].start + 4 && *val_start != '\n' && *val_start != ';'; val_start++);
			val_start = skipWhitespace(val_start);

			switch (i->blocks[oprnd_i].type & BLOCK_TYPE_MASK) {
				case IMM:
					uint32_t constant = readConst(val_start, i);
					if (constant > 32) {
						printf("Bit shift by immediate must be between 0 and 32 inclusive: ");
						lineIssue(i->line);
					}
					encoding |= (constant & 0x1F) << 7;

					break;
				case REG:
					uint8_t reg = readReg(val_start, oprnd_i + 1, false, i);
					if (reg & REG_READ_ERR)
						return 0;

					encoding |= 1 << 4 | ((reg & 0xF) << 8);

					break;
			}
		}
	} else {
		printf("Expected bit shift in operand %d: ", oprnd_i + 1);
		lineIssue(i->line);
	}

	return encoding;
}

uint32_t rdRnOprnd2(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;

	if (i->block_count < 3) {
		printf("Instruction requires at least 3 operands: ");
		lineIssue(i->line);
		return 0;
	}

	uint8_t d_reg_data = readReg(oprnd1_start, 1, false, i);
	if (d_reg_data & REG_READ_ERR)
		return 0;
	encoding |= (d_reg_data & 0xF) << 12;

	uint8_t n_reg_data = readReg(i->blocks[1].start, 2, false, i);
	if (n_reg_data & REG_READ_ERR)
		return 0;
	encoding |= (n_reg_data & 0xF) << 16;

	if (i->blocks[2].type & REG) {
		uint8_t m_reg_data = readReg(i->blocks[2].start, 3, false, i);
		if (m_reg_data & REG_READ_ERR)
			return 0;
		encoding |= m_reg_data & 0xF;

		if (i->block_count >= 4) {
			encoding |= handleShift(3, i);
		}
	} else {
		uint32_t constant = readConst(i->blocks[2].start, i);
		uint16_t imm = imm12(constant, i);

		encoding |= (1 << 25) | (imm & 0xFFF);

		if (i->block_count >= 4 && (i->blocks[3].type & SHIFT)) {
			printf("Bit shift cannot be done to immediate: ");
			lineIssue(i->line);
		}
	}

	return encoding;
}

uint32_t rdRnRm(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;

	if (i->block_count != 3) {
		printf("Instruction requires 3 operands: ");
		lineIssue(i->line);
		return 0;
	}

	uint8_t d_reg_data = readReg(oprnd1_start, 1, false, i);
	if (d_reg_data & REG_READ_ERR)
		return 0;
	encoding |= (d_reg_data & 0xF) << 16;

	uint8_t n_reg_data = readReg(i->blocks[1].start, 2, false, i);
	if (n_reg_data & REG_READ_ERR)
		return 0;
	encoding |= n_reg_data & 0xF;

	uint8_t m_reg_data = readReg(i->blocks[2].start, 3, false, i);
	if (m_reg_data & REG_READ_ERR)
		return 0;
	encoding |= (m_reg_data & 0xF) << 8;

	return encoding;
}

uint32_t rxOprnd2(uint32_t, char *oprnd1_start, bool rx_is_rn, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;

	if (i->block_count < 2) {
		printf("Instruction requires at least 2 operands: ");
		lineIssue(i->line);
		return 0;
	}

	uint8_t reg1_data = readReg(oprnd1_start, 1, false, i);
	if (reg1_data & REG_READ_ERR)
		return 0;

	encoding |= (reg1_data & 0xF) << (12 + (4 * rx_is_rn));

	if (i->blocks[1].type & REG) {
		uint8_t reg2_data = readReg(i->blocks[1].start, 2, false, i);
		if (reg2_data & REG_READ_ERR)
			return 0;

		encoding |= reg2_data & 0xF;

		if (i->block_count >= 3) {
			encoding |= handleShift(2, i);
		}
	} else {
		uint32_t constant = readConst(i->blocks[1].start, i);
		uint16_t imm = imm12(constant, i);

		encoding |= (1 << 25) | (imm & 0xFFF);

		if (i->block_count >= 3 && (i->blocks[2].type & SHIFT)) {
			printf("Bit shift cannot be done to immediate: ");
			lineIssue(i->line);
		}
	}

	return encoding;
}

uint32_t rxRxRxRx(uint32_t, char *oprnd1_start, bool is_long, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;

	if (i->block_count != 4) {
		printf("Instruction requires 4 operands: ");
		lineIssue(i->line);
		return 0;
	}

	uint8_t reg1_data = readReg(oprnd1_start, 1, false, i);
	if (reg1_data & REG_READ_ERR)
		return 0;
	encoding |= (reg1_data & 0xF) << (16 - (4 * is_long));

	uint8_t reg2_data = readReg(i->blocks[1].start, 2, false, i);
	if (reg2_data & REG_READ_ERR)
		return 0;
	encoding |= (reg2_data & 0xF) << (16 * is_long);

	uint8_t reg3_data = readReg(i->blocks[2].start, 3, false, i);
	if (reg3_data & REG_READ_ERR)
		return 0;
	encoding |= (reg3_data & 0xF) << (8 * (!is_long));

	uint8_t reg4_data = readReg(i->blocks[3].start, 4, false, i);
	if (reg4_data & REG_READ_ERR)
		return 0;
	encoding |= (reg4_data & 0xF) << (12 - (4 * is_long));

	return encoding;
}

uint32_t shiftPseudoInst(char *oprnd1_start, enum BlockType type, Inst *i) {
	if (i->block_count < 3 || i->blocks[1].type != REG || !(i->blocks[2].type == IMM || i->blocks[2].type == REG)) {
		printf("Bit shift pseudo instruction is formatted incorrectly: ");
		lineIssue(i->line);
		return 0;
	}

	i->blocks[2].type |= SHIFT | type;
	i->blocks[2].start -= 4;

	return 0x01A00000 | rxOprnd2(0, oprnd1_start, false, NULL, 0, i);
}

uint32_t asr(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	return shiftPseudoInst(oprnd1_start, ASR, i);
}

uint32_t b(uint32_t inst_offset, char *oprnd1_start, bool, struct Label *labels, unsigned long label_tot, Inst *i) {
	uint32_t encoding = 0;

	unsigned int inst_label_length = 0;
	for (; oprnd1_start[inst_label_length] >= 'a' && oprnd1_start[inst_label_length] <= 'z' || oprnd1_start[inst_label_length] >= '0' && oprnd1_start[inst_label_length] <= '9'; inst_label_length++);

	unsigned long label_i = findLabel(oprnd1_start, inst_label_length, labels, label_tot);
	if (label_i == label_tot) { // TODO: maybe someday we can support branching to a direct address (e.g. b $8000000)
		printf("Cannot find label for branch: ");
		lineIssue(i->line);

		return 0;
	}

	uint32_t label_offset = labels[label_i].offset;

	uint32_t imm24 = (label_offset - (inst_offset + 8)) >> 2;

	if (imm24 > 0x00FFFFFF && imm24 < 0x3F000000) { // TODO: make sure these bounds are correct
		printf("Issue creating 24-bit immediate offset: ");
		lineIssue(i->line);
	}

	encoding |= imm24 & 0x00FFFFFF;

	return encoding;
}

uint32_t bx(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;

	uint8_t reg_data = readReg(oprnd1_start, 1, false, i);

	if (reg_data & REG_READ_ERR)
		return 0;

	encoding |= reg_data & 0xF;

	return encoding;
}

uint32_t ldrStr(uint32_t inst_offset, char *oprnd1_start, bool is_ldr, struct Label *labels, unsigned long label_tot, Inst *i) {
	uint32_t encoding = 0;
	bool p = 1;
	bool u = 0;
	bool w = 0;

	if (i->block_count < 2) {
		printf("Instruction requires at least 2 operands: ");
		lineIssue(i->line);
		return 0;
	}

	uint8_t t_reg_data = readReg(oprnd1_start, 1, false, i);
	if (t_reg_data & REG_READ_ERR)
		return 0;
	encoding |= (t_reg_data & 0xF) << 12;

	if (i->blocks[1].type == LBL && is_ldr) {
		printf("LDR/STR Literal\n");

		unsigned int inst_label_length = 0;
		for (; i->blocks[1].start[inst_label_length] >= 'a' && i->blocks[1].start[inst_label_length] <= 'z' || i->blocks[1].start[inst_label_length] >= '0' && i->blocks[1].start[inst_label_length] <= '9'; inst_label_length++);

		unsigned long label_i = findLabel(i->blocks[1].start, inst_label_length, labels, label_tot);
		if (label_i == label_tot) {
			printf("Cannot find label: ");
			lineIssue(i->line);

			return 0;
		}

		uint32_t label_offset = labels[label_i].offset;

		uint32_t val = label_offset - (inst_offset + 8);
		u = val <= 0xFFF;
		val = 0 - (!u * val);

		if (val > 0xFFF) {
			printf("Destination label is too far from instruction to use literal: ");
			lineIssue(i->line);
			return 0;
		}

		encoding |= 0x001F0000 | val;
	} else {
		return unsupportedInstruction(0, NULL, false, NULL, 0, i);
	}

	return encoding | (p << 24) | (u << 23) | (w << 21);
}


uint32_t lsl(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	return shiftPseudoInst(oprnd1_start, LSL, i);
}

uint32_t lsr(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	return shiftPseudoInst(oprnd1_start, LSR, i);
}

uint32_t nop(uint32_t, char *, bool, struct Label *, unsigned long, Inst *) {
	return 0x00000000;
}

uint32_t ror(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	return shiftPseudoInst(oprnd1_start, ROR, i);
}

uint32_t rrx(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	if (i->block_count < 2) {
		printf("Rotate Right with Extend requires 2 operands: ");
		lineIssue(i->line);
		return 0;
	}

	// TODO: create a more robust instruction-building system for pseudo-instructions
	struct InstructionBlock *old_blocks = i->blocks;
	i->blocks = malloc(3 * sizeof (struct InstructionBlock));
	for (int ibi = 0; ibi < 2; ibi++) {
		i->blocks[ibi] = old_blocks[ibi];
	}
	i->blocks[2] = (struct InstructionBlock){"\n", REG};
	i->block_count++;

	free(old_blocks);

	return shiftPseudoInst(oprnd1_start, RRX, i);
}

uint32_t swi(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	uint32_t imm24 = readConst(oprnd1_start, i);

	if (imm24 > 0x00FFFFFF) {
		printf("Software interrupt location is too large: ");
		lineIssue(i->line);
		return 0;
	}

	return imm24;
}

InstEncoding *createEncodings() {
	InstEncoding *encodings = malloc(ENCODINGS_COUNT * sizeof *encodings);

	encodings[0] =	(InstEncoding){rdRnOprnd2,				"adc",		0x00A00000,	3,	false,	SET_FLAGS};
	encodings[1] =	(InstEncoding){rdRnOprnd2,				"add",		0x00800000,	3,	false,	SET_FLAGS};
	encodings[2] =	(InstEncoding){unsupportedInstruction,	"addr",		0x00000000,	4,	false,	COND_ONLY};
	encodings[3] =	(InstEncoding){unsupportedInstruction,	"adr",		0x00000000,	3,	false,	COND_ONLY};
	encodings[4] =	(InstEncoding){unsupportedInstruction,	"adrl",		0x00000000,	4,	false,	COND_ONLY};
	encodings[5] =	(InstEncoding){rdRnOprnd2,				"and",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[6] =	(InstEncoding){asr,						"asr",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[7] =	(InstEncoding){b,						"b",		0x0A000000,	1,	false,	COND_ONLY};
	encodings[8] =	(InstEncoding){rdRnOprnd2,				"bic",		0x01C00000,	3,	false,	SET_FLAGS};
	encodings[9] =	(InstEncoding){b,						"bl",		0x0B000000,	2,	false,	COND_ONLY};
	encodings[10] =	(InstEncoding){bx,						"bx",		0x012FFF10,	2,	false,	COND_ONLY};
	encodings[11] =	(InstEncoding){rxOprnd2,				"cmn",		0x01700000,	3,	true,	COND_ONLY};
	encodings[12] =	(InstEncoding){rxOprnd2,				"cmp",		0x01500000,	3,	true,	COND_ONLY};
	encodings[13] =	(InstEncoding){rdRnOprnd2,				"eor",		0x00200000,	3,	false,	SET_FLAGS};
	encodings[14] =	(InstEncoding){unsupportedInstruction,	"ldm",		0x00000000,	3,	false,	ADDRESSING_MODE};
	encodings[15] =	(InstEncoding){ldrStr,					"ldr",		0x04100000,	3,	true,	DATA_SIZE};
	encodings[16] =	(InstEncoding){lsl,						"lsl",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[17] =	(InstEncoding){lsr,						"lsr",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[18] =	(InstEncoding){rxRxRxRx,				"mla",		0x00200090,	3,	false,	SET_FLAGS};
	encodings[19] =	(InstEncoding){rxOprnd2,				"mov",		0x01A00000,	3,	false,	SET_FLAGS};
	encodings[20] =	(InstEncoding){unsupportedInstruction,	"mrs",		0x00000000,	3,	false,	COND_ONLY};
	encodings[21] =	(InstEncoding){unsupportedInstruction,	"msr",		0x00000000,	3,	false,	COND_ONLY};
	encodings[22] =	(InstEncoding){rdRnRm,					"mul",		0x00000090,	3,	false,	SET_FLAGS};
	encodings[23] =	(InstEncoding){rxOprnd2,				"mvn",		0x01E00000,	3,	false,	SET_FLAGS};
	encodings[24] =	(InstEncoding){unsupportedInstruction,	"neg",		0x00000000,	3,	false,	COND_ONLY};
	encodings[25] =	(InstEncoding){nop,						"nop",		0x00000000,	3,	false,	NOP};
	encodings[26] =	(InstEncoding){rdRnOprnd2,				"orr",		0x01800000,	3,	false,	SET_FLAGS};
	encodings[27] =	(InstEncoding){unsupportedInstruction,	"pop",		0x00000000,	3,	false,	COND_ONLY};
	encodings[28] =	(InstEncoding){unsupportedInstruction,	"push",		0x00000000,	4,	false,	COND_ONLY};
	encodings[29] =	(InstEncoding){ror,						"ror",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[30] =	(InstEncoding){rrx,						"rrx",		0x00000000,	3,	false,	SET_FLAGS};
	encodings[31] =	(InstEncoding){rdRnOprnd2,				"rsb",		0x00600000,	3,	false,	SET_FLAGS};
	encodings[32] =	(InstEncoding){rdRnOprnd2,				"rsc",		0x00E00000,	3,	false,	SET_FLAGS};
	encodings[33] =	(InstEncoding){rdRnOprnd2,				"sbc",		0x00C00000,	3,	false,	SET_FLAGS};
	encodings[34] =	(InstEncoding){rxRxRxRx,				"smlal",	0x00E00090,	5,	true,	SET_FLAGS};
	encodings[35] =	(InstEncoding){rxRxRxRx,				"smull",	0x00C00090,	5,	true,	SET_FLAGS};
	encodings[36] =	(InstEncoding){unsupportedInstruction,	"stm",		0x00000000,	3,	false,	ADDRESSING_MODE};
	encodings[37] =	(InstEncoding){ldrStr,					"str",		0x04000000,	3,	false,	DATA_SIZE};
	encodings[38] =	(InstEncoding){rdRnOprnd2,				"sub",		0x00400000,	3,	false,	SET_FLAGS};
	encodings[39] =	(InstEncoding){swi,						"swi",		0x0F000000,	3,	false,	COND_ONLY};
	encodings[40] =	(InstEncoding){unsupportedInstruction,	"swp",		0x00000000,	3,	false,	BYTE_SIZE};
	encodings[41] =	(InstEncoding){rxOprnd2,				"teq",		0x01300000,	3,	true,	COND_ONLY};
	encodings[42] =	(InstEncoding){rxOprnd2,				"tst",		0x01100000,	3,	true,	COND_ONLY};
	encodings[43] =	(InstEncoding){unsupportedInstruction,	"und",		0x00000000,	3,	false,	COND_ONLY};
	encodings[44] =	(InstEncoding){rxRxRxRx,				"umlal",	0x00A00090,	5,	true,	SET_FLAGS};
	encodings[45] =	(InstEncoding){rxRxRxRx,				"umull",	0x00800090,	5,	true,	SET_FLAGS};

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
				//printf("\nWrite line %lu starting with '%c'\n", file_line_num, *(c + 1));

				blocks[0] = (struct InstructionBlock){c + 1, FIRST};
				count_blocks = 1;
			} else {
				break;
			}
		}

		if (!comment) {
			//printf("<%c>", *c); // TODO: remove debug lines

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
									break;
								}
								// Fall through otherwise
							default:
								blocks[count_blocks - 1].type = LBL;

								char c3[] = {0, 0, 0};
								c3[0] = getLowerChar(*c);
								if (c3[0] != '\n') c3[1] = getLowerChar(*(c + 1));
								if (c3[1] != '\n') c3[2] = getLowerChar(*(c + 2));

								printf("CHARS(%c)(%c)(%c)", c3[0], c3[1], c3[2]);

								if (c3[0] == 'l' && c3[1] == 's' && c3[2] == 'l'
									|| c3[0] == 'l' && c3[1] == 's' && c3[2] == 'r'
									|| c3[0] == 'a' && c3[1] == 's' && c3[2] == 'r'
									|| c3[0] == 'r' && c3[1] == 'o' && c3[2] == 'r'
									|| c3[0] == 'r' && c3[1] == 'r' && c3[2] == 'x') {

									printf("SHIFT");

									if (c3[0] == 'a' && c3[1] == 's' && c3[2] == 'l') { // handle arithmetic left shift
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
									if (skip_shift_whitespace == c + 3) break;

									if (*skip_shift_whitespace == '$' || *skip_shift_whitespace == '#' || *skip_shift_whitespace == '%') {
										blocks[count_blocks - 1].type |= track_operand_state == REGULAR ? SHIFT_IMM : MEM_SHIFT_IMM;
									} else {
										blocks[count_blocks - 1].type |= track_operand_state == REGULAR ? SHIFT_REG : MEM_SHIFT_REG;
									}
								}

								break;
						}
					}

					break;
			}
		}
	}


	InstEncoding *encodings = createEncodings();
	unsigned char *rom = calloc(track_rom_size, 1);
	uint32_t rom_offset = 0;

	//printf("DIR_B = %d, DIR_H = %d, DIR_W = %d, CODE = %d, LABEL = %d, END = %d, DIR_UNK = %d, DIR_A = %d, DIR_I = %d, DIR_T = %d\n", DIR_B, DIR_H, DIR_W, CODE, LABEL, END, DIR_UNK, DIR_A, DIR_I, DIR_T);
	//printf("UNKNOWN_BLOCK = %d, FIRST = %d, REG = %d, IMM = %d, LBL = %d, SHIFT_REG = %d, SHIFT_IMM = %d, MEM_REG = %d, MEM_IMM = %d, MEM_LBL = %d, MEM_SHIFT_REG = %d, MEM_SHIFT_IMM = %d, MUL_REG = %d\n", UNKNOWN_BLOCK, FIRST, REG, IMM, LBL, SHIFT_REG, SHIFT_IMM, MEM_REG, MEM_IMM, MEM_LBL, MEM_SHIFT_REG, MEM_SHIFT_IMM, MUL_REG);
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
							printf("Unrecognized instruction: ");
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
						encoding = success_encoding->opcode | success_encoding->encode(rom_offset, oprnd1_start, success_encoding->secondary_encoding, labels, label_tot, inst);

						enum InstructionCondition cond = mi == success_encoding->mnemonic_length ? AL : getInstCond(mnemonic_start + success_encoding->mnemonic_length); // TODO: when AL comes through, need to check to make sure it is proper.
						printf("Probable instruction condition: 0x%x\n", cond);

						encoding |= cond << 28;

						//printf("MI=%d->'%c'", mi, mnemonic_start[mi]);
						for (; mnemonic_start[mi] != ' ' && mnemonic_start[mi] != '\t' && mnemonic_start[mi] != '\n'; mi++); // TODO: do we really need this, we kind of check this earlier--review
						if (mnemonic_start + mi > asm_buffer_start + 2) {
							char last_two[] = {getLowerChar(mnemonic_start[mi - 2]), getLowerChar(mnemonic_start[mi - 1])};

							switch (success_encoding->suffix) {
								case SET_FLAGS:
									printf("Instruction detected with possible set flags\n");

									encoding |= (last_two[1] == 's') << 20;

									break;
								case DATA_SIZE:
									// TODO: bits 21, 23, and 24 will need to be set later accordingly
									printf("Instruction detected with possible data size ");

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
									printf("Instruction detected with possible addressing mode");

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
								case BYTE_SIZE:
									printf("Instruction detected with possible byte data size\n");

									encoding |= (last_two[1] == 'b') << 22;

									break;
							}
						}
						// TODO: we might want to say somewhere that this assembler will only accept divided syntax for ARM assembly rather than unified syntax / UAL, but then we would have to change it to make sure that all immediate values are required to start with '#'
					} else {
						if (starts_with_last_success) {
							printf("Probable nop\n");
						}

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
