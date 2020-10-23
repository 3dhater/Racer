/*
 * QMovie - a digital movie (may be played on video output)
 * 21-03-97: Created!
 * BUGS:
 * - Audiobuffer size is NOT calculated during Play (set to 100000)
 * - RenderToDMB() assumes DMbuffers are big enough
 * - RenderToBM() reverses fields for PAL movies (stored interleaved; even)
 * This is probably a bug in MV (ofcourse it is).
 * - RenderToOpenGL() doesn't accurately calculate 'time'
 * NOTES:
 * - Old source code fragments available on Cosmo JPEG movies, OpenGL direct
 * play movies etc. Do not delete just yet.
 * (C) MarketGraph/RVG
 */

#if defined(__sgi)

#include <qlib/movie.h>
//#include <qlib/gelwg.h>
#include <qlib/audport.h>
//#include <qlib/compr.h>
#include <qlib/app.h>
#include <stdio.h>
//#include <cl.h>
//#include <dmedia/cl_cosmo.h>
#include <bstring.h>
#include <unistd.h>

#include <qlib/video.h>	// test
#include <qlib/debug.h>
DEBUG_ENABLE

#define MAGIC_SMPS_PER_FRAME	1764

// Number of video frames for audio to be ahead of video buffering
#define PLAY_AUDIO_ADVANCE	0
// ^^^ if >1, I'm afraid it will seek back and forth in the movie file

#undef  DBG_CLASS
#define DBG_CLASS "QMovieTrack"

/**************
* QMOVIETRACK *
**************/
QMovieTrack::QMovieTrack(QMovie *_movie,int trackType)
{ MVid vid;
  int  r;

  movie=_movie;
  vid=movie->GetMVid();
  trackID=0;
  if(!vid)return;
  r=DM_SUCCESS;
  switch(trackType)
  { case QMTT_VIDEO: r=mvFindTrackByMedium(vid,DM_IMAGE,&trackID); break;
    case QMTT_AUDIO: r=mvFindTrackByMedium(vid,DM_AUDIO,&trackID); break;
  }
  if(r!=DM_SUCCESS)
    trackID=0;
}
QMovieTrack::~QMovieTrack()
{
}

bool QMovieTrack::Exists()
{ if(trackID==0)return FALSE;
  return TRUE;
}
int QMovieTrack::GetLength()
{ if(trackID==0)return 0;
  return mvGetTrackLength(trackID);
}
int QMovieTrack::GetImageWidth()
{ if(trackID==0)return 0;
  return mvGetImageWidth(trackID);
}
int QMovieTrack::GetImageHeight()
{ if(trackID==0)return 0;
  return mvGetImageHeight(trackID);
}
int QMovieTrack::GetImageInterlacing()
// Returns DM_IMAGE_NONINTERLACED, DM_IMAGE_INTERLACED_EVEN
// or DM_IMAGE_INTERLACED_ODD
{ if(trackID==0)return 0;
  return mvGetImageInterlacing(trackID);
}

int QMovieTrack::GetCompressedImageSize(int frame)
{
  return mvGetCompressedImageSize(trackID,frame);
}
int QMovieTrack::MapToTrack(QMovieTrack *dstTrack,int srcFrame)
// Maps a source frame number to the same frame in 'dstTrack' (in time)
{ int dstFrame=0;
  mvMapBetweenTracks(trackID,dstTrack->GetMVid(),srcFrame,
    (MVframe*)&dstFrame);
  return dstFrame;
}
bool QMovieTrack::ReadFrames(int frame,int count,void *buf,int bufSize)
{ DMstatus r;
  if(bufSize==-1)bufSize=2000000000;	// Don't care about bufSize (2G)
  r=mvReadFrames(trackID,frame,count,bufSize,buf);
  if(r!=DM_SUCCESS)
  { int n;
    /*qerr("QMovieTrack::ReadFrames(frame %d, count %d,bufSize %d) failed"
         " trackID=%p",
      frame,count,bufSize,trackID);*/
    n=mvGetErrno();
    qerr("QMovieTrack::ReadFrames: Can't mvReadFrames; err %d (%s)\n",n,
      mvGetErrorStr(n));
  }
  return r;
}
bool QMovieTrack::ReadCompressedImage(int frame,void *buf,int bufSize)
{
  if(bufSize==-1)bufSize=2000000000;	// Don't care about bufSize (2G)
  return mvReadCompressedImage(trackID,frame,bufSize,buf);
}

/*********
* QMOVIE *
*********/

#undef  DBG_CLASS
#define DBG_CLASS "QMovie"

