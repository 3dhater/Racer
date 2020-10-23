// bugs.h

#ifndef __QLIB_BUGS_H
#define __QLIB_BUGS_H

#include <stdarg.h>

// Known bugs are collected and resolved here, hopefully

// BUG : Swapbuffers ioctl() fails every now & then
// OS  : IRIX6.5.0 - 6.5.3(f)
// Desc: When glXSwapBuffers() fails the app exits with a GL error
//       Should give warning 'glXFatal in main.o overrides...'
// Sol : Stub out the default GL handler
#define UNBUG_SWAPBUFFERS() \
  extern "C" {\
    void __glXFatal(void *gc,const char *fmt,...) \
    { va_list ap; \
      va_start(ap,fmt); \
      fprintf(stderr,"QLib_GL: Error: "); \
      vfprintf(stderr,fmt,ap); \
      fprintf(stderr,"\n"); \
      va_end(ap); \
    } \
  }


#endif
