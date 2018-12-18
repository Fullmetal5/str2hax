#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h> //lazy cop out for hex -> int conversion

void writeStringToFile(char *filename, char *input){
	FILE *output = fopen(filename, "w");
	if (!output){
		printf("Failed to open %s for writing\n", filename);
		return;
	}
	fwrite(input, strlen(input), 1, output);
	fclose(output);
}

void do_k(int k, int size, char *filename){
	int x = 0;
	if (size){
		x = 1 << k;
	}else{
		x = (1 << (k - 1)) + 1;
	}
	x = (x * 9) - 8;
	printf("Decimal size: %d\n", x);
	
	char* input = (char*)malloc(x + 1);
	
	if (!input){
		printf("Couldn't malloc(%d) bytes\n", x);
		return;
	}
	
	memset(input, '0', x);
	input[x] = 0x00;
	
	writeStringToFile(filename, input);
	
	free(input);
}

void do_c(char *inputFilename, char *outputFilename){
	mpz_t trans;
	mpz_init(trans);
	
	FILE *input = fopen(inputFilename, "r");
	if (!input){
		printf("Couldn't open %s for input file\n", inputFilename);
		return;
	}
	fseek(input, 0L, SEEK_END);
	size_t length = ftell(input);
	rewind(input);
	
	uint64_t* bytes = malloc(length + 8);
	if (!bytes){
		printf("Failed to allocate %ld bytes!\n", length);
		return;
	}
	uint64_t* reversedBytes = malloc(length + 8);
	if (!reversedBytes){
		printf("Failed to allocate %ld bytes!\n", length);
		return;
	}
	fread(bytes, 1, length, input);
	bytes[length/8] = 0x00000000;
	
	fclose(input);
	
	for (int i = 0; i < length/8; i++){
		reversedBytes[i] = bytes[(length/8)-i-1];
	}
	
	free(bytes);
	bytes = NULL;
	
	reversedBytes[length/8] = 0x00000000;
	
	/*input = fopen("test.bin", "wb");
	fwrite(reversedBytes, strlen(reversedBytes), 1, input);
	fclose(input);*/
	
	mpz_set_str(trans, (char*)reversedBytes, 16);
	
	free(reversedBytes);
	reversedBytes = NULL;
	
	FILE *output = fopen(outputFilename, "w");
	if (!output){
		printf("Couldn't open %s for output file\n", outputFilename);
		return;
	}
	
	mpz_out_str(output, 10, trans);
	
	fclose(output);
	
	mpz_clear(trans);
}

void do_s(int target_k){
	printf("k min d / max d (inclusive)\n");
	for (int k = 1, y = 1; k <= target_k; k++, y <<= 1){
		printf("%d %d / %d\n", k, ((y + 1) * 9) - 8, ((y << 1) * 9) - 8);
	}
}

int main(int argc, char *argv[]){
	
	if (argc == 1){
		printf("Usage: %s [operation] [operation args]\n", argv[0]);
		printf("operations:\n");
		printf("\tk min/max # output.bin\t\tOutput a file with the correct number of decminals to reach a k=# allocation\n");
		printf("\t\t\t\t\tmin/max change whether the decmial is just enough or just below limit\n");
		printf("\n");
		printf("\tc input.bin output.bin\t\tConverts binary input file to decimal needed\n");
		printf("\n");
		printf("\ts\t\t\t\tDisplays decminal lengths for all possible values of k\n");
		return 0;
	}
	
	char operation = *argv[1];
	
	switch (operation){
		case 'k':
			if (argc != 5){
				printf("Invalid arguments\n");
				return 0;
			}
			do_k(atoi(argv[3]), strncmp("min", argv[2], 4) ? 1 : 0, argv[4]);
			return 0;
		case 'c':
			if (argc != 4){
				printf("Invalid arguments\n");
				return 0;
			}
			do_c(argv[2], argv[3]);
			break;
		case 's':
			if (argc != 3){
				printf("Invalid arguments\n");
				return 0;
			}
			do_s(atoi(argv[2]));
			break;
		default:
			printf("Unknown operation\n");
			return 0;
	}
	
	return 0;
}
