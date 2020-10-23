// nlib/channel.h

#ifndef __NLIB_CHANNEL_H
#define __NLIB_CHANNEL_H

#ifndef __NLIB_PACKET_H
#include <nlib/packet.h>
#endif
#include <nlib/queue.h>

class QNChannel
{
  QNQueue *queueIn,           // Input/output list of messages
          *queueOut;
  QNSocket *socket;           // Socket to use
  QNAddress addrOut;          // Output address

 public:
  QNChannel();
  ~QNChannel();

  // Attribs
  QNSocket *GetSocket(){ return socket; }
  void AttachSocket(QNSocket *socket);
  QNAddress *GetAddrOut(){ return &addrOut; }

  // Linking up with other addresses
  bool SetTarget(cstring host,int port);

  // Message sending (outgoing)
  bool AddMessage(QNMessage *msg);
  // Messages incoming
  bool GetMessage(QNMessage *msg);

  // Handling incoming/outgoing data (a single poll)
  void Handle();
};

#endif

