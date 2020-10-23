/*
 * QMessage - message & error handling in an expandable fashion
 * 20-04-97: Created!
 * 12-05-99: Handy log functions added
 * 20-11-00: Support for Win32; the console is attached to COM1
 * NOTES:
 * - 3 levels of debug messages exist:
 *   - A system log (QLOG/QLOG.txt) for important long-term errors
 *     qerr() calls etc can copy the texts to this logfile.
 *   - Runtime only errors (qerr(), qwarn())
 *   - Tracing (a lot of text into a single file)
 * (C) MarketGraph/RVG
 */

#include <qlib/message.h>
#include <qlib/serial.h>
#include <qlib/app.h>
#include <stdio.h>
#ifdef linux
#include <time.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <qlib/types.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#ifdef WIN32
// A magic code to avoid recurive qwarn() calls when
// the serial port is unavailable
#define MAGIC_SER   ((QSerial*)0xABCDEF)
static QSerial *ser;
#endif

#ifdef OBS_SEE_H_FILE
// Define LOG to save output to /tmp/qdbg.out
//#define LOG
#ifdef WIN32
#define LOGFILE "C:\\DATA\\LOG\\QLOG.txt"
#else
#define LOGFILE "HOME:QLOG"
#endif
#endif

static int autologMask=QAUTOLOG_DEFAULT;	// Which types of msgs to log
static QMessageHandler qerrHandler,qwarnHandler,qmsgHandler,qdbgHandler;

/***********
* MESSAGES *
***********/
static void defqmsg(cstring s)
// Default message handler
{ fprintf(stderr,"%s",s);
  if(autologMask&QAUTOLOG_MSG)qlog(QLOG_INFO|QLOG_NOSTDERR,s);
#ifdef LOG
  FILE *fp;
  fp=fopen(LOGFILE,"ab");
  fprintf(fp,"%s",s);
  fclose(fp);
#endif
}
void qmsg(const char *fmt,...)
{ va_list args;
  char buf[1024];
  
  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  if(qmsgHandler)qmsgHandler(buf);
  else           defqmsg(buf);
  va_end(args);
}
QMessageHandler QSetMessageHandler(QMessageHandler h)
{ QMessageHandler oldh;
  oldh=qmsgHandler;
  qmsgHandler=h;
  return oldh;
}

/*********
* ERRORS *
*********/
static void defqerr(string s)
// Default error handler; prints an LF
{ fprintf(stderr,"%s",s);
  if(autologMask&QAUTOLOG_ERR)qlog(QLOG_ERR|QLOG_NOSTDERR,&s[10]);
  if(QDebugIsTracing(QDF_LOG_ERR))
  { QDebugTracePrint("%s",s);
    if(QDebugIsTracing(QDF_CS_ERR))
      QDebugTraceCallStack(3);
  }
}
void qerr(const char *fmt,...)
{ va_list args;
  char buf[1024];
  
  va_start(args,fmt);
  strcpy(buf,"** Error: ");
  vsprintf(&buf[10],fmt,args);
  // Append LF if necessary
  if(buf[0]!=0&&buf[strlen(buf)-1]!=10)
    strcat(buf,"\n");
  if(qerrHandler)qerrHandler(buf);
  else           defqerr(buf);
  va_end(args);
}
QMessageHandler QSetErrorHandler(QMessageHandler h)
{ QMessageHandler oldh;
  oldh=qerrHandler;
  qerrHandler=h;
  return oldh;
}

