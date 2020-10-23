// qlib/dmbuffer.h

#ifndef __QLIB_DMBUFFER_H
#define __QLIB_DMBUFFER_H

#include <dmedia/dm_buffer.h>

class QDMBuffer
{
 protected:
  DMbuffer dmbuffer;

 public:
  QDMBuffer(DMbuffer buf=0);
  ~QDMBuffer();

  // Internal queries
  DMbuffer GetDMbuffer(){ return dmbuffer; }

  // DMbuffer operations
  int  GetSize();
  bool Free();
  bool Attach();
};

#endif