QMovie::QMovie(cstring fname)
{
  DBG_C("ctor")
  mvid=0;
  trkImage=trkAudio=0;
  flags=0;

  // Read movie file
  fileName=qstrdup(fname);
  if(IsMovieFile())
  { if(Open(O_RDONLY))
    { // Find video & audio tracks
      //qdbg("QMovie: opened %s\n",fname);
      trkImage=new QMovieTrack(this,QMTT_VIDEO);
      trkAudio=new QMovieTrack(this,QMTT_AUDIO);
      //qdbg("QMovie: audio trackID=%p\n",trkAudio->GetMVid());
    } else qwarn("QMovie: can't load '%s'\n",fname);
  } else qwarn("QMovie: '%s' is not a movie\n",fname);

  // Playback
  firstFrame=lastFrame=0;
  playing=FALSE;
  playMode=0;
  speed=0;
  frameRate=1.0;
  curFrame=0;
  //apPlay=0;
}
QMovie::~QMovie()
{
  DBG_C("dtor")

  if(mvid)mvClose(mvid);
  delete trkImage;
  delete trkAudio;
  if(fileName)qfree(fileName);
}

QMovieTrack *QMovie::GetImageTrack()
{ return trkImage;
}
QMovieTrack *QMovie::GetAudioTrack()
{ return trkAudio;
}

bool QMovie::IsMovieFile()
{ if(fileName)
    if(mvIsMovieFile(QExpandFilename(fileName)))
    { return TRUE;
    }
  return FALSE;
}
bool QMovie::Open(int mode)
{ if(mvOpenFile(QExpandFilename(fileName),mode,&mvid)==DM_SUCCESS)
  { return TRUE;
  }
  printf("%s/%d failed to open movie\n",__FILE__,__LINE__);
  return FALSE;
}

int QMovie::GetMaxImageSize()
// Returns the maximum size of the image (compressed)
{ int frame,frames,max=0,size;
  frames=trkImage->GetLength();
  for(frame=0;frame<frames;frame++)
  { size=mvGetCompressedImageSize(trkImage->GetMVid(),frame);
    if(size>max)max=size;
  }
  return max;
}

//extern QVideoNode *voutdrn,*drn;

void QMovie::SetMute(bool yn)
{ if(yn)flags|=MUTE;
  else  flags&=~MUTE;
}
bool QMovie::GetMute()
{ if(flags&MUTE)return TRUE;
  return FALSE;
}

