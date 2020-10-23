/*
 * QCanvas.cpp - graphics context to paint in drawables
 * 28-04-97: Created! (from qgc.cpp/qlib_Xt)
 * 28-02-99: Uses QDrawable instead of QWindow now.
 * 03-07-99: 2D and 3D modes; more and more 3D work being used (MT's MC)
 * 06-09-99: Blending enhanced; constants brought into the class.
 * NOTES:
 * - Handles reversing of Y coordinates, to make (0,0) the ulhand corner
 *   (if OpenGL is used; X uses the nicer coordinate system for us)
 * - Has a 2D and 3D mode; in 3D, (0,0) is at the centre of the drawable
 *   and the Y+ is upwards to the top of the monitor
 * - Optimize this file (memory drawing functions)
 * - QCanvas is supposed to be a secure place to draw things, but it is
 * not the fastest way, since it resets multiple parameters with each
 * call to a drawing function. But it must be this way, since direct OpenGL
 * calls and canvas calls to the same GLXContext are often mixed. Otherwise,
 * blend mode and such would not have to be reset every time.
 * BUGS:
 * - Clear() touches the glScissor() settings
 * - Clear() uses glClear(), which is probably overkill (use Rectfill())
 * - TextML() touches the given text, so multithreaded apps that use
 * the text must watch out. It returns it unchanged, however.
 * - A lot of paint functions don't look at ISX and just use OpenGL
 * - Line() should reinstate lineStipple pattern/factor
 * - LineStipple() should remember pattern/factor, not do it
 * - Enable(QCanvas::NO_FLIP) only works for QCanvas::Rectfill() sofar.
 * FUTURE:
 * - Blend in more with the GLXContext (more efficient)
 * - 2D and 3D mode should blend in; convert (0,0)-top-left coordinate
 * system into 3D coordinates and always use the 3D projection matrix.
 * It seems switching the matrices is a costly operation.
 * (C) MarketGraph/RVG
 */

#include <qlib/canvas.h>
#include <qlib/opengl.h>
#include <qlib/gc.h>
#include <qlib/blitq.h>
#include <qlib/blit.h>
#include <qlib/font.h>
#include <qlib/app.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QCanvas"

// Define LOG to get canvas info while running
//#define LOG

// Define the next bug to work around the O2's bug (24-6-97) that
// glReadPixels() using a subrectangle does bad read (G=B=0 from line1
// onward).
#define BUG_O2_READPIXELS

// Define SHADY_UI to get nicely shaded UI insides.
// However, this gives problems when restoring/painting subrects (Expose)
#define SHADY_UI

#define ISX	((flags&USEX)!=0)

/*******
* CTOR *
*******/
QCanvas::QCanvas(QDrawable *idrw)
{
  //DBG_F("ctor(idrw)");
  drw=idrw;
  flags=0;
  font=0;
  offsetx=offsety=0;
  clipx=clipy=clipwid=cliphgt=0;
  color=new QColor(255,255,255);
  textStyle=0;
  bq=0;
  gl=0;
  gc=0;
  //mode=QCANVAS_SINGLEBUF;
  mode=DOUBLEBUF;
  blendMode=BLEND_OFF;
}
QCanvas::QCanvas(QCanvas *cv)
// Create a copy of another canvas
{
  drw=cv->GetDrawable();
  gl=cv->GetGLContext();
  // As QCanvas(QWindow *)
  flags=0;
  font=cv->GetFont();
  offsetx=offsety=0;
  clipx=clipy=clipwid=cliphgt=0;
  color=new QColor(255,255,255);
  color->Set(cv->GetColor());
  textStyle=cv->GetTextStyle();
  bq=0;
  gc=0;
  SetMode(cv->GetMode());
  blendMode=BLEND_OFF;
}
QCanvas::~QCanvas()
{
  QDELETE(bq);
  QDELETE(color);
}

static void SetGLColor(QColor *color,int whichCol=0)
// Local function to convert rgba to float
// whichCol: 0=glColor4f(), 1=glBlendColor(EXT)
// Assumes right GLXContext is active
{
  GLfloat cr,cg,cb,ca;
  cr=(GLfloat)color->GetR()/255;
  cg=(GLfloat)color->GetG()/255;
  cb=(GLfloat)color->GetB()/255;
  ca=(GLfloat)color->GetA()/255;
  //glColor4i(color->GetR(),color->GetG(),color->GetB(),color->GetA());
  if(whichCol==0)glColor4f(cr,cg,cb,ca);
  else if(whichCol==1)
  {
#ifdef __sgi
    // May exist on other platforms though, more thorough
    // checking should be done
    glBlendColorEXT(cr,cg,cb,ca);
#else
    // Not supported
    qerr("SetGLColor; whichCol=1 not supported on non-SGI");
#endif
  }
}

void QCanvas::SetFont(QFont *nfont)
{
  DBG_C("SetFont");
  DBG_ARG_P(nfont);

  font=nfont;
  if(gc)gc->SetFont(font);
}
QFont *QCanvas::GetFont()
{ return font;
}

void QCanvas::Enable(int newFlags)
{
#ifdef DEBUG
  // Check flags
#endif
  flags|=newFlags;
}
void QCanvas::Disable(int clrFlags)
{
#ifdef DEBUG
  // Check flags
#endif
  flags&=~clrFlags;
}

