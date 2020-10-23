/*
 * QThread - thread creation and management
 * 19-09-1999: Created!
 * NOTES:
 * - Uses sproc(), not pthreads, for its implementation.
 * - Some thread management is actually quite nifty, so better to use
 * this class than to DYI.
 * - Stack space is not unlimited; the current implementation uses a lot
 * of stack; try not to use too many threads. Max may be 8 (see man pages).
 * BUGS:
 * - A running QThread may not spawn another QThread. Well, it is
 * not threadsafe; StartThread() is called to start the thread, and this
 * is not mutexed! It is protected a bit though, so it will probably work
 * in most situations (99.9%).
 * (C) MG/RVG
 */

#if defined(__sgi)

#include <qlib/thread.h>
#include <qlib/app.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QThread::QThread(QThreadProc _proc,void *_procPtr)
{
  proc=_proc;
  procPtr=_procPtr;
  
  // Default is to share virtual address space, and FDs.
  // This leaves out sharing current working directory, and userid
  // (for example), so the new thread can assume root priority and
  // set its priority high while the main thread runs as a normal user.
  shareAttribs=PR_SADDR|PR_SFDS;
  threadPID=0;
}
QThread::~QThread()
{
  if(IsRunning())
    Kill();
}

bool QThread::IsRunning()
{ if(threadPID==0)return FALSE;
  if(kill(threadPID,0)==0)
  { return TRUE;
  }
  return FALSE;
}

static int stBusy;

static void StopThread(int)
// Stop current active thread (getpid)
{
qdbg("StopThread: pid %d stopping\n",getpid());
  // Detach from any GLX context (otherwise other threads will not be
  // able to connect to it anymore)
  glXMakeCurrent(XDSP,0,0);
  exit(0);
}

static void StartThread(void *p)
// Flat function which inits the thread attribs and starts the user proc
{ QThread *thread=(QThread*)p;
qdbg("StartThread; pid %d running\n",getpid());
qdbg("  thread=%p\n",thread);

  // Die in a coordinated way when receiving a SIGTERM (sent by Kill())
  signal(SIGTERM,StopThread);
  // Die when the parent dies (otherwise it keeps on running when
  // our creator died)
  prctl(PR_TERMCHILD,NULL);
  
  // Call user entry function
  (thread->GetProc())(thread->GetProcPtr());
}

bool QThread::Create()
// Let the thread run
{
  // Wait until others are done creating threads
  while(stBusy)QNap(1);
  // Mark busy (not atomic, so in the future use a semaphore)
  stBusy++;
  threadPID=sproc(StartThread,shareAttribs,this);
  // Set back to 0 (not stBusy--, this may cause lockups in the while() loop)
  stBusy=0;
  // Check for failure of sproc()
  if(threadPID==0)
  { qwarn("QThread:Create(); can't sproc()");
    return FALSE;
  }
  return TRUE;
}

void QThread::Kill()
// Stop the thread, and remove it
{ if(!IsRunning())return;
  // Send terminate signal (could use SIGKILL?)
  kill(threadPID,SIGTERM);
  // Don't leave zombie thread hang around
  waitpid(threadPID,0,0);
}

#endif