#ifdef OBS_NO_DIRECT_PLAY
void QMovie::Play(int from,int to)
// Stream decompression using buffered CL interface
{
  //qdbg("QMovie::Play\n");

  MVtime time;
  int    timeSlice;		// Length of frame
  MVtimescale timeScale;
  QBitMap *bmVideo=0;
  DMstatus status;
  DMparams *imageParams=0;
  int imgWid=720,imgHgt=576;
  QAudioPort *ap=0;
  char *audioBuf=0;
  size_t audioBufSize=100000;	// Calculate!
  size_t audioFilled;
  MVframe framesFilled;
  MVrect r;
  int    curFrame;
  bool   mute;

  mute=GetMute();
  imgWid=mvGetImageWidth(trkImage->GetMVid());
  imgHgt=mvGetImageHeight(trkImage->GetMVid());
  qdbg("QMovie:Play: image %dx%d\n",imgWid,imgHgt);
  if(dmParamsCreate(&imageParams ) != DM_SUCCESS ) {
                  printf( "Out of memory.\n" );
                  exit( 1 );
              }
  if ( dmSetImageDefaults( imageParams,
       imgWid,  /* width */
       imgHgt,  /* height */
       DM_PACKING_RGBX ) != DM_SUCCESS ) {
  printf( "Out of memory.\n" );
  exit( 1 );
  }
  dmParamsSetInt(imageParams,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  dmParamsSetInt(imageParams,MV_IMAGE_STRIDE,0);
  printf( "%d bytes per image.\n",dmImageFrameSize(imageParams));
  dmParamsDestroy( imageParams );

  // Get from/to playing frames (image track)
  if(from==-1)from=0;
  if(to==-1)to=trkImage->GetLength();

  time=50;
  //timeScale=0;
  timeScale=mvGetTrackTimeScale(trkImage->GetMVid());
  qdbg("  DisplayWidth: %d\n",mvGetTrackDisplayWidth(trkImage->GetMVid()));
  qdbg("  DisplayHeight: %d\n",mvGetTrackDisplayHeight(trkImage->GetMVid()));
  time=50*timeScale;
  time=50;
  timeSlice=10;		// 50Hz display, timescale 250
  //bmVideo=new QBitMap(32,imgWid+100,imgHgt+100);

  // Setup audio
  if(!mute)
  { ap=new QAudioPort();
    audioBuf=(char*)1;
    if(!ap->Open())
    {noaud:
      delete ap;
      if(audioBuf==(char*)1)
        qdbg("Audioport can't open\n");
      else
        qdbg("No memory for audio buffer\n");
      return;
    }
    audioBuf=(char*)qcalloc(audioBufSize);
    if(!audioBuf){ goto noaud; }
  }

//qdbg("QMovie:Play EARLY RET\n"); return;

  qdbg("timeScale: %d\n",timeScale);
#define ND_OGL_WORKS
#ifdef ND_OGL_WORKS
  //r.left=100; r.bottom=100;
  //r.right=300; r.top=400;
  //r.right=r.left+imgWid/2;
  //r.top=r.bottom+imgHgt/2;
  r.left=0; r.bottom=0;
  r.right=imgWid; r.top=imgHgt;
  //mvSetMovieRect(mvid,r);
  //for(time=0;time<timeScale*10;time+=timeSlice)
  qdbg("QMovie:Play: playing from %d to %d (image frames)\n",from,to);
  app->GetBC()->GetCanvas()->Select();

//qdbg("QMovie:Play EARLY RET\n"); return;
  for(curFrame=from;curFrame<=to;curFrame++)
  { 
    time=curFrame*timeSlice;
    //qdbg("time=%lld\n",time);
    status=mvRenderMovieToOpenGL(mvid,time,timeScale);
    if(status==DM_FAILURE)
    { qdbg("mvRMTOGL: status=DM_FAILURE\n");
    }
    if(!mute)
    { status=mvRenderMovieToAudioBuffer(mvid,time,timeSlice,timeScale,
        MV_FORWARD,audioBufSize,&audioFilled,&framesFilled,
        audioBuf,0);
      if(status==DM_FAILURE)
      { qdbg("mvRMTOAB: status=DM_FAILURE (audio)\n");
        break;
      }
      //qdbg("audio: audioFilled=%d, framesFilled=%d\n",audioFilled,framesFilled);
      ap->WriteSamps(audioBuf,framesFilled*2);
    }
    app->GetBC()->Swap();
    //char buf[10];
    //printf("Press enter\n"); scanf("%s",buf);
  }
  if(ap)delete ap;
  if(audioBuf)qfree(audioBuf);
  if(bmVideo)delete bmVideo;
  glPixelZoom(1,1);	// Video usually reverses blits (1,-1)
#endif
#ifdef ND_IMAGEBUFFER
  qdbg("mvRMTOIB\n");
  status=mvRenderMovieToImageBuffer(mvid,time,timeScale,
    bmVideo->GetBuffer(),imageParams);
  if(status==DM_FAILURE)
  { qdbg("mvRMTOIB: status=DM_FAILURE\n");
  }
#endif

#ifdef INDY_COSMO
  // Cosmo JPEG decompression using streaming
  size_t cis;
  int  i,r;
  int  frame,imgWid,imgHgt;
  int  free,wrap,newFree;
  void *pFree;
  int lastImageCount;

  int frames,framesAudio;
  int fcs;		// Frame compressed size
  // Temp buffer for splicing data and audio reading
  char *tempBuffer;
  int   tempBufferSize;

  QDecompressor *dc;
  QAudioPort *ap; 			// Audio track, if present
  int   lastAudioFrame,nextAudioFrame;	// Audio clip for video frame
  bool  mute;

  printf("QMovie::Play()\n");
  if(!trkAudio->Exists())mute=TRUE;
  else                   mute=FALSE;

  //printf("Compr. scheme: %s\n",mvGetImageCompression(mvidImage));
  frames=trkImage->GetLength();
  framesAudio=trkAudio->GetLength();
  printf("Frames: %d\n",frames);

  imgWid=trkImage->GetImageWidth();
  imgHgt=trkImage->GetImageHeight();
  imgHgt/=2;				// Interlaced (PAL=odd)
  dc=new QDecompressor(CL_JPEG_COSMO);

  cis=GetMaxImageSize();
  printf("max image (cmpr) size=%d\n",cis);

  // Create a DATA buffer to hold some (compressed) frames
  dc->CreateDataBuffer(cis*10);
  dc->SetImageSize(imgWid,imgHgt);
  dc->SetInternalImageSize(imgWid,imgHgt);
  dc->SetTransferMode(QCTM_AUTO_2_FIELD);

  // Audio init
  ap=new QAudioPort();		// Default 44.1kHz, 16-bit stereo
  // Use dmAudioFrameSize(params) to get bytes per frame
  tempBufferSize=framesAudio/frames+1;
  tempBufferSize*=4;			// Bytes per sample frame
  ap->SetQueueSize((tempBufferSize/4)*30);	// Allow for precaching
  if(!ap->Open())
  { mute=1;
    printf("QMovie:: can't open audioport\n");
  }
  lastAudioFrame=0;		// Start of frame

  // Temporary buffer for wrapping data
  if(cis>tempBufferSize)
    tempBufferSize=cis;
  tempBuffer=(char*)qcalloc(tempBufferSize);
  if(!tempBuffer)
  { // err
    printf("QMovie::Play: not enough memory for temp buf\n");
    return;
  }

  //free=dc->QueryFree(&pFree,&wrap);
  dc->Start();

  //voutdrn->Freeze(FALSE);
  for(frame=0;frame<frames;frame++)
  {
    // Read audio part (BEFORE video)
    if(frame<frames-PLAY_AUDIO_ADVANCE+1)
    { nextAudioFrame=trkImage->MapToTrack(trkAudio,frame+PLAY_AUDIO_ADVANCE);
      printf("  filled audio=%d, fillable=%d, want to fill %d\n",
         ap->GetFilled(),ap->GetFillable(),nextAudioFrame-lastAudioFrame);
      //printf("audio: last=%d, next=%d\n",lastAudioFrame,nextAudioFrame);
      if(nextAudioFrame>framesAudio)
      { // Should never happen
        printf("QMovie::Play: audio read overflow\n");
        nextAudioFrame=framesAudio;
      }
      trkAudio->ReadFrames(lastAudioFrame,nextAudioFrame-lastAudioFrame,
        tempBuffer,tempBufferSize);
      /*printf("read audio frames %d (count %d)\n",frame*audioBufferSize,
        audioBufferSize);*/
      // WriteSamps; write samples times channels
      ap->WriteSamps(tempBuffer,(nextAudioFrame-lastAudioFrame)*2);
      /*printf("  filled audio=%d, fillable=%d\n",
         ap->GetFilled(),ap->GetFillable());*/
      lastAudioFrame=nextAudioFrame;
    }

    fcs=trkImage->GetCompressedImageSize(frame);
   retry_frame:
    free=dc->QueryFree(&pFree,&wrap,fcs);
    /*printf("clQueryFree frame=%d: %d, wrap=%d, total=%d\n",
      frame,free,wrap,free+wrap);*/
    // Check for enough space to place compressed frame
    if(free>fcs)
    { // Read into actual buffer
      //printf("  direct read of fcs=%d\n",fcs);
      trkImage->ReadCompressedImage(frame,pFree,fcs);
      dc->UpdateHead(fcs);
    } else if(free+wrap>fcs)
    { // Read in temp buffer
      //printf("  splice %d in %d/%d\n",fcs,free,fcs-free);
      trkImage->ReadCompressedImage(frame,tempBuffer,fcs);

      // Copy first chunk upto end of buffer
      bcopy(tempBuffer,pFree,free);
      dc->UpdateHead(free);

      // Copy last chunk into start of buffer
      newFree=dc->QueryFree(&pFree,&wrap);
      if(newFree>fcs-free)
      { bcopy(tempBuffer+free,pFree,fcs-free);
        dc->UpdateHead(fcs-free);
      } else printf("not enough space to fill start of buffer\n");
    } else
    { // Wait for a moment, then try again
      sginap(1);
      goto retry_frame;
    }
#ifdef DO_BLIT
    // OpenGL bitmap ops
    glDrawPixels(imgWid,imgHgt,GL_ABGR_EXT,GL_UNSIGNED_BYTE,(void*)frameBuffer);
    gw->Swap();
#endif
  }
  // Wait until the video is done playing
  //printf("wait for video to end\n");
  CLimageInfo imageInfo;
  int waitCount=0;
  lastImageCount=0;
  while(1)
  { if(clGetNextImageInfo(dc->GetCprHandle(),&imageInfo,sizeof(imageInfo))<0)
      break;
    else if(imageInfo.imagecount==lastImageCount)
      break;
    else
    { lastImageCount=imageInfo.imagecount;
      sginap(1);
      //printf("  waiting...\n");
      waitCount++;
    } 
  }
  printf("  waited %d times\n",waitCount);
  // Wait for audio to drain out
  if(!mute)
    while(ap->GetFilled()>0)sginap(1);
  // Freeze video output, if any
  //voutdrn->Freeze(TRUE);

 do_not:
  delete ap;
  delete dc;
#endif
// End COSMO JPEG

}
#endif


#ifdef NOTDEF_SINGLE_FRAME_METHOD
// Works, but slow and not stream-based
void QMovie::Play()
{
  size_t cis;
  MVid mvidImage,mvidAudio;
  int  i,r;
  int  frame,imgWid,imgHgt;
  int  engine;
  CLcompressorHdl hdl;
  int p[50],n;
  int frames,framesAudio;
  void *frameData;
  int fcs;		// Frame compressed size
  int *frameBuffer,fbs;

  printf("QMovie::Play()\n");
  if(mvFindTrackByMedium(mvid,DM_IMAGE,&mvidImage)!=DM_SUCCESS)
  { printf("Can't find image track\n");
  }
  if(mvFindTrackByMedium(mvid,DM_AUDIO,&mvidAudio)!=DM_SUCCESS)
  { printf("Can't find audio track\n");
  }
  for(i=0;i<10;i++)
  { cis=mvGetCompressedImageSize(mvidImage,i);
    printf("  compressed image frame %d=%d\n",i,cis);
  }
  printf("Compr. scheme: %s\n",mvGetImageCompression(mvidImage));
  frames=mvGetTrackLength(mvidImage);
  framesAudio=mvGetTrackLength(mvidImage);
  printf("Frames: video %d, audio %d\n",frames,framesAudio);
  imgWid=mvGetImageWidth(mvidImage);
  imgHgt=mvGetImageHeight(mvidImage);
  imgHgt/=2;				// Interlaced
  engine=CL_JPEG_COSMO;
  r=clOpenDecompressor(engine,&hdl);
  n=0;
  p[n++]=CL_IMAGE_WIDTH;
  p[n++]=imgWid;
  p[n++]=CL_IMAGE_HEIGHT;
  p[n++]=imgHgt;
  p[n++]=CL_INTERNAL_IMAGE_WIDTH;
  p[n++]=imgWid;
  p[n++]=CL_INTERNAL_IMAGE_HEIGHT;
  p[n++]=imgHgt;
  p[n++]=CL_ORIGINAL_FORMAT;		// Decompress format
  p[n++]=CL_RGBX;			// The only poss. for Cosmo Compress
  p[n++]=CL_COSMO_VIDEO_TRANSFER_MODE;
  //p[n++]=CL_COSMO_VIDEO_TRANSFER_MANUAL;
  p[n++]=CL_COSMO_VIDEO_TRANSFER_AUTO_2_FIELD;
  //p[n++]=CL_COSMO_VIDEO_TRANSFER_AUTO_1_FIELD;
  printf("r=%d\n",r);
  r=clSetParams(hdl,p,n);
  printf("r=%d (clSetParams)\n",r);
  frame=0;
  printf("  read frame %d\n",frame);
  cis=mvGetCompressedImageSize(mvidImage,frame);
  cis=cis*2;
  frameData=qcalloc(cis);
  if(!frameData)goto do_not;
  fbs=imgWid*imgHgt*sizeof(int);
  QBitMap *bm;
  bm=new QBitMap(32,imgWid,imgHgt);	// /2=interlace
  //frameBuffer=(int*)qcalloc(fbs);
  frameBuffer=(int*)bm->GetBuffer();
  //for(i=0;i<fbs/sizeof(int);i++)frameBuffer[i]=255;
  if(!frameBuffer){ printf("No framebuffer\n"); goto do_not; } 
  glPixelZoom(1,-1);
  //glDisable(GL_BLEND);
  //glDisable(GL_DITHER);
  gc->Disable(QGCF_BLEND);
  gw->Select();
  gc->Blit(bm,0,-imgHgt); //imgHgt/2);
  for(frame=0;frame<frames;frame++)
 {
  fcs=mvGetCompressedImageSize(mvidImage,frame);
  mvReadCompressedImage(mvidImage,frame,fcs,frameData);
  clDecompress(hdl,1,cis,frameData,CL_EXTERNAL_DEVICE);
  // The next one currently generates a PANIC Kernel fault
  //clDecompress(hdl,CL_CONTINUOUS_NONBLOCK,cis,frameData,CL_EXTERNAL_DEVICE);
  //r=clDecompress(hdl,1,fcs,frameData,frameBuffer);
  // Dump it anywhere
  // printf("gw select\n");
  //printf("gl drawpixels\n");
  //QColor *col=new QColor(0xFFFFFF00);
  //bm->CreateTransparency(col);
  //gc->Blit(bm);
  //gc->Blit(bm);
#ifdef DO_BLIT
  // OpenGL bitmap ops
  glDrawPixels(imgWid,imgHgt,GL_ABGR_EXT,GL_UNSIGNED_BYTE,(void*)frameBuffer);
  gw->Swap();
#endif
  //printf("r=%d (clDecompress)\n",r);
 }
  glPixelZoom(1,1);
  //mvPlay(mvid);
 do_not:
  printf("close decmpr\n");
  clCloseDecompressor(hdl);
}
#endif

/***********
* PLAYBACK *
***********/
bool QMovie::IsPlaying()
{
  if(playing)return TRUE;
  return FALSE;
}
void QMovie::SetPlayMode(int mode)
{
  playMode=mode;
}
void QMovie::SetFirstFrame(int frame)
{
  firstFrame=frame;
}
/*void QMovie::BindAudioPort(QAudioPort *ap)
{
  apPlay=ap;
}*/

//
// Seeking
//
bool QMovie::Goto(int n)
// Go to a certain frame number
{ return SetCurFrame(n);
}
void QMovie::GotoFirst()
// Go to first video frame
{
  Goto(0);
}
void QMovie::GotoLast()
{ if(GetImageTrack())
    Goto(GetImageTrack()->GetLength()-1);
}

void QMovie::Advance()
// Proceed to the next frame, based on speed etc.
{
  //qdbg("QMovie:Advance\n");
  //qdbg("  curFrame=%d, %d-%d\n",curFrame,firstFrame,lastFrame);
  if(!IsPlaying())return;
  if(++curFrame>lastFrame)
  { // Loop?
    //qdbg("  advance at end\n");
    //if(flags&LOOP)
    if(playMode==LOOP)
    { curFrame=firstFrame;
    } else
    { curFrame=lastFrame;		// Hang on last frame
      Stop();
      //qdbg("  movie stopped; playing=%d\n",playing);
    }
  }
}
void QMovie::Play(int from,int to)
{ int imgLen;
  if(!trkImage)
  { qerr("QMovie:Play without image track"); return; }
  
  //qdbg("QMovie::Play(from %d,to %d)\n",from,to);
  // Get from/to playing frames (image track)
  if(from==-1)from=0;
  if(to==-1)
  { to=trkImage->GetLength()-1;
    if(to<0)to=0;
  }
  //qdbg("  to=%d\n",trkImage->GetLength());
  // Check validity; playing outside actual existing frame ranges
  // leads to serious hardware lockups
  imgLen=trkImage->GetLength();
  if(from<0)from=0;
  if(from>imgLen-1)from=imgLen-1;
  if(to<from)to=from;
  if(to>imgLen-1)to=imgLen-1;
  firstFrame=from;
  lastFrame=to;
  curFrame=firstFrame;
  //qdbg("  lastFrame=%d\n",lastFrame);
  playing=TRUE;
}
void QMovie::Stop()
{ if(!IsPlaying())return;
  playing=FALSE;
}
int QMovie::GetCurFrame()
{ return curFrame;
}
bool QMovie::SetCurFrame(int n)
// Sets current frame, but doesn't render anything or such
// May be placed outside of current play range
// Returns FALSE if frame number was out of range
{ bool r=TRUE;
  if(n<0)
  { n=0;
    r=FALSE;
  } else if(n>trkImage->GetLength()-1)
  { n=trkImage->GetLength()-1;
    r=FALSE;
  }
  curFrame=n;
  return r;
}
void QMovie::WaitForEnd()
// Wait until movie is at the last frame
// Reasonably threadfriendly
{
  while(playing)
  { QNap(5);              // 1 PAL frame ~ 20ms
    qdbg("QMovie:WaitForEnd; cur=%4d, last=%4d\r",curFrame,lastFrame);
  }
  qdbg("\n");
#ifdef OBS_LOOP_PERHAPS
  while(playing)
  { if(curFrame!=lastFrame)
    { QNap(5);              // 1 PAL frame ~ 20ms
      qdbg("QMovie:WaitForEnd; cur=%d, last=%d\n",curFrame,lastFrame);
    } else
    { qdbg("curFrame == lastFrame, playing=%d\n",playing);
      break;
    }
  }
#endif
}

/************
* RENDERING *
************/
static char *FindSecondField(char *buf,int size)
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

  //qdbg("QMovie:FindSecondField(): can't separate two fields of the frame");
  return 0;
}
void SizeBuffer(char **imageBuffer,int *curSize,int newSize)
// Out-resize the buffer to be able to contain 'newSize' bytes.
{ char *p;
  p=*imageBuffer;
  if(p==0)
  {do_alloc:
    // Free old
    if(p)
      qfree(p);
    p=(char*)qcalloc(newSize);
    *imageBuffer=p;
    *curSize=newSize;
    return;
  }
  // Check size to be enough
  if(newSize>*curSize)goto do_alloc;
}

