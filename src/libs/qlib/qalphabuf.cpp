/*
 * QAlphaBuf - alpha double buffer, for use under QBC for example
 * 17-00-00: Created!
 * 01-03-01: Max buffers can be set; handy for playing movies.
 * NOTES:
 * - This does nothing much on Win32; video? what? PAL? Huh, friend? ;-)
 * (C) MarketGraph/RVG
 */

#if defined(__sgi) || defined(linux) || defined(WIN32)

#include <qlib/app.h>
#include <qlib/gel.h>
#include <qlib/bc.h>
#include <qlib/common/dmedia.h>
#include <qlib/opengl.h>
#include <qlib/info.h>
#include <stdio.h>
#pragma hdrstop
#include <qlib/alphabuf.h>
#include <unistd.h>
#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#endif
#ifdef OBS
#include <unistd.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif
#include <qlib/debug.h>
DEBUG_ENABLE

// $DEV; should be in the class
// Hold buffers to avoid skipping across many buffers when swapping
static QDMBuffer *holdBuffer[QAlphaBuf::MAX_BUFFER];

QAlphaBuf::QAlphaBuf(int _wid,int _hgt,int _buffers)
  : QDrawable()
{
  int i;
  
  wid=_wid; hgt=_hgt;
_buffers=16;
  buffers=_buffers;
  backBufferIndex=0;
  // Initially, all buffers are available for use
  maxBufferIndex=buffers;
  
  flags=TRIGGER;
  
  transfersComplete=0;
  
  vout=0;
  for(i=0;i<buffers;i++)
  { pbuf[i]=0;
  }
  bufPool=0;
}
QAlphaBuf::~QAlphaBuf()
{
}

GLXDrawable QAlphaBuf::GetGLXDrawable()
{
#if defined(__sgi) || defined(WIN32)
  if(pbuf[backBufferIndex])
    return pbuf[backBufferIndex]->GetGLXPBuffer();
#endif
  return 0;
}

void QAlphaBuf::SetGenlock(bool yn)
// Set genlock mode BEFORE calling Create()
{
  if(yn)flags|=GENLOCK;
  else  flags&=~GENLOCK;
}

bool QAlphaBuf::Create()
{
#if defined(WIN32) || defined(linux)
  qerr("QAlphaBuf:Create() nyi(win32/linux)");
  return FALSE;
#else
  int w,h,i;
  
  // Create video out path
  vout=new QDMVideoOut();
  vout->SetLayout(VL_LAYOUT_GRAPHICS);
  vout->SetPacking(VL_PACKING_ABGR_8);
  vout->SetGenlock(flags&GENLOCK?TRUE:FALSE);
  vout->SetSize(wid,hgt);
  // No fields
  vout->SetCaptureType(VL_CAPTURE_INTERLEAVED);
  //vout->SetSize(wid,hgt/div);
  vout->GetSize(&w,&h);
qdbg("QAlphaBuf: video size: %dx%d\n",w,h);
  
  // Create pbuffer
  for(i=0;i<buffers;i++)
  { pbuf[i]=new QDMPBuffer(w,h);
  }
  
  // Bufferpool; not cached, mapped
  bufPool=new QDMBPool(buffers,wid*hgt*4,FALSE,TRUE);
  bufPool->AddProducer(pbuf[0]);
  bufPool->AddConsumer(vout);
  if(!bufPool->Create())
  { qerr("QAlphaBuf: can't create pbuffer pool");
    return FALSE;
  }
  
  vout->RegisterPool(bufPool);
  
qdbg("bufPool: buffers=%d\n",bufPool->GetFreeBuffers());
#ifdef ND_NO_PREALLOC
  // Allocate the buffers now (we won't let go on them)
  for(i=0;i<buffers;i++)
  {
    dmBuf[i]=bufPool->AllocateDMBuffer();
qdbg("  dmBuffer[%d]=%p SGI %p\n",i,dmBuf[i],dmBuf[i]->GetDMbuffer());
  }
#endif

qdbg("bufPool: buffers=%d\n",bufPool->GetFreeBuffers());

  // Create canvas
  cv=new QCanvas(this);
qdbg("  cv=%p\n",cv);

  // Get rid of extraneous VL buffer
  bufPool->AllocateDMBuffer();
  
  // Get buffers and video installed
  Swap();
  Swap();
  
  return TRUE;
#endif
}
void QAlphaBuf::Destroy()
{
  qerr("QAlphaBuf:Destroy() NYI");
}

