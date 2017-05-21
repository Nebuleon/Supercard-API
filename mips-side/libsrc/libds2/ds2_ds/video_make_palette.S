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

#include <mips.h>
#include <ds2/ds.h>

    .text
    .set     noreorder

    .extern  memset
    .extern  _video_main
    .extern  _video_main_palettes
    .extern  _video_main_rev_palettes

    .ent     _make_palette
    .global  _make_palette
    .type    _make_palette,@function

    /* size_t _make_palette(uint_fast8_t buffer)
     * Creates a dynamic palette for the given Main Screen buffer.
     *
     * In:
     *   argument 1: The Main Screen buffer to be read.
     * Out:
     *   _video_main_palettes[buffer]: Updated to contain the dynamic palette
     *     that fully describes the image in the Main Screen buffer.
     *   _video_main_rev_palettes[buffer]: Updated to contain a mapping from
     *     16-bit color values to 8-bit palette color entries for the image.
     * Environment assumptions:
     * - _video_main_palettes is 256 16-bit elements for each buffer, with 3
     *   palettes laid out next to each other.
     * - _video_main_rev_palettes is 32768 8-bit elements for each buffer,
     *   with 3 reverse palettes laid out next to each other.
     * Returns:
     *   1..252: The number of entries in the dynamic palette.
     *   0: There are too many unique colors in the image.
     */
_make_palette:
    # Stack frame layout:
    #    0: Argument area for this procedure's callees
    #   16: Bit filter, as many bits as there are possible 15-bit pixels
    #       (4096 bytes)
    # 4112: Register save area: ra, s0
    # 4120: End
    addiu   sp, sp, -4120
    sw      ra, 4112(sp)
    sw      s0, 4116(sp)

    move    s0, a0                     # preserve argument 1 in s0

    addiu   a0, sp, 16
    move    a1, zero
    jal     memset                     # memset(filter, 0, sizeof(filter));
    li      a2, 4096

    move    v0, zero
    li      a1, DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT
    addiu   v1, sp, 16

    sll     t9, a1, 1                  # _video_main is 16-bit elements
    mul     t8, s0, t9
    la      t7, _video_main
    addu    a0, t7, t8
    li      t9, 1

    # Register assignment:
    # v0: Palette entry count
    # v1: Palette filter
    # a0: Current source pixel pointer
    # a1: Number of remaining pixels
    # t9: Constant 1
    # s0: Main Screen buffer

    # Implementation considerations:
    # - The entire screen must be read to determine the new palette for it, so
    #   we know that the pixel count is a multiple of 4, and is not 0.
    # - By unrolling to a large extent, we avoid immediately-needed data loads
    #   which stall the processor.
    # - Since the palette does not need to be written into until the filter is
    #   dumped into it, checks for the number of unique colors can be omitted.
    #   In this implementation, 4 pixels are processed between checks.
read_loop:
    lhu     t0, 0(a0)
    lhu     t1, 2(a0)
    lhu     t2, 4(a0)
    lhu     t3, 6(a0)
    # Clear bit 15, which doesn't matter, for lookups in the filter.
    andi    t0, t0, 0x7FFF
    andi    t1, t1, 0x7FFF
    andi    t2, t2, 0x7FFF
    andi    t3, t3, 0x7FFF

    # Pixel A is explained fully; pixels B through D use the same code.
    srl     t4, t0, 3                  # get the byte to look up in the filter
    addu    t4, v1, t4                 # compute its address
    lbu     t5, 0(t4)                  # and grab the byte itself
    # Let these complete in parallel with the load.
    andi    t6, t0, 0x7                # get the bit to look up in the byte
    sllv    t6, t9, t6                 # raise a set bit to that position
    or      t7, t5, t6                 # set that bit in the byte
    sltu    t6, t5, t7                 # 1 if a new bit was set (t7 > t5)
    sb      t7, 0(t4)                  # (delay slot) store the new byte
    # Increment the palette count only if a new bit was set, which SLTU
    # converted into a +1 or +0.
    addu    v0, v0, t6

