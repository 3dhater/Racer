// dmbpool.h - declaration

#ifndef __QLIB_DMBPOOL_H
#define __QLIB_DMBPOOL_H

#include <qlib/dmparams.h>
#include <qlib/dmobject.h>
#include <dmedia/dm_buffer.h>

class QDMBPool
{
 protected:
  DMbufferpool pool;
  QDMParams *p;

 public:
  QDMBPool(int bufs,int bufSize,bool cacheable=FALSE,bool mapped=FALSE);
  ~QDMBPool();

  // Internal things
  DMbufferpool GetDMbufferpool(){ return pool; }
  QDMParams *GetCreateParams(){ return p; }

  // Adding users
  //void AddUser(QDMObject *obj);		// Add a pool user
  void AddProducer(QDMObject *obj);
  void AddConsumer(QDMObject *obj);

  int  GetFD();
  bool Create();

  int  GetFreeBuffers();
  void SetSelectSize(int n);
  DMbuffer AllocateBuffer();
  QDMBuffer *AllocateDMBuffer();
};

#endif
