// qlib/debug.h - Debugging

#ifndef __QLIB_DEBUG_H
#define __QLIB_DEBUG_H

#ifdef DEBUG
// Save some string data chunk by declaring a module-wide filename
#define DEBUG_ENABLE \
 static char*__file__=__FILE__;
#else
#define DEBUG_ENABLE
#endif

#include <stdio.h>
#ifdef sgi
#include <sys/schedctl.h>
#endif
// Profiling is often used
#include <qlib/profile.h>
#include <qlib/types.h>
#include <qlib/error.h>
#include <qlib/message.h>

// Name conflict with SGI's math.h; re-undef here
// because inbetween the first inclusion of qlib/message.h,
// math.h may have been included and the above #include then does nothing.
#undef qlog

// Debug messages
void dprintf(char *fmt,...);

// Never use delete but use QDELETE() instead; delete should
// never leave dangling pointers around so the pointer itself will be set to 0
#define QDELETE(v) {if(v){delete (v);(v)=0;}}
// Never use qfree but use QFREE() instead; this leaves no dangling ptrs
#define QFREE(v) {if(v){qfree(v);(v)=0;}}

// Tracking memory allocations (a lot like Amiga's memwatch)
// Source: qmem.cpp
void  *qcalloc(int size);
void  *qalloc(int size);
void   qfree(void *p);
string qstrdup(cstring s);
// Debugging versions
void  *Mqcalloc(int size,string file,int line);
void   Mqfree(void *p,string file,int line);
string Mqstrdup(cstring s,string file,int line);
void   Mqcheckmem(int flags);		// Check allocations
void   Mqreport(int flags);
int    Mqtotalmem();			// Returns total #bytes allocated
int    Mqtotalallocs();			// Returns total #allocations
void   Mprintf(char *fmt,...);		// Debug printf

// Debugging

enum DebugFlags
// What to debug and how
{
  // Tracing
  QDF_FUNC_ENTRY=1,			// Log function entries
  QDF_FUNC_EXIT=2,			// Log function exits
  QDF_FUNC_ARG=4,			// Log function arguments
  QDF_FUNC_RET=8,			// Log function return value
  QDF_LOG_WARN=16,			// Copy qwarn() to trace file
  QDF_LOG_ERR=0x20,			// Copy qerr() to trace file
  QDF_LOG_DBG=0x40,			// Copy qdbg() to trace file
  QDF_PREFIX_DATE=0x80,			// Add date to each trace line
  QDF_PREFIX_TIME=0x100,		// Add time to each trace line
  QDF_PREFIX_PID=0x200,			// Add getpid() to each trace line
  QDF_CS_ERR=0x400,			// Report call stack for each qerr()
  QDF_CS_WARN=0x800,			// Report call stack for each qwarn()
  QDF_xxx
};

//extern int debugFlags;
void QSetDebugFlags(int mask);		// Old method; sets all
void QDebugEnableFlags(int mask);
void QDebugDisableFlags(int mask);
void QOLOF(string fname,int line);	// Output line of file
void qasserterr(string fname,int line);	// Assertion error printing

//
// TRACING functionality
//

// Start out with empty classname
#define DBG_CLASS ""

enum QDebugTraceLevels
{
  // 3 levels of importance
  QTL_MAJOR=1,
  QTL_MEDIUM,
  QTL_MINOR
};

bool QDebugIsTracing(int flags=0);
bool QDebugTraceLog(cstring fname);
void QDebugTraceFunctionEntry(cstring className,cstring name,void *pThis);
void QDebugTraceFunctionExit(cstring className,cstring name,void *pThis);
void QDebugTracePrint(cstring format,...);
void QDebugTracePrintL(int level,cstring format,...);
void QDebugTraceCallStack(int skip=0);
void QDebugTraceSetLevel(int level);
int  QDebugTraceGetLevel();

class QDebugFunctionCall
{public:
  cstring className,name;
  void   *pThis;

  //QDebugFunctionCall(cstring className,cstring name,cstring file,cstring line)
  QDebugFunctionCall(cstring _className,cstring _name,void *_pThis=(void*)1)
  { className=_className; name=_name; pThis=_pThis;
    QDebugTraceFunctionEntry(className,name,pThis);
  }
  ~QDebugFunctionCall()
  { QDebugTraceFunctionExit(className,name,pThis);
  }
};

