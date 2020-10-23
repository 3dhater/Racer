// QDMPBuffer.h - declaration

#ifndef __QDMPBUFFER_H
#define __QDMPBUFFER_H

#include <qlib/dmobject.h>
#include <qlib/dmbuffer.h>
#include <qlib/opengl.h>
#include <GL/glx.h>

#ifdef linux
// Linux has GLX but no SGI pbuffers
typedef void *GLXFBConfigSGIX;
typedef void *GLXPbufferSGIX;
#endif

class QDMPBuffer : public QDMObject
{
 protected:
  // Class member variables
  GLXFBConfigSGIX *fbConfig;	// Frame buffer config
  GLXPbufferSGIX   pBuffer;
  //GLXContext       pBufCtx;
  QGLContext      *glCtx;	// GLX/OpenGL Context
  DMparams *gfxParams;		// Describes the pbuffer
  int       wid,hgt;		// Size in pixels

 public:
  QDMPBuffer(int wid,int hgt);
  ~QDMPBuffer();

  //GLXContext GetGLXContext(){ return pBufCtx; }
  QGLContext *GetGLContext(){ return glCtx; }
  GLXPbufferSGIX GetGLXPBuffer(){ return pBuffer; }

  int  GetFD();

  void AddProducerParams(QDMBPool *p);
  void AddConsumerParams(QDMBPool *p);
  void RegisterPool(QDMBPool *pool);

  // Make this pbuffer the current OpenGL GLX context
  void Select();

  void Start();
  //void Associate(DMbuffer dmbuffer);
  void Associate(QDMBuffer *dmb);
  void CopyTexture(QGLContext *dstGL);
};

#endif
