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

#include <stdint.h>
#include <time.h>

/* The EPOCH_* values must be synced with time.c. */
#define EPOCH_START_YEAR 2000
/* Sunday is 0. The constant below refers to the weekday on January 1 of the
 * year EPOCH_START_YEAR. */
#define EPOCH_WDAY_JAN_1    6
/* This macro must be synced with mktime.c and time.c. */
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

static const uint8_t month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

struct tm* localtime(const time_t* tp)
{
	static struct tm tm __attribute__((section(".noinit")));

	time_t t = *tp;

	tm.tm_sec = t % 60;
	t /= 60;

	tm.tm_min = t % 60;
	t /= 60;

	tm.tm_hour = t % 24;
	t /= 24;

	tm.tm_wday = (EPOCH_WDAY_JAN_1 + t) % 7;

	tm.tm_year = EPOCH_START_YEAR - 1900;
	while (t >= 365 + isleap(1900 + tm.tm_year)) {
		t -= 365 + isleap(1900 + tm.tm_year);
		tm.tm_year++;
	}

	tm.tm_yday = t;

	tm.tm_mon = 0;
	while (t >= month_days[tm.tm_mon] + (tm.tm_mon == 1 /* Feb */ && isleap(tm.tm_year))) {
		t -= month_days[tm.tm_mon] + (tm.tm_mon == 1 /* Feb */ && isleap(tm.tm_year));
		tm.tm_mon++;
	}

	tm.tm_mday = t + 1;

	/* No information is available on daylight saving time */
	tm.tm_isdst = -1;

	return &tm;
}
