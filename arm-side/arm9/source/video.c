/*
 * This file is part of the DS communication library for the Supercard DSTwo.
 *
 * Copyright 2017 Nebuleon Fumika <nebuleon.fumika@gmail.com>
 *
 * It is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with it.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "video.h"

DTCM_BSS uint16_t* video_main[3];

DTCM_BSS uint16_t* video_sub;

DTCM_BSS bool video_sub_graphics;

uint8_t video_main_current;

/* For each element in this array:
 * true if the given Main Screen buffer is using a palette; false if it's a
 * 16-bit bitmap background. */
static DTCM_BSS bool video_main_use_palette[3];

uint16_t video_main_palette[3][256];

/* Contains the index of the Main Screen buffer for which data was last sent
 * by the Supercard. Tracked separately from video_main_current to allow for
 * skipping flips for two frames sent for the same buffer, even if the first
 * frame was not yet shown. */
static uint8_t video_main_last;

/* The number of meaningful elements in the arrays 'flip_target_buffer' and
 * 'flip_notify'. */
static size_t pending_flip_count;

/* For each element in this array:
 * The index of the Main Screen buffer to be flipped to at a following
 * VBlank. */
static uint8_t flip_target_buffer[3];

/* true if a screen swap change request is pending. */
static bool pending_swap;

/* The new screen swap state to be set in the next VBlank. */
static bool new_swap_state;

PrintConsole sub_text;

u32 vramDefault()  /* Overrides weak vramDefault in libnds */
{
	while (!(REG_DISPSTAT & DISP_IN_VBLANK));

	// 1. First, clear the palettes, because any palette mode displaying any
	// tile will display black.
	dmaFillWords(0, BG_PALETTE, 512);
	dmaFillWords(0, BG_PALETTE_SUB, 512);
	dmaFillWords(0, SPRITE_PALETTE, 512);
	dmaFillWords(0, SPRITE_PALETTE_SUB, 512);
	dmaFillWords(0, OAM, 1024);
	dmaFillWords(0, OAM_SUB, 1024);

	// 2. Then set each screen to display a tiled mode with these palettes
	// while video_init clears the bitmap backgrounds that will actually be
	// used. This ensures that we show black for as long as possible.
	REG_BG0CNT = BG_32x32 | BG_TILE_BASE(0) | BG_PRIORITY_1;
	videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	REG_BG0CNT_SUB = BG_32x32 | BG_TILE_BASE(0) | BG_PRIORITY_1;
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);

	vramSetBankD(VRAM_D_LCD);
	vramSetBankE(VRAM_E_LCD);
	vramSetBankF(VRAM_F_LCD);
	vramSetBankG(VRAM_G_LCD);
	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_LCD);

	return 0;
}

static void copy_palette(uint8_t buffer)
{
	size_t i;
	for (i = 0; i < 256; i++)
		BG_PALETTE[i] = video_main_palette[buffer][i];
}

void set_main_buffer(uint8_t buffer)
{
	/* MAIN can have three backgrounds, because it can manage up to 512 KiB
	 * of memory. Use them for page-swapping.
	 * To display buffers that use palettes, since REG_DISPCNT cannot be used
	 * in bitmap modes to add 64 KiB offsets to the VRAM base, we remap banks
	 * to 0x06000000, making sure that two banks are never mapped there at the
	 * same time. */
	switch (buffer) {
	case 0:
	default:
		if (video_main_use_palette[0]) {
			if (video_main_use_palette[1])
				vramSetBankB(VRAM_B_MAIN_BG_0x06040000);
			if (video_main_use_palette[2])
				vramSetBankD(VRAM_D_MAIN_BG_0x06060000);
			vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
			videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_SCREEN_BASE(0));
			copy_palette(0);
		} else
			videoSetMode(MODE_FB0);
		break;
	case 1:
		if (video_main_use_palette[1]) {
			if (video_main_use_palette[0])
				vramSetBankA(VRAM_B_MAIN_BG_0x06020000);
			if (video_main_use_palette[2])
				vramSetBankD(VRAM_D_MAIN_BG_0x06060000);
			vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
			videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_SCREEN_BASE(0));
			copy_palette(1);
		} else
			videoSetMode(MODE_FB1);
		break;
	case 2:
		if (video_main_use_palette[2]) {
			if (video_main_use_palette[0])
				vramSetBankA(VRAM_B_MAIN_BG_0x06020000);
			if (video_main_use_palette[1])
				vramSetBankB(VRAM_B_MAIN_BG_0x06040000);
			vramSetBankD(VRAM_B_MAIN_BG_0x06000000);
			videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_SCREEN_BASE(0));
			copy_palette(2);
		} else
			videoSetMode(MODE_FB3);
		break;
	}
	video_main_current = buffer;
}

void set_main_buffer_palette(uint8_t buffer, bool value)
{
	switch (buffer) {
	case 0:
	default:
		if (value) {
			vramSetBankA(VRAM_A_MAIN_BG_0x06020000);
			video_main[0] = (uint16_t*) 0x06020000;
		} else {
			vramSetBankA(VRAM_A_LCD);
			video_main[0] = VRAM_A;
		}
		break;
	case 1:
		if (value) {
			vramSetBankB(VRAM_B_MAIN_BG_0x06040000);
			video_main[1] = (uint16_t*) 0x06040000;
		} else {
			vramSetBankB(VRAM_B_LCD);
			video_main[1] = VRAM_B;
		}
		break;
	case 2:
		if (value) {
			vramSetBankD(VRAM_D_MAIN_BG_0x06060000);
			video_main[2] = (uint16_t*) 0x06060000;
		} else {
			vramSetBankD(VRAM_D_LCD);
			video_main[2] = VRAM_D;
		}
		break;
	}
	video_main_use_palette[buffer] = value;
}

