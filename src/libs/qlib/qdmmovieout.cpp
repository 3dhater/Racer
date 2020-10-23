/*
 * QDMMovieOut - create a movie of DMbuffers
 * 10-09-98: Created!
 * NOTES:
 * - To be used as a node in a stream of QDMObjects
 * FUTURE:
 * - Use QMovie class?
 * (C) MG/RVG
 */

#include <qlib/dmmovieout.h>
#include <qlib/dmparams.h>
#include <dmedia/moviefile.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define EMV(s,f) if((f)==DM_FAILURE)MovieError(s)

static void MovieError(string s)
{
  qerr("%s: MV error (%s)",s,mvGetErrorStr(mvGetErrno()));
}

QDMMovieOut::QDMMovieOut(cstring fName,int w,int h)
// Prepare movie output
// 'fName' may be 0 (w and h may also be 0) if you don't know it yet
{
  //DEBUG_C(qdbg("QDMMovieOut(%s,%dx%d) ctor\n",fileName,w,h));
  movie=0;
  if(fName)fileName=fName;
  wid=w;
  hgt=h;
  cpix=0;
  
  mid=0;
  tid=0;
  aid=0;
  //DEBUG_C(qdbg("QDMMovieOut ctor RET\n"));
}
QDMMovieOut::~QDMMovieOut()
{
  Close();
}

void QDMMovieOut::SetName(cstring fname)
// Modify the filename (only before Create())
{
  if(IsOpen())
  { qwarn("QDMMovieOut::SetName(); shouldn't set filename while open");
  }
  fileName=fname;
}
void QDMMovieOut::SetSize(int w,int h)
// Modify the frame size (only before Create())
// Pass the FRAME size, not the FIELD size
{
  if(IsOpen())
  { qwarn("QDMMovieOut::SetName(); shouldn't set movie size while open");
  }
  wid=w;
  hgt=h;
}

bool QDMMovieOut::IsOpen()
// Returns TRUE if movie file is open and active
// An unclosed movie will NOT play back so make sure you close it
{
  if(mid)return TRUE;
  return FALSE;
}

bool QDMMovieOut::Create(bool addAudio)
// Create the movie (open the file and set the track parameters)
// If 'addAudio'==TRUE, an audio track is created as well
{
  // Create movie
  QDMParams *p;
  p=new QDMParams();
  EMV("QDMMovieOut::Create()",mvSetMovieDefaults(p->GetDMparams(),MV_FORMAT_QT));
  EMV("QDMMovieOut::Create()",mvCreateFile(fileName,p->GetDMparams(),0,&mid));
  delete p;

  // Create image track
  p=new QDMParams();
  rate=25.0;
  p->SetImageDefaults(wid,hgt,DM_PACKING_RGBX);
  p->SetString(DM_IMAGE_COMPRESSION,DM_IMAGE_JPEG);
  p->SetFloat(DM_IMAGE_QUALITY_SPATIAL,.9);
  p->SetFloat(DM_IMAGE_RATE,rate);
  p->SetEnum(DM_IMAGE_ORIENTATION,DM_TOP_TO_BOTTOM);
  // Autodetect interlacing
  if(hgt<480)p->SetEnum(DM_IMAGE_INTERLACING,DM_IMAGE_NONINTERLACED);
  else       p->SetEnum(DM_IMAGE_INTERLACING,DM_IMAGE_INTERLACED_EVEN);
  EMV("QDMMovieOut/add imgtrack",mvAddTrack(mid,DM_IMAGE,p->GetDMparams(),0,&tid));
  delete p;

  if(addAudio)
  { p=new QDMParams();
    p->SetAudioDefaults(16,44100,2);
    EMV("QDMMovieOut/add audiotrack",
      mvAddTrack(mid,DM_AUDIO,p->GetDMparams(),0,&aid));
    delete p;
  }
  
  // Get information
  tscale=mvGetTrackTimeScale(tid);
  dur=(long long)((double)tscale/rate);
  return TRUE;
}
void QDMMovieOut::Close()
{ if(mid)
  { mvClose(mid);
    mid=0;
  }
}

void QDMMovieOut::AddProducerParams(QDMBPool *p)
{}
void QDMMovieOut::AddConsumerParams(QDMBPool *p)
{}
void QDMMovieOut::RegisterPool(QDMBPool *pool)
{}

int QDMMovieOut::GetFD()
{
  return -1;
}

void QDMMovieOut::Receive(DMbuffer field1,DMbuffer field2,
  short *audioBuf,int audioFrames)
// Receive and write to the movie the DMbuffers
// 'field2' may be 0 (in case you're writing full frames)
// Audio is optional. If 'audioBuf'==0, no audio is written
{
  //DEBUG_F(qdbg("QDMMovieOut::Receive() 2 fields\n"));
  MVtime mvtime=(long long)(cpix/2)*dur;
  cpix++;
  cpix++;
  if(field2==0)
  { // Full frame
    //DEBUG_F(qdbg("  mvInsertTrackData\n"));
    EMV("QDMMovieOut::Receive()",mvInsertTrackData(tid,1,mvtime,dur,tscale,
      dmBufferGetSize(field1),dmBufferMapData(field1),
      MV_FRAMETYPE_KEY,0));
  } else
  { //DEBUG_F(qdbg("  mvInsertTrackDataFields\n"));
    EMV("QDMMovieOut::Receive()",mvInsertTrackDataFields(tid,mvtime,dur,tscale,
      dmBufferGetSize(field1),dmBufferMapData(field1),
      dmBufferGetSize(field2),dmBufferMapData(field2),
      MV_FRAMETYPE_KEY,0));
  }
  
  // Audio
  if(audioBuf!=0&&audioFrames>0)
  {
    EMV("QDMMovieOut::Receive() audio",
      mvInsertTrackData(aid,audioFrames,mvtime,dur,tscale,
        audioFrames*2*2,(void*)audioBuf,MV_FRAMETYPE_KEY,0));
  }
}

