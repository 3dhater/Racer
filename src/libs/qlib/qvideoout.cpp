/*
 * videoout.cpp - From src to dst
 * 18-01-98: Created!
 * 20-01-98: C++ class QVideoOut instead of flat C
 * 14-10-98: This class is now OBSOLETE (however learnful it was); use QMVOut
 * 13-11-99: Although obsolete, this module is used in some old products,
 * so it was upgraded to the VL6.5 interface. Not yet tested though!!!
 * (products like kunstquiz, dicteevee, kvk)
 * From: stov.c, csource/video/s2v/ (C++) and dmplayO2 source code
 * Notes:
 * - 2 paths; screen -> memory, memory -> video
 * - Movie; use dmIC DMbuffers (dmICReceive) and pump into 2nd part
 * - Can do switching between types of inputs (Movie, Screen)
 * BUGS:
 * - Multiple audio tracks not supported
 * - Switching from Movie to Screen may lead to incorrect field order
 * FUTURE:
 * - Audio!
 * - Non-JPEG decoder support
 * - Slow-motion playback, reverse playback, scrubbing
 */

#if defined(__sgi)

//#include "videoout.h"
#include <qlib/video.h>
#include <bstring.h>
#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <dmedia/dm_imageconvert.h>
#include <GL/gl.h>
#include <qlib/app.h>
#include <qlib/audport.h>
#include <qlib/movie.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Behavior

// Def BUSY_POLLING to use less thread-friendly event polling
// Otherwise, select() is used (best for threading)
//#define BUSY_POLLING
// Def BLITIT to get copies of the video in your BC
//#define BLITIT
// Def SLOW to simulate slow field handling (pauses)
//#define SLOW

// #buffers in DMbufferpool
#define BUF_CNT		bufCnt

// SHARED DATA (Video process is a thread)

static QVideoOut *vo;
static int vpStarted;

// END SHARE DATA

// JPEG imageconverter code
#define DMICJPEG	'jpeg'

static VLEvent event;
static int     threadPri;

QVideoOut::~QVideoOut()
{
  if(ic)dmICDestroy(ic);
  if(ap)delete ap;
}

QVideoOut::QVideoOut()
{
  qlog(QLOG_NOTE,"class QVideoOut is OBSOLETE; use QMVOut instead");
  qlog(QLOG_NOTE,"class QVideoOut uses untested VL6.5 calls");
  vs=0;
  pathIn=pathOut=0;
  srcVid=drnMem=srcMem=drnVid=0;
  ic=0;
  srcSignal=SS_SCREEN;
  movie=0;
  trkImg=trkAud=0;
  mvid=mvidImg=mvidAud=0;
  vo=this;
  okFlags=0;		// Not ok to do anything
  ap=0;
}

