// nlib/global.h

#ifndef __NLIB_GLOBAL_H
#define __NLIB_GLOBAL_H

#ifdef ND_HMM
class QNGlobal
{
 public:
  QNGlobal();
};
#endif

enum QNNetworkTypes
{
  QN_NT_IP                    // IP protocol
};

// Avoid touching this in user code
extern int qnNetworkType;

void qnSelectNetworkType(int type);

// Error handling
void qnError(cstring fmt,...);

// Init/shutdown
bool qnInit();
void qnShutdown();

#endif