/***********
* BLENDING *
***********/
void QCanvas::InstallBlendMode()
// Protected member which sets the OpenGL blend mode to whatever
// was installed into the canvas.
{
  switch(blendMode)
  {
    case BLEND_OFF:
      glDisable(GL_BLEND); break;
    case BLEND_SRC_ALPHA:
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      break;
    case BLEND_COMPOSITE:
      // This is useful for TVPaint-style blending
      // In fact, this may be the most common blending function needed
      // by us to adhere to TVPaint blending standards.
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
      break;
    case BLEND_CONSTANT:
      glEnable(GL_BLEND);
      SetGLColor(&blendColor,1);
      //glBlendColorEXT(0,0,0,((GLfloat)den)/((GLfloat)nom));
      //glBlendFunc(GL_CONSTANT_ALPHA_EXT,GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
#ifdef WIN32
      qerr("glBlendFunc(GL_CONSTANT_COLOR_EXT... not supported on Win32");
#else
      glBlendFunc(GL_CONSTANT_COLOR_EXT,GL_ONE_MINUS_CONSTANT_COLOR_EXT);
#endif
      break;
  }
}

void QCanvas::Blend(bool yn)
// Sets blending on or off. Old obsolete call; use SetBlending() instead.
// This call only turns on the default BLEND_SRC_ALPHA mode
{
  if(yn)SetBlending(BLEND_SRC_ALPHA);
  else  SetBlending(BLEND_OFF);
}
void QCanvas::SetBlendColor(QColor *bCol)
{
  blendColor.Set(bCol);
}
void QCanvas::SetBlending(int mode,QColor *bCol)
// Sets blending mode (enum BlendModes); and optionally blend color
{
  blendMode=mode;
  if(bCol)SetBlendColor(bCol);
}

/**********
* ATTRIBS *
**********/
int QCanvas::GetWidth()
{ return drw->GetWidth();
}
int QCanvas::GetHeight()
{ return drw->GetHeight();
}

bool QCanvas::IsEnabled(int flag)
{
  if(flags&flag)return TRUE;
  return FALSE;
}

void QCanvas::UseX()
// Use GC from X11
{
  if(!gc)
  { QASSERT_V(drw);		// GC needs window (QCanvas::UseX)
    gc=new QGC(drw);
  }
  flags&=USEBM;
  flags|=USEX;
}
void QCanvas::UseGL()
{ flags&=~(USEX|USEBM);
  // Destroy GC?
}
void QCanvas::UseBM(QBitMap *dbm)
{ flags&=USEX;
  flags|=USEBM;
  bm=dbm;
}

bool QCanvas::UsesX()
{ if(flags&USEX)return TRUE;
  return FALSE;
}
bool QCanvas::UsesGL()
{ if(flags&USEX)return FALSE;
  if(flags&USEBM)return FALSE;
  return TRUE;
}
bool QCanvas::UsesBM()
{ if(flags&USEBM)return TRUE;
  return FALSE;
}

void QCanvas::SetMode(int imode)
// Set paint mode (in which buffers to draw)
{ mode=imode;
  Select();
  //qdbg("QCV: SetMode(%d) for %p\n",mode,this);
  if(mode==SINGLEBUF)
    //glDrawBuffer(GL_FRONT_AND_BACK);
    gl->DrawBuffer(GL_FRONT);
  else
    gl->DrawBuffer(GL_BACK);
}

/********
* 2D/3D *
********/
void QCanvas::Set2D()
// Set default 2D mode
// Optimized to not do anything if 2D is already on
{
  if(!(flags&IS3D))return;
//qdbg("QCanvas::Set2D()\n");
  // Initialize 2D mode
  flags&=~IS3D;
  Select();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  gl->Setup2DWindow();
}
void QCanvas::Set3D()
// Set default 3D mode
// Optimized to not do anything if 3D is already on
{
  if(flags&IS3D)return;
//qdbg("QCanvas::Set3D()\n");
  flags|=IS3D;
  gl->Setup3DWindow();
  // Set default 3D state
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}
bool QCanvas::Is2D()
{ if(flags&IS3D)return FALSE;
  return TRUE;
}
bool QCanvas::Is3D()
{ if(flags&IS3D)return TRUE;
  return FALSE;
}
void QCanvas::Map2Dto3D(int *px,int *py)
// Convert 2D coordinates to 3D equivalent (on Z=0)
{
  int w,h;
  w=GetWidth(); h=GetHeight();
  // X axis is shifted to the left
  *px=*px-w/2;
  // Y axis is flipped AND shifted
  *py=-(*py-h/2);
}
void QCanvas::Map3Dto2D(int *px,int *py,int *pz)
// NYI (convert 3D coords to 2D)
{
  int w,h;
  w=GetWidth(); h=GetHeight();
  // X axis is shifted to the left
  *px=*px+w/2;
  // Y axis is flipped AND shifted
  *py=-(*py-h/2);
}

void QCanvas::SetGLContext(QGLContext *ngl)
{ gl=ngl;
}

void QCanvas::Select()
// Make our canvas context current
// Needed for OpenGL
{
  if(!ISX)
    gl->Select();
}

void QCanvas::SetOffset(int ox,int oy)
{ offsetx=ox;
  offsety=oy;
}
void QCanvas::SetClipRect(int x,int y,int wid,int hgt)
{ clipx=x;
  clipy=y;
  clipwid=wid; cliphgt=hgt;
}
void QCanvas::SetClipRect(QRect *r)
{ SetClipRect(r->x,r->y,r->wid,r->hgt);
}
void QCanvas::GetClipRect(QRect *r)
{ r->x=clipx;
  r->y=clipy;
  r->wid=clipwid;
  r->hgt=cliphgt;
}

