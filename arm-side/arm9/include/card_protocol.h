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

#ifndef CARD_PROTOCOL_H
#define CARD_PROTOCOL_H

/* Some of this must be synchronised with the MIPS side's card_protocol.h. */

#include <stdbool.h>
#include <stdint.h>

#include "structs.h"

enum arm_side_link_status {
	LINK_STATUS_NONE,
	LINK_STATUS_ESTABLISHED,
	LINK_STATUS_ERROR
};

/* Status of the link between the DS and the Supercard.
 * volatile because it can be modified by the card line interrupt handler, and
 * is then tested in a loop. */
extern volatile DTCM_DATA enum arm_side_link_status link_status;

/* Number of VBlank interrupts seen so far. Used to detect when the Supercard
 * is taking way too long to return replies.
 * volatile because it can be modified by the VBlank interrupt handler, and is
 * then tested in a loop. */
extern volatile DTCM_BSS uint32_t vblank_count;

/* This is a command directly to the Supercard's FPGA, to ask it to reset our
 * version of the FIFO. The MIPS chip won't see this. */
#define FPGA_COMMAND_FIFO_RESET_BYTE 0xE1

/* This is a command directly to the Supercard's FPGA, to ask it about the
 * status of our FIFO. The MIPS chip won't see this. */
#define FPGA_COMMAND_FIFO_STATUS_BYTE 0xE0

#define FIFO_STATUS_READ_FULL (1 << 0)
#define FIFO_STATUS_READ_ERROR (1 << 6)
#define FIFO_STATUS_WRITE_ERROR (1 << 7)
#define FIFO_STATUS_LEN_BIT 19
#define FIFO_STATUS_LEN_MASK 0x3FE

/* This is a command directly to the Supercard's FPGA, to ask it to ACTUALLY
 * send us the queued data from the FIFO. The MIPS chip won't see this. */
#define FPGA_COMMAND_FIFO_READ_BYTE 0xE8

/* - - - START SHARED PART - - - */
#define SUPERCARD_HELLO_VALUE UINT32_C(0xA1F15CDC)

#define CARD_COMMAND_HELLO_BYTE 0xCF

#define CARD_COMMAND_VBLANK_BYTE 0xC1

#define CARD_COMMAND_RTC_BYTE 0xC2

#define CARD_COMMAND_INPUT_BYTE 0xC3

#define CARD_COMMAND_AUDIO_CONSUMED_BYTE 0xC4

#define CARD_COMMAND_VIDEO_DISPLAYED_BYTE 0xC5

#define CARD_COMMAND_AUDIO_STATUS_BYTE 0xC6

#define CARD_COMMAND_SEND_QUEUE_BYTE 0xC0

struct __attribute__((packed, aligned (4))) card_command_hello {
	uint8_t byte; /* = CARD_COMMAND_HELLO_BYTE */
	uint8_t video_encodings_supported;
	uint8_t audio_encodings_supported;
	uint8_t reserved[5];
};

struct __attribute__((packed, aligned (4))) card_command_input {
	uint8_t byte; /* = CARD_COMMAND_INPUT_BYTE */
	struct DS_InputState data;
	uint8_t zero[3];
};

struct __attribute__((packed, aligned (4))) card_command_rtc {
	uint8_t byte; /* = CARD_COMMAND_RTC_BYTE */
	struct DS_RTC data;
};

struct __attribute__((packed, aligned (4))) card_command_audio_consumed {
	uint8_t byte; /* = CARD_COMMAND_AUDIO_CONSUMED_BYTE */
	uint8_t zero[5];
	uint16_t count; /* number of samples consumed */
};

struct __attribute__((packed, aligned (4))) card_command_audio_status {
	uint8_t byte; /* = CARD_COMMAND_AUDIO_STATUS_BYTE */
	uint8_t zero[6];
	uint8_t status; /* 1 if audio was started, 0 if it was stopped */
};

struct __attribute__((packed, aligned (4))) card_command_video_displayed {
	uint8_t byte; /* = CARD_COMMAND_VIDEO_DISPLAYED_BYTE */
	uint8_t zero[6];
	uint8_t index; /* buffer index currently displayed on the Main Screen */
};

union card_command {
	uint8_t bytes[8];
	uint16_t halfwords[4];
	uint32_t words[2];
	struct card_command_hello hello;
	struct card_command_input input;
	struct card_command_rtc rtc;
	struct card_command_audio_consumed audio_consumed;
	struct card_command_audio_status audio_status;
	struct card_command_video_displayed video_displayed;
};

