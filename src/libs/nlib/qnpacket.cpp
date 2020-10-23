/*
 * QNPacket - a transportation channel
 * 11-08-01: Created!
 * (c) Dolphinity/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNPacket::QNPacket()
{
  len=0;

  // Allocate buffer
  buffer=(char*)qcalloc(MAX_PACKET_LEN);
  if(!buffer)
  { qwarn("QNPacket ctor; not enough memory");
  }
}
QNPacket::~QNPacket()
{
  QFREE(buffer);
}

/****************
* Serialization *
****************/
void QNPacket::Clear()
// Note that this also clears flags like RELIABLE!
{
  len=0;
  // Clear message flags
  flags=0;
}
bool QNPacket::AddMessage(QNMessage *msg)
// Try to add a message to this packet. Returns TRUE
// if succesful, and FALSE if there wasn't enough space.
{
  if(len+msg->GetLength()<=MAX_PACKET_LEN)
  {
    // Enter into this packet
    memcpy(&buffer[len],msg->GetBuffer(),msg->GetLength());
    len+=msg->GetLength();
    return TRUE;
  } else
  {
    return FALSE;
  }
}

