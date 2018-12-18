#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint32_t invert_byteorder(uint32_t data) {
	return (data & 0x000000FF) << 24u | (data & 0x0000FF00) << 8u | (data & 0x00FF0000) >> 8u | (data & 0xFF000000) >> 24u;
}

uint32_t checksum(uint8_t* addr, uint32_t size)
{
	uint32_t sum = 0;
	while (size-- != 0)
		sum -= *addr++;
	return sum;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Usage:\n");
		printf("\t%s EGG\tinput.bin\toutput.bin\n", argv[0]);
		return 0;
	}
	
	FILE* input = fopen(argv[2], "rb");
	if (input == NULL) {
		printf("Error opening %s\n", argv[2]);
		return 0;
	}
	
	FILE* output = fopen(argv[3], "wb");
	if (output == NULL) {
		printf("Error opening %s\n", argv[3]);
		return 0;
	}
	
	fseek(input, 0L, SEEK_END);
	uint32_t size = ftell(input);
	rewind(input);
	
	uint8_t* bytes = malloc(size);
	if (!bytes){
		printf("Failed to allocate %ld bytes!\n", size);
		return 0;
	}
	
	fread(bytes, size, 1, input);
	
	fclose(input);
	input = NULL;
	
	uint32_t chk = checksum(bytes, size);
	
	printf("EGG: %s\n", argv[1]);
	printf("Size: 0x%.8X\n", size);
	printf("Checksum: 0x%.8X\n", chk);
	
	// Convert to big endian
	uint32_t big_size = invert_byteorder(size);
	uint32_t big_chk = invert_byteorder(chk);
	
	fwrite(argv[1], 4, 1, output);
	fwrite(argv[1], 4, 1, output);
	fwrite(&big_size, 4, 1, output);
	fwrite(&big_chk, 4, 1, output);
	fwrite(bytes, size, 1, output);
	
	free(bytes);
	bytes = NULL;
	
	fclose(output);
	output = NULL;
	
	return 0;
}
