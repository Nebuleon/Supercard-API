/*
 * This file is part of the C standard library for the Supercard DSTwo.
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

    .text
    .set     noreorder

    .ent     DS2_StartAwait
    .global  DS2_StartAwait
    .type    DS2_StartAwait,@function

    /* void DS2_StartAwait(void);
     * Begins a block of code which is restarted if an interrupt occurs,
     * allowing a condition check to be retried.
     *
     * Implementation details:
     * The return address is stashed at 0x80001FFC so that the interrupt
     * handler can return there multiple times.
     */
DS2_StartAwait:
    lui     v0, 0x8000
    jr      ra
    sw      ra, 0x1FFC(v0)

    .end     DS2_StartAwait

    .ent     DS2_StopAwait
    .global  DS2_StopAwait
    .type    DS2_StopAwait,@function

    /* void DS2_StopAwait(void);
     * Ends a block of code which is restarted if an interrupt occurs.
     *
     * Implementation details:
     * NULL is written to 0x80001FFC so that the interrupt handler can
     * resume normal operation.
     */
DS2_StopAwait:
    lui     v0, 0x8000
    jr      ra
    sw      zero, 0x1FFC(v0)

    .end     DS2_StopAwait
