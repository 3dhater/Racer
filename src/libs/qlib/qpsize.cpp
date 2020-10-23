/*
 * QProcessSize - process size determination
 * 11-07-99: Created!
 * (C) MarketGraph/RvG
 */

#include <qlib/psize.h>
#include <unistd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QProcessSize::QProcessSize()
{
  UpdateBrk();
}
QProcessSize::~QProcessSize()
{
}

void QProcessSize::UpdateBrk()
{
  lastBrk=sbrk(0);
}

int QProcessSize::GetSize()
// Returns +/- the size of the process
{
  void *oldBrk=lastBrk;
  UpdateBrk();
  return (int)lastBrk;
}

bool QProcessSize::AutoDetectChange(string s)
// Checks if process size has changed since the last update
// and reports changes if so.
{
  void *oldBrk=lastBrk;
  UpdateBrk();
  if(lastBrk!=oldBrk)
  { int d;
    d=(int)lastBrk-(int)oldBrk;
    qdbg("%s: process size changed with %d (0x%x) bytes to 0x%x\n",
      s,d,d,GetSize());
    return true;
  }
  return false;
}

