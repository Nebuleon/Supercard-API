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

#ifndef __DS2_DS_CARD_PROTOCOL_H__
#define __DS2_DS_CARD_PROTOCOL_H__

#include <stdbool.h>
#include <stdint.h>

#include <ds2/ds.h>

/* Some of this must be synchronised with the ARM side's card_protocol.h. */

enum _mips_side_link_status {
	LINK_STATUS_NONE,
	LINK_STATUS_PENDING_RECV,
	LINK_STATUS_ESTABLISHED
};

#define PENDING_RECV_INPUT   0x00000001
#define PENDING_RECV_RTC     0x00000002
#define PENDING_RECV_ALL     (PENDING_RECV_INPUT | PENDING_RECV_RTC)

/* - - - START SHARED PART - - - */
#define SUPERCARD_HELLO_VALUE UINT32_C(0xA1F15CDC)

#define CARD_COMMAND_HELLO_BYTE 0xCF

#define CARD_COMMAND_VBLANK_BYTE 0xC1

#define CARD_COMMAND_RTC_BYTE 0xC2

#define CARD_COMMAND_INPUT_BYTE 0xC3

#define CARD_COMMAND_AUDIO_CONSUMED_BYTE 0xC4

#define CARD_COMMAND_VIDEO_DISPLAYED_BYTE 0xC5

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
	struct card_command_video_displayed video_displayed;
};

struct __attribute__((packed, aligned (4))) card_reply_hello {
	uint32_t hello_value; /* = SUPERCARD_HELLO_VALUE */
	uint8_t video_encodings_supported;
	uint8_t audio_encodings_supported;
	uint8_t reserved[250];
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
	uint8_t reserved[495];
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

extern void _link_establishment_protocol(const union card_command* command);
extern void _pending_recv_protocol(const union card_command* command);
extern void _main_protocol(const union card_command* command);

/* Sets up the Supercard's card send FIFO for a new reply. */
extern void _start_reply(void);

/* Sends a reply word to the Nintendo DS. This word may be followed by more
 * data if there are no intervening calls to 'start_reply'.
 *
 * In:
 *   reply: The 32-bit quantity to be sent. */
extern void _send_reply_4(uint32_t reply);

/* Sends a reply to the Nintendo DS via DMA. Returns immediately, letting the
 * DMA continue in the background.
 *
 * In:
 *   reply: The reply to be sent.
 *   reply_len: The length of the reply, in bytes. Must be a multiple of 4. */
extern void _send_reply(const void* reply, size_t reply_len);

/* Sends a reply to the Nintendo DS via DMA. Returns immediately, letting the
 * DMA continue in the background. The upper bit of each 16-bit quantity gets
 * set, which allows the video to be opaque on the Nintendo DS. Additionally,
 * fixups are applied for the pixel format used on the given engine.
 *
 * In:
 *   reply: The reply to be sent.
 *   reply_len: The length of the reply, in bytes. Must be a multiple of 4.
 *   engine: DS engine (Main or Sub) whose pixel format is to be used. */
extern void _send_video_reply(const void* reply, size_t reply_len, enum DS_Engine engine);

/* Adds one or more things to the list of things to be sent to the Nintendo
 * DS.
 * The caller must protect calls to this function with a critical section or
 * call it with interrupts disabled. */
extern void _add_pending_send(uint32_t mask);

/* Removes one or more things from the list of things to be sent to the
 * Nintendo DS.
 * The caller must protect calls to this function with a critical section or
 * call it with interrupts disabled. */
extern void _remove_pending_send(uint32_t mask);

#endif /* !__DS2_DS_CARD_PROTOCOL_H__ */
