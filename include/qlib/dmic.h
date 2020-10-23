// dmic.h - declaration
// With help from DMU

#ifndef __QLIB_DMIC_H
#define __QLIB_DMIC_H

#include <dmedia/dm_imageconvert.h>
#include <qlib/dmobject.h>

class QDMIC : public QDMObject
{
 protected:
  DMimageconverter ctx;
  // Creation params
  DMicspeed speed;
  DMcodedirection direction;
  DMimagepacking srcPacking,dstPacking;
  char *srcCompression,*dstCompression;

  // Source, destination and conversion parameters
  QDMParams *src,*dst,*conv;

  // Source AND destination size
  int wid,hgt;
  float quality;

  // Results
  int bufSize;			// Size of a fullblown frame
  int fd;

 public:
  QDMIC(int wid,int hgt);
  ~QDMIC();

  // Internal things
  DMimageconverter GetDMimageconverter(){ return ctx; }

  void AddProducerParams(QDMBPool *p);	// Encode output
  void AddConsumerParams(QDMBPool *p);
  //void SetDstPool(QDMBPool *pool);
  bool Create();

  int  GetFD();
  int  GetFrameSize();			// Size of uncompressed frame

  int  GetSrcFilled();			// #items in the to-do queue
  int  GetDstFilled();			// #items processed and ready

  // Modifying parameters after creation
  void SetSourceSize(int wid,int hgt);
  void SetDestinationSize(int wid,int hgt);
  void SetSize(int wid,int hgt);

  // Processing
  void Convert(DMbuffer buf);
};

#endif