void QAlphaBuf::Trigger()
{
#if defined(WIN32) || defined(linux)
#else
  if(!(flags&TRIGGER))return;

qdbg("Trigger vout\n");
  vout->Start();
  flags&=~TRIGGER;
#endif
}

void QAlphaBuf::HandleEvents()
// Waits for a transfercomplete
{
#if defined(WIN32) || defined(linux)
  // No events here
#else
  QVideoServer *vs;
  vs=app->GetVideoServer();
  
  if(flags&TRIGGER)
    Trigger();
  
  int    i,n;
  fd_set fdr,fdw;
  static int    maxFD;
  VLEvent vlEvent;
  
  if(!maxFD)
  { // Create an FD for the output path
    vout->GetFD();
    // Get largest FD for select()
    maxFD=getdtablehi();
  }
return;

qdbg("HandleEvents: transfersComplete=%d\n",transfersComplete);
  do
  {
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    
    FD_SET(vout->GetFD(),&fdr);
    if(select(maxFD+1,&fdr,&fdw,0,0)==-1)
    { qerr("QAlphaBuf: select"); break;
    }
    if(FD_ISSET(vout->GetFD(),&fdr))
    {
      // >=1 events on video out
      n=vs->Pending();
qdbg("  %d events\n",n);
      for(i=0;i<n;i++)
      {
        vs->NextEvent(&vlEvent);
qdbg("vout: %s\n",vs->Reason2Str(vlEvent.reason)); fflush(stderr);
        if(vlEvent.reason==VLTransferComplete)
        { transfersComplete++;
        } else if(vlEvent.reason==VLSequenceLost)
        {
qdbg("vout: seqLost %d frames\n",vlEvent.vlsequencelost.count);
        }
      }
    }
    break;
  } while(transfersComplete<=0);
#endif
}

/***********
* Swapping *
***********/
void QAlphaBuf::Swap()
// Swap to back buffer
{
#if defined(WIN32) || defined(linux)
  // Not supported on Win32
#else
  QDMBuffer *db;
  // Back buffer
  static QDMBuffer *bbuf=0;
  static bool pBufInUse[MAX_BUFFER];

//qdbg("QAlphaBuf:Swap() at vtrace %d\n",app->GetVTraceCount());

  if(bbuf)
  {
    // De-associate old buffer
#ifdef OBS
    QDMBuffer *dmb;
    dmb=new QDMBuffer(0);
#endif
//qdbg("  de-ass old\n");
    if(pBufInUse[backBufferIndex^1])
      pbuf[backBufferIndex^1]->Associate(0);
    //delete dmb;

    // Wait for last frame to have been completed
    HandleEvents();
//qdbg("  cur to vout\n");
    // Send current buffer to video out
    if(cv->GetGLContext())
    {//qdbg("finish\n");
      cv->Select();
      //glFinish();
    }
    vout->Send(bbuf->GetDMbuffer());
    //vout->Send(dmBuf[backBufferIndex]->GetDMbuffer());
//qdbg("  free bbuf\n");
    bbuf->Free();
    delete bbuf;
//qdbg("  evts\n");
    //HandleEvents();
    transfersComplete--;
    backBufferIndex^=1;
//qdbg("  VTB=%d\n",app->GetVTraceCount());
//QNap(50);
  }
  
  // Get next buffer
//qdbg("  Get free buffer\n");
  //db=dmBuf[backBufferIndex];
  while(bufPool->GetFreeBuffers()<=0)
  { //qdbg("waiting for free bufs\n");
    QNap(1);
  }
  bbuf=bufPool->AllocateDMBuffer();
//qdbg("bbuf=%p SGI %p\n",bbuf,bbuf->GetDMbuffer());

  // Associate (back) canvas with new DMbuffer
  // NOTE: old buffer is attempted to be freed by glXAssociateDMPBufferSGIX
//qdbg("declaring back buffer %p\n",db->GetDMbuffer());
//void *p=dmBufferMapData(bbuf->GetDMbuffer());
//qdbg("declaring back buffer %p @%p\n",bbuf->GetDMbuffer(),p);

#ifdef OBS
  db->Attach();
  pbuf->Associate(db);
  db->Free();
#endif
  pbuf[backBufferIndex]->Associate(bbuf);
  pbuf[backBufferIndex]->Select();

  glViewport(0,0,wid,hgt);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0,wid,0,hgt,-1,1);

  cv->SetGLContext(pbuf[backBufferIndex]->GetGLContext());
  cv->GetGLContext()->SetDrawable(this);
