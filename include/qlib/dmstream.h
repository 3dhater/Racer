// QDMStream.h - declaration

#ifndef __QDMSTREAM_H
#define __QDMSTREAM_H

#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <qlib/dmparams.h>
#include <qlib/dmbpool.h>
#include <qlib/dmobject.h>

class QDMStream
{
 protected:
  // Class member variables
  QDMObject *root;          // Tree of DM users (nodes)
  //DMbufferpool pool;		// The pool
  QDMBPool *pool;		// The pool

 public:
  QDMStream();
  ~QDMStream();

  int  GetMaxFD();
  void Add(QDMObject *obj);
  void Start();
  void Create();
};

#endif