void QMovie::RenderToDMB(DMbuffer dmb1,DMbuffer dmb2)
// Render current frame in 2 fields (or, if dmb2 is NULL, only field1 or the
// entire frame) to the DM buffers.
// The rendered data may be COMPRESSED (JPEG etc.) and should
// be sent to dmIC for decompression.
{
  MVid mvidImg;
  int timeScale;
  int index;                // Track index (frame# != trackindex)
  int frameOffset;          // Offset from beginning of data chunk
  size_t fieldSize[2];      // Size of field data
  void *mem[2];             // DMbuffer actual data pointer
  off64_t gap;              // Gap between 1st and 2nd field
  int     r;
  
  static char *imageBuffer; // One buffer for all movies in existance
  static int imageBufferSize;
  
  mvidImg=trkImage->GetMVid();
  
  // PAL hardcodes
  timeScale=25;         // Frames/sec
  frameRate=25.0;
  
    // Map frame# to data index (movies can have repeating frame data)
    // If index==-1, it is an empty edit (see mvInsertTrackGapAtTime)
    // 'frameOffset' is not used (offset from the beginning of the chunk)
    if (mvGetTrackDataIndexAtTime(mvidImg,
         (MVtime)(((double)curFrame) * timeScale / frameRate),
         timeScale, &index, &frameOffset) != DM_SUCCESS) {
        qerr("mvGetTrackDataIndexAtTime failed");
    }
    //printf("frame: %d, data index=%d\n",frame,index);

    // Not all frames have field info in all movies
    // If no field info is present, we must find
    // the field break ourselves
    if(mvTrackDataHasFieldInfo(mvidImg,index)!=DM_TRUE)
    { //qerr("No field info for image track");
      // Get frame (=2 fields) and search field 2 by hand (bleh!)
      // From dmplay
      size_t  size;
      int     f1size,f2size;
      char    *f1,*f2;
      int     paramsId, numFrames;
      MVdatatype      dataType;

      if(mvGetTrackDataInfo(mvidImg,index,&size,&paramsId,
        &dataType, &numFrames )!=DM_SUCCESS)
      {
        qwarn("mvGetTrackDataInfo failed\n");
        return;
      }
      //qdbg("mvReadTrackData\n");
      SizeBuffer(&imageBuffer,&imageBufferSize,size);
      if(mvReadTrackData(mvidImg,index,size,imageBuffer)!=DM_SUCCESS)
      {
        qwarn("mvReadTrackData failed\n");
        return;
      }
      if(dmb2==0)
      { // No 2nd field!
        bcopy(imageBuffer,dmBufferMapData(dmb1),size);
        dmBufferSetSize(dmb1,size);
        return;
      }
      
      // Find 2nd field separator by hand
      f1 = imageBuffer;
      f2 = FindSecondField(f1, size);
      if(f2)
      { f1size = f2 - f1;
        f2size = size - f1size;
      } else
      { // No 2nd field found
        f1size=size;
        f2size=0;
      }

      bcopy(f1, dmBufferMapData(dmb1), f1size);
      dmBufferSetSize(dmb1, f1size);
      //SendDMbuffer(dmb1);

      bcopy(f2, dmBufferMapData(dmb2), f2size);
      dmBufferSetSize(dmb2, f2size);
      //SendDMbuffer(dmb2);

      return;
    }
    //mvGetTrackDataInfo(mvidImg,index,&frameSize,&parmID,&dt,&frames);
    //qdbg("frame %d: size=%d, parmID=%d, frames=%d\n",frame,frameSize,parmID,frames);
    mvGetTrackDataFieldInfo(mvidImg,index,&fieldSize[0],&gap,
      &fieldSize[1]);
    //qdbg("frame %d: size=%d & %d; in=%d, out=%d\n",
      //frame,fieldSize[0],fieldSize[1],inBufSize,outBufSize);
    mem[0]=dmBufferMapData(dmb1);
    mem[1]=dmBufferMapData(dmb2);
    //qdbg("> Read image data\n");
    //mvReadTrackData(mvid,frame,100000,mem);
    int inBufSize,outBufSize;
    inBufSize=outBufSize=1000000;       // Assume enough
    if(mvReadTrackDataFields(mvidImg,index,inBufSize,mem[0],outBufSize,mem[1])
       !=DM_SUCCESS)
    { r=mvGetErrno();
      qdbg("** Can't mvReadTrackDataFields; %d = %s\n",r,
        mvGetErrorStr(r));
    }
    dmBufferSetSize(dmb1,fieldSize[0]);
    dmBufferSetSize(dmb2,fieldSize[1]);
}
void QMovie::RenderToAP(QAudioPort *ap,int curFrame)
// Render this frame's audio to the audioport
{
  int lastAudioFrame,nextAudioFrame;
  static char tempBuffer[16000];
  static int tempBufferSize=16000;
  int frames,framesAudio;
  int dist,i;
  bool mute=FALSE;

  // Audio; check first if any
  if(mute)goto no_audio;
  if(!trkAudio->Exists())return;
#ifdef OBS
  // Mute audio when still
  if(!IsPlaying())
  { goto do_silence;
    //return;
  }
#endif

  frames=trkImage->GetLength();
  framesAudio=trkAudio->GetLength();
  //qdbg("QMovie:RenderToAP: audio trackID=%p\n",trkAudio->GetMVid());
  //if(curFrame<frames-PLAY_AUDIO_ADVANCE-1)
  if(curFrame<frames)
  { 
#ifdef ND_SMART
    if(curFrame>0)
      lastAudioFrame=trkImage->MapToTrack(trkAudio,curFrame+PLAY_AUDIO_ADVANCE+1-1);
    else
      lastAudioFrame=0;
    nextAudioFrame=trkImage->MapToTrack(trkAudio,curFrame+PLAY_AUDIO_ADVANCE+1);
    if(nextAudioFrame<=lastAudioFrame)
    { lastAudioFrame=nextAudioFrame;
      goto no_audio;
    }
#endif
    lastAudioFrame=trkImage->MapToTrack(trkAudio,curFrame);
    nextAudioFrame=trkImage->MapToTrack(trkAudio,curFrame+1);
#ifdef ND_NO_REPORT
qdbg("  curVidFrame=%d, lastAF=%d, nextAF=%d; total=%d\n",
      curFrame,lastAudioFrame,nextAudioFrame,framesAudio);
qdbg("  filled audio=%d, fillable=%d, want to fill %d\n",
      ap->GetFilled(),ap->GetFillable(),nextAudioFrame-lastAudioFrame);
qdbg("audio: last=%d, next=%d\n",lastAudioFrame,nextAudioFrame);
#endif
//qdbg("filled audio=%d\n",ap->GetFilled());
    // Report underruns
    if(ap->GetFilled()==0)
      qdbg("** Audio underrun frame %d\n",curFrame);

    if(nextAudioFrame>framesAudio)
    { // Should never happen
      printf("QMovie::Play: audio read overflow\n");
      nextAudioFrame=framesAudio;
    }
    dist=nextAudioFrame-lastAudioFrame;
    if(dist<MAGIC_SMPS_PER_FRAME)
    {
      // Make up your own sound
     do_silence:
//qdbg("** Filling space (silence)\n");
      dist=MAGIC_SMPS_PER_FRAME;
      for(i=0;i<dist*4;i++)		// 16-bit emptyness
      { tempBuffer[i]=0;
      }
    } else
    { // Read from file
      trkAudio->ReadFrames(lastAudioFrame,nextAudioFrame-lastAudioFrame,
        tempBuffer,tempBufferSize);
      /*printf("read audio frames %d (count %d)\n",frame*audioBufferSize,
        audioBufferSize);*/
    }
    // WriteSamps; write samples times channels
    //qdbg("write samples\n");
    ap->WriteSamps(tempBuffer,dist*2);
    /*printf("  filled audio=%d, fillable=%d\n",
       ap->GetFilled(),ap->GetFillable());*/
    lastAudioFrame=nextAudioFrame;
  }
  // Otherwise no audio for this frame (outside of video frame range)
  //else qerr("QMovie::RenderToAP(); no audio done");
 no_audio:
  return;
}
void QMovie::RenderToAP(QAudioPort *ap)
// Render audio to the audioport
// Advances a certain number of frames to keep audio buffer filled,
// to try to keep audio filled at all times. Otherwise we'd have to
// detect audio underruns, and fill the size of the underrun as silence
// to keep sync.
{
  int lastAudioFrame,nextAudioFrame;
  static char tempBuffer[16000];
  static int tempBufferSize=16000;
  int frames,framesAudio;
  int dist,i;
  bool mute=FALSE;
  //int framesBuffered=8;
  int framesBuffered=1;

  frames=trkImage->GetLength();

  if(curFrame==firstFrame)
  {
//qdbg("QMovie: first frame\n");
    for(i=0;i<framesBuffered;i++)
    {
      RenderToAP(ap,firstFrame+i /*+framesBuffered*/);
    }
  } else
  {
//qdbg("QMovie: middle frame\n");
    // Send oncoming audio to audio buffer
    RenderToAP(ap,curFrame+framesBuffered);
  }
}

