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

#include <inttypes.h>
#include <nds.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "audio.h"
#include "audio_encoding_0.h"
#include "card_protocol.h"
#include "common_ipc.h"
#include "main.h"
#include "mips_assert.h"
#include "mips_except.h"
#include "requests.h"
#include "text_encoding_0.h"
#include "video.h"
#include "video_encoding_0.h"

/* (From GBATEK)
40001A4h - NDS7/NDS9 - ROMCTRL - Gamecard Bus ROMCTRL (R/W)
  Bit   Expl.
  0-12  KEY1 length part1 (0-1FFFh) (forced min 08F8h by BIOS)
  13    KEY2 encrypt data (0=Disable, 1=Enable KEY2 Encryption for Data)
  14     "SE" Unknown? (usually same as Bit13)
  15    KEY2 Apply Seed   (0=No change, 1=Apply Encryption Seed) (Write only)
  16-21 KEY1 length part2 (0-3Fh)   (forced min 18h by BIOS)
  22    KEY2 encrypt cmd  (0=Disable, 1=Enable KEY2 Encryption for Commands)
  23    Data-Word Status  (0=Busy, 1=Ready/DRQ) (Read-only)
  24-26 Data Block size   (0=None, 1..6=100h SHL (1..6) bytes, 7=4 bytes)
  27    Transfer CLK rate (0=6.7MHz=33.51MHz/5, 1=4.2MHz=33.51MHz/8)
  28    Secure Area Mode  (0=Normal, 1=Other)
  29     "RESB" Unknown (always 1 ?) (not read/write-able)
  30     "WR"   Unknown (always 0 ?) (read/write-able)
  31    Block Start/Status (0=Ready, 1=Start/Busy) (IRQ See 40001A0h/Bit14)
*/
#define ROMCTRL_USUAL_FLAGS 0xA0180010

#define ARM_VIDEO_ENCODINGS 1
#define ARM_AUDIO_ENCODINGS 1

#define VBLANK_LAG_MAX 5

volatile DTCM_DATA enum arm_side_link_status link_status = LINK_STATUS_NONE;

volatile DTCM_BSS uint32_t vblank_count;

#ifdef CARD_PROTOCOL_DIAGNOSTICS
/* Header byte of the latest command. */
static uint8_t command_byte;

/* Bytes received from the latest command. */
static uint16_t command_bytes;

/* Bytes expected from the latest command. */
static uint16_t command_bytes_expected;

/* Header byte of the latest command actually sent through the bus. */
static uint8_t subcommand_byte;

/* Bytes received from the latest command actually sent through the bus. */
static uint16_t subcommand_bytes;

/* Bytes expected from the latest command actually sent through the bus. */
static uint16_t subcommand_bytes_expected;
#endif /* CARD_PROTOCOL_DIAGNOSTICS */

/* VBlank count at the start of the latest command. */
static DTCM_BSS uint32_t command_vblank;

__attribute__((format (printf, 1, 2), noreturn, cold))
void fatal_link_error(const char* format, ...)
{
	va_list ap;

	consoleDemoInit();
	printf("DS-Supercard link error:\n\n");

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	irqDisable(IRQ_VBLANK | IRQ_CARD_LINE);
	fifoSendValue32(FIFO_USER_01, IPC_SET_BACKLIGHT | SET_BACKLIGHT_DATA(0x3));
	audio_stop();

	irqDisable(IRQ_ALL);
	while (1) {
		swiIntrWait(1, 0);
	}
}

static void set_reply_len(uint32_t reply_len)
{
	switch (reply_len) {
	case     0: reply_len = 0; break;
	case     4: reply_len = 7; break;
	case   512: reply_len = 1; break;
	case  1024: reply_len = 2; break;
	default:
		fatal_link_error("Nintendo DS attempting to await\na reply of an invalid length\n\nInvalid length: %" PRIu32, reply_len);
		break;
	}

	REG_ROMCTRL = ROMCTRL_USUAL_FLAGS | CARD_BLK_SIZE(reply_len);
}

/* Sends the specified command over the card bus, setting it up to expect a
 * reply of the given length. */
static void raw_send_command(const union card_command* command, size_t reply_len)
{
	int i;

	REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;
	for (i = 0; i < 8; i++) {
		CARD_COMMAND[i] = command->bytes[i];
	}
	set_reply_len(reply_len);

#ifdef CARD_PROTOCOL_DIAGNOSTICS
	subcommand_byte = command->bytes[0];
	subcommand_bytes = 0;
	subcommand_bytes_expected = reply_len;
#endif
}

void raw_send_command_byte(uint8_t byte, size_t reply_len)
{
	int i;

	REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;
	CARD_COMMAND[0] = byte;
	for (i = 1; i < 8; i++) {
		CARD_COMMAND[i] = 0;
	}
	set_reply_len(reply_len);

#ifdef CARD_PROTOCOL_DIAGNOSTICS
	subcommand_byte = byte;
	subcommand_bytes = 0;
	subcommand_bytes_expected = reply_len;
#endif
}

