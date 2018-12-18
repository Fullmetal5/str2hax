#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage:\n");
		printf("\t%s\tin.bin\tout.bin\n", argv[0]);
		return 0;
	}
	
	FILE *input = fopen(argv[1], "rb");
	if (!input) {
		printf("Error opening %s\n", argv[1]);
		return 0;
	}
	
	FILE *output = fopen(argv[2], "wb");
	if (!output) {
		printf("Error opening %s\n", argv[2]);
		return 0;
	}
	
	fseek(input, 0L, SEEK_END);
	size_t size = ftell(input);
	rewind(input);
	
	uint8_t* bytes = malloc(size);
	if (!bytes){
		printf("Failed to allocate %ld bytes!\n", size);
		return 0;
	}
	
	fread(bytes, 1, size, input);
	
	for (size_t i = 0; i != size; i+=4) {
		uint8_t a = bytes[i];
		uint8_t r = bytes[i+1];
		uint8_t g = bytes[i+2];
		uint8_t b = bytes[i+3];
		bytes[i] = r;
		bytes[i+1] = g;
		bytes[i+2] = b;
		bytes[i+3] = a;
	}
	
	fwrite(bytes, size, 1, output);
	
	free(bytes);
	bytes = NULL;
	
	fclose(input);
	input = NULL;
	fclose(output);
	output = NULL;
	
	return 0;
}
