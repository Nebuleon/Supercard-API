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

#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef __INT32_TYPE__ off_t;

/* Also defined in dirent.h. */
#ifndef _INO_T_
#  define _INO_T_
typedef __UINT32_TYPE__ ino_t;
#endif
typedef __UINT32_TYPE__ dev_t;
typedef __UINT32_TYPE__ nlink_t;
typedef __UINT32_TYPE__ uid_t;
typedef __UINT32_TYPE__ gid_t;
typedef __UINT32_TYPE__ blksize_t;
typedef __UINT32_TYPE__ blkcnt_t;
typedef __UINT32_TYPE__ fsblkcnt_t;
typedef __UINT32_TYPE__ fsfilcnt_t;
typedef __UINT32_TYPE__ useconds_t;

/* Also defined in time.h. */
#ifndef _TIME_T_
#  define _TIME_T_
typedef __UINT32_TYPE__ time_t;
#endif

/* Also defined in stddef.h, stdlib.h, string.h, time.h, wchar.h.
 * The name _SIZE_T_ is also used by sysdefs.h. */
#ifndef _SIZE_T_
#  define _SIZE_T_
typedef __SIZE_TYPE__ size_t;
#endif

typedef long ssize_t;

#ifdef __cplusplus
}
#endif

#endif /* !__SYS_TYPES_H__ */