// Movies/dmIC
bool QVideoOut::FindImageConverter()
// Find JPEG image converter that does realtime juggling
{
  DMparams *p;
  int i,count;
  char buf[5];
  int n;
  bool r=FALSE;			// Assume the worst

  // For all image converters
  count=dmICGetNum();
  dmParamsCreate(&p);
  for(i=0;i<count;i++)
  {
    if(dmICGetDescription(i,p)!=DM_SUCCESS)
      continue;
    n=dmParamsGetInt(p,DM_IC_ID);
    strncpy(buf,(const char *)&n,4); buf[4]=0;
    //qdbg("IC: %s = %s\n",buf,dmParamsGetString(p,DM_IC_ENGINE));
    // Check for JPEG, decoder, real-time
    if(n!=DMICJPEG)continue;
    if(dmParamsGetEnum(p,DM_IC_CODE_DIRECTION)!=DM_IC_CODE_DIRECTION_DECODE)
      continue;
    if(dmParamsGetEnum(p,DM_IC_SPEED)!=DM_IC_SPEED_REALTIME)
      continue;
    // Found our man
    if(dmICCreate(i,&ic)!=DM_SUCCESS)
      continue;
    qdbg("M2V: using dmIC '%s'\n",dmParamsGetString(p,DM_IC_ENGINE));
    r=TRUE;
    break;
  }
  dmParamsDestroy(p);
  return r;
}
bool QVideoOut::SetupImageConverter(int ib,int ob)
// Create DMbufferpools for for dmIC conversion
{ DMparams *p;
  int wid,hgt;
  //int size;
  int inBufs,outBufs;		// #buffers to reserve

  // IC instance was created already...

  // Define source
  dmParamsCreate(&p);
  wid=720; hgt=576;
  hgt/=2;			// Fields?
  //hgt*=2;
  dmSetImageDefaults(p,wid,hgt,DM_IMAGE_PACKING_CbYCrY);
  dmParamsSetString(p,DM_IMAGE_COMPRESSION,DM_IMAGE_JPEG);
  dmParamsSetEnum(p,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  if(dmICSetSrcParams(ic,p)!=DM_SUCCESS)
    qerr("dmICSetSrcParams failed");
  okFlags|=OK_INBUF;

  // Define destination
  dmParamsSetString(p,DM_IMAGE_COMPRESSION,DM_IMAGE_UNCOMPRESSED);
  if(dmICSetDstParams(ic,p)!=DM_SUCCESS)
    qerr("dmICSetDstParams failed");
  dmParamsDestroy(p);

  // Create input dmIC bufferpool
  dmParamsCreate(&p);
  inBufSize=wid*hgt*2;		// 1 pixel=2 bytes?
  inBufSize/=4;			// Reasonably JPEG
  inBufs=ib;
  // Note; no caching; faster DMA? but no CPU access please (slow!)
  dmBufferSetPoolDefaults(p,inBufs,inBufSize,DM_FALSE,DM_TRUE);
  if(dmICGetSrcPoolParams(ic,p)!=DM_SUCCESS)
    qerr("Can't get src pool params");
  dmParamsSetEnum(p,DM_POOL_SHARE,DM_POOL_INTERPROCESS);
  if(dmBufferCreatePool(p,&inPool)!=DM_SUCCESS)
    qerr("Can't create dmIC inpool");
  dmParamsDestroy(p);
  inPoolFD=dmBufferGetPoolFD(inPool);
  if(dmBufferSetPoolSelectSize(inPool,inBufSize)!=DM_SUCCESS)
    qerr("dmBufferSetPoolSelectSize failed");

  // Create output dmIC bufferpool
  dmParamsCreate(&p);
  outBufSize=wid*hgt*2;		// 1 pixel=2 bytes?
  outBufs=inBufs;
  outBufs=ob;
  dmBufferSetPoolDefaults(p,outBufs,outBufSize,DM_FALSE,DM_TRUE);
  if(dmICGetDstPoolParams(ic,p)!=DM_SUCCESS)
    qerr("Can't get src pool params");

  // Link in VL in output buffer
#ifdef USE_VL_65
  if(vlDMGetParams(vs->GetSGIServer(),pathIn->GetSGIPath(),drnMem->GetSGINode(),p)<0)
    qerr("vlDMPoolGetParams");
#else
  if(vlDMPoolGetParams(vs->GetSGIServer(),pathIn->GetSGIPath(),drnMem->GetSGINode(),p)<0)
    qerr("vlDMPoolGetParams");
#endif
    qerr("vlDMPoolGetParams");
  dmParamsSetEnum(p,DM_POOL_SHARE,DM_POOL_INTERPROCESS);
  qdbg("Buffercount after vl: %d\n",dmParamsGetInt(p,DM_BUFFER_COUNT));

  if(dmBufferCreatePool(p,&dmbPool)==DM_FAILURE)
  { qerr("Can't create DM output pool");
    return FALSE;
  }
  if(dmBufferSetPoolSelectSize(dmbPool,outBufSize)!=DM_SUCCESS)
    qerr("dmBufferSetPoolSelectSize on output pool failed");

  dmbPoolFD=dmBufferGetPoolFD(dmbPool);
  // Associate with dmIC output
  if(dmICSetDstPool(ic,dmbPool)!=DM_SUCCESS)
    qerr("dmICSetDstPool failed");

  // Output ok
  okFlags|=OK_OUTBUF;

  qdbg("Pool sizes: %d in, %d out\n",
    dmBufferGetAllocSize(inPool),dmBufferGetAllocSize(dmbPool));

  // Associate DM pool with VL pathIn
//#ifdef ND_VL
  if(vlDMPoolRegister(vs->GetSGIServer(),pathIn->GetSGIPath(),drnMem->GetSGINode(),dmbPool)<0)
    qerr("vlDMPoolRegister");
//#endif
  dmParamsDestroy(p);
  return TRUE;
}

bool QVideoOut::Create(QInfo *info)
{ int xferSize;
  int x,y,n;
  int bufCnt;
  bool fScreen=FALSE;		// Use screen grabbing?
  int inBufs,outBufs;

  fScreen=TRUE;
  // Get prefs
  inBufs=8; outBufs=18;
  if(info)
  { bufCnt=info->GetInt("video.output.buffers");
    threadPri=info->GetInt("video.output.pri");
    inBufs=info->GetInt("video.output.inbufs",inBufs);
    outBufs=info->GetInt("video.output.outbufs",outBufs);
    qdbg("bufCnt=%d, threadPri=%d\n",bufCnt,threadPri);
  } else
  { // Defaults
    bufCnt=5;		// In and out (in=3, in+out=5)
    threadPri=PRI_MEDIUM-1;	// Higher than audio
  }

  // Movie
  FindImageConverter();

  // VL
  vs=new QVideoServer();
  // Get video panel settings where possible
  //vs->RestoreSystemDefaults();
  if(fScreen)
  { srcVid=new QVideoNode(vs,VL_SRC,VL_SCREEN,VL_ANY);
  //srcVid=new QVideoNode(vs,VL_SRC,VL_VIDEO,VL_ANY);
    drnMem=new QVideoNode(vs,VL_DRN,VL_MEM,VL_ANY);
  }
  srcMem=new QVideoNode(vs,VL_SRC,VL_MEM,VL_ANY);
  drnVid=new QVideoNode(vs,VL_DRN,VL_VIDEO,VL_ANY);
  if(fScreen)pathIn=new QVideoPath(vs,VL_ANY,srcVid,drnMem);
  pathOut=new QVideoPath(vs,VL_ANY,srcMem,drnVid);

  // Setup paths
  if(fScreen)
  { if(!pathIn->Setup(VL_SHARE,VL_SHARE,pathOut))
    { qerr("Can't setup video in");
      return FALSE;
    }
  }

  // Set controls
  if(fScreen)
  { drnMem->SetControl(VL_PACKING,VL_PACKING_YVYU_422_8);
    //drnMem->SetControl(VL_PACKING,VL_PACKING_ABGR_8);
    //drnMem->SetControl(VL_CAP_TYPE,VL_CAPTURE_FIELDS);
    //drnMem->SetControl(VL_CAP_TYPE,VL_CAPTURE_INTERLEAVED);
    drnMem->SetControl(VL_CAP_TYPE,VL_CAPTURE_NONINTERLEAVED);
    /*drnMem->GetIntControl(VL_CAP_TYPE,&n);
    if(n!=VL_CAPTURE_FIELDS)
      qerr("VL_CAPTURE_FIELDS can't be set.");*/
    drnMem->SetControl(VL_MVP_ZOOMSIZE,512,412);
    drnMem->SetControl(VL_SIZE,512,412);
  }

  // Set timing of screen grab to be that of the video output
#ifdef ND_VCP_RULES
  drnVid->GetIntControl(VL_TIMING,&n);
  printf("drnVid timing: %d\n",n);
  srcVid->SetControl(VL_TIMING,n);
#endif
  // Set PAL timing
  //n=VL_TIMING_625_SQ_PIX;
  n=VL_TIMING_625_CCIR601;
  srcVid->SetControl(VL_TIMING,n);
  drnVid->SetControl(VL_TIMING,n);

  // Set source memory node capture type equal to drained memory
  drnMem->GetIntControl(VL_CAP_TYPE,&n);
  //srcMem->SetControl(VL_CAP_TYPE,VL_CAPTURE_INTERLEAVED);
  srcMem->SetControl(VL_CAP_TYPE,n);

  drnMem->GetIntControl(VL_PACKING,&n);
  srcMem->SetControl(VL_PACKING,n);

  // Filtering
  srcVid->SetControl(VL_FLICKER_FILTER,TRUE);
  //drnVid->SetControl(VL_NOTCH_FILTER,TRUE);

  // Location
  srcVid->SetControl(VL_ORIGIN,45,45);

  // Syncing
  drnVid->SetControl(VL_SYNC,VL_SYNC_INTERNAL);
  //drnVid->SetControl(VL_SYNC,VL_SYNC_GENLOCK);

  // Set size of memory drain according to size of source signal
  // (signal=screen/video)
  drnVid->GetXYControl(VL_SIZE,&x,&y);
  // Interlaced signal
  y/=2;
  drnMem->SetControl(VL_SIZE,x,y);

  // Verify that input == output regarding size
  x=pathIn->GetTransferSize();
  y=pathOut->GetTransferSize();
  if(x!=y)
  { qdbg("** Input path size doesn't match output size; %d!=%d\n",
      x,y);
  }

  // Setup DMbufferpools
  SetupImageConverter(inBufs,outBufs);

  return TRUE;
}

void QVideoOut::SetOrigin(int x,int y)
{
  srcVid->SetControl(VL_ORIGIN,x,y);
  //srcVid->SetControl(VL_H_PHASE,0,x&15);
  //srcMem->SetControl(VL_OFFSET,x&15,x&15);	// Jitters
  //drnVid->SetControl(1297481741,x&255);
}

bool QVideoOut::AttachMovie(QMovie *mv)
// Make sure we know the movie to play
{
  movie=mv;
  if(!movie)
  { trkImg=trkAud=0;
    return TRUE;		// No movie attached
  }
  // Get tracks
  trkImg=movie->GetImageTrack();
  trkAud=movie->GetAudioTrack();
  mvid=movie->GetMVid();
  mvidImg=trkImg->GetMVid();
  mvidAud=trkAud->GetMVid();
  return TRUE;
}

void QVideoOut::GetInputDMbuffer(DMbuffer *bufptr)
// From dmplayO2
{
  fd_set fds;

  /* wait until a dmBuffer is available in inpool */
  do
  { FD_ZERO(&fds);
    FD_SET(inPoolFD,&fds);
    if(select(inPoolFD+1,NULL,&fds,NULL,NULL) < 0)
    { perror("select failed\n");
    }
  } while (dmBufferAllocate(inPool,bufptr) != DM_SUCCESS);
}

void QVideoOut::SendDMbuffer(DMbuffer dmbuf)
// From dmplayO2
{
    int i;
    fd_set fds;
    USTMSCpair ustmsc;
    unsigned long long zust;

    /* Wait till there's room in outpool */
    FD_ZERO(&fds);
    FD_SET(dmbPoolFD, &fds);
    if (select(dmbPoolFD+1, NULL, &fds, NULL, NULL) < 0) {
            //ERROR("select failed\n");
      qerr("select() failed\n");
    }
    /* Send frame or field to decompress */
#ifdef ND
    dmID.inustmsc.msc++;
    dmGetUST((unsigned long long *)&dmID.inustmsc.ust);
    dmBufferSetUSTMSCpair(dmbuf, dmID.inustmsc);
#endif
    if (dmICSend(ic,dmbuf,0,0) != DM_SUCCESS)
    {
      qerr("dmICSend failed");
            //fprintf(stderr, "Frame %d:\n", frameNum);
            //ERROR( dmGetError( NULL, NULL ) );
    }

    return;
}

char *QVideoOut::FindSecondField(char *buf,int size)
// From dmplay, by hand search of field separator in 2 fields worth of data
{
  static char *fldbdry="\xFF\xD9\xFF\xD8";	// Hardcode or what?!

  int i;
  char *p;
  char *last;

  /* Assume the break between the two fields is not in the first 4/9 */
  last = buf + size - 1 - strlen(fldbdry);
  for (p = buf + 4*size/9; p < last; p++)
  { if(*p!=0xFF)continue;
    if (bcmp((const void *)p, (const void *)fldbdry, strlen(fldbdry)) == 0)
    {
      return p+2;
    }
  }

  /* search the first 4/9 as a last resort */
  last = buf + (size*4)/9;
  for (p = buf; p < last; p++)
  { if(*p!=0xFF)continue;
    if (bcmp((const void *)p,(const void *)fldbdry,strlen(fldbdry)) == 0) {
          return p+2;
      }
  }

  qerr( "QVO: Cannot separate two fields of the frame");
  return 0;
}

void VideoOutProcess(void *)
{ if(!vo)return;
  vo->Process();
}

void QVideoOut::ProcessScreen()
// Screen input active; do 1 iteration
{
  // Thread-friendly event waiting with select()
  int pathInFD,maxFD;
  int n,i;
  fd_set fdset;

  //qdbg("QVideoOut:ProcessScreen\n");
#ifdef USE_VL_65
  pathInFD=drnMem->GetFD();
#else
  pathInFD=pathIn->GetFD();
#endif
  maxFD=pathInFD+1;             // Biggest file descriptor
  //qdbg("  maxFD=%d\n",maxFD);
  //while(1)
  { // Re-initialize select
    FD_ZERO(&fdset);
    FD_SET(pathInFD,&fdset);
    // No output events are watched; timing is the same, so no
    // overflow can happen on output path
    //FD_SET(pathOutFD,&fdset);
    //qdbg("select...\n");
    if(select(maxFD,&fdset,0,0, 0)==-1)
    { perror("select()"); exit(1); }
    //qdbg("select RET\n");
    // Events on pathIn
    n=drnMem->GetFilled();
    //qdbg("Events after select: %d\n",n);
    for(i=0;i<n;i++)
    {
#ifdef USE_VL_65
      if(vlPending(vs->GetSGIServer())<=0)
#else
      if(vlEventRecv(vs->GetSGIServer(),pathIn->GetSGIPath(),&event)
         !=DM_SUCCESS)
#endif
      { qdbg("Unexpected; no event for %d out of %d expected\n",i,n);
        break;
      }
#ifdef USE_VL_65
      vlNextEvent(vs->GetSGIServer(),&event);
#endif
      //qdbg("reason=%d\n",event.reason);
      if(event.reason==VLTransferComplete)
      { //if(dmb)dmBufferFree(dmb);
#ifdef USE_VL_65
        vlDMBufferGetValid(vs->GetSGIServer(),pathIn->GetSGIPath(),
          srcVid->GetSGINode(),&dmb);
        // Send field/frame to output path
        vlDMBufferPutValid(vs->GetSGIServer(),pathOut->GetSGIPath(),
          drnVid->GetSGINode(),dmb);
#else
        vlEventToDMBuffer(&event,&dmb);
        // Send field/frame to output path
        vlDMBufferSend(vs->GetSGIServer(),pathOut->GetSGIPath(),dmb);
#endif
        dmBufferFree(dmb);
      } else
      { qdbg("VidOut: unexpected event reason %s\n",
          vlEventToName(event.reason));
      }
    }
  }
  // Detect whether input signal has changed; stop receiving
  // frames if it did (don't hang on the select())
  if(srcSignal!=SS_SCREEN)
    pathIn->EndTransfer();
}

void QVideoOut::ProcessMovie()
// Handle the movie playing
{ 
  DMbuffer dmbf[2];

  // Get 2 DM buffers to put the fields in
  GetInputDMbuffer(&dmbf[0]);
  GetInputDMbuffer(&dmbf[1]);

  // Get fields
  movie->RenderToAP(ap);
  movie->RenderToDMB(dmbf[0],dmbf[1]);
  movie->Advance();
  SendDMbuffer(dmbf[0]);
  SendDMbuffer(dmbf[1]);
 do_converted:
  //qdbg("dmBufferFree\n");
  dmBufferFree(dmbf[0]);
  dmBufferFree(dmbf[1]);

  // Send decompressed frames to video
  int i,n;
  n=dmICGetDstQueueFilled(ic);
  for(i=0;i<n;i++)
  { if(dmICReceive(ic,&dmb)!=DM_SUCCESS)
    { qdbg("** dmICReceive failed\n");
      break;
    }
#ifdef USE_VL_65
    vlDMBufferPutValid(vs->GetSGIServer(),pathOut->GetSGIPath(),
      drnVid->GetSGINode(),dmb);
#else
    vlDMBufferSend(vs->GetSGIServer(),pathOut->GetSGIPath(),dmb);
#endif
    dmBufferFree(dmb);
  }

}

void QVideoOut::Process()
// The video handling thread
{ VLEvent event;
  int x,y,i,n;
  unsigned short *fieldData;
  int eventCnt;
  //int frame;

  // Audio
  //QAudioPort *ap;
  int framesAudio;
  //int fcs;              // Frame compressed size
  // Temp buffer for splicing data and audio reading
  char *tempBuffer;
  int   tempBufferSize;
  bool mute=FALSE;
  int lastAudioFrame,nextAudioFrame;

  drnMem->GetXYControl(VL_SIZE,&x,&y);
  qdbg("VideoInRun: drn = %dx%d\n",x,y);

  // Setup audio
  ap=new QAudioPort();          // Default 44.1kHz, 16-bit stereo
  // Use dmAudioFrameSize(params) to get bytes per frame
  if(trkAud)
  { framesAudio=trkAud->GetLength();
    tempBufferSize=framesAudio/trkImg->GetLength()+1;
  } else
  { framesAudio=0;
    tempBufferSize=1757;	// 44.1kHz/PAL rate
  }
  tempBufferSize*=4;                    // Bytes per sample frame
  ap->SetQueueSize((tempBufferSize/4)*30);      // Allow for precaching
  if(!ap->Open())
  { mute=TRUE;
    printf("QVideoOut:: can't open audioport\n");
  }
  lastAudioFrame=0;             // Start of frame

  // Temporary buffer for wrapping data
  //if(cis>tempBufferSize)
    //tempBufferSize=cis;
  tempBuffer=(char*)qcalloc(tempBufferSize);
  if(!tempBuffer)
  { // err
    printf("QMovie::Play: not enough memory for temp buf\n");
    return;
  }

  // Check if process can do anything
  if(!(okFlags&OK_INOUT))
  { qerr("QVideoOut: not ok to start video processing\n");
    return;
  }

  // Start non-stop transmission
  pathIn->SelectEvents(VLTransferCompleteMask|
                       VLTransferFailedMask|
                       VLStreamPreemptedMask|
                       VLDeviceEventMask);
  // ^^ Only nice on O2/DM
  //qdbg("bt\n");
  if(srcSignal==SS_SCREEN)
  { // Start input signal grabbing
    pathIn->BeginTransfer();
  }
  // Output
  //pathOut->SelectEvents(VLTransferCompleteMask|VLDeviceEventMask);
  pathOut->BeginTransfer();

  //qdbg("forever\n");
  //memData=(unsigned short *)malloc(y*sizeof(*memData));

  QSetThreadPriority(threadPri);
  vpStarted=1;

  // Movie
  //InitMovie();

  //frame=0;
  int parmID;
  size_t frameSize;
  size_t fieldSize[2];
  off64_t gap;
  MVframe frames;
  MVdatatype dt;
  void *mem[2];
  int err;
  int index;
  char errStr[DM_MAX_ERROR_DETAIL];
  DMbuffer dmbf[2];
  char *imageBuffer;
  MVtimescale timeScale;
  double frameRate;
  int frameOffset;

  if(trkImg)frames=trkImg->GetLength();
  qdbg("frames=%d\n",frames);
  imageBuffer=(char*)malloc(100000);
  // PAL hardcodes
  timeScale=25;
  frameRate=25.0;		// PAL
  while(1)
  {
    if(srcSignal==SS_SCREEN)
    { ProcessScreen();
      continue;
    }

    //qdbg("ProcessMovie\n");
    ProcessMovie();
#ifdef OBS_MOVIE_IN_QMOVIE_CPP
    // Movie
    frame=(frame+1)%frames;

    // Audio
#define PLAY_AUDIO_ADVANCE 2
    if(mute)goto no_audio;
    if(frame<frames-PLAY_AUDIO_ADVANCE-1)
    { nextAudioFrame=trkImg->MapToTrack(trkAud,frame+PLAY_AUDIO_ADVANCE+1);
      if(nextAudioFrame<=lastAudioFrame)
      { lastAudioFrame=nextAudioFrame;
        goto no_audio;
      }
      /*qdbg("  lastAF=%d, nextAF=%d; total=%d\n",
        lastAudioFrame,nextAudioFrame,framesAudio);*/
      qdbg("  filled audio=%d, fillable=%d, want to fill %d\n",
         ap->GetFilled(),ap->GetFillable(),nextAudioFrame-lastAudioFrame);
      //qdbg("audio: last=%d, next=%d\n",lastAudioFrame,nextAudioFrame);
      //*/
      if(nextAudioFrame>framesAudio)
      { // Should never happen
        printf("QMovie::Play: audio read overflow\n");
        nextAudioFrame=framesAudio;
      }
      trkAud->ReadFrames(lastAudioFrame,nextAudioFrame-lastAudioFrame,
        tempBuffer,tempBufferSize);
      /*printf("read audio frames %d (count %d)\n",frame*audioBufferSize,
        audioBufferSize);*/
      // WriteSamps; write samples times channels
      //qdbg("write samples\n");
      ap->WriteSamps(tempBuffer,(nextAudioFrame-lastAudioFrame)*2);
      /*printf("  filled audio=%d, fillable=%d\n",
         ap->GetFilled(),ap->GetFillable());*/
      lastAudioFrame=nextAudioFrame;
    }
   no_audio:

    //if(frame>5)break;
    //qdbg("frame=%d\n",frame);
    // Get 2 DM buffers to put the fields in
    GetInputDMbuffer(&dmbf[0]);
    GetInputDMbuffer(&dmbf[1]);

    // Map frame# to data index (movies can have repeating frame data)
    if (mvGetTrackDataIndexAtTime(mvidImg,
         (MVtime)(((double)frame) * timeScale / frameRate),
         timeScale, &index, &frameOffset) != DM_SUCCESS) {
        qerr("mvGetTrackDataIndexAtTime failed");
    }
    //printf("frame: %d, data index=%d\n",frame,index);

    // Not all frames have field info in all movies
    // If no field info is present, we must look ourselves
    if(mvTrackDataHasFieldInfo(mvidImg,index)!=DM_TRUE)
    { //qerr("No field info for image track");
      // Get frame (=2 fields) and search field 2 by hand (bleh!)
      // From dmplay
      size_t  size;
      int     f1size, f2size;
      //u_char  *f1, *f2;
      char  *f1, *f2;
      int     paramsId, numFrames;
      MVdatatype      dataType;

      //qdbg("mvGetTrackDataInfo\n");
      if (mvGetTrackDataInfo( mvidImg, index,
                              &size, &paramsId,
                              &dataType, &numFrames ) != DM_SUCCESS)
      {
          qerr( "mvGetTrackDataInfo failed\n" );
      }
      //qdbg("mvReadTrackData\n");
      if (mvReadTrackData( mvidImg, index,
                           size, imageBuffer ) != DM_SUCCESS)
      {
          qerr( "mvReadTrackData failed\n" );
      }
      //qdbg("FindSecondField\n");
      f1 = imageBuffer;
      f2 = FindSecondField(f1, size);
      f1size = f2 - f1;
      f2size = size - f1size;

      //qdbg("copy\n");
      bcopy(f1, dmBufferMapData(dmbf[0]), f1size);
      dmBufferSetSize(dmbf[0], f1size);
      SendDMbuffer(dmbf[0]);

      //qdbg("copy2\n");
      bcopy(f2, dmBufferMapData(dmbf[1]), f2size);
      dmBufferSetSize(dmbf[1], f2size);
      SendDMbuffer(dmbf[1]);

      //qdbg("goto do_conv\n");
      goto do_converted;
      //continue;
    }
    //mvGetTrackDataInfo(mvidImg,index,&frameSize,&parmID,&dt,&frames);
    //qdbg("frame %d: size=%d, parmID=%d, frames=%d\n",frame,frameSize,parmID,frames);
    mvGetTrackDataFieldInfo(mvidImg,index,&fieldSize[0],&gap,
      &fieldSize[1]);
    //qdbg("frame %d: size=%d & %d; in=%d, out=%d\n",
      //frame,fieldSize[0],fieldSize[1],inBufSize,outBufSize);
    mem[0]=dmBufferMapData(dmbf[0]);
    mem[1]=dmBufferMapData(dmbf[1]);
    //qdbg("> Read image data\n");
    //mvReadTrackData(mvid,frame,100000,mem);
    n=mvReadTrackDataFields(mvidImg,index,inBufSize,mem[0],outBufSize,mem[1]);
    if(n!=DM_SUCCESS)
    { n=mvGetErrno();
      qdbg("** Can't mvReadTrackDataFields; %d = %s\n",n,
        mvGetErrorStr(n));
    }
    dmBufferSetSize(dmbf[0],fieldSize[0]);
    dmBufferSetSize(dmbf[1],fieldSize[1]);
    SendDMbuffer(dmbf[0]);
    SendDMbuffer(dmbf[1]);
   do_converted:
    //qdbg("dmBufferFree\n");
    dmBufferFree(dmbf[0]);
    dmBufferFree(dmbf[1]);

    /*qdbg("# srcFilled=%d, dstFilled=%d #",
      dmICGetSrcQueueFilled(ic),
      dmICGetDstQueueFilled(ic));
    qdbg(".");*/
    n=dmICGetDstQueueFilled(ic);
    for(i=0;i<n;i++)
    { if(dmICReceive(ic,&dmb)!=DM_SUCCESS)
      { qdbg("** dmICReceive failed\n");
        break;
      }
      vlDMBufferSend(vs->GetSGIServer(),pathOut->GetSGIPath(),dmb);
      dmBufferFree(dmb);
    }
    //QNap(5);
#endif
// ^^ MOVIE PLAYING LOGIC IS NOW INSIDE QMOVIE.CPP

  }
#ifdef OBS_BUSY_POLL
  // Not too thread-friendly
  for(;;)
  {
    // Get latest field
    dmb=0;
    //qdbg("for evcnt\n");
    for(eventCnt=0;!eventCnt;)
    { while(vlEventRecv(vs->GetSGIServer(),pathIn->GetSGIPath(),&event)==0)
      { //qdbg("event.reason=%d\n",event.reason);
        if(event.reason==VLTransferComplete)
        { if(dmb)dmBufferFree(dmb);
          vlEventToDMBuffer(&event,&dmb);
          // Send field/frame to output path
          vlDMBufferSend(vs->GetSGIServer(),pathOut->GetSGIPath(),dmb);
          eventCnt++;
        }
      }
    }
    //qdbg("fieldata, dmb=%p\n",dmb);
    fieldData=(unsigned short*)dmBufferMapData(dmb);
#ifdef BLITIT
    app->GetBC()->GetCanvas()->Select();
    glRasterPos2f(0,y);
    glPixelZoom(1,-1);
    glDrawPixels(x,y,GL_YCRCB_422_SGIX,GL_UNSIGNED_BYTE,fieldData);
    //glDrawPixels(x,y,GL_RGBA,GL_UNSIGNED_BYTE,fieldData);
    app->GetBC()->Swap();
#endif
    for(i=0;i<y;i++)
      memData[i]=fieldData[i*x+x/2];
    //qdbg("plus\n");
    //qdbg("qnap\n");
#ifdef SLOW
    QNap(rand()%12);
#endif
    for(i=0;i<y;i++)
      if(memData[i]!=fieldData[i*x+x/2])
        break;
    if(i==y)printf("+");
    else printf("-");
    fflush(stdout);
    //qdbg("bufferfree\n");
    if(dmBufferFree(dmb)==DM_FAILURE)
      qerr("dmBufferFree: failure");
    //qdbg("next\n");
  }
#endif
// ^^ BUSY_POLLING

}

bool QVideoOut::Run()
{ int signum=9;
  // Spawn video out thread
  prctl(PR_SETEXITSIG,signum);
  sproc(VideoOutProcess,PR_SALL);
  while(!vpStarted)QNap(1);
  return TRUE;
}

/****************
* REALTIME TRIX *
****************/
int QVideoOut::GetInput()
{ return srcSignal;
}

void QVideoOut::SetInput(int n)
// Change input video stream on the fly (no rebuilding of video paths)
{
  if(srcSignal==n)return;
  qdbg("QVideoOut: switch input to %d\n",n);
  srcSignal=n;
  if(srcSignal==SS_SCREEN)
  { pathIn->BeginTransfer();
  } else if(srcSignal==SS_MOVIE)
  {
    // No screen frames, please
    qdbg("  EndTransfer\n");
    //pathIn->EndTransfer();
  }
  qdbg("QVO:SetInput RET\n");
}

#endif
