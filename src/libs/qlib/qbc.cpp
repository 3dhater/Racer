/*
 * QBC - broadcast window (PAL/NTSC/SECAM etc)
 * 09-04-97: Created!
 * 22-05-97: Support for automatic 16:9 emulation (every swapped frame)
 * 17-01-00: Support for alpha video out; this uses internal buffers
 * and is QUITE different from the normal operation. No support for movies
 * that way. (QMVOut doesn't work)
 * BUGS:
 * - 16:9 for PAL only; hardcoded values
 * - FULLSCREEN mode select hardcoded 1280x1024 instead of looking at scr size
 * - Safety(FALSE) when safety was on should use local gellist instead of
 *   app->GetGelList()
 * - Alpha doesn't support 16:9 emulation, and no safety
 * FUTURE:
 * - Hold a gellist here, instead of in QApp
 * - Paint() is awfully unefficient
 * (C) MarketGraph/RVG
 */

#include <qlib/app.h>
#include <qlib/gel.h>
#include <qlib/bc.h>
#include <qlib/opengl.h>
#include <qlib/info.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <X11/Xlib.h>
#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#endif
#include <GL/glx.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Uses alpha out?
#define IS_ALPHA        (alphaBuf!=0)

#define METHOD169	m169		// 16:9 calculation algorithm

// Define LETTERBOX to move 16:9 down (to get 2 horizontal black blocks)
// Note that using LETTERBOX is quite a lot slower, although nicer to look at.
#define LETTERBOX	pLetterbox
#define LB_OFFY		72		// Letterbox offset <2*72!

// Define QUICKTEST to use info file method
//#define QUICKTEST
static int m169;
static bool pLetterbox;

// Default BC paint procedure (callback)
static void BCPaint(QDraw *draw,void *ud)
{ QBC *bc;
//qdbg("BCPaint\n");
  bc=(QBC*)ud;
  if(!bc)return;
  //qdbg("BCPaint(%p)\n",bc);
  QGelList *gl;
  gl=app->GetGelList();
  if(gl)gl->MarkDirty();
  //bc->Paint();
//qdbg("BCPaint RET\n");
}

QBC::QBC(QWindow *parent,int typ,int x,int y,int wid,int hgt)
  //: QWindow(parent,x,y,640,480)
  //: QShell(parent,x,y,640,480)
  : QDraw(parent,x,y,640,480)
{ 
//qdbg("QBC ctor\n");
#ifdef WIN32
  // Due to a Voodoo Banshee bug on Win32/Win2000, I make the BC a soft window
  // The driver can only handle 1 OpenGL window (!)
  cstring s;
  s=getenv("QBUGS");
//qdbg("qbc: QBUGS='%s', ONEGL=%d",s,strstr(s?s:"abc","ONEGL"));
  if(s!=0)
  {
    if(strstr(s,"ONEGL"))
    { // Add this window as soft child of QSHELL
      // Though this lets Q_BC take over QSHELL in painting, it gets things going a bit
      // under the Voodoo Banshee driver for Win2000 (4-8-00). The TNT does it better.
qwarn("QBC: Soft window for Banshee!\n");
      MakeSoftWindow();
    }
  }
#endif

  //Move(x,y);
  // Default sizes?
  switch(typ)
  { case PAL: wid=768; hgt=576; break;
    case PAL_DIGITAL: wid=720; hgt=576; break;
    case NTSC: wid=640; hgt=480; break;
    case FULLSCREEN: wid=1280; hgt=1024; break;
  }
  //wid+=4; hgt+=4;
  Size(wid,hgt);
  //if(win)win->Size(wid,hgt);
  // Create own canvas, which clips everything inside the gel
  // Catch pointer wanting to go in
  //flags=KEEP_OUT;
  flags=0;
  bmEmul[0]=bmEmul[1]=0;
  buffer=0;
  
  alphaBuf=0;
  cvScreen=0;
  
  // Create outline around broadcast window
  QRect r(x-2,y-2,wid+4,hgt+4);
  group=new QGroup(parent,&r);
#ifdef QUICKTEST
  QInfo info("test.set");
  m169=info.GetInt("qbc.method");
  pLetterbox=info.GetInt("qbc.letterbox");
#else
  m169=1;
  pLetterbox=1;
#endif
  Catch(CF_MOTIONNOTIFY|CF_BUTTONPRESS|CF_BUTTONRELEASE);
  // Install paint callback
  SetPaintProc(BCPaint,this);
}
QBC::~QBC()
{ //if(canvas)delete canvas;
}

