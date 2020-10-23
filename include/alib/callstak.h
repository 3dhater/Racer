// callstak.h - tracing calls
// 16-11-96: created!

#ifndef __ALIB_CALLSTAK_H
#define __ALIB_CALLSTAK_H

#ifdef __cplusplus
extern "C" {
#endif

#define CS_OPTION_TRACE_ENTRY	1	// Report entries of f's
#define CS_OPTION_TRACE_RET	2	// Report return out of functions

// Use these macros in your program
#ifdef DEBUG
#define CS_PUSH(s)    CS_Push(s)
#define CS_POP()      CS_Pop()
#define CS_OPTIONS(n) CS_Options(n)
#define CS_REPORT()   CS_Report()

void CS_Options(int flags);
void CS_Push(char *fname);
void CS_Pop(void);
void CS_Report(void);
#else
#define CS_PUSH(s)
#define CS_POP()
#define CS_OPTIONS(n)
#define CS_REPORT()
#endif

#ifdef __cplusplus
};
#endif

#endif
