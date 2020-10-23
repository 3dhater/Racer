/*
 * QMovieBob.cpp - a bob that contains movie frames
 * 21-01-98: Created!
 * NOTES:
 * - swid/shgt from Bob is used as movie size
 * - sbm is used as a storage container for still frames when the movie stops
 * BUGS:
 * - Caterpillars, cockroaches, you know
 * - Stop() NYI; should cache image
 * - PAL support only; no NTSC movie files yet
 * (C) MarketGraph/RVG
 */

#ifdef __sgi
// Not supported on Win32/Linux

#include <stdio.h>
#include <string.h>
#include <qlib/text.h>
#include <qlib/point.h>
#include <qlib/moviebob.h>
#include <dmedia/dm_params.h>
#include <dmedia/dm_image.h>
#include <dmedia/moviefile.h>
#include <GL/gl.h>
#include <unistd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Define PAINTPART_SBM to use image buffer to use for PaintPart()
//#define PAINTPART_SBM

#define QTEXT_DEPTH	32	// Let's give it depth

static int marginX,marginY;	// Extra pixel margins
static int offsetX,offsetY;	// Pixel offset

QMovieBob::QMovieBob(int iwid,int ihgt,int flags)
  : QBob(0,flags|QGELF_DBLBUF)
{
  swid=iwid; shgt=ihgt;
  size.wid=iwid; size.hgt=ihgt;
  //frame=0;
  sbm=new QBitMap(32,size.wid,size.hgt);
  //sbm=0;
  cropLeft=cropRight=cropTop=cropBottom=0;
  //playSpeed=0;		// Not playing when first created
  //loopsToGo=0;
  ap=new QAudioPort();
  ap->Open();
  ap->AllocBuffer(20000);
  mute=FALSE;
}
QMovieBob::~QMovieBob()
{ //if(bob)delete bob;
  if(sbm)delete sbm;
  //if(text)qfree(text);
  //if(colg)delete colg;
  //if(col)delete col;
}

void QMovieBob::SetMovie(QMovie *imovie)
{
  movie=imovie;
  swid=movie->GetImageTrack()->GetImageWidth();
  shgt=movie->GetImageTrack()->GetImageHeight();
  if(sbm)delete sbm;
  sbm=new QBitMap(32,swid,shgt);
  //frame=0;
  frames=movie->GetImageTrack()->GetLength();
  // Get frame in bitmap
  CreateStill();
}

bool QMovieBob::IsPlaying()
{
  if(!movie)return FALSE;
  return movie->IsPlaying();
  //if(playSpeed==0)return FALSE;
  //return TRUE;
}

bool QMovieBob::GetFramed()
{
  if(flags&QGELF_INDPAINT)return TRUE;
  return FALSE;
}
void QMovieBob::SetFramed(bool yn)
// Set framed to TRUE if this moviebob is entirely covered by a frame gel
// to optimize the painting process.
// Movies are special because they require time to paint
{
  if(yn)Enable(QGELF_INDPAINT);
  else  Disable(QGELF_INDPAINT);
}

void QMovieBob::Crop(int left,int right,int top,int bottom)
{
  cropLeft=left;
  cropRight=right;
  cropTop=top;
  cropBottom=bottom;
}

void QMovieBob::Loop(int nTimes)
{ //loopsToGo=nTimes;
}

void QMovieBob::Play(int speed)
{ if(IsPlaying())
    return;
  //playSpeed=speed;
  //frame=0;
  if(movie)
    movie->Play();
}
void QMovieBob::CreateStill()
{ 
  if(!movie)return;
  //movie->SetCurFrame(frame);
  movie->RenderToBitMap(sbm);
#ifdef OBS
  // Catch last frame and store in bitmap for bob restores
  char *mem;
  DMparams *p;
  proftime_t t;
  MVrect rect;
  mem=sbm->GetBuffer();
  dmParamsCreate(&p);
  dmParamsSetInt(p,DM_IMAGE_WIDTH,swid);
  dmParamsSetInt(p,DM_IMAGE_HEIGHT,shgt);
  //dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_CbYCrY);
  dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_RGBX);
  // Flip picture
  //dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_BOTTOM_TO_TOP);
  dmParamsSetInt(p,MV_IMAGE_STRIDE,0);
  //printf("mvr2movimgbuf: frame %d\n",frame);
  //cv->Select();
  // Movie blits are NOT optimized (for speed), so flush all
  // pending blits to maintain gel system correctness
  //cv->Flush();
  //profStart(&t);
  if(mvRenderMovieToImageBuffer(movie->GetMVid(),frame,25,mem,p)
     !=DM_SUCCESS)
  { qerr("QMovieBob:CreateStill(); failed to create still; %s\n",
      mvGetErrorStr(mvGetErrno()));
  }
