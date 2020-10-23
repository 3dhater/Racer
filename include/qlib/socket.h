// socket.h

#ifndef __QLIB_SOCKET_H
#define __QLIB_SOCKET_H

#include <qlib/types.h>
#include <netinet/in.h>

//#define QCS_DEFAULT_PORT	5400

class QClientSocket
{
 protected:
  int fd;
  int flags;

 public:
  enum { ISOPEN=1 };		// Socket is open? (connected to host)
  enum
  { // Open flags
    NODELAY=1,			// Flush at every write to fd
    FASTACK=2 			// Fast TCP acknowledge (more bandwidth used)
  };

 public:
  QClientSocket();
  ~QClientSocket();

  int GetFD(){ return fd; }
  bool IsOpen();
  bool Open(cstring hostName,int port,int flags=0);
  void Close();
};

class QServerSocket
{
 protected:
  int fd;
  struct sockaddr_in server;

 public:
  QServerSocket();
  ~QServerSocket();

  bool Open(int port);
  void Close();

  int GetFD(){ return fd; }
  int  Accept(int timeout=0);		// Timeout in microsecs (1M)
};

#endif
