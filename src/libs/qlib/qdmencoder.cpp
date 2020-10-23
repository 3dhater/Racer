/*
 * QDMIC - dmIC encapsulation
 * 08-09-98: Created!
 * (C) MG/RVG
 */

#include <qlib/dmencoder.h>
#include <qlib/dmbpool.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDMEncoder::QDMEncoder(int w,int h,int qual)
  : QDMIC(w,h)
{
  quality=((double)qual)/100;
  direction=DM_IC_CODE_DIRECTION_ENCODE;
  srcCompression=DM_IMAGE_UNCOMPRESSED;
  dstCompression=DM_IMAGE_JPEG;
}
QDMEncoder::~QDMEncoder()
{
}

void QDMEncoder::SetDstPool(QDMBPool *pool)
{ // Set pool as destination
  dmICSetDstPool(ctx,pool->GetDMbufferpool());
}

void QDMEncoder::Convert(DMbuffer buf)
{
  //DEBUG_C(qdbg("QDMIC::Convert()\n"));
  // A buffer is received, should be encoder
  if(dmICSend(ctx,buf,0,0)!=DM_SUCCESS)
  { int e;
    const char *s;
    char e_long[DM_MAX_ERROR_DETAIL];
    s=dmGetError(&e,e_long);
    qerr("QDMEncoder::Convert() failed to dmICSend(); %d: %s (%s)",e,e_long,s);
  }
}

