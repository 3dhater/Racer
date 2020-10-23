// nlib/packet.h

#ifndef __NLIB_PACKET_H
#define __NLIB_PACKET_H

#include <nlib/message.h>

class QNPacket
{
 public:
  enum Max
  {
    MAX_PACKET_LEN=4000          // Just below UDP limit
  };
 protected:
  char *buffer;
  int   len;                     // Currently used #bytes
  int   flags;

 public:
  QNPacket();
  ~QNPacket();

  // Adding messages
  void Clear();
  bool AddMessage(QNMessage *msg);
};

#endif

