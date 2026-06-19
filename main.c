#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

//#include <pthread.h>


/*
struct InstructionTextStruct {
	int length;
	char *start_char;
};


void *parseInstruction(void *args) {
	struct InstructionTextStruct *inst_text = (struct InstructionTextStruct *)args;
	for (int i = 0; i < inst_text->length; i++) {
		printf("%c\n", inst_text->start_char[i]);
		fflush(stdout);
		usleep(50000);
	}

	return NULL;
}

*/


enum LineType {
	CODE,
	LABEL,
	DIRECTIVE,
	END
};

struct Line {
	enum LineType type;
	char *start;
};


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
	struct Line *lines = malloc(lines_arr_size);

	unsigned long line_num = 0;
	lines[0] = (struct Line){CODE, asm_buffer};
	int comment = 0;
	char *asm_buffer_end = asm_buffer + asm_size;
	for (char *c = asm_buffer; c <= asm_buffer_end; c++) {
		if (*c == '\n') {
			comment = 0;

			if (!(c != asm_buffer_end && (*(c + 1) == '\n' || *(c + 1) == ';'))) {
				if (line_num++ == lines_arr_size) {
					lines_arr_size *= 2;
					lines = realloc(lines, lines_arr_size);
				}

				if (c != asm_buffer_end) {
					lines[line_num] = (struct Line){CODE, c + 1};
					if (*(c + 1) == '@') {
						lines[line_num].type = DIRECTIVE;
					}
				}
			}
		}

		if (!comment) {
			if (*c == ':') {
				lines[line_num].type = LABEL;
			} else if (*c == ';') {
				comment = 1;
			}
		}
	}
	lines[line_num] = (struct Line){END, NULL};


	printf("CODE: %d, LABEL: %d, DIRECTIVE: %d, END: %d\n---\n\n", CODE, LABEL, DIRECTIVE, END);
	for (int i = 0; i < line_num; i++) {
		printf("%d:\t", lines[i].type);

		for (char *c = lines[i].start; *c != '\n' && *c != ';'; c++) {
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


	free(lines);
	free(asm_buffer);

	/*
	rewind(asm_file);

	while ((c = fgetc(asm_file)) != EOF) {
		if (c != '\n') {
			printf("%c", c);
		} else {
			printf("\nnewline\n");
		}
	}
	*/

	/*
	struct InstructionTextStruct inst_text1 = {5, "Hello"};
	struct InstructionTextStruct inst_text2 = {6, "World!"};

	printf("Single-thread:\n");
	parseInstruction(&inst_text1);
	parseInstruction(&inst_text2);

	printf("\nMulti-thread:\n");
	// note that for the actual instruction implementation, it might be nice for us to just stick all the threads into an array so we can iterate through easily
	pthread_t t1;
	pthread_t t2;
	int t1_ret = pthread_create(&t1, NULL, parseInstruction, &inst_text1);
	int t2_ret = pthread_create(&t2, NULL, parseInstruction, &inst_text2);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	*/

	return 0;
}
