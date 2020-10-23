/*
 * QLib/types.h
 * (C) MarketGraph/RVG
 */

#ifndef __QLIB_TYPES_H
#define __QLIB_TYPES_H

#ifdef WIN32
// Standard things

// Turn off "int to float loss of data" warning
#pragma warning(disable:4244)

// WinNT missing Unix calls
#define getpid    _getpid
#define rmdir     _rmdir
#define getcwd    _getcwd
#define chmod    _chmod
#endif

#include <alib/types.h>
#include <qlib/string.h>

//typedef int bool;
/*typedef unsigned char *string;*/
//typedef char *string;
typedef float qfloat;
typedef double qdouble;

// 26-7-00: 'byte' is now unsigned to conform with Win32
typedef unsigned char  ubyte;
//typedef signed char    byte;
typedef unsigned char byte;
typedef unsigned short uword;
typedef unsigned short ushort;
typedef unsigned long  ulong;
// MSVC5 doesn't support 'long long' yet
#ifndef WIN32
typedef          long long int64;
typedef unsigned long long uint64;
#endif

#ifndef TRUE
#define TRUE	(1)
#endif
#ifndef FALSE
#define FALSE	(0)
#endif

// Default Q functions for all Q modules
#include <qlib/point.h>
#include <qlib/message.h>

#endif

