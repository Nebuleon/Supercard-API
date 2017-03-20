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

#ifndef __ERRNO_H__
#define __ERRNO_H__

#if !defined __ASSEMBLY__
extern int errno;
#endif

#define EACCES 1
#define EADDRINUSE 2
#define EADDRNOTAVAIL 3
#define EAFNOSUPPORT 4
#define EAGAIN 5
#define EALREADY 6
#define EBADF 7
#define EBADMSG 8
#define EBUSY 9
#define ECANCELED 10
#define ECHILD 11
#define ECONNABORTED 12
#define ECONNREFUSED 13
#define ECONNRESET 14
#define EDEADLK 15
#define EDESTADDRREQ 16
#define EDOM 17
#define EDQUOT 18
#define EEXIST 19
#define EFAULT 20
#define EFBIG 21
#define EHOSTUNREACH 22
#define EIDRM 23
#define EILSEQ 24
#define EINPROGRESS 25
#define EINTR 26
#define EINVAL 27
#define EIO 28
#define EISCONN 29
#define EISDIR 30
#define ELOOP 31
#define EMFILE 32
#define EMLINK 33
#define EMSGSIZE 34
#define EMULTIHOP 35
#define ENAMETOOLONG 36
#define ENETDOWN 37
#define ENETRESET 38
#define ENETUNREACH 39
#define ENFILE 40
#define ENOBUFS 41
#define ENODATA 42
#define ENODEV 43
#define ENOENT 44
#define ENOEXEC 45
#define ENOLCK 46
#define ENOLINK 47
#define ENOMEM 48
#define ENOMSG 49
#define ENOPROTOOPT 50
#define ENOSPC 51
#define ENOSR 52
#define ENOSTR 53
#define ENOSYS 54
#define ENOTCONN 55
#define ENOTDIR 56
#define ENOTEMPTY 57
#define ENOTSOCK 58
#define ENOTSUP 59
#define ENOTTY 60
#define ENXIO 61
#define EOPNOTSUPP 62
#define EOVERFLOW 63
#define EPERM 64
#define EPIPE 65
#define EPROTO 66
#define EPROTONOSUPPORT 67
#define EPROTOTYPE 68
#define ERANGE 69
#define EROFS 70
#define ESPIPE 71
#define ESRCH 72
#define ESTALE 73
#define ETIME 74
#define ETIMEDOUT 75
#define ETXTBSY 76
#define EWOULDBLOCK 77
#define EXDEV 78
#define E2BIG 79

#define ETOOBIG E2BIG

#endif /* !__ERRNO_H__ */