/********
* COLOR *
********/
void QCanvas::SetColor(QColor *col)
{
  color->SetRGBA(col->GetR(),col->GetG(),col->GetB(),col->GetA());
}
void QCanvas::SetColor(int r,int g,int b,int a)
{ color->SetRGBA(r,g,b,a);
}
void QCanvas::SetColor(int pixel)
// Set color pixel (color index) for GC contexts, or virtual
// RGBA pixel colors for OpenGL contexts.
// (RGB values taken with gamma 1.0, /usr/lib/X11/rgb.txt)
{
  if(ISX)
  { gc->SetForeground(pixel);
  } else
  { //qerr("QCanvas::SetColor(pixel) on OpenGL canvas; must use X GC");
    switch(pixel)
    { case QApp::PX_WHITE : SetColor(255,255,255); break;
      case QApp::PX_BLACK : SetColor(0,0,0); break;
#ifdef OBS_OLD_DARK_COLORS
      case QApp::PX_LTGRAY: SetColor(192,192,192); break;
      case QApp::PX_MDGRAY: SetColor(94,94,94); break;
      case QApp::PX_DKGRAY: SetColor(51,51,51); break;
#endif
      case QApp::PX_LTGRAY: SetColor(224,224,224); break;
      case QApp::PX_MDGRAY: SetColor(192,192,192); break;
      case QApp::PX_DKGRAY: SetColor(128,128,128); break;
      case QApp::PX_RED   : SetColor(255,0,0); break;
      default: qwarn("QCanvas::SetColor(%d) unknown PX_xxx",pixel);
               SetColor(0,255,0); break;
    }
  }
}

/***********
* BLITTING *
***********/
void QCanvas::Blit(QBitMap *srcBM,int dx,int dy,int wid,int hgt,int sx,int sy)
// Blit is like DrawPixels(), only Blit() is more safe in varying environments
// This means it sets more of the OpenGL state before actually drawing,
// so it's easier to use in your programs, but also slower, and if you
// do your own complicated blending stuff etc, using DrawPixels() is the
// 'clean' path to glDrawPixels() (although it does do coordinate swapping)
{
  if(ISX)
  { qerr("QCanvas::Blit not implemented for USEX");
    return;
  }
  if(gl)
  { gl->Select();
    InstallBlendMode();
  }
  DrawPixels(srcBM,dx,dy,wid,hgt,sx,sy);
}

void QCanvas::DrawPixels(QBitMap *srcBM,int dx,int dy,int wid,int hgt,
  int sx,int sy)
// Pixel drawing (onto bitmaps, X, OpenGL)
// This is quite a low-level path to glDrawPixels()
// May still not blit if blit queueing is on
// BUGS: Doesn't support blending for bitmaps
{
  char dummy;

  QASSERT_V(srcBM);

  if(ISX)
  { qerr("QCanvas::DrawPixels() not implemented for USEX");
    return;
  }

  Set2D();

  //dbmWid=drw->GetWidth();
  //dbmHgt=drw->GetHeight();
  //printf("QCanvas::Blit(%d,%d %dx%d s=%d,%d, wg=%dx%d, offset %d,%d clip=%d,%d %dx%d)\n",
    //dx,dy,wid,hgt,sx,sy,dbmWid,dbmHgt,offsetx,offsety,clipx,clipy,clipwid,cliphgt);
  if(wid==-1)wid=srcBM->GetWidth();
  if(hgt==-1)hgt=srcBM->GetHeight();
  // Default source location
  if(sx==-1)sx=0;
  if(sy==-1)sy=0;

  // General offset
  dx+=offsetx; dy+=offsety;

  if(flags&CLIP)
  { // Clip left
    if(dx<clipx)
    { wid-=clipx-dx;
      sx+=clipx-dx;
      dx=clipx;
    }
    // Clip top
    if(dy<clipy)
    { hgt-=clipy-dy;
      sy+=clipy-dy;
      dy=clipy;
    }
    // Clip right
    if(dx+wid>clipx+clipwid)
    { wid=clipx+clipwid-dx;
    }
    // Clip bottom
    if(dy+hgt>clipy+cliphgt)
    { hgt=clipy+cliphgt-dy;
    }
  }
  //printf("  clipped to %d,%d %dx%d\n",dx,dy,wid,hgt);
  if(wid<=0||hgt<=0)return;

  if(UsesBM())
  {
    // Reverse Y
    dy=bm->GetHeight()-hgt-dy;
    sy=srcBM->GetHeight()-hgt-sy;
    QBlit(srcBM,sx,sy,bm,dx,dy,wid,hgt,0);
    return;
  } 

  // Reverse Y
  dy=drw->GetHeight()-hgt-dy;
  sy=srcBM->GetHeight()-hgt-sy;

  //qdbg("  opt?\n");
  if(flags&OPTBLIT)
  { // Instead of really blitting, store the cooked result in a queue,
    // which may be optimized (call Flush())
    if(!bq)bq=new QBlitQueue(this);
    bq->AddBlit(srcBM,sx,sy,wid,hgt,dx,dy,(blendMode!=BLEND_OFF)?QBBF_BLEND:0);
    return;
  }

  gl->Select();
  glPixelStorei(GL_UNPACK_ROW_LENGTH,srcBM->GetWidth());
  glPixelStorei(GL_UNPACK_SKIP_ROWS,sy);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,sx);

//printf("  => QCanvas::Blit(%d,%d %dx%d s=%d,%d)\n",dx,dy,wid,hgt,sx,sy);
  // Move to location (keep raster position valid)
  if(dx>=0&&dy>=0)
  { glRasterPos2i(dx,dy);
  } else
  { glRasterPos2i(0,0);
    glBitmap(0,0,0,0,dx,dy,(const GLubyte *)&dummy);
  }
//glEnable(GL_BLEND);
//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  glDrawPixels(wid,hgt,GL_RGBA,GL_UNSIGNED_BYTE,srcBM->GetBuffer());
}

void QCanvas::Flush()
// Flush the painting operations.
// Currently, this can only have effect on Blit() calls.
{
  DBG_C("Flush");
  gl->Select();
  if(bq)
  { //bq->Execute();
    BlitQueue(bq);
  }
}

