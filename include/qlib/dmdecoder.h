// dmdecoder.h - declaration for JPEG encoder

#ifndef __QLIB_DMDECODER_H
#define __QLIB_DMDECODER_H

#include <qlib/dmic.h>

class QDMDecoder : public QDMIC
{
 protected:

 public:
  QDMDecoder(int wid,int hgt);
  ~QDMDecoder();

  void SetDstPool(QDMBPool *pool);
};

#endif
