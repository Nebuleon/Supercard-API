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

#ifndef __TIME_H__
#define __TIME_H__

/* Also defined in stddef.h, stdlib.h, string.h. */
#ifndef NULL
#  define NULL ((void*) 0)
#endif

/* Also defined in stddef.h, stdlib.h, string.h, wchar.h, sys/types.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

typedef __UINT32_TYPE__ clock_t;

typedef unsigned int time_t;

struct tm {
	int tm_sec;    // seconds after the minute: 0..59, 0..60
	int tm_min;    // minutes after the hour: 0..59
	int tm_hour;   // hours since midnight: 0..23
	int tm_mday;   // day of the month: 1..28, 1..29, 1..30, 1..31
	int tm_mon;    // months since January: 0..11
	int tm_year;   // years since 1900: 0..*
	int tm_wday;   // days since Sunday: 0 (Sunday)..6 (Saturday)
	int tm_yday;   // days since January 1: 0..364, 0..365
	int tm_isdst;  // daylight saving time: 0 inactive, + active, - don't know
};

#define CLOCKS_PER_SEC 23437

extern char* asctime(const struct tm* timeptr);

extern clock_t clock(void);

extern char* ctime(const time_t* clock);

extern double difftime(time_t a, time_t b);

extern time_t mktime(struct tm* tm);

extern struct tm* localtime(const time_t* tp);

/* There is no way to tell which timezone is being used for the Nintendo DS's
 * real-time clock. So gmtime, for GMT time conversion, is just localtime. */
#define gmtime localtime

extern size_t strftime(char* s, size_t maxsize, const char* format, const struct tm* timeptr);

extern char* strptime(const char* buf, const char* fmt, struct tm* tm);

extern time_t time(time_t* timer);

#endif /* !__TIME_H__ */
