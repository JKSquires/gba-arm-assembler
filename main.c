#include <stdio.h>
#include <unistd.h>

#include <pthread.h>


struct InstructionTextStruct {
	int length;
	char* start_char;
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


int main(int argc, char **argv) {
	FILE *asm_file; // maybe for when we implement including other asm files in a file we should have an array of those files.
	FILE *gba_file;

	char c;

	int line_count = 0;

	if (argc < 2) {
		printf("Please specify input file.\nUsage: %s <input asm file> [output gba file]\n", argv[0]);

		return -1;
	}

	asm_file = fopen(argv[1], "r");

	if (asm_file == NULL) {
		fprintf(stderr, "ERROR: Error opening %s\n", argv[1]);

		return -101;
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

	while ((c = fgetc(asm_file)) != EOF)
		if (c == '\n')
			line_count++;

	printf("Line Count: %d\n\n", line_count);

	rewind(asm_file);

	while ((c = fgetc(asm_file)) != EOF) {
		if (c != '\n') {
			printf("%c", c);
		} else {
			printf("\nnewline\n");
		}
	}

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


	fclose(asm_file);
	fclose(gba_file);
	*/

	return 0;
}
