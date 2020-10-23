/*
 * QNGlobal - global network
 * 31-07-01: Created!
 * (c) MarketGraph/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
//#include <sys/socket.h>
#include <unistd.h>
//#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
DEBUG_ENABLE

#define MAXHOSTNAMELEN 256

// Type of network to use (IP?)
int qnNetworkType;

// Local host
QNAddress qnLocalHostAddr;

bool qnInit()
{
  static bool fInit=FALSE;

qlog(QLOG_INFO,"qnInit()\n");

  if(fInit)return FALSE;
  fInit=TRUE;

#if defined(WIN32) || defined (WIN64)
  // Open WinSock
  WSADATA libmibWSAdata;

#ifdef ND_NO_2_0
    // first try Winsock 2.0
    if(WSAStartup(MAKEWORD(2, 0),&libmibWSAdata) != 0)
    {
      // Winsock 2.0 failed, so now try 1.1 */
#endif
  if(WSAStartup(MAKEWORD(1,1),&libmibWSAdata)!=0)
  {
    qerr("Can't startup WinSock (WSAStartup)");
    return FALSE;
  }
qlog(QLOG_INFO,"WinSock opened\n");
#endif

  // Detect local address (my computer)
  char buf[MAXHOSTNAMELEN];
  //struct hostent *local;

  gethostname(buf,sizeof(buf));
  qnLocalHostAddr.GetByName(buf);
  //local=gethostbyname(buf);
  //qnLocalHostAddr.GetAddr()->sin_addr.s_addr=*(int*)local->h_addr_list[0];
qlog(QLOG_INFO,"  local host: '%s'/%s\n",qnLocalHostAddr.GetHostName(),
 qnLocalHostAddr.ToString());

  return TRUE;
}
void qnShutdown()
// Shut down the QN library
{
#if defined(WIN32) || defined (WIN64)
  WSACleanup();
#endif
}

/*********
* Errors *
*********/
void qnError(cstring fmt,...)
{ va_list args;
  char buf[1024];
  
  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  va_end(args);

  // Note that 'errno' is global; SGI defines multiple errno's,
  // one per thread. Here, we just use the global one, which may
  // lead to confusion on multithreaded apps.
#ifdef WIN32
  qwarn("[QN] %s (%d)",buf,WSAGetLastError());
#else
  qwarn("[QN] %s (%s)",buf,strerror(errno));
#endif
}

/**********
* Options *
**********/
void qnSelectNetworkType(int type)
// Select a global preferred type.
// NYI
{
  qnNetworkType=type;
}