#endif
}
void QMovieBob::Stop()
{ if(!IsPlaying())
    return;
  //playSpeed=0;
  if(movie)
    movie->Stop();
  CreateStill();
}

void QMovieBob::Update()
{ bool mute=FALSE;
  int time,timeSlice,timeScale;
  static char audioBuf[100000];
  int audioBufSize=100000;
  size_t audioFilled;
  MVframe framesFilled;
  int status;

  //qdbg("QMB: Update\n");
  // Are we playing?
  if(!IsPlaying())return;
  time=movie->GetCurFrame();
  timeSlice=1;
  timeScale=25;
  if(!mute)
  { status=mvRenderMovieToAudioBuffer(movie->GetMVid(),
      time,timeSlice,timeScale,
      MV_FORWARD,ap->GetBufferSize(),&audioFilled,&framesFilled,
      ap->GetBuffer(),0);
    if(status==DM_FAILURE)
    { qdbg("mvRMTOAB: status=DM_FAILURE (audio)\n");
      //break;
    }
    //qdbg("audio: audioFilled=%d, framesFilled=%d\n",audioFilled,framesFilled);
    ap->WriteSamps(ap->GetBuffer(),framesFilled*2);
  }

  movie->Advance();
#ifdef OBS
  frame++;
  if(frame==frames)
  { // End mode decision
    if(loopsToGo==-1)
    { // Infinite loop
      frame=0;
    } else if(loopsToGo>0)
    { loopsToGo--;
      frame=0;
    } else Stop();
  }
#endif
  // Notice that the image has changed for other gels
  MarkDirty();
}

void QMovieBob::Paint()
{
  if(flags&QGELF_INVISIBLE)return;
  if(cv==0)return;
  // If moviebob is framed, don't paint entirely (let restoration of
  // frame do the job)
  if(GetFramed())return;

  //qdbg("QMB:Paint(); flags=%d\n",flags);
  loc.x=x->Get(); loc.y=y->Get();
  //loc.x=10; loc.y=10;
  if(!movie)
  { // No movie = black rect
    QRect r(0,0,swid,shgt);
    //cv->Clear(0,0,320,240);
    //cv->Clear(&r);
    //cv->Disable(QCANVASF_BLEND);
    cv->Blend(FALSE);
    cv->Blit(sbm,loc.x,loc.y,size.wid,size.hgt,0,0);
    return;
  }
  //cv->Blit(bm,10,10);
  char *mem;
  DMparams *p;
  proftime_t t;
  MVrect rect;
  mem=sbm->GetBuffer();
  /*qdbg("trkImg: %dx%d\n",movie->GetImageTrack()->GetImageWidth(),
    movie->GetImageTrack()->GetImageHeight());*/
  dmParamsCreate(&p);
  dmParamsSetInt(p,DM_IMAGE_WIDTH,swid*2/3);
  dmParamsSetInt(p,DM_IMAGE_HEIGHT,shgt*2/3);
  dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_CbYCrY);
  //dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_YVYU_422_8);
  dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  //dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_BOTTOM_TO_TOP);
  dmParamsSetInt(p,MV_IMAGE_STRIDE,0);
  //printf("mvr2movimgbuf: frame %d\n",frame);
  cv->Select();
  // Movie blits are NOT optimized (for speed), so flush all
  // pending blits to maintain gel system correctness
  cv->Flush();
  //profStart(&t);
  //mvRenderMovieToImageBuffer(movie->GetMVid(),frame,25,mem,p);
  glPixelZoom(1,-1);
  //glRasterPos2i(loc.x,576-loc.y);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glDisable(GL_BLEND);
  //rect.left=loc.x; rect.bottom=576-loc.y;
  rect.left=loc.x; rect.bottom=576-(loc.y+shgt-1);
  rect.right=rect.left+swid-1; rect.top=576-loc.y;
  glEnable(GL_SCISSOR_TEST);
  glScissor(rect.left+cropLeft,rect.bottom+cropBottom,
    rect.right-rect.left-cropLeft-cropRight,rect.top-rect.bottom-cropTop);
  mvSetMovieRect(movie->GetMVid(),rect);
  //glDrawPixels(swid,shgt,GL_RGBA,GL_UNSIGNED_BYTE,mem);
  mvRenderMovieToOpenGL(movie->GetMVid(),movie->GetCurFrame(),25);
  glPixelZoom(1,1);
  glDisable(GL_SCISSOR_TEST);
  //profReport(&t,"mvRenderToImageBuffer");
  dmParamsDestroy(p);
  //cv->Disable(QCANVASF_BLEND);
  //cv->Select();
  //cv->Blit(sbm);
}