void QBC::Paint(QRect *r)
// Repaint entirely (after an expose)
{ 
  //qdbg("QBC::Paint\n");
  //QRect pos;
  //GetPos(&pos);
  //pos.x=pos.y=0;
  //app->GetShell()->GetCanvas()->Inline(pos.x-2,pos.y-2,pos.wid+4,pos.hgt+4);
  QDraw::Paint();
  /*QGelList *gl;
  gl=app->GetGelList();
  if(gl)gl->MarkDirty();*/
}

void QBC::KeepOut(bool yn)
{ if(yn)flags|=KEEP_OUT;
  else  flags&=~KEEP_OUT;
}

bool QBC::EvMotionNotify(int x,int y)
{
#ifdef OBS
  int m,dx,dy,adx,ady;
  bool warp=FALSE;

  if(flags&KEEP_OUT)
  { // The pointer should not come here; push it out
    dx=dy=0;
    // Calculate warp distances for X/Y
    if(x>=pos->x&&x<pos->x+pos->wid)
    { m=pos->x+pos->wid/2;
      if(x<m)dx=pos->x-x;
      else   dx=pos->x+pos->wid-x;
      warp=TRUE;
    }
    if(y>=pos->y&&y<pos->y+pos->hgt)
    { m=pos->y+pos->hgt/2;
      if(y<m)dy=pos->y-y;
      else   dy=pos->y+pos->hgt-y;
      warp=TRUE;
    }
    if(warp)
    { printf("QBC: warp outside\n");
      // Only warp the direction that is most logical
      adx=dx<0?-dx:dx;
      ady=dy<0?-dy:dy;
      if(adx<ady)x+=dx;
      else       y+=dy;
      zapp->WarpPointer(x,y);
    }
  }
#endif
  return FALSE;			// Continue to parse event
}

/**********
* QUERIES *
**********/
QGelList *QBC::GetGelList()
// Future; return local gellist
{
  return app->GetGelList();
}
bool QBC::IsAlpha()
{
  if(IS_ALPHA)return TRUE;
  return FALSE;
}