static void lag_check()
{
	if (vblank_count - command_vblank > VBLANK_LAG_MAX) {
#ifdef CARD_PROTOCOL_DIAGNOSTICS
		fatal_link_error("Supercard did not reply to a\n"
		                 "command for %i frames\n\n"
		                 " Command             Size   Got\n"
		                 " %02" PRIX8 " to Supercard    %5" PRIu16" %5" PRIu16 "\n"
		                 " %02" PRIX8 " to card bus     %5" PRIu16" %5" PRIu16 "\n\n"
		                 "%s\n%s\n",
		                 VBLANK_LAG_MAX, command_byte, command_bytes_expected, command_bytes,
		                 subcommand_byte, subcommand_bytes_expected, subcommand_bytes,
		                 (REG_ROMCTRL & CARD_DATA_READY) ? "More data is available" : "All data has been consumed",
		                 (REG_ROMCTRL & CARD_BUSY) ? "The card bus is busy" : "The card bus is not busy");
#else /* starting !CARD_PROTOCOL_DIAGNOSTICS */
		fatal_link_error("Supercard did not reply to a\ncommand for %i frames", VBLANK_LAG_MAX);
#endif /* [!]CARD_PROTOCOL_DIAGNOSTICS */
	}
}

void card_finish_reply()
{
	while (REG_ROMCTRL & CARD_BUSY)
		lag_check();
}

uint32_t card_read_word(bool reply_ends)
{
	while (!(REG_ROMCTRL & CARD_DATA_READY))
		lag_check();
	uint32_t reply = CARD_DATA_RD;
#ifdef CARD_PROTOCOL_DIAGNOSTICS
	subcommand_bytes += 4;
#endif
	if (reply_ends) card_finish_reply();
	return reply;
}

void card_read_data(size_t reply_len, void* reply, bool reply_ends)
{
	uint32_t* reply_words = (uint32_t*) reply;
	size_t i;

	for (i = 0; i < reply_len / 4; i++) {
		while (!(REG_ROMCTRL & CARD_DATA_READY))
			lag_check();
		reply_words[i] = CARD_DATA_RD;
#ifdef CARD_PROTOCOL_DIAGNOSTICS
		subcommand_bytes += 4;
#endif
	}
	if (reply_ends) card_finish_reply();
}

void card_ignore_reply()
{
	uint32_t romctrl;
	while ((romctrl = REG_ROMCTRL) & CARD_BUSY) {
		if (romctrl & CARD_DATA_READY) {
			CARD_DATA_RD;
#ifdef CARD_PROTOCOL_DIAGNOSTICS
			subcommand_bytes += 4;
#endif
		} else
			lag_check();
	}
}

static void wait_for_fifo(size_t length)
{
	uint32_t data;

	do {
		raw_send_command_byte(FPGA_COMMAND_FIFO_STATUS_BYTE, 4);
		data = card_read_word(true);
#ifdef CARD_PROTOCOL_DIAGNOSTICS
		command_bytes = (data >> FIFO_STATUS_LEN_BIT) & FIFO_STATUS_LEN_MASK;
#endif
	} while (!(data & FIFO_STATUS_READ_FULL)
	      && ((data >> FIFO_STATUS_LEN_BIT) & FIFO_STATUS_LEN_MASK) < length);
}

void card_send_command(const union card_command* command, size_t reply_len)
{
#ifdef CARD_PROTOCOL_DIAGNOSTICS
	command_byte = command->bytes[0];
	command_bytes = 0;
	command_bytes_expected = reply_len;
#endif
	command_vblank = vblank_count;

	raw_send_command_byte(FPGA_COMMAND_FIFO_RESET_BYTE, 4);
	card_ignore_reply();

	raw_send_command(command, 0);
	card_finish_reply();

	wait_for_fifo(reply_len);

	raw_send_command_byte(FPGA_COMMAND_FIFO_READ_BYTE, reply_len);
}

void card_send_command_byte(uint8_t byte, size_t reply_len)
{
#ifdef CARD_PROTOCOL_DIAGNOSTICS
	command_byte = byte;
	command_bytes = 0;
	command_bytes_expected = reply_len;
#endif
	command_vblank = vblank_count;

	raw_send_command_byte(FPGA_COMMAND_FIFO_RESET_BYTE, 4);
	card_ignore_reply();

	raw_send_command_byte(byte, 0);
	card_finish_reply();

	wait_for_fifo(reply_len);

	raw_send_command_byte(FPGA_COMMAND_FIFO_READ_BYTE, reply_len);
}

