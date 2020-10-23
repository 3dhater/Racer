// qlib/process.h - process and process group control

#ifndef __QLIB_PROCESS_H
#define __QLIB_PROCESS_H

#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#include <signal.h>
#include <errno.h>
#include <unistd.h>

bool QProcessExists(pid_t pid);
bool QProcessIsDefunct(pid_t pid);
bool QProcessStop(pid_t pid);
bool QProcessContinue(pid_t pid);
bool QProcessKill(pid_t pid);
bool QProcessGroupSignal(int gid,int sig);
bool QProcessGroupStop(int gid);
bool QProcessGroupContinue(int gid);
bool QProcessGroupKill(int gid);

#endif