void QCanvas::BlitQueue(QBlitQueue *bq)
// Optimized queue blit; tries to avoid setting gl parameters
// Note that the blits are assumed correct (generated by QCanvas::Blit()),
// so checks are often left out.
{ int i,n;
  QBBlit *b;
  bool blend;
  int area;
  static int nCall;

  gl->Select();
  n=bq->GetCount();
  if(!n)return;
  bq->Optimize();
  //printf("QCanvas::BlitQueue (optimized) %d blits\n",n);
  // Be optimistic; few blending blits
  blend=FALSE;
  gl->Disable(GL_BLEND);
  gl->BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  //printf("blit glxctx=%p\n",glXGetCurrentContext());
  area=0;
  for(i=0;i<n;i++)
  { b=bq->GetQBlit(i);
    if(b->flags&QBBF_OBSOLETE)continue;
#ifdef ND_SHOW_IT
    b->sy=b->sbm->GetHeight()-b->hgt-b->sy;
    b->dy=drw->GetHeight()-b->hgt-b->dy;
    qdbg("BQ: s=%d,%d d=%d,%d, size=%dx%d, flags=%d, sbm=%dx%d\n",
      b->sx,b->sy,b->dx,b->dy,b->wid,b->hgt,b->flags,
      b->sbm->GetWidth(),b->sbm->GetHeight());
    b->sy=b->sbm->GetHeight()-b->hgt-b->sy;
    b->dy=drw->GetHeight()-b->hgt-b->dy;
#endif
/*
    Disable(QCANVASF_OPTBLIT);
    Disable(QCANVASF_BLEND);
    Blit(b->sbm,b->dx,b->dy,b->wid,b->hgt,b->sx,b->sy);
*/
//#ifdef ND_TEST
    glPixelStorei(GL_UNPACK_ROW_LENGTH,b->sbm->GetWidth());
    glPixelStorei(GL_UNPACK_SKIP_ROWS,b->sy);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,b->sx);

    // Raster position is always valid; but only if clipping is enabled
    if(b->dx>=0&&b->dy>=0)
    { glRasterPos2i(b->dx,b->dy);
    } else
    { char dummy;
      glRasterPos2i(0,0);
      glBitmap(0,0,0,0,b->dx,b->dy,(const GLubyte *)&dummy);
    }
    //glRasterPos2i(b->dx,b->dy);
    if(b->flags&QBBF_BLEND)
    { if(!blend)
      { glEnable(GL_BLEND);
        blend=TRUE;
      }
    } else if(blend)
    { glDisable(GL_BLEND);
      blend=FALSE;
    }
    glDrawPixels(b->wid,b->hgt,GL_RGBA,GL_UNSIGNED_BYTE,b->sbm->GetBuffer());
//#endif
    area+=b->wid*b->hgt;
  }
  //qdbg("---\n");
  bq->SetCount(0);
#ifdef LOG
  qdbg("QCanvas::BlitQueue[%d]: %d pixels blitted (in %d parts)\n",
    nCall++,area,n);
#endif
}

void QCanvas::ReadPixels(QBitMap *bmDst,int sx,int sy,int wid,int hgt,
  int dx,int dy)
{
  Set2D();

  //qdbg("QCV:ReadPix; this=%d\n",this);
  // General offset
  sx+=offsetx; sy+=offsety;
  dx+=offsetx; dy+=offsety;

  //qdbg("  gl=%p\n",gl);
  //gl->Select();
  //Select();
  //printf("QCanvas::ReadPixels: sx/y=%d,%d %dx%d\n",sx,sy,wid,hgt);
  //qdbg("  bmDst=%p\n",bmDst);
  //qdbg("    %dx%d\n",bmDst->GetWidth(),bmDst->GetHeight());
  if(wid==-1)wid=bmDst->GetWidth();
  if(hgt==-1)hgt=bmDst->GetHeight();
  // Reverse Y
  //qdbg("  drw=%p\n",drw);
  sy=drw->GetHeight()-hgt-sy;
  dy=bmDst->GetHeight()-hgt-dy;

  //qdbg("QCV:ReadPix2\n");
  /*printf("  => QCanvas::ReadPixels: sx/y=%d,%d %dx%d, d=%d,%d, bmwid=%d\n",
    sx,sy,wid,hgt,dx,dy,bmDst->GetWidth());*/
  gl->Select();
  glPixelStorei(GL_PACK_ROW_LENGTH,bmDst->GetWidth());
  glPixelStorei(GL_PACK_SKIP_PIXELS,dx);
  glPixelStorei(GL_PACK_SKIP_ROWS,dy);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  //glReadBuffer(GL_BACK);
#ifdef BUG_O2_READPIXELS
  // Read per line
  int i;
  if(wid==bmDst->GetWidth())
  { // Fast
    glReadPixels(sx,sy,wid,hgt,GL_RGBA,GL_UNSIGNED_BYTE,bmDst->GetBuffer());
  } else
  { // Bug; must read per line
    for(i=0;i<hgt;i++)
    { glPixelStorei(GL_PACK_SKIP_ROWS,dy+i);
      glReadPixels(sx,sy+i,wid,1,GL_RGBA,GL_UNSIGNED_BYTE,bmDst->GetBuffer());
    }
  }
#else
  // Normal thing to do
  glReadPixels(sx,sy,wid,hgt,GL_RGBA,GL_UNSIGNED_BYTE,bmDst->GetBuffer());
#endif
}