#ifdef QTRACE
// Macros to use
#define DBG_TRACE_ON() QDebugTraceLog("trace.log");
#define DBG_TRACE_OFF() QDebugTraceLog(0);
// Call function tracing for class member functions
#define DBG_C(func) QDebugFunctionCall __dbgFC(DBG_CLASS,(func),this);
// Call function tracing for flat functions
#define DBG_C_FLAT(func) QDebugFunctionCall __dbgFC("flat",(func));
// Argument tracing
#define DBG_ARG_I(arg) QDebugTracePrint("arg '%s'=%d/0x%x",#arg,arg,arg);
#define DBG_ARG_B(arg) QDebugTracePrint("arg '%s'=(bool)%d",#arg,arg);
// DBG_ARG_F works for doubles too
#define DBG_ARG_F(arg) QDebugTracePrint("arg '%s'=(float)%f",#arg,(float)arg);
// DBG_ARG_D works for floats too
#define DBG_ARG_D(arg) QDebugTracePrint("arg '%s'=(double)%g",#arg,(double)arg);
// DBG_ARG_S works for qstrings too
#define DBG_ARG_S(arg)\
  QDebugTracePrint("arg '%s'=\"%s\"",#arg,(cstring)arg);
#define DBG_ARG_P(arg) QDebugTracePrint("arg '%s'=(&)%p",#arg,(void*)arg);

// Generic variable tracing
#define DBG_I(arg) QDebugTracePrint("'%s'=%d/0x%x",#arg,arg,arg);
#define DBG_B(arg) QDebugTracePrint("'%s'=(bool)%d",#arg,arg);
#define DBG_F(arg) QDebugTracePrint("'%s'=(float)%f",#arg,(float)arg);
#define DBG_D(arg) QDebugTracePrint("'%s'=(double)%g",#arg,(double)arg);
#define DBG_S(arg) QDebugTracePrint("'%s'=\"%s\"",#arg,(cstring)arg);
// Often used handy shortcuts
#define DBG_II(n1,n2) QDebugTracePrint("'%s'=%d, '%s'=%d",#n1,n1,#n2,n2);

// Generic note for the log
#define DBG_NOTE(s) QDebugTracePrint("%s",s);

#else

// Empty trace macros
#define DBG_TRACE_ON()
#define DBG_TRACE_OFF()
#define DBG_C(func)
#define DBG_C_FLAT(func)
#define DBG_ARG_I(arg)
#define DBG_ARG_B(arg)
#define DBG_ARG_F(arg)
#define DBG_ARG_D(arg)
#define DBG_ARG_S(arg)
#define DBG_ARG_P(arg)
#define DBG_I(arg)
#define DBG_B(arg)
#define DBG_F(arg)
#define DBG_D(arg)
#define DBG_S(arg)
#define DBG_II(n1,n2)
#define DBG_NOTE(s)

#endif
// !QTRACE


#ifdef DEBUG
#define QTRACE_PRINT    QDebugTracePrint
#define QTRACE_PRINT_L  QDebugTracePrintL
#else
// Make tracing disappear in Release versions (use optimizer)
#define QTRACE_PRINT    1?(void)0:QDebugTracePrint
#define QTRACE_PRINT_L  1?(void)0:QDebugTracePrintL
#endif

//
// End tracing
//

// Miscellaneous functions from qmisc.cpp
//
void QNap(int nTicks);		// Dependant on CLK_TCK (sysconf(3))
void QWait(int msecs);		// Always milliseconds
void QHexDump(char *p,int len);
// Win32's MSVCRT.lib/LIBC.lib already contains strupr()/strlwr()
#ifndef WIN32
void strupr(string s);
void strlwr(string s);
#endif
bool QMatch(cstring pat,cstring str);
int  Eval(cstring s);

// Loading things
bool    QQuickSave(cstring fname,void *p,int len);
bool    QQuickLoad(cstring fname,void *p,int len);
bool    QCopyFile(cstring src,cstring dst);

// DOS-type operations
bool    QRemoveFile(cstring name);
bool    QRemoveDirectory(cstring name);
bool    QFileExists(cstring fname);
int     QFileSize(cstring fname);
cstring QExpandFilename(cstring fname);		// To get 'assigns'
cstring QFileBase(cstring fname);
cstring QFileExtension(cstring s);
bool    QIsDir(cstring path);
void    QFollowDir(cstring path,cstring newDir,string out);

// Math
//
int     QNearestPowerOf2(int n,bool round=FALSE);	// 2**n

// Endianness
//
unsigned long _QPC2Host_L(unsigned long v);
void _QPC2Host_LA(unsigned long *p,int n);
void _QPC2Host_IA(int *p,int n);	// Special 'int' version (typechecked)
unsigned short _QPC2Host_S(unsigned short v);
void _QPC2Host_SA(unsigned short *p,int n);
float _QPC2Host_F(float v);
void _QPC2Host_FA(float *p,int n);
#if defined(WIN32) || defined(linux)
// Big-endian (Intel Pentium etc) machines don't need any conversion
#define QPC2Host_L(v) (v)
#define QPC2Host_S(v) (v)
#define QPC2Host_F(v) (v)
#define QPC2Host_SA(p,n)
#define QPC2Host_IA(p,n)
#define QPC2Host_LA(p,n)
#define QPC2Host_FA(p,n)
#define QHost2PC_L(v) (v)
#define QHost2PC_S(v) (v)
#define QHost2PC_F(v) (v)
#define QHost2PC_SA(p,n)
#define QHost2PC_IA(p,n)
#define QHost2PC_LA(p,n)
#define QHost2PC_FA(p,n)
#else
// Little-endian (MIPS) need to do some work
#define QPC2Host_L(v)    _QPC2Host_L(v)
#define QPC2Host_S(v)    _QPC2Host_S(v)
#define QPC2Host_F(v)    _QPC2Host_F(v)
#define QPC2Host_SA(p,n) _QPC2Host_SA((p),(n))
#define QPC2Host_IA(p,n) _QPC2Host_IA((p),(n))
#define QPC2Host_LA(p,n) _QPC2Host_LA((p),(n))
#define QPC2Host_FA(p,n) _QPC2Host_FA((p),(n))
// Since the reverse is the same, it uses the same actual functions.
#define QHost2PC_L(v)    _QPC2Host_L(v)
#define QHost2PC_S(v)    _QPC2Host_S(v)
#define QHost2PC_F(v)    _QPC2Host_F(v)
#define QHost2PC_SA(p,n) _QPC2Host_SA((p),(n))
#define QHost2PC_IA(p,n) _QPC2Host_IA((p),(n))
#define QHost2PC_LA(p,n) _QPC2Host_LA((p),(n))
#define QHost2PC_FA(p,n) _QPC2Host_FA((p),(n))
#endif

// Some priorities to use for thread prioritizing
#define PRI_HIGH	NDPHIMAX
#define PRI_MEDIUM	((NDPHIMIN+NDPHIMAX)/2)
#define PRI_LOW		NDPHIMIN

bool QSetRoot(bool root);
bool QSetThreadPriority(int n);

#ifdef DEBUG

// Redirect allocations to the debugging functions
// Notice that qalloc() should really point to Mqalloc()
// in the future.
#define qcalloc(s) Mqcalloc(s,__file__,__LINE__)
#define qalloc(s)  Mqcalloc(s,__file__,__LINE__)
#define qfree(p)   Mqfree(p,__file__,__LINE__)
#define qstrdup(s) Mqstrdup(s,__file__,__LINE__)

// Working asserts

#define QDEBUG_DO(f)	(f)

// Q variants to avoid name conflicts on other OS'es (MFC/Win32 for example)
#define QASSERT(e) \
 if(!(e)){qasserterr(__file__,__LINE__);}
#define QASSERT_X(e) \
 if(!(e)){qasserterr(__file__,__LINE__);exit(0);}
#define QASSERT_0(e) \
 if(!(e)){qasserterr(__file__,__LINE__);return(0);}
#define QASSERT_V(e) \
 if(!(e)){qasserterr(__file__,__LINE__);return;}
// More flexible variant of _0, _T and _F
// Returns an expression if the assert failed
#define QASSERT_RET(e,r) \
 if(!(e)){qasserterr(__file__,__LINE__);return(r);}
// Don't stop, but take some action
#define QASSERT_DO(e,f) \
 if(!(e)){qasserterr(__file__,__LINE__);(f);}

#define QASSERT_F(e) \
 if(!(e)){qasserterr(__file__,__LINE__);return(0);}

// Validate object; may call Valid() member function?
#define QASSERT_VALID() \
 if(!this){qerr("Nullptr 'this' in class %s, file %s, line %d",\
 DBG_CLASS,__file__,__LINE__);}

#else

//
// No debugging; stub most debugging functions
//
#define QASSERT(e)
#define QASSERT_0(e)
#define QASSERT_X(e)
#define QASSERT_F(e)
#define QASSERT_V(e)

#define QASSERT_VALID()

#define QDEBUG_DO(f)

#endif

#endif

