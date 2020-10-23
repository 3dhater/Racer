// nlib/queue.h

#ifndef __NLIB_QUEUE_H
#define __NLIB_QUEUE_H

#ifndef __NLIB_PACKET_H
#include <nlib/packet.h>
#endif

class QNQueue
// A queue with messages to be processed
// Note the explicit lack of dynamic allocation
{
  int maxMessage;             // Max #messages stored
  QNMessage **msg;

 public:
  QNQueue(int maxMessage);
  ~QNQueue();

  void AddMessage(QNMessage *msg);
};

#endif