void link_establishment_protocol()
{
	union card_command command;
	struct card_reply_hello reply;
	size_t i;

	command.hello.byte = CARD_COMMAND_HELLO_BYTE;
	command.hello.video_encodings_supported = ARM_VIDEO_ENCODINGS;
	command.hello.audio_encodings_supported = ARM_AUDIO_ENCODINGS;
	memset(command.hello.reserved, 0, sizeof(command.hello.reserved));

	card_send_command(&command, 512);
	card_read_data(512, &reply, true);

	if (reply.hello_value != SUPERCARD_HELLO_VALUE) {
		fatal_link_error("Supercard sent the wrong magic\nvalue\n\nExpected 0x%08" PRIX32 "\nGot      0x%08" PRIX32, SUPERCARD_HELLO_VALUE, reply.hello_value);
	}

	for (i = 0; i < sizeof(reply.end_sync); i++) {
		if (reply.end_sync[i] != i) {
			fatal_link_error("Initial packet is not properly\nsynchronized\n\nByte %zu is not correct\n\nExpected 0x%02" PRIX8 "\nGot      0x%02" PRIX8, i, (uint8_t) i, reply.end_sync[i]);
		}
	}

	for (i = 0; i < sizeof(reply.reserved); i++) {
		if (reply.reserved[i] != 0) {
			fatal_link_error("Supercard protocol extension\n#%zu not supported", sizeof(reply.extensions) + i);
		}
	}

	if (reply.extensions.audio_status) {
		audio_status_required = true;
	}

	link_status = LINK_STATUS_ESTABLISHED;
}

void process_send_queue()
{
	REG_IME = IME_ENABLE;
	card_send_command_byte(CARD_COMMAND_SEND_QUEUE_BYTE, 512);
	uint32_t header = card_read_word(false);
	REG_IME = IME_DISABLE;
	uint8_t encoding = (header & DATA_ENCODING_MASK) >> DATA_ENCODING_BIT;

	if (!(header & DATA_END))
		add_pending_send(PENDING_SEND_QUEUE);

	switch (header & DATA_KIND_MASK) {
	case DATA_KIND_VIDEO:
	{
		REG_IME = IME_ENABLE;
		uint32_t header_2 = card_read_word(false);
		uint16_t pixel_offset = (header_2 & VIDEO_PIXEL_OFFSET_MASK) >> VIDEO_PIXEL_OFFSET_BIT;
		size_t max_pixels;
		bool is_main = (header_2 & VIDEO_ENGINE_MASK) == VIDEO_ENGINE_MAIN;
		unsigned int buffer = (header_2 & VIDEO_BUFFER_MASK) >> VIDEO_BUFFER_BIT;
		uint16_t* dest;

		if (pixel_offset >= SCREEN_WIDTH * SCREEN_HEIGHT) {
			fatal_link_error("Supercard sent video data that\nexceeds screen boundaries");
		} else if (pixel_offset & 1) {
			/* 4-byte alignment is required by all video encodings to access
			 * VRAM efficiently. Video encoding 0, in particular, absolutely
			 * needs this alignment, because it uses card_read_data to write
			 * 32-bit quantities directly into VRAM. */
			fatal_link_error("Supercard sent video data that\ndoes not start on an even pixel");
		} else if (!is_main && buffer != 0) {
			fatal_link_error("Supercard attempted to use\nmultiple buffering on the\nSub Screen");
		} else if (is_main && buffer > 2) {
			fatal_link_error("Supercard attempted to use\nquadruple buffering on the\nMain Screen");
		}

		if (!is_main)
			set_sub_graphics();

		max_pixels = SCREEN_WIDTH * SCREEN_HEIGHT - pixel_offset;

		switch (encoding) {
		case 0:
			if (is_main)
				set_main_buffer_palette(buffer, false);
			dest = (is_main ? video_main[buffer] : video_sub) + pixel_offset;
			video_encoding_0(header, dest, max_pixels);
			break;
		default:
			fatal_link_error("Supercard sent video data using\nunsupported encoding %" PRIu8, encoding);
			break;
		}

		if (is_main && (header_2 & VIDEO_END_FRAME)) {
			REG_IME = IME_DISABLE;
			add_pending_flip(buffer);
			REG_IME = IME_ENABLE;
		}
		break;
	}

	case DATA_KIND_TEXT:
		REG_IME = IME_ENABLE;
		set_sub_text();

		switch (encoding) {
		case 0:   text_encoding_0(header); break;
		default:
			fatal_link_error("Supercard sent text using\nunsupported encoding %" PRIu8, encoding);
			break;
		}
		break;

	case DATA_KIND_AUDIO:
		switch (encoding) {
		case 0:   audio_encoding_0(header); break;
		default:
			fatal_link_error("Supercard sent audio data using\nunsupported encoding %" PRIu8, encoding);
			break;
		}
		REG_IME = IME_ENABLE;
		break;

	case DATA_KIND_REQUESTS:
		process_requests();
		break;

	case DATA_KIND_MIPS_ASSERT:
		process_assert();
		break;

	case DATA_KIND_MIPS_EXCEPTION:
		process_exception();
		break;

	case DATA_KIND_NONE:
	default:
		REG_IME = IME_ENABLE;
		card_ignore_reply();
		break;
	}
}
