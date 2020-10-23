// dmencoder.h - declaration for JPEG encoder

#ifndef __QLIB_DMENCODE_H
#define __QLIB_DMENCODE_H

#include <qlib/dmic.h>

class QDMEncoder : public QDMIC
{
 protected:

 public:
  QDMEncoder(int wid,int hgt,int quality);
  ~QDMEncoder();

  void SetDstPool(QDMBPool *pool);

  // Processing
  void Convert(DMbuffer buf);
};

#endif
