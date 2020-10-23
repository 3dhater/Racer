/*
 * QNSocket
 * 09-08-01: Created!
 * NOTES:
 * - Windows doesn't implement BSD sockets like on Unix systems.
 * See http://www.sockets.com/winsock.htm#Deviation_ErrorCodes
 * for some important differences.
 * (c) Dolphinity/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <nlib/socket.h>
//#include <sys/socket.h>
#ifndef WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <errno.h>
DEBUG_ENABLE

// Maximum UDP packet length
#define MAX_UDPBUFFERSIZE  4096

unsigned int thisAddress;         // Address of this host

QNSocket::QNSocket()
{
  fd=0;
  flags=0;
}
QNSocket::~QNSocket()
{
  if(fd>0)
  {
#ifdef WIN32
    closesocket(fd);
#else
    close(fd);
#endif
  }
}

/************
* Low-level *
************/
int QNSocket::Bind(QNAddress *addr)
// Attempt to bind with the address
{
  // Check
  if(fd<=0)return -1;

  int r;
  r=bind(fd,(struct sockaddr *)addr->GetAddr(),sizeof(struct sockaddr));
  if(r<0)
  {
    qnError("QNSocket:Bind(); can't bind to address");
  }
  return r;
}

/**********
* Options *
**********/
int QNSocket::SetReuseAddr(bool yn)
// Turn on/off SO_REUSEADDR
{
  if(fd<=0)return -1;

  int i=(int)yn,r;
  r=setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char*)&i,sizeof(i));
  if(r<0)
  {
    qnError("QNSocket:SetReuseAddr(); can't set");
  }
#ifdef ND
  if(yn)flags|=BROADCAST;
  else  flags&=~BROADCAST;
#endif
  return r;
}
int QNSocket::SetNonBlocking(bool yn)
// Turn on/off non-blocking (read/write will not block)
{
  if(fd<=0)return -1;

  int r;
#ifdef WIN32
  unsigned long i=(int)yn;
  r=ioctl(fd,FIONBIO,&i);
#else
  // Unix
  int i=(int)yn;
  r=ioctl(fd,FIONBIO,(char*)&i);
#endif
  if(r<0)
  {
    qnError("QNSocket:SetNonBlocking(); can't set");
  }
  return r;
}
int QNSocket::SetBroadcast(bool yn)
// Turn on/off broadcasting
{
  if(fd<=0)return -1;

  int i=(int)yn,r;
  r=setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(char*)&i,sizeof(i));
  if(r<0)
  {
    qnError("QNSocket:SetBroadcast(); can't set");
  }
  if(yn)flags|=BROADCAST;
  else  flags&=~BROADCAST;
  return r;
}

/*******
* Open *
*******/
bool QNSocket::Open(int port,int _type,int _flags)
// Opens a server on port #'port'
{
  flags=_flags;

  // Remember type for later on
  type=_type;
  // Open a socket with the right type
  switch(type)
  {
    case RELIABLE:
      // Use TCP/IP
      fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
      break;
    case UNRELIABLE:
      // Use UDP
      fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
      // Quake1 does ioctl(s,FIONBIO,1) here. Why?
      break;
    default:
      qnError("QNSocket:Open(); unknown type %d",type);
      return FALSE;
  }

  // Check socket
#ifdef WIN32
  if(fd==INVALID_SOCKET)
#else
  if(fd<0)
#endif
  {
    qnError("QNSocket:Open(); can't create socket");
    return FALSE;
  }

  // Set ReuseAddr and NonBlocking here

  if(type==UNRELIABLE)
  {
    // Set reuses of address, so that a server and client
    // can be active on the same machine in different processes.
    if(flags&REUSE_ADDR)
    {
      if(SetReuseAddr(TRUE)<0)
        return FALSE;
    }

    // Bind UDP sockets now for a specific port
    localAddr.SetAttr(AF_INET,INADDR_ANY,port);

    if(Bind(&localAddr)<0)
      return FALSE;

    if(flags&BROADCAST)
    {
      // Enable broadcasting
      if(SetBroadcast(TRUE)<0)
        return FALSE;
    }
  }

  // Set TCPNoDelay and KeepAlive here
  
  return TRUE;
}

