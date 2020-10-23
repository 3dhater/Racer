// nlib/message.h

#ifndef __NLIB_MESSAGE_H
#define __NLIB_MESSAGE_H

#ifndef __QLIB_TYPES_H
#include <qlib/types.h>
#endif
#include <nlib/socket.h>

struct QNMessage
// A generic message
{
  enum Max
  {
    MAX_MESSAGE_LEN=4000
  };
  enum Flags
  {
    RELIABLE=1                // Message must make it across
  };

  char  id;
  char *buffer;
  int   len;                  // Currently used space
  int   getIndex;             // Where to read?
  char  flags;
  QNAddress addrFrom;         // Where did it come from?

 public:
  QNMessage();
  virtual ~QNMessage();

  // Attribs
  void  MakeReliable();
  bool  IsReliable(){ if(flags&RELIABLE)return TRUE; return FALSE; }
  int   GetLength(){ return len; }
  void  SetLength(int _len){ len=_len; }
  int   GetBufferLength(){ return MAX_MESSAGE_LEN; }
  char *GetBuffer(){ return buffer; }
  QNAddress *GetAddrFrom(){ return &addrFrom; }

  // Message building (serialization)
  void Clear();
  void AddChar(char v);
  void AddShort(short v);
  void AddInt(int v);
  void AddFloats(float *v,int n);
  //void AddLong(long v);
  void AddString(cstring s);

  // Deserialization (unpacking)
  void  BeginGet();
  char  GetChar();
  short GetShort();
  int   GetInt();
  void  GetFloats(float *v,int n);
  int   GetString(string d,int maxLen=256);
};

// Useful messages
enum MsgId
{
  QN_MSG_LOGINREQUEST=1,
  QN_MSG_LOGOFF,
  QN_MSG_xxx
};

struct QNMsgLoginRequest : public QNMessage
{
  char app;
  char versionMajor,
       versionMinor;
  char loginName[32];
  char password[32];
};

#endif
