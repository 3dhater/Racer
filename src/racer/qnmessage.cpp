/*
 * QNMessage - a transportation channel
 * 11-08-01: Created!
 * FUTURE:
 * - Deal with endianness; send all in PC format (QPC2Host)
 * (c) Dolphinity/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNMessage::QNMessage()
{
  id=0;
  len=0;
  getIndex=0;

  // Allocate buffer
  buffer=(char*)qcalloc(MAX_MESSAGE_LEN);
  if(!buffer)
  { qwarn("QNMessage ctor; not enough memory");
  }

#ifdef OBS
  // Set default associated address to localhost
  addrFrom.SetAttr(AF_INET,INADDR_ANY,25000); //RMGR->network->GetPort());
  addrFrom.GetByName("localhost");
#endif
}
QNMessage::~QNMessage()
{
  QFREE(buffer);
}

/****************
* Serialization *
****************/
void QNMessage::MakeReliable()
// Declare message reliable
{
  flags|=RELIABLE;
}

/****************
* Serialization *
****************/
void QNMessage::Clear()
// Note that this also clears flags like RELIABLE!
{
  len=0;
  // Clear message flags
  flags=0;
}
void QNMessage::AddChar(char v)
{
  if(len<MAX_MESSAGE_LEN-sizeof(char))
  {
    buffer[len]=v;
    len++;
  } else qnError("QNMessage:AddChar() overflow");
}
void QNMessage::AddShort(short v)
{
  if(len<MAX_MESSAGE_LEN-sizeof(short))
  {
    v=QHost2PC_S(v);
    *(short*)(&buffer[len])=v;
    v=QPC2Host_S(v);
    len+=sizeof(short);
  } else qnError("QNMessage:AddShort() overflow");
}
void QNMessage::AddInt(int c)
{
  if(len<MAX_MESSAGE_LEN-sizeof(int))
  {
    c=QHost2PC_L(c);
#ifdef __sgi
    memcpy(&buffer[len],&c,sizeof(int));
#else
    *(int*)(&buffer[len])=c;
#endif
    c=QPC2Host_L(c);
    len+=sizeof(int);
  } else qnError("QNMessage:AddInt() overflow");
}
void QNMessage::AddFloats(float *v,int n)
// Add an array of floats
{
  if(len<MAX_MESSAGE_LEN-n*sizeof(float))
  {
    QHost2PC_FA(v,n);
    memcpy(&buffer[len],v,n*sizeof(float));
    QPC2Host_FA(v,n);
    len+=n*sizeof(float);
  }
}
void QNMessage::AddString(cstring s)
// Add a variable size string
{
  int slen;

  slen=strlen(s)+1;
  if(slen>255)
  {
    qnError("QNMessage:AddString() can't add strings of len>255");
  }
  if(len<MAX_MESSAGE_LEN-slen)
  {
    *(unsigned char*)(&buffer[len])=(unsigned char)(slen-1);
    memcpy(&buffer[len+1],s,slen-1);
    len+=slen;
  } else qnError("QNMessage:AddInt() overflow");
}

/******************
* Deserialization *
******************/
void QNMessage::BeginGet()
// Start the unpacking of the message contents
{
  getIndex=0;
}
char QNMessage::GetChar()
{
  if(getIndex>len-sizeof(char))
  {
    qnError("QNMessage:GetChar(); get overflow");
    return 0;
  }
  getIndex++;
  return buffer[getIndex-1];
}
short QNMessage::GetShort()
{
  short v;

  if(getIndex>len-sizeof(short))
  {
    qnError("QNMessage:GetShort(); get overflow");
    return 0;
  }
  v=*(short*)(&buffer[getIndex]);
  getIndex+=sizeof(short);
  return QPC2Host_S(v);
}
int QNMessage::GetInt()
{
  int v;

  if(getIndex>len-sizeof(int))
  {
//qdbg("getIndex=%d, len=%d, sizeof(int)=%d\n",getIndex,len,sizeof(int));
    qnError("QNMessage:GetInt(); get overflow");
    return 0;
  }
#ifdef __sgi
  // Can't handle odd addresses
  memcpy(&v,&buffer[getIndex],sizeof(int));
#else
  v=*(int*)(&buffer[getIndex]);
#endif
  getIndex+=sizeof(int);
  return QPC2Host_L(v);
}
void QNMessage::GetFloats(float *v,int n)
{
  if(getIndex>len-n*sizeof(float))
  {
    qnError("QNMessage:GetFloats(); get overflow");
    return;
  }
  memcpy(v,&buffer[getIndex],n*sizeof(float));
  QPC2Host_FA(v,n);
  getIndex+=n*sizeof(float);
}
int QNMessage::GetString(string d,int maxLen)
// Get a string from the buffer into 'd', which is at least maxLen bytes large.
// Returns length of string.
{
  int slen;
  slen=GetChar();
  if(getIndex>len-slen)
  {
    qnError("QNMessage:GetString(); get overflow");
    // Clear string for safety
    *d=0;
    return 0;
  }
  memcpy(d,&buffer[getIndex],slen);
  // Zero terminate string
  d[slen]=0;
  getIndex+=slen;
  return slen;
}