void QCanvas::CopyPixels(int sx,int sy,int wid,int hgt,int dx,int dy)
{
  char dummy;
  Set2D();
  gl->Select();
  //Select();

  // General offset
  sx+=offsetx; sy+=offsety;
  dx+=offsetx; dy+=offsety;

  // Reverse Y
  sy=drw->GetHeight()-hgt-sy;
  dy=drw->GetHeight()-hgt-dy;
 
  // Set dst loc
  gl->Select();
  glRasterPos2i(0,0);
  glBitmap(0,0,0,0,dx,dy,(const GLubyte *)&dummy);

  if(wid==drw->GetWidth())
  { glPixelStorei(GL_UNPACK_ROW_LENGTH,0);	// Assuming this is faster
    glPixelStorei(GL_PACK_ROW_LENGTH,0);
  } else
  { glPixelStorei(GL_UNPACK_ROW_LENGTH,drw->GetWidth());
    glPixelStorei(GL_PACK_ROW_LENGTH,drw->GetWidth());
  }
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_ROWS,0);

  glDisable(GL_DITHER);
  glDisable(GL_BLEND);
  glCopyPixels(sx,sy,wid,hgt,GL_COLOR);
  glEnable(GL_DITHER);
  // Blend?
}

/******************
* FILL OPERATIONS *
******************/
void QCanvas::Fill(long pixel,int x,int y,int wid,int hgt,QBitMap *bmDst)
{ printf("QCanvas::Fill nyi\n");
}
void QCanvas::FillAlpha(int alpha,int x,int y,int wid,int hgt,QBitMap *bmDst)
// Fills the alpha channel to a fixed value
// If bmDst==0, the framebuffer is filled (oops, nyi)
{
  int lx,ly;
  char *p;
  int  mod;

  QASSERT_V(bmDst);	// QCanvas::FillAlpha doesn't implement framebuffer fills
  // Reverse Y
  y=drw->GetHeight()-hgt-y;

  mod=drw->GetWidth()-wid;
  mod*=4;
  /*printf("buffer=%p, x/y=%d,%d, wgWid=%d\n",
    bmDst->GetBuffer(),x,y,wg->GetWidth());*/
  p=bmDst->GetBuffer()+(y*drw->GetWidth()+x)*4;	// 1 pixel=long
  //printf("  fillstart=%p\n",p);
  //printf("  offset=%d\n",p-bmDst->GetBuffer());
  p+=3;						// Alpha component

  if(!mod)
  { // Optimized fill for complete scanlines
    //char *endP;
    for(mod=wid*hgt;mod>0;mod--,p+=4)
      *p=alpha;
    //for(endP=p+4*wid*hgt;p<endP;p+=4)	// Ptr arithmetic quirks?
      //*p=alpha;
  } else
  { // Fill per scanline
    for(ly=0;ly<hgt;ly++,p+=mod)
    { for(lx=0;lx<wid;lx++,p+=4)
        *p=alpha;
    }
  }
}

void QCanvas::Clear(QRect *r)
// Clear a rectangle within the drawable
// It's a crappy method, and it disturbs any glScissor() settings currently.
{ // Tricky clear by clipping
  int y;
  gl->Select();
  //Select();
  //printf("clear %d,%d %dx%d\n",r->x,r->y,r->wid,r->hgt);
  y=drw->GetHeight()-r->hgt-r->y;
  //glFinish();
  //glViewport(r->x,r->y,r->wid,r->hgt);
  glScissor(r->x,y,r->wid,r->hgt);
  glEnable(GL_SCISSOR_TEST);
  //glFinish();
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  //glFinish();
  //glViewport(0,0,wg->GetWidth(),wg->GetHeight());
  glDisable(GL_SCISSOR_TEST);
  //glFinish();
  //printf("clear done\n");
  //sginap(5);
}
void QCanvas::Clear()
{ QRect r;
  r.x=0; r.y=0; r.wid=drw->GetWidth(); r.hgt=drw->GetHeight();
  Clear(&r);
}

/******************
* TEXT OPERATIONS *
******************/
void QCanvas::SetTextStyle(QTextStyle *style)
// Set new text style, or 0 to indicate single color (GetColor())
{
  textStyle=style;
}

