// nlib/client.h

#ifndef __NLIB_CLIENT_H
#define __NLIB_CLIENT_H

#ifndef __QLIB_TYPES_H
#include <qlib/types.h>
#endif

class QNClient
// A network client
{
 protected:
  qstring name;           // Client name
  int     id;             // Unique ID
  //ipAddr;
  int     connectTime;    // At which time did the client connect
  int     lastMsgTime;    // Time at which last msg arrived
  int     stage;          // What is the user doing?
  //messageQueue;         // List of message waiting to be operated upon

 public:
  QNClient();
  ~QNClient();

  bool Open(int port,int type);
  void Close();
};

#endif

