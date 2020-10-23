// pbuffer.h - declaration

#ifndef __QLIB_PBUFFER_H
#define __QLIB_PBUFFER_H

#include <qlib/drawable.h>

class QPBuffer : public QDrawable
{
 public:
  string ClassName(){ return "pbuffer"; }

 protected:
  // Creation
  int cwid,chgt;

  // Class member variables
  GLXFBConfigSGIX *fbConfig;	// Frame buffer config
  GLXPbufferSGIX   pBuffer;
  QGLContext      *glCtx;	// GLX/OpenGL Context
  DMparams        *gfxParams;	// Describes the pbuffer

 public:
  QPBuffer(int wid,int hgt);
  ~QPBuffer();

  // QDrawable virtuals
  GLXDrawable GetGLXDrawable(){ return pBuffer; }

  //QGLContext *GetGLContext(){ return glCtx; }

  void Associate(QDMBuffer *dmb);
  void CopyTexture(QGLContext *dstGL);
};

#endif
