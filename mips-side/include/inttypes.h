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

#ifndef __INTTYPES_H__
#define __INTTYPES_H__

#include <stdint.h>

typedef struct _imaxdiv_t {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

extern imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) __attribute__((const));

extern intmax_t imaxabs(intmax_t n) __attribute__((const));

// TODO Implement this function
// intmax_t strtoimax(const char* restrict s, char** restrict end, int base);

// TODO Implement this function
// uintmax_t strtoumax(const char* restrict s, char** restrict end, int base);

#define __PRI32_PREFIX "l"
#define __PRI64_PREFIX "ll"
#define __PRIPTR_PREFIX

#define PRId8 "d"
#define PRId16 "d"
#define PRId32 __PRI32_PREFIX "d"
#define PRId64 __PRI64_PREFIX "d"

#define PRIdLEAST8 "d"
#define PRIdLEAST16 "d"
#define PRIdLEAST32 __PRI32_PREFIX "d"
#define PRIdLEAST64 __PRI64_PREFIX "d"

#define PRIdFAST8 "d"
#define PRIdFAST16 "d"
#define PRIdFAST32 __PRI32_PREFIX "d"
#define PRIdFAST64 __PRI64_PREFIX "d"

#define PRIdMAX __PRI64_PREFIX "d"
#define PRIdPTR __PRIPTR_PREFIX "d"

#define PRIi8 "i"
#define PRIi16 "i"
#define PRIi32 __PRI32_PREFIX "i"
#define PRIi64 __PRI64_PREFIX "i"

#define PRIiLEAST8 "i"
#define PRIiLEAST16 "i"
#define PRIiLEAST32 __PRI32_PREFIX "i"
#define PRIiLEAST64 __PRI64_PREFIX "i"

#define PRIiFAST8 "i"
#define PRIiFAST16 "i"
#define PRIiFAST32 __PRI32_PREFIX "i"
#define PRIiFAST64 __PRI64_PREFIX "i"

#define PRIiMAX __PRI64_PREFIX "i"
#define PRIiPTR __PRIPTR_PREFIX "i"

#define PRIo8 "o"
#define PRIo16 "o"
#define PRIo32 __PRI32_PREFIX "o"
#define PRIo64 __PRI64_PREFIX "o"

#define PRIoLEAST8 "o"
#define PRIoLEAST16 "o"
#define PRIoLEAST32 __PRI32_PREFIX "o"
#define PRIoLEAST64 __PRI64_PREFIX "o"

#define PRIoFAST8 "o"
#define PRIoFAST16 "o"
#define PRIoFAST32 __PRI32_PREFIX "o"
#define PRIoFAST64 __PRI64_PREFIX "o"

#define PRIoMAX __PRI64_PREFIX "o"
#define PRIoPTR __PRIPTR_PREFIX "o"

#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 __PRI32_PREFIX "u"
#define PRIu64 __PRI64_PREFIX "u"

#define PRIuLEAST8 "u"
#define PRIuLEAST16 "u"
#define PRIuLEAST32 __PRI32_PREFIX "u"
#define PRIuLEAST64 __PRI64_PREFIX "u"

#define PRIuFAST8 "u"
#define PRIuFAST16 "u"
#define PRIuFAST32 __PRI32_PREFIX "u"
#define PRIuFAST64 __PRI64_PREFIX "u"

#define PRIuMAX __PRI64_PREFIX "u"
#define PRIuPTR __PRIPTR_PREFIX "u"

#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 __PRI32_PREFIX "x"
#define PRIx64 __PRI64_PREFIX "x"

#define PRIxLEAST8 "x"
#define PRIxLEAST16 "x"
#define PRIxLEAST32 __PRI32_PREFIX "x"
#define PRIxLEAST64 __PRI64_PREFIX "x"

#define PRIxFAST8 "x"
#define PRIxFAST16 "x"
#define PRIxFAST32 __PRI32_PREFIX "x"
#define PRIxFAST64 __PRI64_PREFIX "x"

#define PRIxMAX __PRI64_PREFIX "x"
#define PRIxPTR __PRIPTR_PREFIX "x"

#define PRIX8 "X"
#define PRIX16 "X"
#define PRIX32 __PRI32_PREFIX "X"
#define PRIX64 __PRI64_PREFIX "X"

#define PRIXLEAST8 "X"
#define PRIXLEAST16 "X"
#define PRIXLEAST32 __PRI32_PREFIX "X"
#define PRIXLEAST64 __PRI64_PREFIX "X"

#define PRIXFAST8 "X"
#define PRIXFAST16 "X"
#define PRIXFAST32 __PRI32_PREFIX "X"
#define PRIXFAST64 __PRI64_PREFIX "X"

#define PRIXMAX __PRI64_PREFIX "X"
#define PRIXPTR __PRIPTR_PREFIX "X"

#endif /* !__INTTYPES_H__ */