void QMovieBob::PaintPart(QRect *r)
{ 
  //qdbg("QMB:PaintPart: flags=%d\n",flags);
  //Paint(); return;
  if(flags&QGELF_INVISIBLE)return;
  //if(flags&QGELF_DIRTYFILTER)ApplyFilters();
  //printf("QMBob::PaintPart(%d,%d,%dx%d)\n",r->x,r->y,r->wid,r->hgt);
  if(cv==0)return;
#ifdef OBS
  // Trick; when a frame is on top of the movie, and the movie is playing,
  // avoid redrawing the image buffer (which may not contain the playing
  // frame) by ignoring PaintPart() called as part of RestoreGel(frameGel)
  // Better would be to get a different functionality of the gel updates.
  if(IsPlaying())
  { qdbg("QMovieBob:PaintPart; ignored because playing\n");
    return;
  }
#endif

#ifdef PAINTPART_SBM
  cv->Flush();
  cv->Disable(QCANVASF_BLEND);
  MVrect rect;
  rect.left=loc.x; rect.bottom=576-(loc.y+shgt-1);
  rect.right=rect.left+swid-1; rect.top=576-loc.y;
  // pending blits to maintain gel system correctness
  glEnable(GL_SCISSOR_TEST);
  glScissor(rect.left+cropLeft,rect.bottom+cropBottom,
    rect.right-rect.left-cropLeft-cropRight,rect.top-rect.bottom-cropTop);
  /*cv->Blit(sbm,loc.x+r->x+cropLeft,loc.y+r->y+cropTop,r->wid-cropLeft-cropRight,
    r->hgt-cropTop-cropBottom,sx+r->x+cropLeft,sy+r->y+cropTop);*/
  cv->Blit(sbm,loc.x+r->x,loc.y+r->y,r->wid,r->hgt,sx+r->x,sy+r->y);
  cv->Flush();
  glDisable(GL_SCISSOR_TEST);
#else
  // Use realtime decompression to paint the part (correct image)
  bool framed;
  cv->Flush();
  //cv->Disable(QCANVASF_BLEND);
  MVrect rect;
  rect.left=loc.x; rect.bottom=576-(loc.y+shgt-1);
  rect.right=rect.left+swid-1; rect.top=576-loc.y;
  // Adjust crop to fake PaintPart() in Paint()
  int oldL,oldR,oldB,oldT;
  oldL=cropLeft;
  oldR=cropRight;
  oldB=cropBottom;
  oldT=cropTop;
#ifdef ND_NOT_WORKING
  // Crop it
  cropLeft=r->x;
  cropRight=GetWidth()-r->x-r->wid;
  cropTop=r->y;
  cropBottom=GetHeight()-r->y-r->hgt;
  /*glEnable(GL_SCISSOR_TEST);
  glScissor(rect.left+cropLeft,rect.bottom+cropBottom,
    rect.right-rect.left-cropLeft-cropRight,rect.top-rect.bottom-cropTop);*/
#endif
  // Temporarily set 'framed' to false, so Paint() will paint.
  framed=GetFramed();
  if(framed)SetFramed(FALSE);
  Paint();
  if(framed)SetFramed(TRUE);
  cropLeft=oldL;
  cropRight=oldR;
  cropBottom=oldB;
  cropTop=oldT;
  //cv->Blit(sbm,loc.x+r->x,loc.y+r->y,r->wid,r->hgt,sx+r->x,sy+r->y);
  cv->Flush();
  glDisable(GL_SCISSOR_TEST);
#endif
}

#endif