void QCanvas::Text(cstring txt,int dx,int dy,int len)
// Paint text in the current style @dx,dy
// BUGS: doesn't use textstyle in 'gc' (X drawing)
// Doesn't use OpenGL in Win32, but GDI.
{ char dummy;
  QASSERT_V(txt);
  QASSERT_V(font);

  //printf("QCanvas::Text(%s,%d,%d,%d)\n",txt,dx,dy,len);
  //int n;
  //glGetIntegerv(GL_DRAW_BUFFER,&n);
  //printf("  drawbuffer: %d, GL_FRONT=%d, GL_BACK=%d\n",n,GL_FRONT,GL_BACK);
  if(!font)return;		// Always

  if(len==-1)len=strlen(txt);

  // General offset
  dx+=offsetx; dy+=offsety;

  // Automatically add ascent
  dy+=font->GetAscent();

  if(ISX)
  {
    if(textStyle)
      qwarn("QCanvas:Text() doesn't support textstyles in X GC's");
    gc->Text(txt,dx,dy,len);
    return;
  }

//#ifndef WIN32
  gl->Select();
  // Reverse Y
  dy=drw->GetHeight()-dy;
//#endif

  InstallBlendMode();

  if(textStyle==0)
  { // Single colored simple text
#ifdef ND_WIN32
    // Win32 uses GDI; was supposed NOT to work with double buffered windows
    // In fact, GDI calls in dblbuf'd win's work in an undefined way; it seems to always
    // be painted into the FRONT buffer, but for the Banshee it is never picked up into
    // the internal video framebuffer. Thus highly unusable
    // wgl is the way to go (see QFont)
    QWindow  *w=(QWindow*)drw;
    HDC       hDC=w->GetQXWindow()->GetHDC();
    HFONT     hFont=font->GetHFONT();
    HFONT     oldFont;
    SetBkMode(hDC,TRANSPARENT);
    oldFont=SelectObject(hDC,hFont);
    SetTextColor(hDC,RGB(color->GetR(),color->GetG(),color->GetB()));
    if(!TextOut(w->GetQXWindow()->GetHDC(),dx,dy,txt,len))
    {
      LPVOID lpMsgBuf;
      FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
      );
      qwarn((cstring)lpMsgBuf);
      LocalFree(lpMsgBuf);
    }
    SelectObject(hDC,oldFont);
    // Strange as it is, the text on Win32/Banshee will appear in the FRONT buffer
    // but will NOT be drawn until glFlush()
    // On BB's notepad this is different
    glFlush();
/*    BOOL TextOut(

    HDC hdc,	// handle of device context 
    int nXStart,	// x-coordinate of starting position  
    int nYStart,	// y-coordinate of starting position  
    LPCTSTR lpString,	// address of string 
    int cbString 	// number of characters in string 
   );*/
#else
    // OpenGL display list based font drawing
    SetGLColor(color);
    // Move to location (keep raster position valid)
    glRasterPos2i(0,0);
    glBitmap(0,0,0,0,dx,dy,(const GLubyte *)&dummy);
  
    glListBase(font->GetListBase());
    glCallLists(len,GL_UNSIGNED_BYTE,(GLubyte*)txt);
#endif
  } else
  { // Stylized text
#ifdef WIN32
    qerr("QCanvas:Text(); can't handle styles nyi/win32");
#else
    int i,layers;
    QTextStyleLayer *layer;

    layers=textStyle->GetLayers();
    for(i=0;i<layers;i++)
    { layer=textStyle->GetLayer(i);
      // Move to location (keep raster position valid) and set color
      SetGLColor(&layer->color);
//qdbg("QCanvas: ts layer%d: col=%x\n",i,layer->color.GetRGBA());
      glRasterPos2i(0,0);
      // Don't forget to invert layer Y offset for OpenGL coord system
      glBitmap(0,0,0,0,dx+layer->x,dy-layer->y,(const GLubyte *)&dummy);

      glListBase(font->GetListBase());
      glCallLists(len,GL_UNSIGNED_BYTE,(GLubyte*)txt);
    }
#endif
  }
}
#define QCML_MAX_LINE	30
void QCanvas::TextML(cstring txt,QRect *r,int align)
// Multiline text formatter
// Alignment may center hor/vert
{ int    lines;
  int    i;
  int    lineSpacing;
  string line[QCML_MAX_LINE];
  string s;
  char   buf[1024];
  QRect  bbox;

#ifdef OBS
  if(textStyle)
  { qwarn("QCanvas::TextML() doesn't support textStyle\n");
  }
#endif

  // Don't overflow text buffer
  if(strlen(txt)>=sizeof(buf))
  { qwarn("QCanvas::TextML(); text>temp buffer");
    return;
  }
  strcpy(buf,txt);

  //qdbg("QCV: Text '%s'\n",txt);
  // Calculate #lines in text; and find the start of each line
  line[0]=buf;
  for(lines=1,s=buf;*s;s++)
    if(*s==10)
    { lines++;
      if(lines==QCML_MAX_LINE)
      { qwarn("QCanvas::TextML() max #lines reached (10)");
        lines--;
      }
      line[lines-1]=s+1;
      *s=0;		// Splice the lines
      //s++;
    }
  //if(lines==0)return;

  // Calculate box of text
  bbox.x=0; bbox.y=0;
#ifdef OBS
  maxX=0;
  for(i=0;i<lines;i++)
  { len=font->GetWidth(line[i]);
    if(len>maxX)maxX=len;
  }
#endif
  lineSpacing=font->GetHeight()*4/4;
  //bbox.wid=maxX;
  bbox.hgt=lineSpacing*lines;
  // Center box in 'r'
  if(align&ALIGN_CENTERV)
  { bbox.y=r->y+(r->hgt-bbox.hgt)/2;
    /*qdbg("  QCV: align bbox; bbox.y=%d, r->y=%d,rhgt=%d, bhgt=%d\n",
      bbox.y,r->y,r->hgt,bbox.hgt);*/
  } else
  { bbox.y=r->y;
  }
  // Draw every line
  int x;
  for(i=0;i<lines;i++)
  { 
    x=r->x;
    if(align&ALIGN_CENTERH)
      x+=(r->wid-font->GetWidth(line[i]))/2;
    //qdbg("QCV:TextML: Text(%s,%d,%d)\n",line[i],x,bbox.y+i*lineSpacing);
    Text(line[i],x,bbox.y+i*lineSpacing);
  }
  // Restore LFs
  for(i=1;i<lines;i++)
  { *(line[i]-1)=10;
  }
}

/*************
* RECTANGLES *
*************/
void QCanvas::Rectangle(int x,int y,int wid,int hgt)
{ int bw=1;                             // Border width
  //SetColor(245,245,245);
  Rectfill(x,y,bw,hgt);
  Rectfill(x,y,wid,bw);
  //SetColor(30,30,30);
  Rectfill(x+1,y+hgt-bw,wid-1-bw,bw);
  Rectfill(x+wid-bw,y+1,bw,hgt-1);
}
void QCanvas::Rectangle(QRect *r)
{ Rectangle(r->x,r->y,r->wid,r->hgt);
}

static void printColor(QColor *c)
{
  printf("r%d,g%d,b%d\n",c->GetR(),c->GetG(),c->GetB());
}
void QCanvas::Rectfill(QRect *rz,QColor *colUL,QColor *colUR,QColor *colLR,
  QColor *colLL)
