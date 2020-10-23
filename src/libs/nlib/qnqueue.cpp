/*
 * QNQueue - a list of messages to be processed
 * 12-08-01: Created!
 * (c) Dolphinity/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNQueue::QNQueue(int maxMsg)
{
  maxMessage=maxMsg;

  // Allocate space for all message ptrs
  msg=(QNMessage**)qcalloc(maxMessage*sizeof(QNMessage*));
  if(!msg)
  {
    qnError("QNQueue ctor: out of memory");
  }

}
QNQueue::~QNQueue()
{
  QFREE(msg);
}