/***************
* 16:9 support *
***************/
static void ConvertTo16_9(QBC *bc,int method)
// PAL -> PALplus (4:3 -> 16:9)
// Convert front buffer to HDTV (16:9) to allow for checking on PAL
// video out how the HDTV/PALplus picture will look like
// Uses OpenGL.
// Method: 16:9 = .75 * 4:3, leave out every 4th line, which is the same
// as copy 3 lines, skip one. Then add some bands.
{
  QCanvas *cv=bc->GetCanvas();
  //QBitMap *bm;
  QGLContext *gl=cv->GetGLContext();
  int sy,dy;
  proftime_t t;

  if(method==0)
  { //bm=new QBitMap(32,768,3);
    // Backing store
    cv->GetGLContext()->ReadBuffer(GL_BACK);
    //cv->Disable(QCANVASF_BLEND);
    cv->SetBlending(QCanvas::BLEND_OFF);
    cv->ReadPixels(bc->bmEmul[bc->buffer],0,0,
      bc->GetWidth(),bc->GetHeight(),0,0);
    // Compress
    gl->Clear(GL_COLOR_BUFFER_BIT);
    //gl->ReadBuffer(GL_FRONT);
    //gl->DrawBuffer(GL_BACK);
    dy=(576-432)/2;
    for(sy=0;sy<576;sy+=4)
    { //cv->ReadPixels(bm,0,sy,768,3);
      cv->Blit(bc->bmEmul[bc->buffer],0,dy,bc->GetWidth(),3,0,sy);
      dy+=3;
    }
  } else if(method==1)
  {
//qdbg("ConvertTo16_9 method 1\n");
    cv->Set2D();
    //glFinish(); profStart(&t);
    // First, compress the buffer on the top; first 3 lines are already OK
    dy=3;
    //cv->Disable(QCANVASF_BLEND);
    cv->Blend(FALSE);
    //gl->Clear(GL_COLOR_BUFFER_BIT);
    for(sy=4;sy<576;sy+=4)
    { cv->CopyPixels(0,sy,bc->GetWidth(),3,0,dy);
      dy+=3;
    }
    //glFinish(); profReport(&t,"method 1 compress (line blit)");
    if(pLetterbox)
    { // Move down for letterbox view
      cv->CopyPixels(0,0,bc->GetWidth(),432,0,LB_OFFY);
      QRect r(0,0,768,LB_OFFY);
      cv->Clear(&r);
      r.SetXY(0,432+LB_OFFY);
      r.SetSize(768,576-432-LB_OFFY);
      cv->Clear(&r);
    }
  } else if(method==2)
  { // Inline zoom; lossy
    glFinish(); //profStart(&t);
    gl->Select();
    glPixelZoom(1,.75);
    cv->CopyPixels(0,0,bc->GetWidth(),576,0,0 /* 576-432*/);
    glPixelZoom(1,1);
    glFinish(); //profReport(&t,"method 2 compress (pixelzoom)");
  }
}
static void ConvertToFullSize(QBC *bc,int method)
// Restore the back buffer to full size
{ int sy,dy;
  QCanvas *cv=bc->GetCanvas();
  QGLContext *gl=cv->GetGLContext();
  if(method==0)
  { bc->buffer^=1;
    cv->Blit(bc->bmEmul[bc->buffer],0,0);
  } else if(method==1)
  { 
    if(pLetterbox)
    { // Get rid of letterbox offset
      cv->CopyPixels(0,LB_OFFY,bc->GetWidth(),432,0,0);
    }
    // Expand
    dy=576-4;
    //cv->Disable(QCANVASF_BLEND);
    cv->Blend(FALSE);
    //gl->Clear(GL_COLOR_BUFFER_BIT);
    for(sy=432-3;sy>0;sy-=3)
    { cv->CopyPixels(0,sy,bc->GetWidth(),4,0,dy);
      dy-=4;
    }
  } else if(method==2)
  { 
    /*
    // Zooming back has the pixel order problem
    gl->Select();
    glPixelZoom(1,1.3333333);
    cv->CopyPixels(0,576-432,bc->GetWidth(),432,0,0);
    glPixelZoom(1,1);
    */
    // Expand
    dy=0;
    //cv->Disable(QCANVASF_BLEND);
    cv->Blend(FALSE);
    //gl->Clear(GL_COLOR_BUFFER_BIT);
    for(sy=576-432;sy<576;sy+=3)
    { cv->CopyPixels(0,sy,bc->GetWidth(),3,0,dy);
      dy+=4;
    }
  }
}

void QBC::Emulate16x9(bool yn)
{ if(yn)
  { flags|=EMULATE_16_9;
    // Allocate bitmap to holds true PAL contents
/*
    if(!bmEmul[0])
    { bmEmul[0]=new QBitMap(GetDepth(),GetWidth(),GetHeight());
      bmEmul[1]=new QBitMap(GetDepth(),GetWidth(),GetHeight());
      // Fill buffer for the first frame
      cv->Disable(QCANVASF_BLEND);
      cv->ReadPixels(bmEmul[buffer^1],0,0,GetWidth(),GetHeight(),0,0);
    }
*/
    //app->GetGelList()->MarkDirty();
    //app->GetGelList()->Show();
  } else
  { flags&=~EMULATE_16_9;
    /*if(bmEmul[0])
    { delete bmEmul[0]; bmEmul[0]=0;
      delete bmEmul[1]; bmEmul[1]=0;
    }*/
  }
  app->GetGelList()->MarkDirty();
}