void QMovie::RenderToBitMap(QBitMap *bm)
// Render current frame to a Q bitmap (often used to get stills)
{
  // Catch last frame and store in bitmap for bob restores
  char *mem;
  DMparams *p;
  proftime_t t;
  MVrect rect;
  int swid,shgt;
  
  mem=bm->GetBuffer();
  swid=bm->GetWidth();
  shgt=bm->GetHeight();
  
  dmParamsCreate(&p);
  dmParamsSetInt(p,DM_IMAGE_WIDTH,swid);
  dmParamsSetInt(p,DM_IMAGE_HEIGHT,shgt);
  // Destination colorspace; YUV or RGBX
  //dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_CbYCrY);
  dmParamsSetInt(p,DM_IMAGE_PACKING,DM_IMAGE_PACKING_RGBX);
  // Flip picture
  //dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  dmParamsSetInt(p,DM_IMAGE_ORIENTATION,DM_IMAGE_BOTTOM_TO_TOP);
  //dmParamsSetInt(p,DM_IMAGE_INTERLACING,DM_IMAGE_INTERLACED_EVEN);	// PAL
  dmParamsSetInt(p,MV_IMAGE_STRIDE,0);

  if(mvRenderMovieToImageBuffer(mvid,curFrame,25,mem,p)
     !=DM_SUCCESS)
  { qerr("QMovieBob:CreateStill(); failed to create still; %s\n",
      mvGetErrorStr(mvGetErrno()));
  }
}

