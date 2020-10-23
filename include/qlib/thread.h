// qlib/thread.h

#ifndef __QLIB_THREAD_H
#define __QLIB_THREAD_H

#include <qlib/types.h>
#include <sys/types.h>
#include <sys/prctl.h>

typedef void (*QThreadProc)(void *p);

class QThread
{
  QThreadProc proc;
  void       *procPtr;

  pid_t threadPID;		// Process/thread ID
  int   shareAttribs;

 public:
  QThread(QThreadProc proc,void *procPtr);
  ~QThread();

  QThreadProc GetProc(){ return proc; }
  void *GetProcPtr(){ return procPtr; }
  pid_t GetPID(){ return threadPID; }

  bool IsRunning();
  bool Create();
  void Kill();

  // Thread signalling
  void Stop();
  void Continue();
};

#endif
