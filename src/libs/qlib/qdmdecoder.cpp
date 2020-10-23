/*
 * QDMIC - dmIC encapsulation
 * 08-09-98: Created!
 * (C) MG/RVG
 */

#include <qlib/dmdecoder.h>
#include <qlib/dmbpool.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDMDecoder::QDMDecoder(int w,int h)
  : QDMIC(w,h)
{
  direction=DM_IC_CODE_DIRECTION_DECODE;
  srcCompression=DM_IMAGE_JPEG;
  dstCompression=DM_IMAGE_UNCOMPRESSED;
}
QDMDecoder::~QDMDecoder()
{
}

void QDMDecoder::SetDstPool(QDMBPool *pool)
{ // Set pool as destination
  dmICSetDstPool(ctx,pool->GetDMbufferpool());
}