struct __attribute__((packed, aligned (4))) card_reply_hello {
	uint32_t hello_value; /* = SUPERCARD_HELLO_VALUE */
	uint8_t video_encodings_supported;
	uint8_t audio_encodings_supported;
	struct {
		/* Non-zero if card_command_audio_status is required by the Supercard when
		 * the Nintendo DS starts or stops audio output.
		 * Zero if the Supercard does not recognise card_command_audio_status. */
		uint8_t audio_status;
	} extensions;
	uint8_t reserved[249];
	uint8_t end_sync[256]; /* 0x00 to 0xFF in sequence, just to check the link */
};

/* These definitions are for the first header word of Supercard send queue
 * data. */

/* What kind of data is being transferred in this reply? */
#define DATA_KIND_BIT            24
#define DATA_KIND_MASK           (0xFF << DATA_KIND_BIT)
#define DATA_KIND_NONE           (0 << DATA_KIND_BIT)
#define DATA_KIND_VIDEO          (1 << DATA_KIND_BIT)
#define DATA_KIND_AUDIO          (2 << DATA_KIND_BIT)
#define DATA_KIND_REQUESTS       (3 << DATA_KIND_BIT)
#define DATA_KIND_TEXT           (4 << DATA_KIND_BIT)
#define DATA_KIND_MIPS_ASSERT    (0xFD << DATA_KIND_BIT)
#define DATA_KIND_MIPS_EXCEPTION (0xFE << DATA_KIND_BIT)
/* Which encoding (compression, backwards compatibility mode, etc.) is being
 * used in this reply? */
#define DATA_ENCODING_BIT        16
#define DATA_ENCODING_MASK       (0xFF << DATA_ENCODING_BIT)
#define DATA_ENCODING(n)         ((uint32_t) (n) << DATA_ENCODING_BIT)
/* How many bytes are meaningful in this reply, after the 1 or 2 header
 * words? */
#define DATA_BYTE_COUNT_BIT      6
#define DATA_BYTE_COUNT_MASK     (0x3FF << DATA_BYTE_COUNT_BIT)
#define DATA_BYTE_COUNT(n)       ((uint32_t) (n) << DATA_BYTE_COUNT_BIT)
/* If set, this bit indicates that no more data is in the Supercard's send
 * queue. The current reply may still be meaningful, if its data kind is not
 * 0. */
#define DATA_END                 (1 << 0)

/* These definitions are for the second header word of video data. */

/* At which pixel in the target buffer does this reply start writing? */
#define VIDEO_PIXEL_OFFSET_BIT   16
#define VIDEO_PIXEL_OFFSET_MASK  (0xFFFF << VIDEO_PIXEL_OFFSET_BIT)
#define VIDEO_PIXEL_OFFSET(n)    ((uint32_t) (n) << VIDEO_PIXEL_OFFSET_BIT)
/* Which engine's screen buffer is to be modified? */
#define VIDEO_ENGINE_BIT   15
#define VIDEO_ENGINE_MASK  (1 << VIDEO_ENGINE_BIT)
#define VIDEO_ENGINE_MAIN  (1 << VIDEO_ENGINE_BIT)
#define VIDEO_ENGINE_SUB   (0 << VIDEO_ENGINE_BIT)
#define VIDEO_ENGINE(n)    ((uint32_t) (n) << VIDEO_ENGINE_BIT)
/* If the Main engine was selected, which buffer is to be written into? */
#define VIDEO_BUFFER_BIT   13
#define VIDEO_BUFFER_MASK  (3 << VIDEO_BUFFER_BIT)
#define VIDEO_BUFFER(n)    ((uint32_t) (n) << VIDEO_BUFFER_BIT)
/* Must the Main Screen buffer be flipped to the buffer in VIDEO_BUFFER_MASK
 * after this bit of video in order to ensure that the new data is shown? */
#define VIDEO_END_FRAME    (1 << 12)

struct __attribute__((packed, aligned (4))) card_reply_mips_assert {
	uint32_t line;
	uint8_t file_len;
	uint8_t text_len;
	/* The first file_len bytes are the file name; the next text_len bytes are
	 * the text of the asserted condition. */
	uint8_t data[502];
};

struct __attribute__((packed, aligned (4))) card_reply_mips_exception {
	uint32_t excode;
	uint32_t at;
	uint32_t v0;
	uint32_t v1;
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t t0;
	uint32_t t1;
	uint32_t t2;
	uint32_t t3;
	uint32_t t4;
	uint32_t t5;
	uint32_t t6;
	uint32_t t7;
	uint32_t s0;
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
	uint32_t s4;
	uint32_t s5;
	uint32_t s6;
	uint32_t s7;
	uint32_t t8;
	uint32_t t9;
	uint32_t gp;
	uint32_t sp;
	uint32_t fp;
	uint32_t ra;
	uint32_t hi;
	uint32_t lo;
	uint32_t c0_epc;  /* Address of the instruction causing the exception */
	uint32_t op;      /* Operation code causing the exception */
	uint32_t next_op; /* Instruction following the one causing the exception */
	uint32_t mapped;  /* 0 if the instructions are not at mapped addresses */
	uint8_t reserved[364];
};

