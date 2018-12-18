// Copyright 2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


#include <stdarg.h>

#include "loader.h"


static void put(char c)
{
	fb_putc(c);
}


// __umoddi3() and friends are very big, and more general than we need:
// radix is always (very) small, so we can work by much bigger chunks
// than single bits, always.
static int extract_dig(u64 *x, u32 radix)
{
	u32 hi = *x >> 32;
	u32 lo = *x;
	u32 mod = hi % radix;
	hi /= radix;
	u32 n = (mod << 16) | (lo >> 16);
	mod = n % radix;
	n /= radix;
	lo = (mod << 16) | (lo & 0xffff);
	mod = lo % radix;
	lo /= radix;
	lo |= (n << 16);
	*x = ((u64)hi << 32) | lo;
	return mod;
}


// This implements conversions %{0}{number}{l,ll}[%cdsux] only.
// Field length is obeyed for numbers only.
// Always returns 0.

int printf(const char *restrict format, ...)
{
	va_list ap;

	va_start(ap, format);

	while (*format) {
		if (*format != '%') {
			put(*format++);
			continue;
		}
		format++;

		int zero = 0;
		int prec = 0;

		if (*format == '0') {
			zero = 1;
			format++;
		}

		while (*format >= '0' && *format <= '9')
			prec = 10*prec + (*format++ - '0');

		int ll = 0;
		while (*format == 'l') {
			ll++;
			format++;
		}

		int radix = 10;
		int is_signed = 1;

		switch (*format++) {
		case '%':
			put('%');
			break;

		case 'c':
			put(va_arg(ap, int));
			break;

		case 's':
			;
			char *s = va_arg(ap, char *);
			while (*s)
				put(*s++);
			break;

		case 'x':
			radix = 16; // Fallthrough

		case 'u':
			is_signed = 0; // Fallthrough

		case 'd':
			;
			u64 x;
			if (is_signed) {
				if (ll == 0)
					x = va_arg(ap, int);
				else if (ll == 1)
					x = va_arg(ap, long);
				else
					x = va_arg(ap, long long);
			} else {
				if (ll == 0)
					x = va_arg(ap, unsigned int);
				else if (ll == 1)
					x = va_arg(ap, unsigned long);
				else
					x = va_arg(ap, unsigned long long);
			}

			if (is_signed) {
				if ((long long)x < 0)
					x = -x;
				else
					is_signed = 0;
			}

			char hold[22];
			char *hld = &hold[sizeof hold];
			*--hld = 0;

			int len = 0;
			do {
				int dig = extract_dig(&x, radix);
				if (dig >= 10)
					dig += 'a' - 10;
				else
					dig += '0';
				*--hld = dig;
				len++;
			} while (x);
			if (is_signed)
				*--hld = '-';

			while (len < prec) {
				put(zero ? '0' : ' ');
				len++;
			}
			while (*hld)
				put(*hld++);
		}
	}

	va_end(ap);

	return 0;
}
