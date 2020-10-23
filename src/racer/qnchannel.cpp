/*
 * QNChannel - a transportation channel
 * 11-08-01: Created!
 * (c) Dolphinity/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNChannel::QNChannel()
{
  int maxMsg=100;

  socket=0;

  // Setup message queues for in and output
  queueIn=new QNQueue(maxMsg);
  queueOut=new QNQueue(maxMsg);
}
QNChannel::~QNChannel()
{
  QDELETE(queueIn);
  QDELETE(queueOut);
}

/**********
* Attribs *
**********/
void QNChannel::AttachSocket(QNSocket *sock)
// Declare 'socket' to be used for communications
// Perhaps we should allocate our own
{
  socket=sock;
}

/**********
* Linking *
**********/
bool QNChannel::SetTarget(cstring host,int port)
// Convenience function to set host and port for the communication endpoint
// 'host' can be a name or IP address ('myhost', '127.0.0.1').
// Assumes internet address, UDP.
// Returns FALSE if target couldn't be set.
{
  addrOut.SetAttr(AF_INET,INADDR_ANY,port);
  addrOut.GetByName(host);
  return TRUE;
}

/***********
* Messages *
***********/
bool QNChannel::AddMessage(QNMessage *msg)
// Adds a message to be output at the next occasion
// Message will be sent to the associated address (inside 'msg')
{
  if(!socket)
  {
    qnError("QNChannel:AddMessage(); no socket");
    return FALSE;
  }

  // For now, make a packet immediately
  // Really a shortcut; it should go into
  // queueIn, then a packet should be created
  // in 'Handle', possibly with other msgs
  // Then sent out the machine, possibly
  // removed from the queue etc.
  //if(socket->Write(msg->GetBuffer(),msg->GetLength(),&addrOut)<0)
//qdbg("QNChannel:AddMessage(); going to '%s'\n",msg->GetAddrFrom()->ToString());
  if(socket->Write(msg->GetBuffer(),msg->GetLength(),msg->GetAddrFrom())<0)
    return FALSE;
  return TRUE;
}
bool QNChannel::GetMessage(QNMessage *msg)
// Returns a queued incoming message in 'msg', and returns TRUE.
// Returns FALSE if no incoming messages exist.
// Note that QNMessage will contain the address from which the packet
// originated.
{
  int n;

  // Normall,y let 'Handle()' do the rough work
  // Quick start here; just read the socket directly
  n=socket->Read(msg->GetBuffer(),msg->GetBufferLength(),msg->GetAddrFrom());
  if(n<=0)
  {
    // Error or no bytes available
    return FALSE;
  }
  // Got a message
//qdbg("QNChannel:GetMessage(); n=%d\n",n);
  msg->SetLength(n);

  return TRUE;
}

void QNChannel::Handle()
// Call this function to handle the queues; output messsages from queueOut
// and input incoming messages into queueIn.
{

  // Output
  // Output is done directly in AddMessage(). Just starting this.
  //...

  // Input
  // Check for any new incoming packets
}

