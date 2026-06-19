#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

struct instruction_text_struct {
	int length;
	char* start_char;
};

void* parseInstruction(void* args) {
	struct instruction_text_struct* inst_text = (struct instruction_text_struct*)args;
	for (int i = 0; i < inst_text->length; i++) {
		printf("%c\n", inst_text->start_char[i]);
		fflush(stdout);
		sleep(1);
	}

	return NULL;
}

int main() {
	struct instruction_text_struct inst_text1 = {5, "Hello"};
	struct instruction_text_struct inst_text2 = {6, "World!"};

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

	return 0;
}
