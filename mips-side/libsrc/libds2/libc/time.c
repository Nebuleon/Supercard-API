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

#include "../ds2_ds/globals.h"
#include "../intc.h"

time_t time(time_t* tp)
{
	time_t result;
	struct tm tm;

	uint32_t section = DS2_EnterCriticalSection();
	tm.tm_year = (2000 /* DS RTC base year */ + _rtc.year) - 1900 /* struct tm base year */;
	tm.tm_mon = _rtc.month - 1;
	tm.tm_mday = _rtc.day;
	/* Map hours 40..51 (PM 0..11) to 12..23, others are 24-hour or AM */
	tm.tm_hour = (_rtc.hour >= 40 ? _rtc.hour - 28 : _rtc.hour);
	tm.tm_min = _rtc.minute;
	tm.tm_sec = _rtc.second;
	DS2_LeaveCriticalSection(section);
	tm.tm_isdst = -1;
	tm.tm_wday = tm.tm_yday = -1;

	result = mktime(&tm);
	if (tp) *tp = result;
	return result;
}
