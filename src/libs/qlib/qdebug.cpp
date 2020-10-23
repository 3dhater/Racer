/*
 * QDebug - debugging functions
 * 25-10-99: Tracing functionality started; much like DTF from SCO Unixware
 * NOTES:
 * - Also check qmem.cpp
 * BUGS:
 * - Debug tracing is not yet thread-safe; callDepth should
 * be mainted per-thread.
 * (C) MG/RVG
 */

#include <qlib/time.h>
#include <sys/types.h>
#ifdef WIN32
#include <process.h>
#endif
#ifdef linux
#include <unistd.h>
#endif
#ifdef __sgi
#include <unistd.h>
#include <libexc.h>
#endif
#include <stdarg.h>
#include <qlib/debug.h>
DEBUG_ENABLE

static int debugFlags=QDF_FUNC_ENTRY|QDF_FUNC_EXIT|QDF_FUNC_ARG|
  QDF_FUNC_RET|QDF_LOG_WARN|QDF_LOG_ERR|QDF_LOG_DBG|
  QDF_PREFIX_DATE|QDF_PREFIX_TIME|QDF_PREFIX_PID|
  QDF_CS_ERR|QDF_CS_WARN;

/**************
* DEBUG FLAGS *
**************/
void QSetDebugFlags(int flags)
{
  debugFlags=flags;
}
void QDebugEnableFlags(int flags)
{
  debugFlags|=flags;
}
void QDebugDisableFlags(int flags)
{
  debugFlags&=~flags;
}

/************************
* SOURCE CODE INCLUSION *
************************/
void QOLOF(string fname,int line)
// From the Amiga's debug.c (MemWatch); OutputLineOfFile
// Outputs source code line.
{ FILE *fr;
  char buf[128];
  long i;
  fr=fopen(fname,"rb"); if(fr){ Mprintf("  "); goto do_fr; }
  sprintf(buf,"/usr/people/rvg/csource/libs/qlib/%s",fname);
  fr=fopen(buf,"rb"); if(fr){ Mprintf("Q "); goto do_fr; }
  sprintf(buf,"/usr/people/rvg/csource/libs/zlib/%s",fname);
  fr=fopen(buf,"rb"); if(fr){ Mprintf("Z "); goto do_fr; }
  return;                               // No source file found
 do_fr:
  for(i=0;i<line;i++)
  { fgets(buf,127,fr);
    if(feof(fr)){ fclose(fr); return; }
  }
  Mprintf("> %s",buf);
  fclose(fr);
}

/**********
* ASSERTS *
**********/
void qasserterr(string fname,int line)
// Called when an assert fails
{ 
  char buf[256];

  sprintf(buf,"Assertion failed in '%s', line %d",fname,line);
  qerr(buf);
  QOLOF(fname,line);
}

/****************
* STACK TRACING *
****************/

/**********
* TRACING *
**********/

static FILE *fpTraceLog;      // Output file
static int   callDepth=0;
static int   mainPID;         // Process ID of main thread

// Level of trace printing
static int qDebugTraceLevel=QTL_MINOR;		// Default is trace all

void QDebugTraceSetLevel(int level)
{
  qDebugTraceLevel=level;
}
int  QDebugTraceGetLevel()
{
  return qDebugTraceLevel;
}

FILE *QDebugTraceGetLogFile()
{
  return fpTraceLog;
#ifdef OBS
  if(fpTraceLog)return fpTraceLog;
  fpTraceLog=fopen("trace.log","wb");
  return fpTraceLog;
#endif
}
bool QDebugIsTracing(int flags)
// Returns TRUE if tracing is turned on, and the 'flags' are set
// If 'flags'==0 (default) then no flags are checked. Otherwise
// it only returns TRUE if ALL the flags are actually on.
{
  if(!fpTraceLog)return FALSE;
  if(flags==0)return TRUE;
  // Check flags
  if((debugFlags&flags)!=flags)return FALSE;
  // All flags turned on
  return TRUE;
}

