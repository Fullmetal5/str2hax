// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "loader.h"

// Timebase frequency is bus frequency / 4.  Ignore roundoff, this
// doesn't have to be very accurate.
#define TICKS_PER_USEC (243/4)

static u32 mftb(void)
{
	u32 x;

	asm volatile("mftb %0" : "=r"(x));

	return x;
}

static void __delay(u32 ticks)
{
	u32 start = mftb();

	while (mftb() - start < ticks)
		;
}

void udelay(u32 us)
{
	__delay(TICKS_PER_USEC * us);
}