// Colorized rectfill (Gouraud polygon)
{ QRect rr,*r;
  int yy;
  static int col;

  Set2D();

//qdbg("QCV:Rectfill(%d,%d %dx%d\n",rz->x,rz->y,rz->wid,rz->hgt);
  //printColor(colUL);
  //printColor(colUR);
  //printColor(colLR);

  rr.x=rz->x+offsetx; rr.y=rz->y+offsety;
  rr.wid=rz->wid; rr.hgt=rz->hgt;

//qdbg("offset: %d,%d\n",offsetx,offsety);
//qdbg("Rectfill: clip=%d\n",flags&CLIP);
//qdbg("  rr: %d,%d %dx%d\n",rr.x,rr.y,rr.wid,rr.hgt);
  // Clip
  if(flags&CLIP)
  {
    /*printf("clip in: %d,%d %dx%d to %d,%d %dx%d\n",
      rr.x,rr.y,rr.wid,rr.hgt,clipx,clipy,clipwid,cliphgt);*/
    // Left
    if(rr.x<clipx)
    { rr.wid-=clipx-rr.x;
      rr.x=clipx;
    }
    // Top
    if(rr.y<clipy)
    { rr.hgt-=clipy-rr.y;
      rr.y=clipy;
    }
    if(rr.wid<=0||rr.hgt<=0)return;
    // Right
    if(rr.x+rr.wid>clipx+clipwid)
    { rr.wid=clipx+clipwid-rr.x;
    }
    // Bottom
    if(rr.y+rr.hgt>clipy+cliphgt)
    { rr.hgt=clipy+cliphgt-rr.y;
    }
    if(rr.wid<=0||rr.hgt<=0)return;
    /*printf("clip out: %d,%d %dx%d\n",
      rr.x,rr.y,rr.wid,rr.hgt);*/
  }

  if(ISX)
  { gc->Rectfill(&rr,colUL,colUR,colLR,colLL);
    return;
  }

  // ?OpenGL bug? width & height seem to need 1 extra
  rr.y--;
  rr.wid++; rr.hgt++;

  Select();
  r=&rr;
  if(IsNoFlip())
  {
    yy=r->y;
//qdbg("Noflip -> yy=%d\n",r->y);
  } else
  {
    yy=drw->GetHeight()-r->hgt-r->y;
/*qdbg("QCanvas::Rectfill: drw=%s, hgt=%d\n",drw->ClassName(),
  drw->GetHeight());
qdbg("  rz=%d,%d %dx%d\n",rz->x,rz->y,rz->wid,rz->hgt);
qdbg("  rr=%d,%d %dx%d\n",r->x,r->y,r->wid,r->hgt);*/
    if(yy<0)
    { r->hgt+=yy;
      yy=0;
    }
  }

  // Anything to draw?
  if(r->wid<=0||r->hgt<=0)return;

//qdbg("  rect %d,%d %dx%d yy=%d\n",r->x,r->y,r->wid,r->hgt,yy);

  InstallBlendMode();

  glBegin(GL_QUADS);
  if(IsNoFlip())
  {
    /*printf("color rgb=%d,%d,%d a%d\n",
      color->GetR(),color->GetG(),color->GetB(),color->GetA());*/
    SetGLColor(colLL);
    glVertex2i(r->x,yy);
    SetGLColor(colLR);
    glVertex2i(r->x+r->wid-1,yy);
    SetGLColor(colUR);
    glVertex2i(r->x+r->wid-1,yy-r->hgt+1);
    SetGLColor(colUL);
    glVertex2i(r->x,yy-r->hgt+1);
  } else
  {
    SetGLColor(colLL);
    glVertex2i(r->x,yy);
    SetGLColor(colLR);
    glVertex2i(r->x+r->wid-1,yy);
    SetGLColor(colUR);
    glVertex2i(r->x+r->wid-1,yy+r->hgt-1);
    SetGLColor(colUL);
    glVertex2i(r->x,yy+r->hgt-1);
  }
  glEnd();
//qdbg("v%d,%d v%d,%d v%d,%d v%d,%d\n",r->x,yy,
//r->x+r->wid-1,yy, r->x+r->wid-1,yy+r->hgt-1, r->x,yy+r->hgt-1);
}
void QCanvas::Rectfill(int x,int y,int wid,int hgt)
{ QRect r(x,y,wid,hgt);
  Rectfill(&r);
}
void QCanvas::Rectfill(QRect *r)
{ Rectfill(r,color,color,color,color);
}

/********
* LINES *
********/
void QCanvas::Line(int x1,int y1,int x2,int y2)
{
  if(ISX)
  { qerr("Canvas:Line not supported for X");
    return;
  }

  x1+=offsetx; x2+=offsetx;
  y1+=offsety; y2+=offsety;

  Select();
  y1=drw->GetHeight()-y1-1;
  y2=drw->GetHeight()-y2-1;
  SetGLColor(color);
  InstallBlendMode();
  glBegin(GL_LINES);
    glVertex2i(x1,y1);
    glVertex2i(x2,y2);
  glEnd();
}
void QCanvas::LineStipple(unsigned short pattern,int factor)
{
  Select();
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(factor,pattern);
}
void QCanvas::DisableLineStipple()
// Turns off line stippling
{
  Select();
  glDisable(GL_LINE_STIPPLE);
}
void QCanvas::Rectline(int x,int y,int wid,int hgt)
// Draws a line rectangle
// This is used to get stippled rectangles for example
{
  Line(x,y,x+wid-1,y);
  Line(x+wid-1,y,x+wid-1,y+hgt-1);
  Line(x+wid-1,y+hgt-1,x,y+hgt-1);
  Line(x,y+hgt-1,x,y);
}

/************
* TRIANGLES *
************/
void QCanvas::Triangle(int x1,int y1,int x2,int y2,int x3,int y3,
  QColor *col1,QColor *col2,QColor *col3)
