/*
 * QProcess - process and process group control
 * 31-03-00: Created!
 * NOTES:
 * - Created from Medit's main.cpp
 * (c) MarketGraph/RVG
 */

#include <qlib/process.h>
#ifdef linux
// Prototypes for getsid()
#include <unistd.h>
extern __pid_t getsid(__pid_t __pid) __THROW;
#endif
#include <qlib/debug.h>
DEBUG_ENABLE

/******************
* PROCESS CONTROL *
******************/
//static pid_t execPID;			// Process which is being exec'd
//static bool  execStopped;

static void myIgnore(int sig)
// Signal handler that just returns
// Note that for IRIX, the signal handler will have been set to SIG_DFL
// so we don't need to do anything here.
{
  //printf("myIgnore\n");
  //signal(sig,SIG_DFL);
}

bool QProcessExists(pid_t pid)
// Returns TRUE if process 'pid' exists
// NOTE: process may exist, but be 'defunct', meaning it will
// need to be removed later on. (waitpid() for example)
{
  // Don't allow illegal pid's (does funny things with kill())
  if(pid<=0)
    return FALSE;
  if(kill(pid,0)==-1)
  { // pid process was not found
#ifndef linux
    if(oserror()==ESRCH)
      return FALSE;
#endif
    // Otherwise kill() failed, assume process does not exist
    return FALSE;
  }
//qdbg("QProcessExists: kill(%d,0) worked\n",pid);
//perror("qpex");
  return TRUE;
}
bool QProcessIsDefunct(pid_t pid)
// Returns TRUE if process 'pid' exists but is defunct
{
  int sid;

//qdbg("QProcessIsDefunct(%d)\n",pid);
  if(!QProcessExists(pid))return FALSE;
  // Don't allow illegal pid's (does funny things with kill())
  if(pid<=0)
    return FALSE;
  // Defunct processes seem to have the session id set to 0 (see 'ps')
  // Caveat is: kill(defunctPID,0) works, getsid(defunctPID) returns -1
  // with the error 'process not found'.
//qdbg("  getsid(%d)=%d\n",pid,getsid(pid));
  sid=getsid(pid);
  if(sid==-1)
  { // Error
//perror("QProcessIsDefunct(): getsid():");
    // Since QProcessExists() returned TRUE, this is assumed to
    // be a defunct process (which isn't seen anymore by getsid())
    return TRUE;
  } 
  // Unexpected values; assume not defunct
  if(sid!=0)return FALSE;
  // Session is 0, assuming defunct process
  return TRUE;
}
bool QProcessStop(pid_t pid)
// Returns TRUE if process 'pid' could be stopped
{
//qdbg("QProcessStop(%d) (TSTP)\n",pid);
  // Don't allow illegal pid's (does funny things with kill())
  if(pid<=0)
    return FALSE;
  // STOP signal cannot be stopped (signal() does not work for STOP)
  // However, TSTP does work and looks the same
  // Send it to all group processes (mainly child processes) and
  // make sure we don't stop ourselves
  // Send STOP signal
  //if(kill(pid,SIGSTOP)==-1)
  //signal(SIGSTOP,myIgnore);
  signal(SIGTSTP,myIgnore);
//perror("QProcessStop: SIGSTOP signal func");
  //if(kill(0,SIGSTOP)==-1)		// Send to all in same process group
  if(kill(0,SIGTSTP)==-1)		// Send to all in same process group
  { // Error
    perror("QProcessStop(): kill(0,SIGTSTP):");
    return FALSE;
  }
  return TRUE;
}
bool QProcessContinue(pid_t pid)
// Returns TRUE if process 'pid' could be continued
{
//qdbg("QProcessContinue(%d)\n",pid);
  // Don't allow illegal pid's (does funny things with kill())
  if(pid<=0)
    return FALSE;
  // Send STOP signal
  //if(kill(pid,SIGCONT)==-1)
  if(kill(0,SIGCONT)==-1)		// Send to all processes in this pgroup
  { // Error
    perror("QProcessContinue()");
    return FALSE;
  }
  return TRUE;
}
bool QProcessKill(pid_t pid)
// Returns TRUE if process 'pid' could be killed
{
//qdbg("QProcessKill(%d) (TERM)\n",pid);
  // Don't allow illegal pid's (does funny things with kill())
  if(pid<=0)
    return FALSE;
  // KILL signal cannot be stopped (signal() does not work for KILL)
  // However, SIGTERM saves the day
  // Send it to all group processes (mainly child processes) and
  // make sure we don't die ourselves
  //if(kill(pid,SIGSTOP)==-1)
  //signal(SIGSTOP,myIgnore);
  signal(SIGTERM,myIgnore);
//perror("QProcessStop: SIGSTOP signal func");
  //if(kill(0,SIGSTOP)==-1)             // Send to all in same process group
  if(kill(0,SIGTERM)==-1)               // Send to all in same process group
  { // Error
    perror("QProcessKill()");
    return FALSE;
  }
  return TRUE;
}

/************************
* PROCESS GROUP CONTROL *
************************/
bool QProcessGroupSignal(int gid,int sig)
// Sends a signal to a *group* of processes
{
  // kill() has a LOT of variations
  if(kill(-gid,sig)==-1)             // Send to all in same process group
  { // Error
    perror("QProcessGroupSignal(): kill(-gid,sig):");
    return FALSE;
  }
  return TRUE;
}
bool QProcessGroupStop(int gid)
// Sends a SIGSTOP signal to the group 'gid'
{ return QProcessGroupSignal(gid,SIGSTOP);
}
bool QProcessGroupContinue(int gid)
// Sends a SIGCONT signal to the group 'gid'
{ return QProcessGroupSignal(gid,SIGCONT);
}
bool QProcessGroupKill(int gid)
// Sends a SIGKILL signal to the group 'gid'
{ return QProcessGroupSignal(gid,SIGKILL);
}