void QMovie::RenderToOpenGL(int x,int y,int alpha)
// Render current frame to the current OpenGL context.
// (this is used by libdmu to get graphics layout frames into
//  a pbuffer)
// Should only be used really in class functions, since it is too direct
// (glDisable(GL_BLEND) or such may be necessary)
// 'x' and 'y' give the position to draw (0,0 is the default)
// Note that 'y' is currently in OpenGL coordinates (reversed wrt QCanvas)!
// 'alpha' gives an option to transparency drawing (FUTURE option)
{
  DBG_C("RenderToOpenGL")
  DBG_ARG_I(alpha)

  int timeScale;
  MVtime time;
  MVrect rect;
  
  glDisable(GL_BLEND);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glRasterPos2i(x,y);
  rect.bottom=y; rect.left=x;
  rect.top=y+GetImageTrack()->GetImageHeight();
  rect.right=x+GetImageTrack()->GetImageWidth();
  mvSetMovieRect(mvid,rect);
  
  // PAL hardcodes
  timeScale=25;         // Frames/sec
  time=curFrame;
  if(mvRenderMovieToOpenGL(mvid,time,timeScale)==DM_FAILURE)
  { qerr("QMovie::RenderToOpenGL() failed; %s",
      mvGetErrorStr(mvGetErrno()));
    mvClearErrno();
  }
  // The above function modifies glPixelZoom() (for one) and this led
  // to a high number of 'black screen' effects. So restore it.
  glPixelZoom(1,1);
}

#endif
