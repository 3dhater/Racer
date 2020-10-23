// nlib/server.h

#ifndef __NLIB_SERVER_H
#define __NLIB_SERVER_H

#ifndef __QLIB_TYPES_H
#include <qlib/types.h>
#endif

class QNServer
// A network server
{
 protected:
  qstring name;           // Server name
  int     id;             // Unique ID
  //ipAddr;
  int     stage;          // What is the app doing?
  //messageQueue;         // List of message waiting to be operated upon

 public:
  QNServer();
  ~QNServer();

  bool Open(int port,int type=0);
  void Close();
};

#endif