pixel_b:
    beq     t1, t0, pixel_c            # if B == A, skip processing
    srl     t4, t1, 3                  # (harmless delay slot)
    addu    t4, v1, t4
    lbu     t5, 0(t4)
    andi    t6, t1, 0x7
    sllv    t6, t9, t6
    or      t7, t5, t6
    sltu    t6, t5, t7
    sb      t7, 0(t4)
    addu    v0, v0, t6

pixel_c:
    beq     t2, t1, pixel_d
    srl     t4, t2, 3
    addu    t4, v1, t4
    lbu     t5, 0(t4)
    andi    t6, t2, 0x7
    sllv    t6, t9, t6
    or      t7, t5, t6
    sltu    t6, t5, t7
    sb      t7, 0(t4)
    addu    v0, v0, t6

pixel_d:
    beq     t3, t2, read_loop_trailer
    srl     t4, t3, 3
    addu    t4, v1, t4
    lbu     t5, 0(t4)
    andi    t6, t3, 0x7
    sllv    t6, t9, t6
    or      t7, t5, t6
    sltu    t6, t5, t7
    sb      t7, 0(t4)
    addu    v0, v0, t6

read_loop_trailer:
    sltiu   t4, v0, 253                # there must be <= 252 palette entries
    beq     t4, zero, fail             # if there are more, we have failed
    addiu   a1, a1, -4                 # (delay slot) 4 fewer pixels remain
    bne     a1, zero, read_loop        # if some pixels still remain, go back
    addiu   a0, a0, 8                  # advance the source pointer by 8 bytes

    # Here, we have a palette. We can iterate through the filter to make it.
    # The video encoding function also needs the "reverse palette", i.e. the
    # mapping from pixels to palette entries. The palette need not be sorted
    # because of this reverse palette; however, the iteration may sort it to
    # some extent.
    sll     t8, s0, 9
    la      t7, _video_main_palettes
    addu    a1, t7, t8

    sll     t8, s0, 15
    la      t7, _video_main_rev_palettes
    addu    a3, t7, t8

    move    a0, zero
    move    a2, zero
    addiu   t0, v1, 4096

    # Register assignment:
    # v0: Palette entry count
    # v1: Current palette filter pointer
    # a0: Current bit index in the filter (= upper bits in the pixel values)
    # a1: Current (forward) palette pointer
    # a2: Current index in the (forward) palette
    # a3: Reverse palette
    # t0: Pointer to one byte past the end of the filter
    # t9: Constant 1
    # s0: Main Screen buffer

filter_word_loop:
    lw      t1, 0(v1)
    beq     t1, zero, filter_word_loop_trailer
    addiu   v1, v1, 4                  # (delay slot)

word_bit_loop:
    # Determine the bit index, from the top, of the highest set bit.
    clz     t2, t1
    # Convert the number so that it refers to bits from the bottom:
    # 31 - bit.
    xori    t2, t2, 31
    addu    t3, a0, t2                 # this is now a pixel value
    # Store to the palettes.
    sh      t3, 0(a1)                  # forward palette
    addiu   a1, a1, 2
    addu    t3, a3, t3                 # reverse palette
    sb      a2, 0(t3)
    # Remove the bit from the word.
    sllv    t2, t9, t2
    xor     t1, t1, t2

    bne     t1, zero, word_bit_loop    # go back if any bits are left
    addiu   a2, a2, 1                  # (delay slot) increment palette index

filter_word_loop_trailer:
    bne     v1, t0, filter_word_loop
    addiu   a0, a0, 32                 # (delay slot)

end:
    lw      ra, 4112(sp)
    lw      s0, 4116(sp)
    jr      ra
    addiu   sp, sp, 4120               # (delay slot)

fail:
    # There were too many unique colors in the image. Bail out.
    b       end
    move    v0, zero                   # (delay slot)

    .end     _make_palette
