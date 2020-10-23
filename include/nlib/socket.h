// nlib/socket.h

#ifndef __NLIB_SOCKET_H
#define __NLIB_SOCKET_H

#ifndef __QLIB_TYPES_H
#include <qlib/types.h>
#endif
#if defined(__sgi) || defined(linux)
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#ifdef WIN32
// Windows uses WinSock
#include <winsock.h>
// Get ioctl()
#define ioctl ioctlsocket
#endif

class QNAddress
// A machine address
{
  struct sockaddr_in addr;
  qstring            hostName;

 public:
  QNAddress()
  {
    Reset();
  }
  ~QNAddress();

  // Attribs
  struct sockaddr_in *GetAddr(){ return &addr; }
  cstring GetHostName(){ return hostName; }

  void Reset()
  {
    memset(&addr,0,sizeof(addr));
    hostName="<unknown>";
  }
  void Set(QNAddress *addr);
  void SetAttr(int family,long addr,int port);
  bool GetByName(cstring hostName);
  cstring ToString();
  unsigned int ToInt();

  bool Compare(QNAddress *addr);
};

class QNSocket
// A socket
{
 public:
  enum Types
  {
    RELIABLE,                // TCP for example
    UNRELIABLE
  };
  enum Flags
  {
    BROADCAST=1,             // Broadcast around
    MULTICAST=2,
    REUSE_ADDR=4,            // May use address more than once on host
    QN_TCP_NODELAY=8         // NYI
  };

 protected:
  int     fd;
  int     flags;
  int     type;
  QNAddress localAddr;

 public:
  QNSocket();
  ~QNSocket();

  // Attribs
  int GetFD(){ return fd; }
  bool IsBroadcast(){ if(flags&BROADCAST)return TRUE; return FALSE; }

  // Low level socket interface
  int  Bind(QNAddress *addr);

  // Socket options; call BEFORE bind
  //... reuseaddr, nonblocking
  int  SetReuseAddr(bool yn);
  int  SetNonBlocking(bool yn);
  // Socket options; call AFTER bind
  int  SetBroadcast(bool yn);
  // ...tcpnodelay, keepalive

  // Higher level
  bool Open(int port,int type=RELIABLE,int flags=0);
  void GetLocalAddress(QNAddress *address);

  int  Write(const void *buffer,int size,QNAddress *addrTo);
  int  Read(void *buffer,int size,QNAddress *addrFrom=0);
};

#endif