/***********
* WARNINGS *
***********/
static void defqwarn(string s)
// Default error handler; prints an LF
{ fprintf(stderr,"%s",s);
  if(autologMask&QAUTOLOG_WARN)
#ifdef WIN32
    qlog(QLOG_WARN|QLOG_NOSTDERR,&s[12]);
#else
    // Unix
    qlog(QLOG_WARN|QLOG_NOSTDERR|QLOG_NOCONSOLE,&s[12]);
#endif
  if(QDebugIsTracing(QDF_LOG_WARN))
  { QDebugTracePrint("%s",s);
    if(QDebugIsTracing(QDF_CS_WARN))
      QDebugTraceCallStack(3);
  }
}
void qwarn(const char *fmt,...)
{ va_list args;
  char buf[1024];
  
  va_start(args,fmt);
  strcpy(buf,"** Warning: ");
  vsprintf(&buf[12],fmt,args);
  // Append LF if necessary
  if(buf[0]!=0&&buf[strlen(buf)-1]!=10)
    strcat(buf,"\n");
//sprintf(buf,"HOME=%s\n",getenv("HOME"));
//MessageBox(0,buf,"qwarn",MB_OK);
  if(qwarnHandler)qwarnHandler(buf);
  else            defqwarn(buf);
  va_end(args);
}
QMessageHandler QSetWarningHandler(QMessageHandler h)
{ QMessageHandler oldh;
  oldh=qwarnHandler;
  qwarnHandler=h;
  return oldh;
}

/********
* DEBUG *
********/
static void defqdbg(string s)
// Default message handler
{ fprintf(stderr,"%s",s);
#ifdef WIN32
  //if(autologMask&QAUTOLOG_MSG)qlog(QLOG_INFO|QLOG_NOSTDERR,s);
  //qlog(QLOG_NOSTDERR,s);
  // Write to debug output
  /*if(ser!=0&&ser!=MAGIC_SER)
    ser->Write(s,strlen(s));*/
#endif
  if(QDebugIsTracing(QDF_LOG_DBG))
  { QDebugTracePrint("%s",s);
  }
}
void qdbg(const char *fmt,...)
{ va_list args;
  char buf[1024];
  
  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  if(qdbgHandler)qdbgHandler(buf);
  else           defqdbg(buf);
  va_end(args);
}
QMessageHandler QSetDebugHandler(QMessageHandler h)
{ QMessageHandler oldh;
  oldh=qdbgHandler;
  qdbgHandler=h;
  return oldh;
}

/**********
* LOGGING *
**********/
#ifdef OBS_DEV
#undef QLOG_MAXSIZE
#define QLOG_MAXSIZE 100
#endif

static void qlogRotate()
// Rotates the log file
{
  char oldFile[256],newFile[256];
  
  // Move old log to other file (over last old file)
//qdbg("$Exp(%s)=%s\n",QLOG_OLDFILE,QExpandFilename(QLOG_OLDFILE));
//qdbg("$Exp(%s)=%s\n",QLOG_FILE,QExpandFilename(QLOG_FILE));
  strcpy(oldFile,QExpandFilename(QLOG_OLDFILE));
  strcpy(newFile,QExpandFilename(QLOG_FILE));
  remove(oldFile);
  rename(newFile,oldFile);
  remove(newFile);
  
  // Make new file +rwx for all, because sometimes we will be
  // executing as root.user or root.sys, and this file needs
  // to be written later by rvg.user for example.
  FILE *fp;
  fp=fopen(newFile,"ab");
  if(fp)
  {
#ifdef WIN32
    // Don't think we'll have to do much in this OS
#else
    chmod(newFile,0666);        // Uhm, 666?
#endif
    fclose(fp);
  }
}

