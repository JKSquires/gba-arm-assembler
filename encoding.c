uint32_t unsupportedInstruction(uint32_t, char *, bool, struct Label *, unsigned long, Inst *i) {
	printf("Unsupported instruction: ");
	lineIssue(i->line);

	return 0;
}

uint32_t handleShift(uint8_t oprnd_i, Inst *i) {
	struct InstructionBlock block = i->blocks[oprnd_i];
	uint32_t encoding = 0;

	if (block.type & SHIFT) {
		encoding |= (block.type & BLOCK_TYPE_SHIFT_MASK) & 0x7F;

		if ((block.type & BLOCK_TYPE_SHIFT_MASK) != RRX) {
			char *val_start = block.start;
			for (; val_start < block.start + 4 && *val_start != '\n' && *val_start != ';'; val_start++);
			val_start = skipWhitespace(val_start);

			switch (block.type & BLOCK_TYPE_MASK) {
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

uint32_t ldrStr(uint32_t inst_offset, char *oprnd1_start, bool h_sh_sb, struct Label *labels, unsigned long label_tot, Inst *i) {
	uint32_t encoding = 0;
	bool p = 0;
	bool u = 1;
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

	if (i->blocks[1].type == MEM_REG) {
		uint8_t n_reg_data = readReg(i->blocks[1].start, 2, false, i);
		if (n_reg_data & REG_READ_ERR)
			return 0;
		encoding |= (n_reg_data & 0xF) << 16;

		if (i->block_count >= 3) {
			if (i->blocks[2].type & REG) {
				char *c2 = i->blocks[2].start;
				if (*c2 == '-') {
					c2++;
					u = 0;
				}
				uint8_t m_reg_data = readReg(c2, 3, false, i);
				if (m_reg_data & REG_READ_ERR)
					return 0;
				encoding |= (1 << 25) | m_reg_data;

				if (i->blocks[2].type & MEM) {
					p = 1;

					for (; *c2 != '!' && *c2 != '\n' && *c2 != ';'; c2++);
					if (*c2 == '!') w = 1;
				}

				if (!h_sh_sb) {
					if (i->block_count >= 4) {
						encoding |= handleShift(3, i);
					}

					if (i->block_count > 4) {
						printf("Load/store register (register) cannot have more than 4 operands: ");
						lineIssue(i->line);
					}
				} else if (i->block_count > 3) {
					printf("Load/store halfword/signed halfword/signed byte (register) cannot have more than 3 operands: ");
					lineIssue(i->line);
				}
			} else if (i->blocks[2].type & IMM) {
				uint32_t constant = readConst(i->blocks[2].start, i);
				if (constant >= 1 << 31) {
					constant = 0 - constant;
					u = 0;
				}

				if (constant > (0xFF | (0xF00 * !h_sh_sb))) {
					printf("Destination address offset is too large for immediate: ");
					lineIssue(i->line);
					return 0;
				}

				if (!h_sh_sb) {
					encoding |= constant & 0xFFF;
				} else {
					encoding |= (1 << 22) | ((constant & 0xF0) << 8) | (constant & 0x0F);
				}

				if (i->blocks[2].type & MEM) {
					p = 1;

					char *c = i->blocks[2].start;
					for (; *c != '!' && *c != '\n' && *c != ';'; c++);
					if (*c == '!') w = 1;
				}

				if (i->block_count > 3) {
					printf("Load/store register (immediate) cannot have more than 3 operands: ");
					lineIssue(i->line);
				}
			} else {
				printf("Unrecognized load/store instruction: ");
				lineIssue(i->line);
				return 0;
			}
		} else if (h_sh_sb) {
			encoding |= 1 << 22;
		}
	} else if (i->blocks[1].type == LBL && *(i->blocks[0].start) == 'l') {
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

		if (val > (0xFF | (0xF00 * !h_sh_sb))) {
			printf("Destination label is too far from load/store instruction to use literal: ");
			lineIssue(i->line);
			return 0;
		}

		encoding |= 0x001F0000;
		if (!h_sh_sb) {
			encoding |= val;
		} else {
			encoding |= ((val & 0xF0) << 8) | (val & 0x0F);
		}

		p = 1;
	} else {
		printf("Unrecognized load/store instruction: ");
		lineIssue(i->line);
		return 0;
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

	// FIXME: create a more robust instruction-building system for pseudo-instructions
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

uint32_t und(uint32_t, char *oprnd1_start, bool, struct Label *, unsigned long, Inst *i) {
	uint32_t encoding = 0;
	uint16_t expr = 0;
	if (*oprnd1_start != '\n' && *oprnd1_start != ';') {
		uint32_t constant = readConst(oprnd1_start, i);
		if (constant > 0xFFFF) {
			printf("Generate undefined instruction with expr requires that expr <= 0xFFFF: ");
			lineIssue(i->line);
			return 0;
		}
		expr = (uint16_t)constant;
	}

	return encoding | ((expr & 0xFFF0) << 4) | expr & 0xF;
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
	encodings[15] =	(InstEncoding){ldrStr,					"ldr",		0x04100000,	3,	false,	DATA_SIZE};
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
	encodings[43] =	(InstEncoding){und,						"und",		0x07F000F0,	3,	false,	COND_ONLY};
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
