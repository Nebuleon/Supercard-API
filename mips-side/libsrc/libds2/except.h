#ifndef __EXCEPT_H__
#define __EXCEPT_H__

#include <ds2/except.h>

extern exception_handler _exception_handlers[32];

extern void* _exception_data[32];

#endif /* !__EXCEPT_H__ */