//qdbg("CV glctx %p\n",cv->GetGLContext());
//qdbg("  pbuf glctx=%p\n",pbuf->GetGLContext());
  cv->Select();
  
  pBufInUse[backBufferIndex]=TRUE;
#endif
}

void QAlphaBuf::Select()
{
#if defined(WIN32) || defined(linux)
  // No support
  QTRACE_PRINT("QAlphaBuf:Select() nyi on Win32/Linux");
#else
  pbuf[backBufferIndex]->Select();
#endif
}

/*************
* Equalizing *
*************/
void QAlphaBuf::Equalize(QRect *r)
// Equalizing the alpha buffers is NOT trivial
// Both pbuf[]'s are held on, and changed during a Swap()
// This way, we can copy from one pbuf to another
// BIG HACK!!!
{
#if defined(WIN32) || defined(linux)
  return;
#else
  // SGI IRIX
  GLXContext ctxBack;
  int dummy;
  
  if(r)
  { // Reverse Y (OpenGL coord system)
    r->y=576-r->hgt-r->y;
  }

  SetCurrentQGLContext(0);
  ctxBack=cv->GetGLContext()->GetGLXContext();
  glXMakeCurrentReadSGI(XDSP,
    pbuf[backBufferIndex]->GetGLXPBuffer(),   // Draw
    pbuf[backBufferIndex^1]->GetGLXPBuffer(),   // Read
    ctxBack);                                 // Context
  glDisable(GL_BLEND);
  glDisable(GL_DITHER);
  glRasterPos2i(0,0);
  if(r)glBitmap(0,0,0,0,r->x,r->y,(const GLubyte *)&dummy);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_ROWS,0);
  glPixelStorei(GL_PACK_SKIP_PIXELS,0);
  if(!r)glCopyPixels(0,0,wid,hgt,GL_COLOR);
  else  glCopyPixels(r->x,r->y,r->wid,r->hgt,GL_COLOR);
  glEnable(GL_DITHER);
#endif
}

/************************
* Holding on to buffers *
************************/
void QAlphaBuf::HoldBuffers(int n)
// When you have an alpha buffer pool with more than 2 buffers,
// the usual Swap() will walk along all buffers. This makes
// regular programming which assumes just 1 front and 1 back buffer
// go bad.
// With this function you can hold on to buffers, to leave 2 buffers
// when normal drawing is in place (as expected by many gfx routines).
// We need the extra buffers however for movie playback for example;
// this would be too slow when only 2 buffers are allowed. Perhaps
// a different heuristic for swapping might help too. Alpha swapping
// is slow as it is.
// Note that the total number of buffers held is 'n'; if you specify:
//   HoldBuffers(5); HoldBuffers(3); then the total #buffers held is 3.
// (the other 2 are freed in fact)
{
#if defined(WIN32) || defined(linux)
  // NYI on Win32/Linux
  return;
#else
  int i,count;
  
  // Check arg
  if(n>buffers)
  {
    qwarn("QAlphaBuf:HoldBuffers(%d); max held buffers is %d\n",n,buffers);
    return;
  }

qdbg("HoldBuffers(%d)\n",n);
qdbg("  buffers=%d, pre: free=%d\n",buffers,bufPool->GetFreeBuffers());
  while(bufPool->GetFreeBuffers()<n)
  { 
    qdbg("Waiting for enough (%d) free buffers (free=%d)\n",
      n,bufPool->GetFreeBuffers());
    QNap(10);
  }
  // Count #buffers
  // Make sure 'n' buffers are held
  for(i=0;i<n;i++)
  {
    if(!holdBuffer[i])
    {
      // Allocate it
//qdbg("  allocate %d\n",i);
      holdBuffer[i]=bufPool->AllocateDMBuffer();
    } // else it's already being held
  }
  // Free the rest
  for(;i<buffers;i++)
  {
    if(holdBuffer[i])
    {
      // Free it
//qdbg("  free %d\n",i);
      holdBuffer[i]->Free();
      delete holdBuffer[i];
      holdBuffer[i]=0;
    }
  }
#ifdef OBS
qdbg("  after: free=%d\n",bufPool->GetFreeBuffers());
QNap(50);
qdbg("  later: free=%d\n",bufPool->GetFreeBuffers());
#endif
#endif
}

#endif