void QBC::Swap()
{ //static int buffer;
//qdbg("QBC:Swap\n");

  if(IS_ALPHA)
  {
    // If not, this results in GLX giving an error
    cvScreen->GetGLContext()->Select(qxwin,cv->GetDrawable());
    if(flags&PREVIEW_ALPHA)
    {
//qdbg("QBC:Swap(); copy alpha contents\n");
      // Copy alpha buffer contents to screen window
    //cvScreen->GetGLContext()->Select(qxwin,cv->GetDrawable());
      glDisable(GL_BLEND);
      glDisable(GL_DITHER);
      glDisable(GL_DEPTH_TEST);
      glDrawBuffer(GL_FRONT);
      glCopyPixels(0,0,GetWidth(),GetHeight(),GL_COLOR);
      //qxwin->Swap();
    }
    // This is something else; we need to output the backbuffer
    // of the alpha buffer
    alphaBuf->Swap();
    return;
  }
  
  if(flags&EMULATE_16_9)
  { // Convert the back buffer to 16:9
    ConvertTo16_9(this,METHOD169);
  }
  //qdbg("QBC:Swap\n");
  //qdbg("  cv->Mode=%d; dbl=%d\n",cv->GetMode(),QCANVAS_DOUBLEBUF);
  if(flags&SAFETY)
  { PaintSafety();
  }
  if(qxwin)qxwin->Swap();
  //QNap(15);
  if(flags&EMULATE_16_9)
  { // Reset the old front buffer to the original PAL size
    ConvertToFullSize(this,METHOD169);
  }
}

/*********
* Safety *
*********/
void QBC::PaintSafety()
{ int x,y,wid,hgt;
  int dx,dy;
  wid=GetWidth(); hgt=GetHeight();
  x=0; y=0;
  dx=48; dy=32;
  cv->GetGLContext()->Setup2DWindow();
  cv->SetColor(255,255,255);
  cv->LineStipple(0x3333,1);
  // Outer line
  cv->Line(x+dx,y+dy,x+wid-dx-1,y+dy);
  cv->Line(x+dx,y+dy,x+dx,y+hgt-dy-1);
  cv->Line(x+dx,y+hgt-dy-1,x+wid-dx-1,y+hgt-dy-1);
  cv->Line(x+wid-dx-1,y+dy,x+wid-dx-1,y+hgt-dy-1);
  // Centers
  cv->LineStipple(0xF0F0,2);
  cv->Line(x+wid/2,y,x+wid/2,y+hgt-1);
  cv->Line(x,y+hgt/2,x+wid-1,y+hgt/2);
}

void QBC::Safety(bool yn)
{
  if(yn)
  { flags|=SAFETY;
  } else
  { if(flags&SAFETY)
    { // Restore screen
      app->GetGelList()->MarkDirty();
      flags&=~SAFETY;
    }
  }
}

/********
* ALPHA *
********/
bool QBC::SwitchToAlpha(bool genlock)
// Ignore normal screen window, and turn on alpha video output
{
qdbg("QBC:SwitchToAlpha\n");
  if(IS_ALPHA)return TRUE;
  
  // Create alpha buffer (class does video out by itself)
  // This is 2 PBuffers attached to DMbuffers with an OpenGL context
  alphaBuf=new QAlphaBuf(GetWidth(),GetHeight(),2);
  if(genlock)
    alphaBuf->SetGenlock(TRUE);
  alphaBuf->Create();

  // Redirect our canvas to the alpha buffer, so applications
  // will transparently draw into the alpha buffer
  cvScreen=cv;
  cv=alphaBuf->GetCanvas();
  
  // Preview direct video output onscreen
  //flags|=PREVIEW_ALPHA;
  
  return TRUE;
}

void QBC::EnableAlphaPreview()
{
  flags|=PREVIEW_ALPHA;
}
void QBC::DisableAlphaPreview()
{
  flags&=~PREVIEW_ALPHA;
}
