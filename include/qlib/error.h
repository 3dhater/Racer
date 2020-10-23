// qlib/error.h

#ifndef __QLIB_ERROR_H
#define __QLIB_ERROR_H

#include <qlib/types.h>

enum QGLFlagsLight
{
  QGLFL_LIGHTING=1,
  QGLFL_NORMALIZE=2,
  QGLFL_XXX
};
enum QGLFlagsTests
{
  QGLFT_SCISSOR=1,
  QGLFT_ALPHA=2,
  QGLFT_DEPTH=4,
  QGLFT_STENCIL=8,
  QGLFT_XXX
};
enum QGLFlagsValid
{
  QGLFV_RASTERPOS=1,
  QGLFV_XXX
};
enum QGLFlagsMisc
{
  QGLFM_CULL_FACE=1,
  QGLFM_BLEND=2,
  QGLFM_DITHER=4,
  QGLFM_DOUBLEBUFFER=8,
  QGLFM_RGBA_MODE=16,
  QGLFM_XXX
};

class QGLState
// Contains a full state
// Useful to compare OpenGL states when problems arise
// Not ALL state is stored here, but it may grow as new problems appear
{
 public:
  float curColor[4];
  float rasPos[4];
  float rasDist;
  float rasColor[4];
  int   flagsLight,flagsTests,
        flagsMisc,flagsValid;
  int   drawBuffer,readBuffer,
        shadeModel;
  int   unpackRowLen,unpackSkipRows,
        unpackSkipPixels,unpackAlignment;
  int   packRowLen,packSkipRows,
        packSkipPixels,packAlignment;
  float zoomX,zoomY;
 public:
  QGLState();
  ~QGLState();

  void Update();
  bool Compare(QGLState *otherState);

  void DbgPrint(cstring txt);
};

void QShowGLErrors(cstring s);
void QShowDMErrors(cstring s);
void QShowMVErrors(cstring s);

void QShowGLState(cstring s=0,int flags=0);

#endif
