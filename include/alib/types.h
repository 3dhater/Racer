/* alib/types.h */

#ifndef __ALIB_TYPES_H
#define __ALIB_TYPES_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*typedef int BOOL;*/

/*typedef int bool;*/
// MSVC5 doesn't support 'long long' yet (int64)
#ifndef WIN32
typedef unsigned long long uint64;
typedef long long int64;
#endif
typedef unsigned int uint32;
typedef int int32;

typedef char *string;
typedef const char *cstring;

#endif
