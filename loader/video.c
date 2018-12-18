// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Copyright 2003-2004  Felix Domke <tmbinc@elitedvb.net>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "loader.h"

extern u8 console_font_10x16x4[];

#define FONT_XSIZE  10
#define FONT_YSIZE  16
#define FONT_XGAP   0
#define FONT_YGAP   2

static struct {
	u32 xres, yres, stride;

	u32 cursor_x, cursor_y;

	u32 border_left, border_right, border_top, border_bottom;
} fb;

static void fb_write(u32 offset, u32 x)
{
//	write32(0x00f00000 + offset, x);
	u32 *p = (u32 *)(0x80f00000 + offset);
	*p = x;
	sync_after_write(p, 4);
}

static u32 fb_read(u32 offset)
{
//	return read32(0x00f00000 + offset);
	u32 *p = (u32 *)(0x80f00000 + offset);

	return *p;
}

static void fb_clear_lines(u32 top, u32 lines)
{
	u32 x, y;
	u32 offset;

	offset = fb.stride * top;

	for (y = 0; y < lines; y++) {
		for (x = 0; x < fb.xres/2; x++)
			fb_write(offset + 4*x, 0x00800080);

		offset += fb.stride;
	}
}

static void fb_scroll_line(void)
{
	u32 x, y;
	u32 offset, delta;
	u32 lines = FONT_YSIZE + FONT_YGAP;

	offset = fb.stride * fb.border_top;
	delta = fb.stride * lines;

	for (y = fb.border_top; y < fb.yres - lines; y++) {
		for (x = 0; x < fb.xres/2; x++)
			fb_write(offset + 4*x, fb_read(offset + 4*x + delta));

		offset += fb.stride;
	}

	fb_clear_lines(fb.yres - lines, lines);

	fb.cursor_y -= lines;
}

static void fb_drawc(u32 x, u32 y, u8 c)
{
	if (c < 0x20 || c > 0x7f)
		c = 0x7f;
	c -= 0x20;

	u32 offset = fb.stride*y + 2*x;
	u8 *font = &console_font_10x16x4[c * FONT_XSIZE * FONT_YSIZE / 2];

	u32 ax, ay;
	for (ay = 0; ay < FONT_YSIZE; ay++) {
		for (ax = 0; ax < FONT_XSIZE / 2; ax++) {
			u8 bits = *font++;
			u32 nybh = bits & 0xf0;
			u32 nybl = bits & 0x0f;
			u32 q = 0x00800080;
			q |= (nybh << 24) | (nybh << 20);
			q |= (nybl << 12) | (nybl << 8);
			fb_write(offset + 4*ax, q);
		}
		offset += fb.stride;
	}
}

void fb_putc(char c)
{
	switch (c) {
	case '\n':
		fb.cursor_y += FONT_YSIZE + FONT_YGAP; // Fallthrough

	case '\r':
		fb.cursor_x = fb.border_left;
		break;

	default:
		fb_drawc(fb.cursor_x, fb.cursor_y, c);
		fb.cursor_x += FONT_XSIZE + FONT_XGAP;
		if ((fb.cursor_x + FONT_XSIZE) > fb.border_right) {
			fb.cursor_y += FONT_YSIZE + FONT_YGAP;
			fb.cursor_x = fb.border_left;
		}
	}

	if (fb.cursor_y + FONT_YSIZE >= fb.border_bottom)
		fb_scroll_line();
}


static void fb_init(u32 xres, u32 yres, u32 stride)
{
	fb.xres = xres;
	fb.yres = yres;
	fb.stride = stride;

	fb.border_left = 30;
	fb.border_top = 30;
	fb.border_right = fb.xres - 30;
	fb.border_bottom = fb.yres - 30;

	fb.cursor_x = fb.border_left;
	fb.cursor_y = fb.border_top;

	fb_clear_lines(0, fb.yres);
}

void video_init(void)
{
	// read VTR register to determine linecount and mode
	u32 vtr = read16(0x0c002000);
	u32 lines = vtr >> 4;

	if ((vtr & 0x0f) > 10) {	// progressive
		// set framebuffer position
		write32(0x0c00201c, 0x00f00000);
		write32(0x0c002024, 0x00f00000);
	} else {			//interlaced
		lines *= 2;

		u32 vto = read32(0x0c00200c);
		u32 vte = read32(0x0c002010);

		// set framebuffer position
		// try to figure out the interlacing order
		if ((vto & 0x03ff) < (vte & 0x03ff)) {
			write32(0x0c00201c, 0x00f00000);
			write32(0x0c002024, 0x00f00000 + 2*640);
		} else {
			write32(0x0c00201c, 0x00f00000 + 2*640);
			write32(0x0c002024, 0x00f00000);
		}
	}

	fb_init(640, lines, 2*640);
}
