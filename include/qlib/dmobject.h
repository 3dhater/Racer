// dmobject.h - declaration

#ifndef __QLIB_DMOBJECT_H
#define __QLIB_DMOBJECT_H

#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <qlib/dmparams.h>
#include <qlib/dmbuffer.h>

class QDMBPool;

class QDMObject
{
 public:
  QDMObject *next,*prev;

 public:
  QDMObject();
  virtual ~QDMObject();

  // Pool attribs
  //virtual void AddPoolParams(QDMBPool *p);	// Get necessary params
  virtual void AddProducerParams(QDMBPool *p);	// Get necessary params
  virtual void AddConsumerParams(QDMBPool *p);	// Get necessary params
  virtual void RegisterPool(QDMBPool *pool);

  // Start doing things
  virtual void Start();
  virtual int  GetFD();
};

#endif