bool QDebugTraceLog(cstring fname)
// Set trace logging file
// If 'fname'==0, logging is disabled
{
  if(fpTraceLog)
  { fclose(fpTraceLog);
    fpTraceLog=0;
  }
  if(!fname)return TRUE;
  fpTraceLog=fopen(fname,"wb");
  if(fpTraceLog)
  { mainPID=getpid();
    return TRUE;
  }
  return FALSE;
}

//
// Tracing function calls
//
static cstring dbgPrefix()
// Returns default timestamp format
// Includes process ID (thread id)
{
  int i;
  static char buf[60],buf2[20];
  char *d;
  
  buf[0]=0;
  if(debugFlags&QDF_PREFIX_DATE)
  { sprintf(buf,"%s ",QCurrentDate("%d-%m-%y"));
  }
  if(debugFlags&QDF_PREFIX_TIME)
  {
    sprintf(buf2,"%s ",QCurrentTime());
    strcat(buf,buf2);
  }
  if(debugFlags&QDF_PREFIX_PID)
  {
    sprintf(buf2,"%6d ",getpid());
    strcat(buf,buf2);
  }
  // Insert depth string; take care of not overflowing
  // Hardcoded size (26) of timestamp! Defensive -5.
  d=buf+strlen(buf);
  for(i=0;i<callDepth&&i<(sizeof(buf)-26)/2-5;i++)
  { *d++='|'; *d++=' ';
  }
  *d=0;
  return buf;
}
void QDebugTracePrint(cstring fmt,...)
// Trace printf()
{
  FILE *fp=QDebugTraceGetLogFile();
  va_list args;
  char    buf[1024];
  
  if(!fp)return;
  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  fprintf(fp,"%s%s\n",dbgPrefix(),buf);
  va_end(args);
  fflush(fp);
}
void QDebugTracePrintL(int level,cstring fmt,...)
// Trace printf(); print only when the level is high enough
{
  // Check level first
  if(level>qDebugTraceLevel)
    return;

  FILE *fp=QDebugTraceGetLogFile();
  va_list args;
  char    buf[1024];
 
  if(!fp)return;
  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  fprintf(fp,"%s%s\n",dbgPrefix(),buf);
  va_end(args);
  fflush(fp);
}


/*************************
* TRACING FUNCTION CALLS *
*************************/
void QDebugTraceFunctionEntry(cstring className,cstring name,void *pThis)
// Function entry; log
// Note that a magic ptr '1' is used to indicate pThis is NOT used.
// So NULL this pointer are printed anyway (which is important to see)
{ FILE *fp=QDebugTraceGetLogFile();
  if(!fp)return;
  if(pThis!=(void*)1)
    fprintf(fp,"%s>%s::%s (this=0x%p)\n",dbgPrefix(),className,name,pThis);
  else
    fprintf(fp,"%s>%s::%s\n",dbgPrefix(),className,name);
  fflush(fp);
  if(getpid()==mainPID)callDepth++;
}
void QDebugTraceFunctionExit(cstring className,cstring name,void *pThis)
{ FILE *fp=QDebugTraceGetLogFile();
  if(!fp)return;
  if(getpid()==mainPID)callDepth--;
  if(pThis!=(void*)1)
    fprintf(fp,"%s<%s::%s (this=0x%p)\n",dbgPrefix(),className,name,pThis);
  else
    fprintf(fp,"%s<%s::%s\n",dbgPrefix(),className,name);
  fflush(fp);
}
void QDebugTraceCallStack(int skip)
// Output the call stack to the trace file
{
#if defined(WIN32) || defined(linux)
  // No stack tracing in Win32/Linux
#else
  // SGI/IRIX
  __uint64_t addr[20];
  char      *func[20];
  int i,n,max=20;
  FILE *fp=QDebugTraceGetLogFile();
  
  if(!fp)return;
  for(i=0;i<max;i++)
  { addr[i]=0;
    func[i]=(char*)qcalloc(200);
  }
  n=trace_back_stack(0,addr,func,20,200-1);
  for(i=skip;i<n;i++)
  {
    QDebugTracePrint("  %p %s",addr[i],func[i]);
  }
  for(i=0;i<max;i++)
  { qfree(func[i]);
  }
#endif
}
