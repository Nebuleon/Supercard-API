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

#include <errno.h>

char* strerror(int errnum)
{
	switch (errnum) {
		case E2BIG:           return "Argument list too long";
		case EACCES:          return "Permission denied";
		case EADDRINUSE:      return "Address already in use";
		case EADDRNOTAVAIL:   return "Address not available";
		case EAFNOSUPPORT:    return "Address family not supported";
		case EAGAIN:          return "Resource temporarily unavailable";
		case EALREADY:        return "Connection already in progress";
		case EBADF:           return "Operation invalid for file descriptor";
		case EBADMSG:         return "Bad message";
		case EBUSY:           return "Resource busy";
		case ECANCELED:       return "Operation cancelled";
		case ECHILD:          return "Operation attempted on no process";
		case ECONNABORTED:    return "Connection aborted";
		case ECONNREFUSED:    return "Connection refused";
		case ECONNRESET:      return "Connection reset by other end";
		case EDEADLK:         return "Deadlock would occur and has been prevented";
		case EDESTADDRREQ:    return "Destination address required";
		case EDOM:            return "Input value out of range";
		case EDQUOT:          return "Storage quota exhausted";
		case EEXIST:          return "File or directory already exists";
		case EFAULT:          return "Memory address invalid";
		case EFBIG:           return "File size limit reached";
		case EHOSTUNREACH:    return "Host not reachable";
		case EIDRM:           return "Identifier removed";
		case EILSEQ:          return "Illegal byte sequence";
		case EINPROGRESS:     return "Operation in progress";
		case EINTR:           return "Operation interrupted";
		case EINVAL:          return "Invalid argument";
		case EIO:             return "Input/output error";
		case EISCONN:         return "Socket connected";
		case EISDIR:          return "Operation invalid on a directory";
		case ELOOP:           return "Symbolic link chain too complex";
		case EMFILE:          return "Open file count limit reached";
		case EMLINK:          return "Link count limit reached";
		case EMSGSIZE:        return "Message too large";
		case EMULTIHOP:       return "Remote file too far away";
		case ENAMETOOLONG:    return "File name too long";
		case ENETDOWN:        return "Network not available";
		case ENETRESET:       return "Connection reset by network";
		case ENETUNREACH:     return "Destination network not reachable";
		case ENFILE:          return "System open file count limit reached";
		case ENOBUFS:         return "Buffer space not available";
		case ENODATA:         return "No messages are available";
		case ENODEV:          return "Device does not exist";
		case ENOENT:          return "File or directory does not exist";
		case ENOEXEC:         return "Execution error";
		case ENOLCK:          return "Lock count limit reached";
		case ENOLINK:         return "ENOLINK"; /* reserved, no good description */
		case ENOMEM:          return "Memory exhausted";
		case ENOMSG:          return "No messages are available";
		case ENOPROTOOPT:     return "Protocol not available";
		case ENOSPC:          return "Storage space exhausted";
		case ENOSR:           return "No streams are available";
		case ENOSTR:          return "Stream operation attempted on non-stream";
		case ENOSYS:          return "Operation not available on this system";
		case ENOTCONN:        return "Socket not connected";
		case ENOTDIR:         return "Operation only valid on directories";
		case ENOTEMPTY:       return "Directory not empty";
		case ENOTSOCK:        return "Operation only valid on sockets";
		case ENOTSUP:         return "Operation not supported";
		case ENOTTY:          return "Operation only valid on TTYs";
		case ENXIO:           return "Device or address does not exist";
		case EOPNOTSUPP:      return "Operation invalid on a socket";
		case EOVERFLOW:       return "Value too large for data type";
		case EPERM:           return "Operation not permitted";
		case EPIPE:           return "Pipe closed by the other end";
		case EPROTO:          return "Protocol error";
		case EPROTONOSUPPORT: return "Protocol not supported";
		case EPROTOTYPE:      return "Invalid protocol for socket";
		case ERANGE:          return "Output value out of range";
		case EROFS:           return "Write attempted on read-only storage";
		case ESPIPE:          return "Seek attempted on a pipe";
		case ESRCH:           return "Process does not exist";
		case ESTALE:          return "Remote file modified in an incompatible way";
		case ETIME:           return "Stream operation timed out";
		case ETIMEDOUT:       return "Connection timed out";
		case ETXTBSY:         return "Text file busy";
		case EWOULDBLOCK:     return "Blocking operation on non-blocking resource";
		case EXDEV:           return "Link attempted across devices";
		default:              return "No description available for error number";
	}
}
