/*
 * QVinVout - video in -> video out (using QDM classes)
 * 14-09-98: Created!
 * 13-11-99: Support for VL_65; not yet tested!!!
 * BUGS:
 * - dtor should kill thread
 * NOTES:
 * - DM: vin -> vinPool -> vout
 * - Very handy for SCREEN->VIDEO OUT
 * (C) MG/RVG
 */

#if defined(__sgi)

#include <qlib/common/dmedia.h>
#include <qlib/vinvout.h>
#include <qlib/app.h>
#include <signal.h>
#include <sys/wait.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Default video timing if not deduced
#define DEFAULT_TIMING	VL_TIMING_625_CCIR601

// Default #buffers to use (they store fields, thus frames=buffers/2)
#define DEFAULT_BUFFERS	10

QVinVout::QVinVout(QDraw *draw)
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
  genlock=TRUE;
  buffers=DEFAULT_BUFFERS;
  vClass=VL_SCREEN;
  vSource=VL_ANY;
  vin=0;
  vout=0;
  vinPool=0;
  readThreadPID=-1;
}
QVinVout::~QVinVout()
{
  // Kill proc
  if(readThreadPID!=-1)
  { kill(readThreadPID,SIGTERM);
    waitpid(readThreadPID,0,0);
  }
  if(vin)delete vin;
  if(vout)delete vout;
  if(vinPool)delete vinPool;
}

/*************
* PRE-CREATE *
*************/
void QVinVout::SetGenlock(bool gl)
{ genlock=gl;
  if(vout)vout->SetGenlock(gl);
}
void QVinVout::SetBuffers(int bufs)
{ buffers=bufs;
}

bool QVinVout::Create()
{
  // Create DM objects
  vin=new QDMVideoIn(VL_ANY,VL_SCREEN);
  vin->SetTiming(timing);
  vin->SetPacking(VL_PACKING_YVYU_422_8);
  vin->SetLayout(VL_LAYOUT_LINEAR);
  vin->SetCaptureType(VL_CAPTURE_NONINTERLEAVED);
  vin->SetOrigin(ox,oy);
  //vin->SetZoomSize(texWid,texHgt);

  vout=new QDMVideoOut();
  vout->SetPacking(VL_PACKING_YVYU_422_8);
  vout->SetLayout(VL_LAYOUT_LINEAR);
  vout->SetCaptureType(VL_CAPTURE_NONINTERLEAVED);
  //vout->SetTiming(VL_TIMING_625_CCIR601);
  vout->SetTiming(timing);
  vout->SetGenlock(genlock);

  // Pools; video input, JPEG-encoded fields/frames
  vinPool=new QDMBPool(buffers,vin->GetTransferSize(),FALSE,FALSE);
  vinPool->AddProducer(vin);
  vinPool->AddConsumer(vout);
  if(!vinPool->Create())
  { qerr("Can't create vinPool");
    return FALSE;
  }
  // Register pool
  vin->RegisterPool(vinPool);
  return TRUE;
}

static void stopit(int)
{
  qwarn("QVinVout: stopit() called to end thread\n");
  exit(0);
}

static void runit(void *runObject)
{ QVinVout *v;
  v=(QVinVout*)runObject;
//qdbg("runit: v=%p\n",v);
  v->Thread();
}

void QVinVout::Run()
{
  vin->Start();
  vout->Start();
  // Spawn thread
//qdbg("QVV:Run this=%p\n",this);
  readThreadPID=sproc(runit,PR_SADDR|PR_SFDS,this);
  if(readThreadPID<0)
  { qerr("QVinVout::Run(); can't spawn thread");
  }
}

void QVinVout::Thread()
{
  // Handle stream events
  fd_set fdr;
  int    maxFD;
  VLEvent vlEvent;
  DMbuffer dmb;

  // Let's die when we catch sigterm from our parent too.
  signal(SIGTERM,stopit);

  // Let's die when our parent dies
  prctl(PR_TERMCHILD,NULL);

  maxFD=vin->GetFD();
  while(1)
  {
    FD_ZERO(&fdr);
    FD_SET(vin->GetFD(),&fdr);
    if(select(maxFD+1,&fdr,0,0,0)==-1)
    { perror("select"); return; }
    if(FD_ISSET(vin->GetFD(),&fdr))
    {
      //qdbg("vin event\n");
      vin->GetEvent(&vlEvent);
      /*qdbg("vin event (%s)\n",
        app->GetVideoServer()->Reason2Str(vlEvent.reason));*/
      if(vlEvent.reason==VLTransferComplete)
      { 
#ifdef USE_VL_65
        vlDMBufferGetValid(app->GetVideoServer()->GetSGIServer(),
          vin->GetPath()->GetSGIPath(),
          vin->GetSourceNode()->GetSGINode(),&dmb);
#else
        vlEventToDMBuffer(&vlEvent,&dmb);
#endif
        // Send to video out
        vout->Send(dmb);
        dmBufferFree(dmb);
      } else if(vlEvent.reason==VLSequenceLost)
      { qdbg("QVinVout: sequenceLost: field=%d, number=%d, count=%d\n",
          vlEvent.vlsequencelost.field,
          vlEvent.vlsequencelost.number,
          vlEvent.vlsequencelost.count);
      } else
      { //qdbg("event reason %d\n",vlEvent.reason);
        qdbg("QVinVout: event %s\n",
          app->GetVideoServer()->Reason2Str(vlEvent.reason));
      }
    } else
    { qerr("QVinVout: unknown stream event\n");
    }
  }
}

#endif
