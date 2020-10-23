/*
 * QServerSocket- class to connect to QClientSocket
 * 01-10-98: Created!
 * NOTES:
 * - Only Internet family sockets, stream sockets
 * - Every connection spawn a file
 * (C) MG/RVG
 */

#include <qlib/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef linux
#include <sys/time.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Display incoming IP upon connect? (debug)
//#define DISPLAY_IP

QServerSocket::QServerSocket()
{
  fd=0;
}

QServerSocket::~QServerSocket()
{
  Close();
}

bool QServerSocket::Open(int port)
{
  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0)
  { qerr("QServerSocket ctor; can't create (inet/stream) socket\n");
    return FALSE;
  }
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=port;
  if(bind(fd,(struct sockaddr *)&server,sizeof(server))<0)
  { qwarn("QServerSocket: bind failed; %s",strerror(errno));
    Close();
    return FALSE;
  }
  if(listen(fd,20)<0)
  { qwarn("QServerSocket: listen() failed");
    Close();
    return FALSE;
  }
  return TRUE;
}

void QServerSocket::Close()
{
  if(fd)
  { if(shutdown(fd,2)==-1)
      perror("QServerSocket::Close(): shutdown");
    close(fd);
    fd=0;
  }
}

int QServerSocket::Accept(int timeout)
// Accept a connection (blocks)
// Timeout is in microsecs (1000000=1 sec); if 0 then this blocks
// until an accept is made
// Returns: fd>0     connection fd
//          fd==0    timeout occurred (no real error)
//          fd<0     error occurred
{
  int sockFD;
  int len=sizeof(server);

  if(timeout)
  { // Make sure accept won't block
    struct timeval tv;
    tv.tv_sec=timeout/1000000;
    tv.tv_usec=timeout%1000000;
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd,&fdset);
//qdbg("QServerSocket:Accept; select() to wait for accept\n");
    select(fd+1,&fdset,0,0,&tv);
    if(!FD_ISSET(fd,&fdset))
    { // Timeout
      return 0;
    }
  }
#ifdef __sgi
  // socketlen_t is not defined on SGI IRIX6.5.12?
  sockFD=accept(fd,(struct sockaddr *)&server,&len);
#else
  sockFD=accept(fd,(struct sockaddr *)&server,(socklen_t *)&len);
#endif

  if(sockFD<0)
  { qwarn("QServerSocket::Accept() failed to accept");
    return sockFD;
  }
  // Get IP address
#ifdef DISPLAY_IP
  if(1)
  { unsigned int ip;
    unsigned char ipc[4];
    ip=server.sin_addr.s_addr;
    memcpy(ipc,&ip,4);
    qdbg("QServerSocket accepts connection from IP %d.%d.%d.%d\n",
      ipc[0],ipc[1],ipc[2],ipc[3]);
  }
#endif
  return sockFD;
}