void apply_pending_flip()
{
	if (pending_flip_count > 0) {
		size_t i;
		set_main_buffer(flip_target_buffer[0]);
		add_pending_send(PENDING_SEND_VIDEO_DISPLAYED);
		for (i = 1; i < pending_flip_count; i++) {
			flip_target_buffer[i - 1] = flip_target_buffer[i];
		}
		pending_flip_count--;
	}
}

void add_pending_flip(unsigned int buffer)
{
	if (buffer != video_main_last
	 && pending_flip_count < sizeof(flip_target_buffer) / sizeof(flip_target_buffer[0])) {
		flip_target_buffer[pending_flip_count] = buffer;
		pending_flip_count++;
		video_main_last = buffer;
	}
}

void apply_pending_swap()
{
	if (pending_swap) {
		pending_swap = false;
		if (new_swap_state)
			powerOn(POWER_SWAP_LCDS);
		else
			powerOff(POWER_SWAP_LCDS);
	}
}

void set_pending_swap(bool swap)
{
	new_swap_state = swap;
	pending_swap = true;
	if (REG_DISPSTAT & DISP_IN_VBLANK) {
		apply_pending_swap();
	}
}

void video_init()
{
	/* Set up VRAM banks A, B and D for Main Screen buffers FB0, FB1 and FB3
	 * and clear them to black. */
	set_main_buffer_palette(0, false);
	dmaFillWords(0x80008000, video_main[0], SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
	set_main_buffer_palette(1, false);
	dmaFillWords(0x80008000, video_main[1], SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));
	set_main_buffer_palette(2, false);
	dmaFillWords(0x80008000, video_main[2], SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));

	/* Start displaying the first one straight away. */
	set_main_buffer(0);

	/* Clear VRAM bank C to black. We're mapping it to the LCD just so we can
	 * access it, but it will be Sub background memory with bitmap base 0 when
	 * it's actually used for video. Those two are at different addresses! */
	vramSetBankC(VRAM_C_LCD);
	video_sub = BG_BMP_RAM_SUB(0);
	dmaFillWords(0x80008000, VRAM_C, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16));

	/* Set SUB background 2 as a 256x256 (131072-byte) extended rotation
	 * bitmap background with no rotation or scaling. It starts at VRAM_SUB +
	 * 0 * 16 KiB. */
	REG_BG2CNT_SUB = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY_1;
	REG_BG2PA_SUB = 1 << 8;  /* Identity scaling */
	REG_BG2PD_SUB = 1 << 8;  /* Identity scaling */
	REG_BG2PB_SUB = 0;  /* No rotation */
	REG_BG2PC_SUB = 0;  /* No rotation */
	REG_BG2Y_SUB = 0;  /* Rotation reference Y coordinate */
	REG_BG2X_SUB = 0;  /* Rotation reference X coordinate */

	/* When a MAIN background is shown as an indexed color bitmap, it becomes
	 * like a 256x256 (65536-byte) extended rotation bitmap background, using
	 * no rotation or scaling. Any buffer will be mapped to background 2, and
	 * then the bitmap base in the video mode will need to be adjusted. */
	REG_BG2CNT = BG_BMP8_256x256 | BG_MAP_BASE(0) | BG_PRIORITY_1;
	REG_BG2PA = 1 << 8;  /* Identity scaling */
	REG_BG2PD = 1 << 8;  /* Identity scaling */
	REG_BG2PB = 0;  /* No rotation */
	REG_BG2PC = 0;  /* No rotation */
	REG_BG2Y = 0;  /* Rotation reference Y coordinate */
	REG_BG2X = 0;  /* Rotation reference X coordinate */

	/* Initialise the console at sub_text. Bank H must be mapped at its proper
	 * location for the graphics to be loaded there. */
	vramSetBankH(VRAM_H_SUB_BG);
	consoleInit(&sub_text, /* layer */ 0, BgType_Text4bpp, BgSize_T_256x256,
		/* map base */ 0 /* * 2 KiB */, /* tile base */ 1 /* * 16 KiB */,
		/* main: */ false, /* load graphics */ true);

	vramSetBankE(VRAM_E_LCD);
	vramSetBankF(VRAM_F_LCD);
	vramSetBankG(VRAM_G_LCD);
	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_LCD);

	set_sub_graphics();
}

void set_sub_graphics()
{
	if (!video_sub_graphics) {
		/* While bank C is used for Sub graphics, unmap bank H. Unmap it first
		 * so that banks C and H aren't mapped to the same place at once. */
		vramSetBankH(VRAM_H_LCD);
		vramSetBankC(VRAM_C_SUB_BG_0x06200000);
		videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
		video_sub_graphics = true;
	}
}

void set_sub_text()
{
	if (video_sub_graphics) {
		/* While bank H is used for Sub text, unmap bank C. Unmap it first
		 * so that banks C and H aren't mapped to the same place at once. */
		vramSetBankC(VRAM_C_LCD);
		vramSetBankH(VRAM_H_SUB_BG);
		videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
		video_sub_graphics = false;
	}
}