struct __attribute__((packed, aligned (4))) card_reply_requests {
	uint8_t start_audio;   /* 1 if the DS should start audio */
	uint16_t audio_freq;   /*    audio frequency to use */
	uint16_t buffer_size;  /*    audio buffer size, in samples */
	uint8_t is_16bit;      /*    1 if signed 16-bit audio, 0 if unsigned 8-bit */
	uint8_t is_stereo;     /*    1 if stereo, 0 if mono */
	uint8_t stop_audio;    /* 1 if the DS should stop audio */
	uint8_t change_swap;   /* 1 if the DS should modify the screen swapping */
	uint8_t swap_screens;  /*    1 if swapping (Main Screen on the top) */
	uint8_t change_backlight; /* 1 if the DS should modify the screen backlights */
	uint8_t screen_backlights; /*   backlights to turn on, 1 = bottom, 2 = top */
	uint8_t reset;         /* 1 to initiate a reset sequence */
	uint8_t sleep;         /* 1 to make the Nintendo DS sleep */
	uint8_t shutdown;      /* 1 to shut down the Nintendo DS */
	uint8_t reserved[493];
};

union card_reply_4 {
	uint8_t bytes[4];
	uint16_t halfwords[2];
	uint32_t word;
};

union card_reply_512 {
	uint8_t bytes[512];
	uint16_t halfwords[256];
	uint32_t words[128];
};
/* - - - END SHARED PART - - - */

/* Sends the specified byte over the card bus, followed by 7 null bytes.
 * Also sets it up to expect a reply of the given length. */
extern void raw_send_command_byte(uint8_t byte, size_t reply_len);

/* Waits until the card bus is no longer busy after a reply. */
extern void card_finish_reply(void);

/* Sends a card command to the Supercard. First, the Supercard FPGA's FIFO
 * is reset. Then, the command is sent. Then, the function waits until the
 * Supercard FPGA's FIFO contains 'reply_len' bytes. Finally, the Supercard
 * FPGA is instructed to hand over the contents of its FIFO, and the
 * card_read_* functions can be used after that.
 *
 * Not retrieving the reply advances the protocol incorrectly, because the
 * Supercard FPGA's FIFO would desynchronise with the MIPS side. Use the
 * function card_ignore_reply to ignore the reply if you don't want it.
 *
 * In:
 *   command: The command to be sent to the Supercard after resetting the
 *     FIFO.
 *   reply_len: The length of the reply.
 */
extern void card_send_command(const union card_command* command, size_t reply_len);

/* Sends a card command to the Supercard. First, the Supercard FPGA's FIFO
 * is reset. Then, the command is sent. Then, the function waits until the
 * Supercard FPGA's FIFO contains 'reply_len' bytes. Finally, the Supercard
 * FPGA is instructed to hand over the contents of its FIFO, and the
 * card_read_* functions can be used after that.
 *
 * Not retrieving the reply advances the protocol incorrectly, because the
 * Supercard FPGA's FIFO would desynchronise with the MIPS side. Use the
 * function card_ignore_reply to ignore the reply if you don't want it.
 *
 * In:
 *   command: The command byte to be sent to the Supercard after resetting
 *     the FIFO. The other 7 bytes are zero.
 *   reply_len: The length of the reply.
 */
extern void card_send_command_byte(uint8_t byte, size_t reply_len);

/* Reads the next 32-bit value from the card bus.
 *
 * In:
 *   reply_ends: true if card_finish_reply should be called as a convenience
 *     because the caller wants to read the last word; false if not.
 * Returns:
 *   The 32-bit value read.
 */
extern uint32_t card_read_word(bool reply_ends);

/* Reads the next 32-bit values from the card bus.
 *
 * In:
 *   reply_len: The number of bytes of data to be read.
 *   reply_ends: true if card_finish_reply should be called as a convenience
 *     because the caller wants to read the last words; false if not.
 * Out:
 *   reply: A pointer to a data area to which the reply will be written.
 * Assumptions:
 *   'reply_len' bytes are mapped and writable at 'reply'.
 */
extern void card_read_data(size_t reply_len, void* reply, bool reply_ends);

/* Ignores (i.e. reads into nothing, to advance the protocol properly) the
 * remainder of the current reply from the card bus. */
extern void card_ignore_reply(void);

extern void fatal_link_error(const char* format, ...) __attribute__((format (printf, 1, 2), noreturn, cold));

extern void link_establishment_protocol(void);

extern void process_send_queue(void);

#endif /* !CARD_PROTOCOL_H */
