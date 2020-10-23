/*
 * QDMBPool - DMbufferpool encapsulation
 * 08-09-98: Created!
 * NOTES:
 * - Totally unsupported by Win32
 * (C) MG/RVG
 */

#include <qlib/dmbpool.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDMBPool::QDMBPool(int bufs,int bufSize,bool cacheable,bool mapped)
{
  p=new QDMParams();
  pool=0;
#ifndef WIN32
  // Set initial defaults
  dmBufferSetPoolDefaults(p->GetDMparams(),bufs,bufSize,cacheable,mapped);
#endif
  // After this, pool requirements of subsystems should be added
  // (VL, GL, dmIC)
}

QDMBPool::~QDMBPool()
{
#ifndef WIN32
  if(pool)dmBufferDestroyPool(pool);
#endif
  if(p)delete p;
}

void QDMBPool::AddProducer(QDMObject *obj)
// An object is going to use this pool, add its requirements
{
  obj->AddProducerParams(this);
}
void QDMBPool::AddConsumer(QDMObject *obj)
// An object is going to use this pool, add its requirements
{
  obj->AddConsumerParams(this);
}
bool QDMBPool::Create()
{
  //qdbg("QDMBPool:Create with these params:\n");
  //p->DbgPrint();
  //qdbg("-----\n");
#ifndef WIN32
  if(dmBufferCreatePool(p->GetDMparams(),&pool)!=DM_SUCCESS)
  { qwarn("QDMBPool::Create() failed\n");
    return FALSE;
  }
#endif
  return TRUE;
}

int QDMBPool::GetFD()
{
#ifdef WIN32
  return -1;
#else
  return dmBufferGetPoolFD(pool);
#endif
}

int QDMBPool::GetFreeBuffers()
// Returns the number of buffers that are still available
{
#ifdef WIN32
  return 0;
#else
  long long nBytes;
  int  bufs;
  if(dmBufferGetPoolState(pool,&nBytes,&bufs)!=DM_SUCCESS)
  { qwarn("QDMBPool::GetFreeBuffers() failed\n");
    return 0;
  }
  return bufs;
#endif
}

void QDMBPool::SetSelectSize(int n)
// Sets the size before a pool returns from select()
{
#ifndef WIN32
  if(dmBufferSetPoolSelectSize(pool,n)!=DM_SUCCESS)
  { qwarn("QDMBPool:SetSelectSize() failed");
  }
#endif
}

DMbuffer QDMBPool::AllocateBuffer()
{
#ifdef WIN32
  return 0;
#else
  DMbuffer b;
  if(dmBufferAllocate(pool,&b)!=DM_SUCCESS)
  { qwarn("QDMBPool:AllocateBuffer() failed");
    return 0;
  }
  return b;
#endif
}

QDMBuffer *QDMBPool::AllocateDMBuffer()
// The C++ preferred method of allocating the DM buffer (AllocateBuffer())
{
#ifdef WIN32
  return 0;
#else
  DMbuffer b;
  QDMBuffer *dmb;
  if(dmBufferAllocate(pool,&b)!=DM_SUCCESS)
  { qwarn("QDMBPool:AllocateDMBuffer() failed");
    return 0;
  }
  dmb=new QDMBuffer(b);
  return dmb;
#endif
}

