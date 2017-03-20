/*
 * This file allows the Nintendo DS to receive the next binary to run.
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

    .text
    .align 2

    .arm
    .global arm7_loader
    .type   arm7_loader STT_FUNC
arm7_loader:
    ldr    r0, =0x03810000
    @ Set temporary supervisor, IRQ and system stacks.
    mov    r1, #0x12
    msr    CPSR_fc, r1                 @ select the IRQ stack pointer
    sub    sp, r0, #0x60               @ writing into this SP changes IRQ SP
    mov    r1, #0x13
    msr    CPSR_fc, r1                 @ select the supervisor stack pointer
    sub    sp, r0, #0xA0
    mov    r1, #0x1F
    msr    CPSR_fc, r1                 @ select the system stack pointer
    sub    sp, r0, #0x100
    add    r0, pc, #1                  @ grab the address of the instruction
    bx     r0                          @ 2 away, and branch to it as Thumb

    .thumb

    bl     wait_for_fifo               @ wait for the Supercard to be ready

    ldr    r0, =0x027FFE00             @ r0 = Nintendo DS header address
    movs   r1, #0                      @ r1 = 0
    movs   r2, #23
    lsl    r2, r2, #4                  @ r2 = 368
    bl     card_read_multi_512         @ receive the Nintendo DS header

    ldr    r4, =0x027FFE00             @ r4 (saved) = Nintendo DS header addr
    ldr    r0, [r4, #0x28]             @ r0 = ARM9 RAM address
    ldr    r1, [r4, #0x20]             @ r1 = ARM9 ROM offset
    ldr    r2, [r4, #0x2C]             @ r2 = ARM9 code size
    bl     card_read_multi_512         @ load enough bytes for the ARM9 side

    ldr    r0, [r4, #0x38]             @ r0 = ARM7 RAM address
    ldr    r1, [r4, #0x30]             @ r1 = ARM7 ROM offset
    ldr    r2, [r4, #0x3C]             @ r2 = ARM7 code size
    bl     card_read_multi_512         @ load enough bytes for the ARM7 side

    movs   r0, #0x15
    movs   r1, #0
    movs   r2, #0
    bl     card_read_4                 @ wait for the Supercard's signal to go

    @ Send the ARM9 to its own soft reset.
    @ The ARM9 was running this code:
    @    027FFDF4: swi    0
    @    027FFDF6: bx     lr
    @ -> 027FFDF8: ldr    pc, [pc, #-4]
    @    027FFDFC: .word  0x027FFDF8
    @ Storing 0x027FFDF5 at 0x027FFDFC will send the loop to 'swi 0' in Thumb
    @ mode.
    ldr    r2, =0x027FFDF5
    add    r3, r2, #0x7
    str    r2, [r3, #0]
    @ Then reset us, the ARM7.
    swi    0
    @ The next instruction to be executed is the one found at the location
    @ contained in 0x027FFE34.

    @ void card_set_command(uint8_t header, uint32_t arg1, uint32_t arg2)
    @ Sets the card command bytes to the specified values, but doesn't send
    @ the command on the bus.
    @ In:
    @   header: The first byte to set.
    @   arg1: The second (arg1's MSB) to fifth (arg1's LSB) bytes to set.
    @   arg2: The sixth (arg2 bits 23..16) to eighth (arg2's LSB) bytes to set.
card_set_command:                      @ At address 0080 in the original
    push   {r4}
    ldr    r4, =0x040001A0             @ r4 = address of AUXSPICNT
    movs   r3, #0x80
    neg    r3, r3
    strb   r3, [r4, #0x1]              @ AUXSPICNT high byte = some flags
    strb   r0, [r4, #0x8]              @ CARD_COMMAND[0] = (argument 1) & 0xFF
    lsr    r0, r1, #24
    strb   r0, [r4, #0x9]              @ CARD_COMMAND[1] = ((argument 2) >> 24) & 0xFF
    lsr    r0, r1, #16
    strb   r0, [r4, #0xA]              @ CARD_COMMAND[2] = ((argument 2) >> 16) & 0xFF
    lsr    r0, r1, #8
    strb   r0, [r4, #0xB]              @ CARD_COMMAND[3] = ((argument 2) >> 8) & 0xFF
    strb   r1, [r4, #0xC]              @ CARD_COMMAND[4] = (argument 2) & 0xFF
    lsr    r0, r2, #16
    strb   r0, [r4, #0xD]              @ CARD_COMMAND[5] = ((argument 3) >> 16) & 0xFF
    lsr    r0, r2, #8
    strb   r0, [r4, #0xE]              @ CARD_COMMAND[6] = ((argument 3) >> 8) & 0xFF
    strb   r2, [r4, #0xF]              @ CARD_COMMAND[7] = (argument 3) & 0xFF
    pop    {r4}
    bx     lr

    @ uint32_t card_read_4(uint8_t header, uint32_t arg1, uint32_t arg2)
    @ After setting the command bytes and reply length, reads a reply
    @ consisting of a single word from the card bus.
    @ In:
    @   header: The first byte to set.
    @   arg1: The second (arg1's MSB) to fifth (arg1's LSB) bytes to set.
    @   arg2: The sixth (arg2 bits 23..16) to eighth (arg2's LSB) bytes to set.
    @ Returns:
    @   The last word read from the card bus.
card_read_4:                           @ At address 00E4 in the original
    push   {r4, lr}
    bl     card_set_command
    ldr    r2, =0x040001A4             @ r2 = address of ROMCTRL
    @ In 0xA7180910:
    @ Bit 31 = 1: Start block transfer
    @ Bit 28 = 0: Hold CLK high during gaps
    @ Bit 27 = 0: Transfer CLK rate = 6.7 MHz = 33.51 MHz / 5
    @ Bits 24..26 = 111: Data block size = 4 bytes
    @ Bit 23 = 0: Data word status busy
    @ Bit 22 = 0: Disable KEY2 encryption for commands
    @ Bits 16..21 = 01 1000: KEY1 gap2 length = 0x18
    @ Bit 15 = 0: No KEY2 change
    @ Bit 13 = 0: Disable KEY2 encryption for data
    @ Bits 0..12 = 0 1001 0001 0000: KEY1 gap1 length = 0x910
    ldr    r3, =0xA7180910
    movs   r1, #0x80
    str    r3, [r2, #0]                @ ROMCTRL = our flags;
    ldr    r0, =0x04100010             @ r0 = address of CARD_RD
    lsl    r1, r1, #16                 @ r1 = 0x00800000

label_00f6:
    ldr    r3, [r2, #0]                @ re-read ROMCTRL
    tst    r3, r1                      @ ROMCTRL & CARD_DATA_READY
    beq    label_00fe
    ldr    r4, [r0, #0]                @ r4 = CARD_RD;

label_00fe:
    ldr    r3, [r2, #0]                @ re-read ROMCTRL
    cmp    r3, #0
    blt    label_00f6                  @ bit 31 (sign) is CARD_BUSY

    add    r0, r4, #0                  @ return the last read word
    pop    {r4}
    pop    {r1}
    bx     r1

    @ void card_read_512(void* dst, size_t n)
    @ After setting the command bytes and reply length, reads a reply
    @ consisting of up to 512 bytes to 'dst'.
    @ In:
    @   n: The number of bytes to be read. Bytes beyond this number are read
    @     but ignored.
    @ Out:
    @   dst: Pointer to the buffer that will receive the reply from the card
    @     bus.
card_read_512:                         @ At address 0118 in the original
    push   {r4, r5, lr}
    @ In 0xA1180210:
    @ Bit 31 = 1: Start block transfer
    @ Bit 28 = 0: Hold CLK high during gaps
    @ Bit 27 = 0: Transfer CLK rate = 6.7 MHz = 33.51 MHz / 5
    @ Bits 24..26 = 001: Data block size = 512 bytes
    @ Bit 23 = 0: Data word status busy
    @ Bit 22 = 0: Disable KEY2 encryption for commands
    @ Bits 16..21 = 01 1000: KEY1 gap2 length = 0x18
    @ Bit 15 = 0: No KEY2 change
    @ Bit 13 = 0: Disable KEY2 encryption for data
    @ Bits 0..12 = 0 0010 0001 0000: KEY1 gap1 length = 0x210
    ldr    r3, =0xA1180210
    ldr    r2, =0x040001A4             @ r2 = address of ROMCTRL
    str    r3, [r2, #0]                @ ROMCTRL = our flags;

    movs   r3, #3
    tst    r0, r3                      @ if (argument 1) & 3 != 0...
    bne    label_012a                  @ go there

    tst    r1, r3                      @ if (argument 2) & 3 == 0...
    beq    label_0168                  @ go there

label_012a:
    ldr    r3, =0x040001A4             @ r3 = address of ROMCTRL
    ldr    r3, [r3, #0]                @ read ROMCTRL
    lsl    r2, r3, #8
    bpl    label_015a                  @ if bit 23 (CARD_DATA_READY) is unset

    ldr    r3, =0x04100010             @ r3 = address of CARD_RD
    ldr    r3, [r3, #0]                @ read CARD_RD
    cmp    r1, #0                      @ no bytes left? (argument 2 == 0)
    ble    label_015a

    sub    r1, #1                      @ one less byte left
    strb   r3, [r0, #0]                @ store the lowest byte of CARD_RD
    add    r2, r0, #1
    lsr    r3, r3, #8
    add    r0, #4
    cmp    r1, #0                      @ no bytes left? (argument 2 == 0)
    beq    label_0158

label_0148:
    strb   r3, [r2, #0]                @ store the second lowest byte of CARD_RD
    add    r2, #1
    sub    r1, #1
    cmp    r2, r0                      @ no bytes left? (cur == end)
    beq    label_0158

    lsr    r3, r3, #8                  @ get the next lowest byte
    cmp    r1, #0                      @ more bytes left?
    bne    label_0148

label_0158:
    add    r0, r2, #0
label_015a:
    ldr    r3, =0x040001A4             @ r3 = address of ROMCTRL
    ldr    r3, [r3, #0]                @ read ROMCTRL
    cmp    r3, #0
    blt    label_012a                  @ bit 31 (sign) is CARD_BUSY

label_0162:
    pop    {r4, r5}
    pop    {r0}
    bx     r0

label_0168:
    add    r4, r2, #0
    ldr    r5, =0x04100010             @ r5 = address of CARD_RD
    movs   r2, #0x80
    lsl    r2, r2, #16                 @ r2 = 0x00800000

label_0170:
    ldr    r3, [r4, #0]                @ read ROMCTRL
    tst    r3, r2                      @ check if CARD_DATA_READY
    beq    label_0180                  @ no? go there

    cmp    r1, #0                      @ any words left?
    ble    label_0188                  @ no? ignore the remainder

    ldr    r3, [r5, #0]                @ read from CARD_RD
    stmia  r0!, {r3}                   @ store aligned and increment r0 by 4
    sub    r1, #4                      @ 4 fewer bytes to get

label_0180:
    ldr    r3, [r4, #0]                @ read ROMCTRL
    cmp    r3, #0
    blt    label_0170                  @ bit 31 (sign) is CARD_BUSY
    b      label_0162

label_0188:
    ldr    r3, [r5, #0]                @ read from CARD_RD
    b      label_0180

    @ void card_read_multi_512(void* dst, off_t rom_offset, size_t n)
    @ Sets up a 0x18 command to retrieve 512-byte packets from the card bus
    @ until 'n' bytes have been read.
    @ In:
    @   rom_offset: The offset within the NDS executable to start retrieving
    @     from.
    @   n: The number of bytes to be retrieved. Bytes beyond this number are
    @     read but ignored.
    @ Out:
    @   dst: Pointer to the buffer that will receive the reply from the card
    @     bus.
card_read_multi_512:
    push   {r4, r5, r6, r7, lr}
    add    r6, r0, #0
    add    r5, r1, #0
    add    r4, r2, #0
    cmp    r2, #0                      @ is the size greater than 0?
    ble    label_01d8                  @ no? we're done

    movs   r7, #0x80
    lsl    r7, r7, #2                  @ r7 = 0x200 (512)

label_01aa:
    add    r1, r5, #0                  @ r1 = original r1 saved through r5
    movs   r0, #24                     @ r0 = 24
    movs   r2, #0                      @ r2 = 0
    bl     card_set_command
    add    r1, r4, #0                  @ r1 = original r2 saved through r4
    cmp    r4, r7                      @ do we have less than a block left?
    ble    label_01be                  @ yes, use the remainder as the size

    movs   r1, r7                      @ otherwise, just get 0x200 (512)

label_01be:
    add    r0, r6, #0                  @ r0 = original r0 saved through r6
    bl     card_read_512

    add    r6, r6, r7                  @ advance r6 (= orig r0) by a block
    sub    r4, r4, r7                  @ reduce r4 (= orig r2) by a block
    add    r5, r5, r7                  @ advance r5 (= orig r1) by a block

    cmp    r4, #0                      @ more left?
    bgt    label_01aa                  @ yep

label_01d8:
    pop    {r4, r5, r6, r7}
    pop    {r1}
    bx     r1

    @ void wait_for_fifo(void)
    @ Sends 0xE0 commands until there are 256 bytes available in the FIFO,
    @ indicating that the Supercard is ready to send the NDS executable.
wait_for_fifo:                         @ At address 01F8 in the original
    push   {r4, lr}
    movs   r4, #0x80
    lsl    r4, r4, #1                  @ r4 (saved) = 0x100 (256)
    b      label_0208

label_0200:
    movs   r0, #0x80
    lsl    r0, r0, #1                  @ r0 = 0x100 (256)
    swi    3                           @ STOP (until the next interrupt)
label_0208:
    movs   r0, #0xE0                   @ FPGA_COMMAND_FIFO_STATUS_BYTE
    movs   r1, #0                      @ argument 2 = 0
    movs   r2, #0                      @ argument 3 = 0
    bl     card_read_4

    tst    r0, r4                      @ return value & 0x100? (256 halfwords in FIFO)
    bne    label_0200                  @ no? try again
    b      label_0220

label_0218:
    movs   r0, #0x80
    lsl    r0, r0, #1                  @ r0 = 0x100 (256)
    swi    3                           @ STOP (until the next interrupt)

label_0220:
    ldr    r4, =0x12345678
    movs   r0, #0x16
    add    r1, r4, #0                  @ r1 = 0x12345678
    movs   r2, #0
    bl     card_read_4

    mvn    r0, r0                      @ complement the return value
    cmp    r0, r4                      @ ~ret == 0x12345678? (ret == 0xEDCBA987?)
    bne    label_0218

    pop    {r4}
    pop    {r1}
    bx     r1

    .pool

    .align 2

    .arm
    .global arm7_loader_end
    .type   arm7_loader_end STT_FUNC
arm7_loader_end:
