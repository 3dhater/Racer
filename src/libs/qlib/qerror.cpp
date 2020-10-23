/*
 * Error handling from X11/OpenGL etc.
 * 21-02-99: Created!
 * 23-11-00: OpenGL state expanded to a class.
 * (C) MarketGraph/RVG
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <dmedia/dmedia.h>
#include <dmedia/moviefile.h>
#include <GL/glx.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/***************
* OPENGL STATE *
***************/
void QShowGLErrors(cstring s)
{
  GLenum error;
  while((error=glGetError())!=GL_NO_ERROR)
    qerr("%s: OpenGL error (%d): %s",s,error,gluErrorString(error));
}
void QShowGLState(cstring txt,int flags)
// Output a number of (hopefully) relevant OpenGL states to find
// out why 1 situation works, and the other doesn't
// Created upon seeing glClear() clear part of the FRONT buffer when asked
// to clear the BACK buffer in the 'supermarket' MT app.
{
  QGLState s;
  s.Update();
  s.DbgPrint(txt);
}

/***********
* QGLSTATE *
***********/
QGLState::QGLState()
{
}
QGLState::~QGLState()
{
}

void QGLState::Update()
// Get current state of OpenGL pipeline
// Quite expensive, but useful during debugging
{
  GLboolean b;

  // Current values and associated data
  // (see the Red Book, page 423 onwards)
  glGetFloatv(GL_CURRENT_RASTER_POSITION,rasPos);
  glGetFloatv(GL_CURRENT_RASTER_DISTANCE,&rasDist);
  glGetFloatv(GL_CURRENT_RASTER_COLOR,rasColor);
  glGetFloatv(GL_CURRENT_COLOR,curColor);

  glGetIntegerv(GL_DRAW_BUFFER,&drawBuffer);
  glGetIntegerv(GL_READ_BUFFER,&readBuffer);
  
  // Coloring
  glGetIntegerv(GL_SHADE_MODEL,&shadeModel);

  glGetIntegerv(GL_UNPACK_ROW_LENGTH,&unpackRowLen);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS,&unpackSkipRows);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&unpackSkipPixels);
  glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpackAlignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH,&packRowLen);
  glGetIntegerv(GL_PACK_SKIP_ROWS,&packSkipRows);
  glGetIntegerv(GL_PACK_SKIP_PIXELS,&packSkipPixels);
  glGetIntegerv(GL_PACK_ALIGNMENT,&packAlignment);

  glGetFloatv(GL_ZOOM_X,&zoomY);
  glGetFloatv(GL_ZOOM_Y,&zoomX);

  // Flags: valid
  flagsValid=0;
  glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID,&b);
  if(b)flagsValid|=QGLFV_RASTERPOS;

  // Flags: light
  flagsLight=0;
  if(glIsEnabled(GL_LIGHTING))flagsLight|=QGLFL_LIGHTING;
  if(glIsEnabled(GL_NORMALIZE))flagsLight|=QGLFL_NORMALIZE;

  // Flags: tests
  flagsTests=0;
  if(glIsEnabled(GL_SCISSOR_TEST))flagsTests|=QGLFT_SCISSOR;
  if(glIsEnabled(GL_DEPTH_TEST))flagsTests|=QGLFT_DEPTH;
  if(glIsEnabled(GL_ALPHA_TEST))flagsTests|=QGLFT_ALPHA;
  if(glIsEnabled(GL_STENCIL_TEST))flagsTests|=QGLFT_STENCIL;

  // Flags: misc
  flagsMisc=0;
  if(glIsEnabled(GL_CULL_FACE))flagsMisc|=QGLFM_CULL_FACE;
  if(glIsEnabled(GL_DITHER))flagsMisc|=QGLFM_DITHER;
  if(glIsEnabled(GL_BLEND))flagsMisc|=QGLFM_BLEND;
  glGetBooleanv(GL_RGBA_MODE,&b);
  if(b)flagsMisc|=QGLFM_RGBA_MODE;
  glGetBooleanv(GL_DOUBLEBUFFER,&b);
  if(b)flagsMisc|=QGLFM_DOUBLEBUFFER;
}

