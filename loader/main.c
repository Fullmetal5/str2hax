// Copyright 2017-2018  Dexter Gerig <dexgerig@gmail.com>
// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "loader.h"

static u8 *const code_buffer = (u8 *)0x90100000;
static u8 *const trampoline_buffer = (u8 *)0x80001800;

static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

static u32 reboot_trampoline[] = {
	0x3c209000, // lis 1,0x9000
	0x60210020, // ori 1,1,0x0020
	0x7c2903a6, // mtctr 1
	0x4e800420  // bctr
};

// Starlet takes the top 12MB of high ram
#define STARLET_AREA_SIZE (12 * 1024 * 1024)

#define LOW_RAM_BASE 0x80000000
#define LOW_RAM_END (LOW_RAM_BASE + 0x017FFFFF)
#define HIGH_RAM_BASE 0x90000000
#define HIGH_RAM_END (HIGH_RAM_BASE + 0x03FFFFFF - STARLET_AREA_SIZE)

static u32 checksum(u8* addr, size_t size)
{
	u32 sum = 0;
	while (size-- != 0)
		sum -= *addr++;
	return sum;
}

static int valid_memory_payload(u32* payload)
{
	u32 size = *payload++;
	s32 stored_checksum = *payload++;
	u32 payload_end = (u32)payload + size;
	
	printf("  start: %x\n", payload);
	printf("  size: %x\n", size);
	printf("  payload_end: %x\n", payload_end);
	printf("  stored_checksum: %x\n", stored_checksum);
	
	// Make sure we don't go off the end of ram trying to calculate the payload checksum.
	if (!(((payload_end > LOW_RAM_BASE) && (payload_end < LOW_RAM_END)) || ((payload_end > HIGH_RAM_BASE) && (payload_end < HIGH_RAM_END)))) {
		printf("  SIZE FAIL.\n");
		return 0;
	}
	
	s32 calculated_checksum = checksum((u8*)payload, size);
	printf("  calculated_checksum: %x\n", calculated_checksum);
	if (stored_checksum == calculated_checksum) {
		printf("  OK.\n");
		return 1;
	}
	
	printf("  CHECKSUM FAIL.\n");
	return 0;
}

u32 EGG = 0x504F4E59;
u32* find_memory_payload()
{
	u32* addr = (u32*)LOW_RAM_BASE;
	while (addr != (u32*)(LOW_RAM_END - 3)) {
		if (*addr++ == EGG && *addr == EGG) {
			printf(" %x:\n", addr + 1);
			if (valid_memory_payload(addr + 1)) {
				//printf("OK.\n");
				return addr + 1;
			}
			//printf("FAIL.\n");
		}
	}
	
	addr = (u32*)HIGH_RAM_BASE;
	while (addr != (u32*)(HIGH_RAM_END - 3)) {
		if (*addr++ == EGG && *addr == EGG) {
			printf(" %x:\n", addr + 1);
			if (valid_memory_payload(addr + 1)) {
				//printf("OK.\n");
				return addr + 1;
			}
			//printf("FAIL.\n");
		}
	}
	
	return NULL;
}

static int decompress_memory_payload(u32* payload)
{
	//payload += 2; // EGG EGG
	u32 size = *payload++; // SIZE
	payload++; // CHECKSUM
	size_t result = tinfl_decompress_mem_to_mem(code_buffer, 0x92000000 - (unsigned int)code_buffer, payload, size, TINFL_FLAG_PARSE_ZLIB_HEADER);
	if (result == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
		return 1;
	}
	sync_after_write(code_buffer, result);
	sync_before_exec(code_buffer, result);
	return 0;
}

int main()
{
	dsp_reset();

	exception_init();

	// Install trampoline at 80001800; some payloads like to jump
	// there to restart.  Sometimes this can even work.
	memcpy(trampoline_buffer, reboot_trampoline, sizeof(reboot_trampoline));
	sync_before_exec(trampoline_buffer, sizeof(reboot_trampoline));

	// Clear interrupt mask.
	write32(0x0c003004, 0);

	// Unlock EXI.
	write32(0x0d00643c, 0);

	video_init();

	printf("savezelda %s\n", version);
	printf("\n");
	printf("Copyright 2017-2018  Dexter Gerig\n");
	printf("Copyright 2011  Rich Geldreich\n");
	printf("Copyright 2008,2009  Segher Boessenkool\n");
	printf("Copyright 2008  Haxx Enterprises\n");
	printf("Copyright 2008  Hector Martin (\"marcan\")\n");
	printf("Copyright 2003,2004  Felix Domke\n");
	printf("\n");
	printf("This code is licensed to you under the terms of the\n");
	printf("GNU GPL, version 2; see the file COPYING\n");
	printf("\n");
	printf("Font and graphics by Freddy Leitner\n");
	printf("\n");
	printf("\n");

	printf("Cleaning up environment... ");

	reset_ios();

	printf("OK.\n");
	
	printf("Finding payload...\n");
	u32* payload = find_memory_payload();
	if (payload == NULL) {
		printf("FAIL.\n");
		printf("We *may* have smashed it on accident.\n");
		printf("Hanging.\n");
		goto hang;
	}
	
	printf("Decompressing network loader... ");
	if (decompress_memory_payload(payload)) {
		printf("FAIL.\n");
		printf("Did you compress the payload correctly?\n");
		printf("Hanging.\n");
		goto hang;
	}
	printf("OK.\n");

	if (valid_elf_image(code_buffer)) {
		printf("Valid ELF image detected.\n");
		void (*entry)() = load_elf_image(code_buffer);
		entry();
		printf("Program returned to loader, hanging.\n");
	} else {
		printf("No valid ELF image detected, hanging.\n");
	}

hang:
	for (;;)
		;
}

void __eabi(){}