// Colorized triangle (Gouraud polygon)
{ QRect rr,*r;
  QColor colBlack(0,0,0);
  int yy;
  static int col;

  x1+=offsetx; x2+=offsetx; x3+=offsetx;
  y1+=offsety; y2+=offsety; y3+=offsety;
  if(!col1)col1=&colBlack;
  if(!col2)col2=&colBlack;
  if(!col3)col3=&colBlack;

  if(ISX)
  { // X GC
    gc->Triangle(x1,y1,x2,y2,x3,y3);
  } else
  { // OpenGL
    Select();

    InstallBlendMode();

    y1=drw->GetHeight()-y1;
    y2=drw->GetHeight()-y2;
    y3=drw->GetHeight()-y3;
    glBegin(GL_TRIANGLES);
      SetGLColor(col1);
      glVertex2i(x1,y1);
      if(col1!=col2)SetGLColor(col2);
      glVertex2i(x2,y2);
      if(col2!=col3)SetGLColor(col3);
      glVertex2i(x3,y3);
    glEnd();
  }
}

/*************************
* USER INTERFACE SUPPORT *
*************************/
void QCanvas::Outline(int x,int y,int wid,int hgt)
// Draw a Q default outline (3D look)
// Not supported for ISX anymore.
{ int bw=2;                             // Border width
  Select();
  if(ISX)qwarn("QCanvas:Outline not support for ISX");
  // New Win95/W2K style
  SetColor(192,192,192);
  Rectfill(x,y,1,hgt-1);
  Rectfill(x,y,wid-1,1);
  SetColor(255,255,255);
  Rectfill(x+1,y+1,1,hgt-2);
  Rectfill(x+1,y+1,wid-2,1);
  SetColor(128,128,128);
  Rectfill(x+wid-2,y+1,1,hgt-2);
  Rectfill(x+1,y+hgt-2,wid-2,1);
  SetColor(0,0,0);
  Rectfill(x+wid-1,y,1,hgt);
  Rectfill(x,y+hgt-1,wid-1,1);
#ifdef OBS_OLD
  Rectfill(x,y,bw,hgt-bw);
  Rectfill(x,y,wid,bw);
  if(ISX)SetColor(QApp::PX_BLACK);
  else   SetColor(30,30,30);
  Rectfill(x+1,y+hgt-bw,wid-1-bw,bw);
  Rectfill(x+wid-bw,y+1,bw,hgt-1);
#endif
}
void QCanvas::Inline(int x,int y,int wid,int hgt)
// Draw a Q default inline (3D look)
{ int bw=2;                             // Border width
  Select();
  if(ISX)qwarn("QCanvas:Outline not support for ISX");
  // New Win95/W2K style
  SetColor(128,128,128);
  Rectfill(x,y,1,hgt-1);
  Rectfill(x,y,wid-1,1);
  SetColor(0,0,0);
  Rectfill(x+1,y+1,1,hgt-2);
  Rectfill(x+1,y+1,wid-2,1);
  SetColor(192,192,192);
  Rectfill(x+wid-2,y+1,1,hgt-2);
  Rectfill(x+1,y+hgt-2,wid-2,1);
  SetColor(255,255,255);
  Rectfill(x+wid-1,y,1,hgt);
  Rectfill(x,y+hgt-1,wid-1,1);
#ifdef OBS_OLD
  if(ISX)SetColor(QApp::PX_BLACK);
  else   SetColor(30,30,30);
  Rectfill(x,y,bw,hgt-bw);
  Rectfill(x,y,wid,bw);
  if(ISX)SetColor(QApp::PX_WHITE);
  else   SetColor(245,245,245);
  Rectfill(x+1,y+hgt-bw,wid-1-bw,bw);
  Rectfill(x+wid-bw,y+1,bw,hgt-1);
#endif
}
void QCanvas::Separator(int x,int y,int wid)
{
  Select();
  SetColor(255,255,255);
  Rectfill(x,y,wid,2);
  SetColor(0,0,0);
  Rectfill(x,y+2,wid,2);
}
void QCanvas::Shade(int x,int y,int wid,int hgt)
{ qwarn("QCanvas::Shade obsolete call");
  /*SetColor(40,40,40);
  Rectfill(x,y,wid,hgt);*/
}
void QCanvas::Insides(int x,int y,int wid,int hgt,bool shade)
// Draw default UI inside rectangle
// Shaded (nicer) for buttons, non-shaded for edit controls
// where we need to update selected regions
{ QRect r(x,y,wid,hgt);
  //qdbg("QCV:Insides %p\n",this);
  QASSERT_V(gl);
  QColor ciUL(118,118,118),ciUR(166,166,166);
  QColor ciLR(190,190,190),ciLL(166,166,166);
  QColor colNeutral(192,192,192);
  //qdbg("  select gl=%p\n",gl);
  Select();
  //qdbg("  rectfill\n");
  if(ISX)
  { SetColor(QApp::PX_LTGRAY);
  }
#ifdef SHADY_UI
  if(shade)Rectfill(&r,&ciUL,&ciUR,&ciLR,&ciLL);
  else     Rectfill(&r,&colNeutral,&colNeutral,&colNeutral,&colNeutral);
#else
  SetColor(&colNeutral);
  Rectfill(&r); //,&ciUR,&ciUR,&ciUR,&ciUR);
#endif
  //qdbg("QCV:Ins RET\n");
}

void QCanvas::StippleRect(int x,int y,int wid,int hgt)
// Draw a stippled rectangle (like in buttons around the text when focused)
{
  LineStipple(0xAAAA);
  SetColor(0,0,0);
  Rectline(x,y,wid,hgt);
  //Rectangle(x,y,wid,hgt);
  DisableLineStipple();
}

