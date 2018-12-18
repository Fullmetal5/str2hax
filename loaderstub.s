# Copyright 2017-2018  Dexter Gerig <dexgerig@gmail.com>
# Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
# Copyright      2011  Bernhard Urban <lewurm@gmail.com>
# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

	.section .start,"ax"

start:
	bl getstart
getstart:
	mflr 4 ; subi 4,4,8

	# Disable interrupts, enable FP.
	mfmsr 3 ; rlwinm 3,3,0,17,15 ; ori 3,3,0x2000 ; mtmsr 3 ; isync

	# Move code into place
	lis 3,main@h ; ori 3,3,main@l ; addi 5,3,-4
	addi 4,4,end-start

	li 0,0 ; ori 0,0,0x8000 ; mtctr 0
0:	lwzu 0,4(4) ; stwu 0,4(5) ; bdnz 0b

	# Sync caches on it.
	li 0,0x1000 ; mtctr 0 ; mr 5,3
0:	dcbst 0,5 ; sync ; icbi 0,5 ; addi 5,5,0x20 ; bdnz 0b
	sync ; isync

	# Go for it!
	mtctr 3 ; bctr
end:
