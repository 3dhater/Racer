/*
 * QMVOut - video in -> video out (using QDM classes)
 * 14-09-98: Created!
 * 03-11-98: Default is now to use GENLOCK sync
 * 11-11-98: Default is now to use INTERNAL sync; AV2 is very sensitive
 * to no genlock signal when outputting video; it blocks.
 * 13-11-99: Support for VL_65; must test this.
 * BUGS:
 * - First movie frame audio is played multiple times; movie->curFrame
 * is updated out of sync with audio playing.
 * NOTES:
 * - DM: vin -> vinPool -> vout
 * - Very handy for SCREEN->VIDEO OUT
 * - The thread communication works as follows:
 *   - The video thread takes care of the REQUEST_ACK flag
 *   - The other thread sets the REQUEST_CRITICAL flag
 *   - Neither thread should touch the other one's flag, because
 *     race conditions will occur (!)
 * (C) MG/RVG
 */

#if defined(__sgi)

#include <qlib/common/dmedia.h>
#include <qlib/mvout.h>
#include <qlib/app.h>
#include <signal.h>
#include <unistd.h>
#ifdef WIN32
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include <qlib/process.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Test slow harddisk environment?
//#define TEST_SLOW_READS

#define SHOW_OUT(s)\
  qdbg("mvout: outPool free %d (%s)\n",outPool->GetFreeBuffers(),s);
#define SHOW_FREE(s)\
  qdbg("mvout: minPool free %d outPool free %d (%s)\n",minPool->GetFreeBuffers(),outPool->GetFreeBuffers(),s);
#define SHOW_AUDIO(s)\
  qdbg("mvout: audio filled %d (%s)   \r",ap->GetFilled(),s)
#define SHOW_IN_DECODER(s)\
  qdbg("mvout: buffers queued in decoder %d\n",stats.inDecoder);
#define SHOW_IN_VOUT(s)\
  qdbg("mvout: buffers queued in vidout %d\n",stats.inVOut);

// DEBUG
static int minCount,outCount,decCount;	// Count pool contents

// Disable SCREEN grabs? (no video in littering)
//#define NO_SCREEN

// Default video timing if not deduced
#define DEFAULT_TIMING	VL_TIMING_625_CCIR601

// Default #buffers to use (they store fields, thus frames=buffers/2)
#define DEFAULT_BUFFERS	14
#define MBUF_BUFFERS	6
#define OBUF_BUFFERS	2
#define MBUF_BUFFERS_MIN  2         // At which point to start decompr.

#define MAX(x,y)	((x)>(y)?(x):(y))

// Global vars
static QGLContext *glCtxPreview;

struct Stats
{
  int inDecoder;		// How many buffers in decoding phase?
  int inVOut;			// How many buffers in video out pipe?
};
static Stats stats;

QMVOut::QMVOut(QDraw *draw)
{ int w,h;
  timing=DEFAULT_TIMING;
  if(draw)
  { // Deduce timing
    w=draw->GetWidth();
    h=draw->GetHeight();
    if(w==720)
    { if(h==486)timing=VL_TIMING_525_CCIR601;
      else timing=VL_TIMING_625_CCIR601;
    } else if(w==768)
    { timing=VL_TIMING_625_SQ_PIX;
    } else if(w==640)
    { timing=VL_TIMING_525_SQ_PIX;
    }
    ox=draw->GetX();
    oy=draw->GetY();
  } else
  { ox=oy=0;
  }
  flags=0;
  buffers=DEFAULT_BUFFERS;
  targetFreeBufs=-1;
  vClass=VL_SCREEN;
  vSource=VL_ANY;
  vin=0;
  vout=0;
  minPool=0;
  readThreadPID=-1;
  movie=0; movieFuture=0;
  mbuf[0]=mbuf[1]=0;
  mbufIndex=0;
  obuf[0]=obuf[1]=0;
  obufIndex=0;
  input=SCREEN;
  ap=0;

  // Create a new OpenGL context that points to Q_BC
  // This way our thread can safely draw in Q_BC without the main
  // thread trying to do the same and thereby selecting the same
  // GLXContext in 2 different thread (which ends either process).
  // Without this, presenting dialogs etc would expose Q_BC and
  // conflicts arose.
  // It still conflicts with the caching of qopengl.cpp
  // That module has a global var for the current GLXContext, which
  // should be per thread (!).
  // This might lead to Xlib async reply errors and such.
  glCtxPreview=new QGLContext(Q_BC->GetQXWindow(),
    Q_BC->GetQXWindow()->GetVisualInfo(),
    Q_BC->GetCanvas()->GetGLContext(),TRUE);
  glCtxPreview->Setup2DWindow();
  glCtxPreview->Select(0,0,QGLContext::SILENT);
#ifdef OBS_NO_NEED_ANYMORE
  SetCurrentQGLContext(0);
  Q_BC->GetCanvas()->Select();
#endif
}
QMVOut::~QMVOut()
{
  // Kill proc
  if(readThreadPID!=-1)
  { kill(readThreadPID,SIGTERM);
    waitpid(readThreadPID,0,0);
    readThreadPID=0;
  }
  if(vin)delete vin;
  if(vout)delete vout;
  if(minPool)delete minPool;
}

/***********
* CRITICAL *
***********/
void QMVOut::EnterCritical()
// Use this when modifying variables which affect the video thread
{
  // Thread running?
  if(readThreadPID==0)return;
qdbg("pid exists=%d; defunct=%d\n",QProcessExists(readThreadPID),
QProcessIsDefunct(readThreadPID));
  // Stop thread for a while
  flags|=REQUEST_CRITICAL;
  // Wait for thread to halt
qdbg("QMVOut:EnterCritical() wait\n");
  while(!(flags&REQUEST_ACK))
  { QNap(1);
    qdbg("  flags=%d\n",flags);
qdbg("  pid exists=%d; defunct=%d\n",QProcessExists(readThreadPID),
QProcessIsDefunct(readThreadPID));
  }
qdbg("QMVOut:EnterCritical() RET\n");
}
void QMVOut::LeaveCritical()
// Non-video thread notifies the video thread can continue
{
  // Let thread know the critical section is over
  flags&=~REQUEST_CRITICAL;
  // Wait for video thread to see the end
  while(flags&REQUEST_ACK)QNap(1);
}

/*************
* PRE-CREATE *
*************/
void QMVOut::AttachMovie(QMovie *m)
// Given an open movie, use it for video output
// Movies smaller than the video output MAY be given, but will
// only be output currently in a graphics preview.
{
  // Make sure the thread is not using the CURRENT movie
  EnterCritical();
  movie=m;
  if(movie)
    movie->GotoFirst();
  LeaveCritical();
  // Make sure we have an audioport to play audio to
  if(!ap)
  { ap=new QAudioPort();
    ap->SetQueueSize(100000);
    if(ap==0||!ap->Open())
    { qerr("QMVOut::AttachMovie() failed to open audio port");
    }
  }
  if(movie)
  {
    QMovieTrack *it;
    it=movie->GetImageTrack();
    // Adjust decoder for this movie's size
    if(decoder!=0&&it!=0)
      decoder->SetSize(it->GetImageWidth(),it->GetImageHeight());
    // Check for movie fields or frames
    if(it)
    { if(it->GetImageInterlacing()==DM_IMAGE_NONINTERLACED)
      { flags&=~MOVIE_FIELDS;
      } else
      { flags|=MOVIE_FIELDS;         // Odd or Even
      }
    }
  }
}
void QMVOut::AttachFutureMovie(QMovie *m)
// After this movie has played, begin playing the next one at once!
{ if(!movie)
  { AttachMovie(m);
    return;
  }
  if(movieFuture)
  { qwarn("QMVOut::AttachFutureMovie(); future movie was already set");
  }
  movieFuture=m;
}

/*************
* ATTRIBUTES *
*************/
void QMVOut::SetOrigin(int x,int y)
{
  ox=x; oy=y;
}
bool QMVOut::IsRunning()
// Returns TRUE if video thread is running
{
  if(flags&IS_RUNNING)return TRUE;
  return FALSE;
}
void QMVOut::EnablePreview(bool yn)
// Enable preview on Q_BC?
{
  if(yn)flags|=DISPLAY_GFX;
  else  flags&=~DISPLAY_GFX;
}
void QMVOut::SetGenlock(bool gl)
{
  if(gl)flags|=GENLOCK;
  else  flags&=~GENLOCK;
  if(vout)
  { // Live setting of genlock
    vout->SetGenlock(gl);
  }
}
void QMVOut::SetSilentDrops(bool yn)
// If silent drops is activated, no dropped fields will be reported.
// This is useful in debugging circumstances where the screen is set to 72Hz
// and QMVOut is dropping fields because it can only output at 50Hz.
// The messages are then annoying, while working at 50Hz itself is annoying
// too.
{
  if(yn)flags|=SILENTDROPS;
  else  flags&=~SILENTDROPS;
}
void QMVOut::SetBuffers(int bufs)
{ buffers=bufs;
}
void QMVOut::SetTargetFreeBuffers(int bufs)
// To maintain a steady video delay, this target can be set
// to something >=0 and the video stream will try to keep
// the number of FREE buffers in the videoout pool to this
// number. This for having a constant audio delay with the picture.
// Note that this is useful only for video input -> video output
// streams, where video input could never catch up with output.
{
  targetFreeBufs=bufs;
}

bool QMVOut::Create()
// Returns TRUE if succesful
// May be called without a movie attached
{
  bool genlock;

  // Already created?
  if(vin)return TRUE;

  // Create DM objects
  vin=new QDMVideoIn(VL_ANY,VL_SCREEN);
  vin->SetTiming(timing);
  vin->SetPacking(VL_PACKING_YVYU_422_8);
  vin->SetLayout(VL_LAYOUT_LINEAR);
  vin->SetCaptureType(VL_CAPTURE_NONINTERLEAVED);
  vin->SetOrigin(ox,oy);
  //vin->SetZoomSize(texWid,texHgt);

  int w,h;
  //min=new QDMMovieIn(movie);
  //min->GetSize(&w,&h);
  vin->GetSize(&w,&h);
  decoder=new QDMDecoder(w,h);
  decoder->Create();

  vout=new QDMVideoOut();
  vout->SetPacking(VL_PACKING_YVYU_422_8);
  vout->SetLayout(VL_LAYOUT_LINEAR);
  vout->SetCaptureType(VL_CAPTURE_NONINTERLEAVED);
  vout->SetTiming(VL_TIMING_625_CCIR601);
  if(flags&GENLOCK)genlock=TRUE;
  else             genlock=FALSE;
  vout->SetGenlock(genlock);

  // Pools; video input, JPEG-encoded fields/frames
  minPool=new QDMBPool(buffers,vout->GetTransferSize(),FALSE,TRUE);
//qdbg("minPool: vout tfr size=%d\n",vout->GetTransferSize());
  //minPool->AddProducer(min);
  minPool->AddConsumer(decoder);
  if(!minPool->Create())
  { qerr("Can't create minPool");
    return FALSE;
  }

  outPool=new QDMBPool(buffers,vout->GetTransferSize(),FALSE,TRUE);
  outPool->AddProducer(vin);
  outPool->AddProducer(decoder);
  outPool->AddConsumer(vout);
  if(!outPool->Create())
  { qerr("Can't create outPool");
    return FALSE;
  }
qdbg("outPool: create; freebufs=%d\n",outPool->GetFreeBuffers());

  // Register pool
  vin->RegisterPool(outPool);
  vout->RegisterPool(outPool);
  decoder->SetDstPool(outPool);
  return TRUE;
}

static void stopit(int)
{
  qwarn("QMVOut: stopit() called to end thread\n");
  exit(0);
}

static void runit(void *runObject)
{ QMVOut *v;
  v=(QMVOut*)runObject;
//qdbg("runit: v=%p\n",v);
  v->Thread();
}

void QMVOut::Run()
{
  // Check if created already
  QASSERT_V(vin);               // Create() QMVOut first before Run()!

  if(IsRunning())return;

#ifndef NO_SCREEN
  vin->Start();
#endif
  vout->Start();
  // Spawn thread
//qdbg("QVV:Run this=%p\n",this);
  readThreadPID=sproc(runit,PR_SADDR|PR_SFDS,this);
  if(readThreadPID<0)
  { qerr("QMVOut::Run(); can't spawn thread");
  }
  // Notice the thread
  flags|=IS_RUNNING;
}

void QMVOut::DecompressFrame()
// 2 input and 2 output buffers are available; decompress a frame
{
  int i;

//qdbg("DecompressFrame mbufIndex=%d, obufIndex=%d\n",mbufIndex,obufIndex);
  // If no movie, don't try to play it
  if(!movie)return;

  // Free output buffers; they will be allocated by decoder->Convert()
  //SHOW_OUT("BEFORE");
  dmBufferFree(obuf[0]);
  dmBufferFree(obuf[1]);
  //SHOW_OUT("AFTER");
//qdbg("  out: free %p, %p\n",obuf[0],obuf[1]);
outCount-=2;
  movie->RenderToDMB(mbuf[0],mbuf[1]);
#ifdef TEST_SLOW_READS
  QNap(2);
#endif
  
  // Send JPEG data to decoder
  decoder->Convert(mbuf[0]);

  // Remember how many buffers are queued for decoding
  stats.inDecoder++;

  dmBufferFree(mbuf[0]);
minCount--;
  // Check for 2nd field to have been found
  if(dmBufferGetSize(mbuf[1])>0)
  {
    decoder->Convert(mbuf[1]);
    // Remember how many buffers are queued for decoding
    stats.inDecoder++;
  }
  dmBufferFree(mbuf[1]);
minCount--;
//qdbg("  min: free %p, %p\n",mbuf[0],mbuf[1]);

  // Shift buffers (fall down into [0] and [1])
  for(i=2;i<MBUF_BUFFERS;i++)
  { mbuf[i-2]=mbuf[i];
  }
  for(i=2;i<OBUF_BUFFERS;i++)
  { obuf[i-2]=obuf[i];
  }
  mbufIndex-=2;
  obufIndex-=2;
  movie->Advance();
  // Check end of play
  if(!movie->IsPlaying())
  { // Future movie? If so, switch to that one
    //movie->Play();
    if(movieFuture)
    { movie=movieFuture;
      movieFuture=0;
      // Play, but preserve loop
      int ff;
      ff=movie->GetFirstFrame();
      movie->Play();
      movie->SetFirstFrame(ff);
    }
  }
}
void QMVOut::PushAudio()
// Play a frame of audio
{
  if(movie->GetAudioTrack()->Exists())
  {
    if(movie->IsPlaying())
      movie->RenderToAP(ap);
//qdbg("QMVOut: audio filled: %d\n",ap->GetFilled());
  }
}

static void GetDMBuffer(QDMBPool *pool,DMbuffer *bufptr)
{
  fd_set fds;
  int poolFD=pool->GetFD();

  /* wait until a dmBuffer is available in inpool */
  do
  { FD_ZERO(&fds);
    FD_SET(poolFD,&fds);
    if(select(poolFD+1,NULL,&fds,NULL,NULL) < 0)
    { perror("select failed\n");
    }
  } while (dmBufferAllocate(pool->GetDMbufferpool(),bufptr) != DM_SUCCESS);
}
static void WaitForRead(int fd)
{
  fd_set fds;

  /* wait until a dmBuffer is available in inpool */
  while(1)
  { FD_ZERO(&fds);
    FD_SET(fd,&fds);
    if(select(fd+1,&fds,NULL,NULL,NULL) < 0)
    { perror("select failed\n");
    }
    break;
  }
}

static void PaintDMBuffer(DMbuffer dmb,int w,int h,int field,int flags)
// Paint the buffer in Q_BC
{
  char *pixels;
  int bw,bh;
  int x,y;
  float sx,sy;
 
  if(!(flags&QMVOut::DISPLAY_GFX))return;

  pixels=(char*)dmBufferMapData(dmb);
  glCtxPreview->Select(0,0,QGLContext::SILENT);
  glDrawBuffer(GL_FRONT);
  // Find out drawable dimensions
  bw=Q_BC->GetWidth();
  bh=Q_BC->GetHeight()/2;
  if(flags&QMVOut::MOVIE_FIELDS)
  {
    // Correct image dimensions
    h/=2;
    // Use interlaced preview
    glEnable(GL_INTERLACE_SGIX);
  } else
  {
    field=0;
    glDisable(GL_INTERLACE_SGIX);
  }
  if(0)
  { // Scale to fit
    sx=(float)bw/(float)w;
    sy=(float)bh/(float)h;
    //glPixelZoom(sx,-sy*2);
    glPixelZoom(sx,sy*2);
  } else
  { glPixelZoom(1,-1);
  }
  x=0; y=Q_BC->GetHeight()-1+(field^1);
//qdbg("PaintDMB: size=%dx%d\n",w,h);
  if(w==bw&&h==bh)
  { // Video frame sizes match; paint in 1 stroke
    glRasterPos2i(x,y);
    glDrawPixels(w,h,GL_YCRCB_422_SGIX,GL_UNSIGNED_BYTE,pixels);
  } else
  { // JPEG decoded frame is inside a bigger memory buffer
#ifdef OBS
    glPixelStorei(GL_UNPACK_ROW_LENGTH,w);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
#endif
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glRasterPos2i(x,y);
    glDrawPixels(w,h,GL_YCRCB_422_SGIX,GL_UNSIGNED_BYTE,pixels);
  }
}

void QMVOut::Thread()
{
  static int field;
  static int holdVOut;

  // Handle stream events
  fd_set fdr,fdw;
  int    maxFD;
  VLEvent vlEvent;
  DMbuffer dmb;

//#ifdef ND_TEST
  // Let's die when we catch sigterm from our parent too.
  signal(SIGTERM,stopit);

  // Let's die when our parent dies
  prctl(PR_TERMCHILD,NULL);
//#endif

  // Get the fd's to wait for
  // NOTE: Explicitly get all the video fd's, even if you don't
  // use them yet. The fd will only be created once you call
  // GetFD(), so getdtablehi() will only work after ALL GetFD()
  // calls have been done. Otherwise you'll miss the fd's that
  // are GetFD()-ed AFTER the getdtablehi()!
#ifdef NO_SCREEN
  maxFD=decoder->GetFD();
#else
  maxFD=MAX(vin->GetFD(),decoder->GetFD());
 #endif
  maxFD=MAX(maxFD,minPool->GetFD());
  maxFD=MAX(maxFD,outPool->GetFD());
  maxFD=MAX(maxFD,vout->GetFD());
qdbg("maxFD=%d\n",maxFD);
  maxFD=getdtablehi();
qdbg("maxFD=%d (getdtablehi)\n",maxFD);
  holdVOut=buffers;
//qdbg("movie=%p\n",movie);
//qdbg("Paths: vin=%p, vout=%p\n",vin->GetPath()->GetSGIPath(),
  //vout->GetPath()->GetSGIPath());

qdbg("vin fd=%d, vout fd=%d\n",vin->GetFD(),vout->GetFD());

  while(1)
  {
    if(flags&REQUEST_CRITICAL)
    { // Wait here
      flags|=REQUEST_ACK;
      // Wait until main thread lets us continue
qdbg("QMVOut:Thread; wait for critical end\n");
      while(flags&REQUEST_CRITICAL)QNap(1);
qdbg("QMVOut:Thread; critical section ended\n");
      flags&=~REQUEST_ACK;
    }

    //
    // Setup select() to wait for events
    //
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
#ifndef NO_SCREEN
    FD_SET(vin->GetFD(),&fdr);
#endif
    if(input==MOVIE)
    { 
      // Listen to free movie input pool and free output pool
      if(mbufIndex<MBUF_BUFFERS)FD_SET(minPool->GetFD(),&fdw);
      if(obufIndex<OBUF_BUFFERS)FD_SET(outPool->GetFD(),&fdw);
    }
    // Listen to the decoder, and video output events
    FD_SET(decoder->GetFD(),&fdr);
    FD_SET(vout->GetFD(),&fdr);

    // Wait for an event
    if(select(maxFD+1,&fdr,&fdw,0,0)==-1)
    { perror("select"); return; }

    // This is a good place to give some status
    //
//SHOW_IN_DECODER();
//SHOW_IN_VOUT();
//qdbg("mvout: select returned\n");
//qdbg("----- select() boundary\n");
//qdbg("  alloc count: min=%d, out=%d, dec=%d\n",minCount,outCount,decCount);
//qdbg("mvout: minPool free %d; outPool free %d; audio filled %d\n",
  //minPool->GetFreeBuffers(),outPool->GetFreeBuffers(),
  //ap?ap->GetFilled():0);
#ifdef OBS
qdbg("QMVPS: free movieIn=%d, out=%d, OKtoMin:%d, OKtoOut:%d\n",
minPool->GetFreeBuffers(),outPool->GetFreeBuffers(),
mbufIndex<MBUF_BUFFERS,obufIndex<OBUF_BUFFERS);
#endif
#ifdef OBS
SHOW_FREE("post-select");
SHOW_OUT("post-select");
//qdbg("input=%d (MOVIE=%d)\n",input,MOVIE);
#endif

    //
    // What event(s) happened?
    //
    if(input==MOVIE)
    { if(FD_ISSET(minPool->GetFD(),&fdw))
      { 
//qdbg("minPool ready to receive\n");
        field^=1; if(field)PushAudio();
        mbuf[mbufIndex]=minPool->AllocateBuffer();
        if(!mbuf[mbufIndex])
        { qerr("Can't to allocate min buffer");
          continue;
        }
//qdbg("minPool alloc'd %p\n",mbuf[mbufIndex]);
minCount++;
        mbufIndex++;
        if(mbufIndex>=MBUF_BUFFERS_MIN&&obufIndex>=OBUF_BUFFERS)
          DecompressFrame();
//qdbg("mvout: (minPool w-ev) minPool free %d; outPool free %d; audio filled %d\n",minPool->GetFreeBuffers(),outPool->GetFreeBuffers(),ap->GetFilled());
      }
      if(FD_ISSET(outPool->GetFD(),&fdw))
      {
//qdbg("outPool ready to receive\n");
        obuf[obufIndex]=outPool->AllocateBuffer();
        if(!obuf[obufIndex])
        { qerr("Can't to allocate out buffer");
          continue;
        }
//qdbg("outPool alloc'd %p\n",obuf[obufIndex]);
outCount++;
        obufIndex++;
        if(mbufIndex>=MBUF_BUFFERS_MIN&&obufIndex>=OBUF_BUFFERS)
          DecompressFrame();
      }
    }

    // Video input/output events?
    //
    if(FD_ISSET(vin->GetFD(),&fdr)||
       FD_ISSET(vout->GetFD(),&fdr))
    {
      if(!vin->GetEvent(&vlEvent))
      { qerr("QMVOut: vin event get failed");
      }
      if(vlEvent.vlany.path==vout->GetPath()->GetSGIPath())
      { // Really a vout event
        goto event_vout;
      }
      
//qdbg("vin event; path=%p\n",vlEvent.vlany.path);
//qdbg("vin event (%s)\n",app->GetVideoServer()->Reason2Str(vlEvent.reason));
      //vlEventToDMBuffer(&vlEvent,&dmb);
      //dmBufferFree(dmb);
      if(vlEvent.reason==VLTransferComplete)
      {
        vlDMBufferGetValid(app->GetVideoServer()->GetSGIServer(),
          vin->GetPath()->GetSGIPath(),
          vin->GetSourceNode()->GetSGINode(),&dmb);
//qdbg("mvout: got video transfercompl, SCREEN=%d\n",input==SCREEN);
        // Send to video out
        if(input==SCREEN)
        { vout->Send(dmb);
          // Keep status of video out
          stats.inVOut++;
          if(targetFreeBufs!=-1)
          { // Try to keep the number of free buffers constant,
            // to keep any corresponding audio delayed the same amount
            // Always sync in 2 fields at a time
            int nFree;
            nFree=outPool->GetFreeBuffers()/2;
            if(nFree>targetFreeBufs)
            { // Too many free buffers; fill up output stream with clones
              qdbg("QMVOut: need to sync; free bufs=%d, target=%d\n",
                nFree,targetFreeBufs);
              int i;
              for(i=0;i<(nFree-targetFreeBufs)*2;i++)
              {
                vout->Send(dmb);
                stats.inVOut++;
              }
            }
          }
        }
        dmBufferFree(dmb);
      } else if(vlEvent.reason==VLSequenceLost)
      { if(!(flags&SILENTDROPS))
          qdbg("QMVOut thread: %d input fields lost\n",
            vlEvent.vlsequencelost.count);
        //dmBufferFree(dmb);
      } else
      { //qdbg("event reason %d\n",vlEvent.reason);
        qdbg("QMVOut: vin event %s\n",
          app->GetVideoServer()->Reason2Str(vlEvent.reason));
        //dmBufferFree(dmb);
      }
    }

    // Code chunk to handle video out events
    // As all events arrive at one place in IRIX6.5, this looks
    // a bit weird. (the if(0))
    //
    //if(FD_ISSET(vout->GetFD(),&fdr))
    if(0)
    {
      vout->GetEvent(&vlEvent);
     event_vout:
//qdbg("QMVOut: vout event %s\n",app->GetVideoServer()->Reason2Str(vlEvent.reason));
      if(vlEvent.reason==VLTransferComplete)
      { //vlEventToDMBuffer(&vlEvent,&dmb);
//SHOW_FREE("vout complete");
        // 1 buffer out the pipeline
        stats.inVOut--;
#ifdef ND_HMM
        if(flags&MOVIE_FIELDS)
          stats.inVOut-=2;
        else
          stats.inVOut--;
#endif
        holdVOut--;
      } else if(vlEvent.reason==VLSequenceLost)
      {
        // Dropped a frame? (always important)
        //if(!(flags&SILENTDROPS))
          qdbg("QMVOut thread: %d OUTPUT fields lost\n",
            vlEvent.vlsequencelost.count);
      } else
      {
        qdbg("QMVOut: vout event %s\n",
          app->GetVideoServer()->Reason2Str(vlEvent.reason));
      }
    }

    // Check the JPEG decoder
    //
    if(FD_ISSET(decoder->GetFD(),&fdr))
    {
      static int field;
//SHOW_FREE("decoder field ready");
      DMbuffer b;
      if(!dmICReceive(decoder->GetDMimageconverter(),&b))
        QShowDMErrors("QMVOut::dmICReceive");

      // One less decoded buffers
      stats.inDecoder--;

      vout->Send(b);
      stats.inVOut++;

      // Preview in BC
      if(movie)
      { PaintDMBuffer(b,
          movie->GetImageTrack()->GetImageWidth(),
          movie->GetImageTrack()->GetImageHeight(),
          field,flags);
      }
      field^=1;

      holdVOut++;
      dmBufferFree(b);
//SHOW_FREE("decoder field sent to vout");
//QShowDMErrors("decoder");
    }
//SHOW_FREE("select END");
//if(ap)SHOW_AUDIO("vout");
  }
}


void QMVOut::SetInput(int n)
{
  if(input==n)return;
  if(vin)
  { // Don't overflow with video input
    if(n==MOVIE)
    { vin->Stop();
    } else
    {
#ifndef NO_SCREEN
      vin->Start();
#endif
    }
  }
  input=n;
}

#endif
