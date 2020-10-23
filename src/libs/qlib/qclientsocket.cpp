/*
 * QClientSocket- class to connect to QServerSocket
 * 01-10-98: Created!
 * 25-10-99: Now handles broken connections; used to exit the process
 * with a SIGPIPE. Now returns EPIPE (signal set to SIG_IGN) for
 * read()/write().
 * NOTES:
 * - IRIX6.3 didn't have TCP_FASTACK yet
 * - Only Internet family sockets, stream sockets
 * - Much easier than server because server may spawn connections
 * (C) MG/RVG
 */

#include <qlib/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef DBG_CLASS
#define DBG_CLASS "QClientSocket"

#ifndef TCP_FASTACK
#define TCP_FASTACK	0		// Don't use feature
#endif

// Use SIGPIPE handler? (broken pipes)
// If not, using a socket that is broken will result in an exit
// of the program
#define CATCH_SIGPIPE

#ifdef CATCH_SIGPIPE
void catchSigPipe(int sig)
{
}
#endif

QClientSocket::QClientSocket()
{
  DBG_C("ctor");
  flags=0;

#ifdef CATCH_SIGPIPE
  // Catch SIGPIPE signals so they won't exit the app
  //signal(SIGPIPE,catchSigPipe);
  signal(SIGPIPE,SIG_IGN);
#endif

  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0)
  { qerr("QClientSocket ctor; can't create (inet/stream) socket\n");
  }
}

QClientSocket::~QClientSocket()
{
  DBG_C("dtor");
  Close();
}

bool QClientSocket::IsOpen()
// Returns TRUE if socket is open (connected to a host)
// Note that this->GetFD() may be !=0. This does NOT mean the socket is
// connected, just that the socket fd exists.
// Perhaps Open() should be called Connected() and IsOpen() => IsConnected()
{
  DBG_C("IsOpen");
  if(flags&ISOPEN)return TRUE;
  return FALSE;
}

bool QClientSocket::Open(cstring hostName,int port,int oflags)
// Open to a server
{
  DBG_C("Open");
  struct sockaddr_in server;
  struct hostent *hp;

//qdbg("QCS:Open\n");
  server.sin_family=AF_INET;
  hp=gethostbyname(hostName);
  if(!hp)
  { qwarn("QClientSocket::Open(); host '%s' unknown",hostName);
    return FALSE;
  }
  // Copy address
  memcpy((char*)&server.sin_addr,(char*)hp->h_addr,hp->h_length);
  server.sin_port=port;
  if(connect(fd,(struct sockaddr *)&server,sizeof(server))<0)
  { qwarn("QClientSocket::Open(); can't connect to '%s'",hostName);
    return FALSE;
  }
  // Flush after every write and fast acknowledge (more bandwidth used)
  int v,len;
  len=sizeof(v); v=TRUE;
  if(oflags&NODELAY)
  { if(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&v,len)!=0)
    { perror("QClientSocket:Open(); TCP_NODELAY");
    }
  }
  if(oflags&FASTACK)
  { if(setsockopt(fd,IPPROTO_TCP,TCP_FASTACK,&v,len)!=0)
    { perror("QClientSocket:Open(); TCP_FASTACK");
    }
  }
  //setsockopt(fd,IPPROTO_TCP,TCP_FASTACK);
  flags|=ISOPEN;
  return TRUE;
}
void QClientSocket::Close()
{
  DBG_C("Close");
  if(fd)
  {
#ifdef ND_NO_SHUTDOWN
    if(shutdown(fd,2)==-1)
      perror("QClientSocket::Close(): shutdown");
#endif
    close(fd);
  }
  flags&=~ISOPEN;
  fd=0;
}

