
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define isupperhex(X)   (((X) >= 'A') && ((X) <= 'F'))
#define islowerhex(X)   (((X) >= 'a') && ((X) <= 'f'))

static size_t ScanLong(const char *text, int radix, long *valuep)
{
    const char *textstart = text;
    long value = 0;
    bool negative = false;

    if ( *text == '-' ) {
        negative = true;
        ++text;
    }
    if ( radix == 16 && strncmp(text, "0x", 2) == 0 ) {
        text += 2;
    }
    for ( ; ; ) {
        int v;
        if ( isdigit((unsigned char) *text) ) {
            v = *text - '0';
        } else if ( radix == 16 && isupperhex(*text) ) {
            v = 10 + (*text - 'A');
        } else if ( radix == 16 && islowerhex(*text) ) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if ( valuep ) {
        if ( negative && value ) {
            *valuep = -value;
        } else {
            *valuep = value;
        }
    }
    return (text - textstart);
}

static size_t ScanUnsignedLong(const char *text, int radix, unsigned long *valuep)
{
    const char *textstart = text;
    unsigned long value = 0;

    if ( radix == 16 && strncmp(text, "0x", 2) == 0 ) {
        text += 2;
    }
    for ( ; ; ) {
        int v;
        if ( isdigit((unsigned char) *text) ) {
            v = *text - '0';
        } else if ( radix == 16 && isupperhex(*text) ) {
            v = 10 + (*text - 'A');
        } else if ( radix == 16 && islowerhex(*text) ) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if ( valuep ) {
        *valuep = value;
    }
    return (text - textstart);
}

static size_t ScanUintPtrT(const char *text, int radix, uintptr_t *valuep)
{
    const char *textstart = text;
    uintptr_t value = 0;

    if ( radix == 16 && strncmp(text, "0x", 2) == 0 ) {
        text += 2;
    }
    for ( ; ; ) {
        int v;
        if ( isdigit((unsigned char) *text) ) {
            v = *text - '0';
        } else if ( radix == 16 && isupperhex(*text) ) {
            v = 10 + (*text - 'A');
        } else if ( radix == 16 && islowerhex(*text) ) {
            v = 10 + (*text - 'a');
        } else {
            break;
        }
        value *= radix;
        value += v;
        ++text;
    }
    if ( valuep ) {
        *valuep = value;
    }
    return (text - textstart);
}

static size_t ScanFloat(const char *text, double *valuep)
{
    const char *textstart = text;
    unsigned long lvalue = 0;
    double value = 0.0;
    bool negative = false;

    if ( *text == '-' ) {
        negative = true;
        ++text;
    }
    text += ScanUnsignedLong(text, 10, &lvalue);
    value += lvalue;
    if ( *text == '.' ) {
        int mult = 10;
        ++text;
        while ( isdigit((unsigned char) *text) ) {
            lvalue = *text - '0';
            value += (double)lvalue / mult;
            mult *= 10;
            ++text;
        }
    }
    if ( valuep ) {
        if ( negative && value ) {
            *valuep = -value;
        } else {
            *valuep = value;
        }
    }
    return (text - textstart);
}

int sscanf(const char *text, const char *fmt, ...)
{
    va_list ap;
    int retval = 0;

    va_start(ap, fmt);
    while ( *fmt ) {
        if ( *fmt == ' ' ) {
            while ( isspace((unsigned char) *text) ) {
                ++text;
            }
            ++fmt;
            continue;
        }
        if ( *fmt == '%' ) {
            bool done = false;
            long count = 0;
            int radix = 10;
            enum {
                DO_SHORT,
                DO_INT,
                DO_LONG,
                DO_LONGLONG
            } inttype = DO_INT;
            bool suppress = false;

            ++fmt;
            if ( *fmt == '%' ) {
                if ( *text == '%' ) {
                    ++text;
                    ++fmt;
                    continue;
                }
                break;
            }
            if ( *fmt == '*' ) {
                suppress = true;
                ++fmt;
            }
            fmt += ScanLong(fmt, 10, &count);

            if ( *fmt == 'c' ) {
                if ( ! count ) {
                    count = 1;
                }
                if ( suppress ) {
                    while ( count-- ) {
                        ++text;
                    }
                } else {
                    char *valuep = va_arg(ap, char*);
                    while ( count-- ) {
                        *valuep++ = *text++;
                    }
                    ++retval;
                }
                continue;
            }

            while ( isspace((unsigned char) *text) ) {
                ++text;
            }

            /* FIXME: implement more of the format specifiers */
            while (!done) {
                switch(*fmt) {
                    case '*':
                        suppress = true;
                        break;
                    case 'h':
                        if ( inttype > DO_SHORT ) {
                            ++inttype;
                        }
                        break;
                    case 'l':
                        if ( inttype < DO_LONGLONG ) {
                            ++inttype;
                        }
                        break;
                    case 'I':
                        if ( strncmp(fmt, "I64", 3) == 0 ) {
                            fmt += 2;
                            inttype = DO_LONGLONG;
                        }
                        break;
                    case 'i':
                        {
                            int index = 0;
                            if ( text[index] == '-' ) {
                                ++index;
                            }
                            if ( text[index] == '0' ) {
                                if ( tolower((unsigned char) text[index+1]) == 'x' ) {
                                    radix = 16;
                                } else {
                                    radix = 8;
                                }
                            }
                        }
                        /* Fall through to %d handling */
                    case 'd':
			{
		            long value;
		            text += ScanLong(text, radix, &value);
		            if ( ! suppress ) {
		                switch (inttype) {
		                    case DO_SHORT:
		                        { short* valuep = va_arg(ap, short*);
		                            *valuep = (short)value;
		                        }
		                        break;
		                    case DO_INT:
		                        { int* valuep = va_arg(ap, int*);
		                            *valuep = (int)value;
		                        }
		                        break;
		                    case DO_LONG:
		                        { long* valuep = va_arg(ap, long*);
		                            *valuep = value;
		                        }
		                        break;
		                    case DO_LONGLONG:
		                        /* Handled above */
		                        break;
		                }
		                ++retval;
			    }
                        done = true;
			}
                        break;
                    case 'o':
                        if ( radix == 10 ) {
                            radix = 8;
                        }
                        /* Fall through to unsigned handling */
                    case 'x':
                    case 'X':
                        if ( radix == 10 ) {
                            radix = 16;
                        }
                        /* Fall through to unsigned handling */
                    case 'u':
                        {
                            unsigned long value;
                            text += ScanUnsignedLong(text, radix, &value);
                            if ( ! suppress ) {
                                switch (inttype) {
                                    case DO_SHORT:
                                        { short* valuep = va_arg(ap, short*);
                                            *valuep = (short)value;
                                        }
                                        break;
                                    case DO_INT:
                                        { int* valuep = va_arg(ap, int*);
                                            *valuep = (int)value;
                                        }
                                        break;
                                    case DO_LONG:
                                        { long* valuep = va_arg(ap, long*);
                                            *valuep = value;
                                        }
                                        break;
                                    case DO_LONGLONG:
                                        /* Handled above */
                                        break;
                                }
                                ++retval;
                            }
                        }
                        done = true;
                        break;
                    case 'p':
                        {
                            uintptr_t value;
                            text += ScanUintPtrT(text, 16, &value);
                            if ( ! suppress ) {
                                void** valuep = va_arg(ap, void**);
                                *valuep = (void*)value;
                                ++retval;
                            }
                        }
                        done = true;
                        break;
                    case 'f':
                        {
                            double value;
                            text += ScanFloat(text, &value);
                            if ( ! suppress ) {
                                float* valuep = va_arg(ap, float*);
                                *valuep = (float)value;
                                ++retval;
                            }
                        }
                        done = true;
                        break;
                    case 's':
                        if ( suppress ) {
                            while ( !isspace((unsigned char) *text) ) {
                                ++text;
                                if ( count ) {
                                    if ( --count == 0 ) {
                                        break;
                                    }
                                }
                            }
                        } else {
                            char *valuep = va_arg(ap, char*);
                            while ( !isspace((unsigned char) *text) ) {
                                *valuep++ = *text++;
                                if ( count ) {
                                    if ( --count == 0 ) {
                                        break;
                                    }
                                }
                            }
                            *valuep = '\0';
                            ++retval;
                        }
                        done = true;
                        break;
                    default:
                        done = true;
                        break;
                }
                ++fmt;
            }
            continue;
        }
        if ( *text == *fmt ) {
            ++text;
            ++fmt;
            continue;
        }
        /* Text didn't match format specifier */
        break;
    }
    va_end(ap);

    return retval;
}

