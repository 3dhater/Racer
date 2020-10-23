/*
 * QNAddress - machine address
 * 10-08-01: Created!
 * (c) MarketGraph/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <nlib/socket.h>
#include <unistd.h>
DEBUG_ENABLE

#define MAXHOSTNAME    256

QNAddress::~QNAddress()
{
  memset(&addr,0,sizeof(addr));
}

void QNAddress::Set(QNAddress *otherAddr)
// Set address to 'otherAddr'
{
  addr.sin_addr.s_addr=otherAddr->GetAddr()->sin_addr.s_addr;
  addr.sin_port=otherAddr->GetAddr()->sin_port;
  addr.sin_family=otherAddr->GetAddr()->sin_family;
}

cstring QNAddress::ToString()
{
  static char buf[80];

  unsigned long addrL;
  unsigned short port;
  addrL=ntohl(addr.sin_addr.s_addr);
  port=ntohs(addr.sin_port);
  if(port==0)
  {
    sprintf(buf,"%lu.%lu.%lu.%lu",(addrL>>24)&0xFF,
      (addrL>>16)&0xFF,(addrL>>8)&0xFF,addrL&0xFF);
  } else
  {
    // Always exclude port
    sprintf(buf,"%lu.%lu.%lu.%lu",(addrL>>24)&0xFF,
      (addrL>>16)&0xFF,(addrL>>8)&0xFF,addrL&0xFF);
#ifdef ND_INCLUDE_PORT
    sprintf(buf,"%lu.%lu.%lu.%lu:%d",(addrL>>24)&0xFF,
      (addrL>>16)&0xFF,(addrL>>8)&0xFF,addrL&0xFF,port);
#endif
  }
  return buf;
}

void QNAddress::SetAttr(int family,long addrL,int port)
// Set several commonly used attributes at once
{
  addr.sin_family=family;
  addr.sin_addr.s_addr=htonl(addrL);
  addr.sin_port=htons(port);
}

bool QNAddress::GetByName(cstring name)
// Retrieve address from a name
{
  struct hostent *local;

  local=gethostbyname(name);
  if(!local)
  {
    qnError("QNAddress:GetByName(%s) failed",name);
    return FALSE;
  }
  // Retrieve address (IPv4)
  addr.sin_addr.s_addr=*(int*)local->h_addr_list[0];
  //hostName=qstrdup(name);
  hostName=name;

qdbg("QNAddress::GetByName(%s)='%s'\n",hostName.cstr(),local->h_name);

  return TRUE;
}

unsigned int QNAddress::ToInt()
// Returns address as a 32-bit number.
// Note that IPv4 allows this, but for IPv6 this may be inadequate.
// Use of this function may be limited.
{
  return (unsigned int)addr.sin_addr.s_addr;
}

bool QNAddress::Compare(QNAddress *otherAddr)
// Returns TRUE if both addresses seem equal
{
  if(otherAddr->GetAddr()->sin_addr.s_addr==addr.sin_addr.s_addr)
    return TRUE;
  return FALSE;
}