void QNSocket::GetLocalAddress(QNAddress *addr)
// Retrieve name of local address
{
  struct sockaddr_in *sa;
#if defined(__sgi) || defined(WIN32) || defined(WIN64)
  // Could we PLEASE make up our mind on socklen_t?
  int len;
#else
  unsigned int len;
#endif

  addr->Reset();
  sa=addr->GetAddr();
  //sa->sa_family=AF_INET;   // ?
  len=sizeof(struct sockaddr_in);
  // If the socket is connected, this should get
  // the correct address on a multihomed system
  if(getsockname(fd,(struct sockaddr *)sa,&len)<0)
  {
    qnError("QNSocket::GetLocalAddress(); getsockname() failed");
    return;
  }
  // If not connected, substitute the NIC address
  if(sa->sin_addr.s_addr==INADDR_ANY)
    sa->sin_addr.s_addr=thisAddress;
}

/*************
* Read/write *
*************/
int QNSocket::Write(const void *buffer,int size,QNAddress *addr)
// Write
{
  int count;

  if(size<0)return 0;
  if(type==UNRELIABLE)
  {
    // Unconnected UDP
    //if(size>MAX_PACKET_LEN)return -1;
    if(IsBroadcast())
    {
      // Probably doesn't work
      localAddr.GetAddr()->sin_addr.s_addr=INADDR_BROADCAST;
      count=sendto(fd,(char*)buffer,size,0,
        (struct sockaddr *)localAddr.GetAddr(),sizeof(struct sockaddr_in));
    } else
    {
      // Send UDP packet to destination address
      count=sendto(fd,(char*)buffer,size,0,
        (struct sockaddr *)addr->GetAddr(),sizeof(struct sockaddr_in));
    }
  } else
  {
    // Not yet implemented
    qwarn("QNSocket:Write(); unsupported network type");
    count=0;
  }

  if(count<0)
  {
    qnError("QNSocket:Write(); failed");
    return count;
  }
//qdbg("QNSocket:Write(); count=%d\n",count);
  return count;
}

int QNSocket::Read(void *buffer,int size,QNAddress *addrFrom)
// Read incoming data. You can optionally pass 'addrFrom' to retrieve
// the address of the machine that sent the data. If 'addrFrom'==0, this
// address will be ignored.
// Returns -1 for an error, 0 for no bytes available, and >0
// for the #bytes read.
{
  int n;
  //QNAddress addrFrom;
  //struct sockaddr_in addrFrom;
#if defined(__sgi) || defined(WIN32) || defined(WIN64)
  int    fromLen;
#else
  unsigned int fromLen;
#endif

  if(size>MAX_UDPBUFFERSIZE)
    size=MAX_UDPBUFFERSIZE;
  if(addrFrom)
  {
    // Read with address
    fromLen=sizeof(*addrFrom->GetAddr());
    n=recvfrom(fd,(char*)buffer,size,0,(struct sockaddr *)addrFrom->GetAddr(),
        &fromLen);
  } else
  {
    fromLen=0;
    n=recvfrom(fd,(char*)buffer,size,0,0,&fromLen);
  }
#if defined(WIN32) || defined(WIN64)
  if(n==SOCKET_ERROR)
#else
  if(n==-1)
#endif
  {
    // Check if the socket would block, waiting for data
    // If so, just return 0 for 'no bytes read'
#if defined(WIN32) || defined(WIN64)
    // Windows doesn't know about ECONNREFUSED
    if(WSAGetLastError()==WSAEWOULDBLOCK /*EAGAIN*/)
#else
    // Unix
    if(errno==EAGAIN||errno==ECONNREFUSED)
#endif
      return 0;
    qnError("QNSocket:Read() failed");
    return -1;
  }
//qdbg("QNSocket:Read(); %d incoming bytes from '%s'\n",n,addrFrom.ToString());
  return n;
}