void qlog(int msgType,const char *fmt,...)
// Logs a message into various output points
// May be console, log file, stderr or a mix of these
{ va_list args;
  char buf[1024];
  char tbuf[80];
  string msgName;
  static bool first=TRUE;

//MessageBox(0,fmt,"qlog",MB_OK);
  if(first)
  { // Insert marker for this session
    first=FALSE;
    qlog((msgType&(~QLOG_TYPE_MASK))|QLOG_NOTE,"--- session separator ---");
  }

  va_start(args,fmt);
  vsprintf(buf,fmt,args);

  // Append LF if necessary
  if(buf[0]!=0&&buf[strlen(buf)-1]!=10)
    strcat(buf,"\n");

  // Print on stderr
  if(!(msgType&QLOG_NOSTDERR))
  { fprintf(stderr,"%s",buf);
  }
  // Print on console
  if(!(msgType&QLOG_NOCONSOLE))
  {
#ifdef WIN32
    // Win32: Write to serial
    // To avoid recursive calls to qlog(), a magic code
    // is used to avoid opening ser recursively if the port
    // is unavailable (QSerial may give a warning, which ends
    // up here again).
    // Future may see a TCP/IP link instead (much faster)
    if(ser!=MAGIC_SER)
    {
      // Only output to serial port if this is wanted (otherwise
      // we might interfere with wheel hardware for example)
      string s=getenv("QDBG_COM");
      if(s!=0&&strcmp(s,"1")==0)
      //if(1)
      {
        // Try to open serial port
        if(!ser)
        {
          // Avoid recursive call
          QSerial *serFuture;
          ser=MAGIC_SER;
          serFuture=new QSerial(QSerial::UNIT1,9600);
          if(serFuture!=0&&serFuture->IsOpen())
            ser=serFuture;
        }
      } else
      {
        // Don't write to serial port (might conflict
        // with wheels for example)
        ser=MAGIC_SER;
      }
      // Open may have failed, so check that
      if(ser!=MAGIC_SER)
        ser->Write(buf,strlen(buf));
    }
#else
    // Unix
    FILE *fp;
    fp=fopen("/dev/console","wb");
    if(fp)
    { fprintf(fp,"%s",buf);
      fclose(fp);
    }
#endif
  }
  // Write to log file
//if(fmt[0]=='$')return;
  FILE *fp;
//MessageBox(0,QExpandFilename(QLOG_FILE),"qlog file",MB_OK);
 retry:
  fp=fopen(QExpandFilename(QLOG_FILE),"a");
  if(fp)
  {
#ifdef WIN32
    // Bug in Win32 keeps ftell(fp) at 0 even though we specified "a"
    // for append. Seek explicitly.
    fseek(fp,0,SEEK_END);
//if(fmt[0]!='$')qdbg("$qlog(); logfile size=%d\n",ftell(fp));
#endif
    if(ftell(fp)>QLOG_MAXSIZE)
    { // Auto-rotate log file
//if(fmt[0]!='$')qdbg("$qlog(); logfile rotating (%d)\n",ftell(fp));
      fclose(fp);
      qlogRotate();
      goto retry;
    }

    // Prepend time of log message
    time_t t;
    cstring appName;
    t=time(NULL);
    // Deduce message type string
    msgName="?";
    if(msgType&QLOG_ERR)msgName="ERR";
    if(msgType&QLOG_WARN)msgName="WARN";
    if(msgType&QLOG_CRIT)msgName="CRIT";
    if(msgType&QLOG_NOTICE)msgName="NOTE";
    if(msgType&QLOG_INFO)msgName="INFO";
    //strcpy(buf,asctime(localtime(&t)));
    strftime(tbuf,sizeof(tbuf),"%a %b %d %H:%M:%S",localtime(&t));
    if(!app)appName="noqapp";
    else    appName=app->GetAppName();
    fprintf(fp,"%s (%-4s) : [%s/%d] %s",tbuf,msgName,appName,getpid(),buf);
    fclose(fp);
  } else
  {
    // Can't open log file; this is quite annoying
    cstring appName;
    if(!app)appName="noqapp";
    else    appName=app->GetAppName();
    sprintf(buf,"[%s/%d] Can't open log file (QLOG)",appName,getpid());
#ifndef WIN32
    syslog(LOG_ERR,"%s (%m)",buf);
#endif
    fprintf(stderr,"** %s\n",buf);
  }
  va_end(args);
}
void qautolog(int mask)
{ autologMask=mask;
}

//#define TEST
#ifdef TEST
void myHandler(string s)
{ printf("myH: %s",s);
}

void main()
{
  qmessage("This number is %d\n",5);
  QSetMessageHandler(myHandler);
  qmessage("This number is %d\n",5);
  qerror("This is an error");
  qwarning("This is a warning");
  qdebug("Debug message");
}

#endif