bool QGLState::Compare(QGLState *other)
// Returns TRUE if both states seem equal
{
  // Not sure whether the next only looks at member variables,
  // or virtual functions too (though there aren't any)
  if(memcmp(this,other,sizeof(QGLState)))
  {
    return FALSE;
  }
  // No differences found
  return TRUE;
}

void QGLState::DbgPrint(cstring t)
// Dumps the OpenGL state on the Q debug stream
{
#if WIN32
  qdbg("OpenGL state: (context %p) (%s)\n",wglGetCurrentContext(),t?t:"");
#else
  qdbg("OpenGL state: (context %p) (%s)\n",glXGetCurrentContext(),t?t:"");
#endif

  //GLfloat vf[4];
  GLboolean vb[4];
  //GLint vi[4];
  qdbg("Raster position      : %f,%f,%f,%f\n",
    rasPos[0],rasPos[1],rasPos[2],rasPos[3]);
  qdbg("Raster color         : %f,%f,%f,%f\n",
    rasColor[0],rasColor[1],rasColor[2],rasColor[3]);
  qdbg("Raster position valid: %d\n",(flagsValid&QGLFV_RASTERPOS)!=0);
  qdbg("Shade model: %d\n",shadeModel);
  vb[0]=(flagsLight&QGLFL_LIGHTING)!=0;
  vb[1]=(flagsMisc&QGLFM_CULL_FACE)!=0;
  vb[2]=(flagsMisc&QGLFM_BLEND)!=0;
  vb[3]=(flagsMisc&QGLFM_DITHER)!=0;
  qdbg("Enabled: lighting=%d, cullface=%d, blend=%d, dither=%d\n",
    vb[0],vb[1],vb[2],vb[3]);
  vb[0]=(flagsTests&QGLFT_SCISSOR)!=0;
  vb[1]=(flagsTests&QGLFT_STENCIL)!=0;
  vb[2]=(flagsTests&QGLFT_ALPHA)!=0;
  vb[3]=(flagsTests&QGLFT_DEPTH)!=0;
  qdbg("Tests  : scissor =%d, stencil =%d, alpha=%d, depth =%d\n",
    vb[0],vb[1],vb[2],vb[3]);
  qdbg("Buffers: draw=%d, read=%d\n",drawBuffer,readBuffer);

  // Packing
  qdbg("Unpack : rowlength=%d, skip rows=%d, skip pixels=%d, alignment=%d\n",
    unpackRowLen,unpackSkipRows,unpackSkipPixels,unpackAlignment);

  qdbg("Pack   : rowlength=%d, skip rows=%d, skip pixels=%d, alignment=%d\n",
    packRowLen,packSkipRows,packSkipPixels,packAlignment);

  qdbg("Zoom   : X: %f, Y: %f\n",zoomX,zoomY);
  vb[0]=(flagsMisc&QGLFM_RGBA_MODE)!=0;
  vb[1]=(flagsMisc&QGLFM_DOUBLEBUFFER)!=0;
  qdbg("RGBA mode: %d, double buffer: %d\n",vb[0],vb[1]);
  QShowGLErrors((string)t);
  qdbg("----------------\n");
}

/***********************
* DIGITAL MEDIA ERRORS *
***********************/
void QShowDMErrors(cstring s)
{
#ifdef WIN32
#endif
#ifdef linux
#endif
#if defined(__sgi)
  string err;
  char error_detail[DM_MAX_ERROR_DETAIL];
  int  n;

  while(1)
  { err=(string)dmGetError(&n,error_detail);
    if(!err)break;
    qerr("%s: DM error (%d): %s (%s)",s,n,err,error_detail);

    break;		// Only 1 error is stored in DM
  }
#endif
}

void QShowMVErrors(cstring s)
{
#ifdef WIN32
#endif
#ifdef linux
#endif
#if defined(__sgi)
  int n;
  n=mvGetErrno();
  if(n)
    qerr("%s: MV error (%d): %s",s,n,mvGetErrorStr(n));
#endif
}

