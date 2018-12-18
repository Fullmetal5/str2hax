# Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

CROSS ?= powerpc-eabi-


ifeq ($(origin CC), default)
	CC := $(CROSS)gcc -m32
endif
ifeq ($(origin LD), default)
	LD := $(CROSS)ld
endif
OBJCOPY ?= $(CROSS)objcopy


CFLAGS := -g -Wall -W -O2 -ffreestanding -std=gnu99 -Wstrict-aliasing=2 \
	-mno-eabi -mno-sdata -mcpu=750


targets := loader.bin

objs := crt0.o main.o string.o video.o ios.o elf.o sync.o font.o \
	console.o exception.o exception_2200.o time.o version.o tinfl.o
ppms := font.ppm


ifeq ($(V),1)
	Q :=
else
	Q := @
	MAKEFLAGS += --no-print-directory
endif


all: $(targets)

$(targets): %.bin: %.elf
	@echo "  OBJCOPY   $@"
	$(Q)$(OBJCOPY) -O binary $< $@

elfs := $(targets:.bin=.elf)
$(elfs): %.elf: %.lds $(objs)
	@echo "  LINK      $@"
	$(Q)$(LD) $(LDFLAGS) -n -T $^ -o $@

%.o: %.c loader.h
	@echo "  COMPILE   $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

crt0.o exception_2200.o: %.o: %.s
	@echo "  ASSEMBLE  $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

version.c: .version
	@echo "  VERSION   $@"
	$(Q)echo "const char version[] = \"`cat $^` (`whoami`@`hostname -s`)\";" > $@

.version: FORCE

$(ppms): %.ppm: %.png
	@echo "  PPM       $@"
	$(Q)convert $< $@

font.c: %.c: %.ppm font2c.pl
	@echo "  FONT2C    $@"
	$(Q)perl font2c.pl < $*.ppm > $@

FORCE:

clean:
	rm -rf $(objs) $(targets) $(elfs) font.c version.c
