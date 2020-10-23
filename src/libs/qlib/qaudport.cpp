/*
 * QAudPort - SGI audio port encapsulation (using AL)
 * 01-04-97: Created!
 * 11-10-99: AL2.0 names used in some places.
 * (c) MarketGraph/RVG
 */

#include <stdio.h>
#include <stdlib.h>
#include <qlib/audport.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Shortcut
//#define IAP	internal->audioPort
#define IAP	audioPort

//#define QAP_DEFAULT_QUEUESIZE	32768

#undef  DBG_CLASS
#define DBG_CLASS "QAudioPort"

QAudioPort::QAudioPort()
{
  DBG_C("ctor")

  //internal=new QAPInternal();
  //internal->audioConfig=ALnewconfig();
  //internal->audioPort=0;
#if defined(__sgi)
  audioConfig=ALnewconfig();
#endif
  audioPort=0;
  buffer=0;
  bufferSize=0;
  // Default settings
  //SetWidth(AL_SAMPLE_16);
  SetWidth(WIDTH_16_BIT);
  SetChannels(2);
  SetQueueSize(DEFAULT_QUEUESIZE);
}

QAudioPort::~QAudioPort()
{
  DBG_C("dtor")

  Close();
  //if(internal)delete internal;
  FreeBuffer();
}

// Pre-open functionality
int QAudioPort::SetWidth(int smpWidth)
{
#if defined(__sgi)
  int r;
  r=ALsetwidth(audioConfig,smpWidth);
  return r;
#else
  return 0;
#endif
}
int QAudioPort::SetChannels(int nChannels)
{
#if defined(WIN32) || defined(linux)
  return 0;
#else
  int r;
  r=ALsetchannels(audioConfig,nChannels);
  return r;
#endif
}
int QAudioPort::SetQueueSize(int queueSize)
{
#if defined(WIN32) || defined(linux)
  return 0;
#else
  int r;
  r=ALsetqueuesize(audioConfig,queueSize);
  return r;
#endif
}
int QAudioPort::GetQueueSize()
{
#if defined(WIN32) || defined(linux)
  return 0;
#else
  return (int)ALgetqueuesize(audioConfig);
#endif
}

bool QAudioPort::Open(bool write)
{
  DBG_C("Open")
  DBG_ARG_B(write);
  // Already open?
  if(audioPort)return TRUE;

#if defined(WIN32) || defined(linux)
  return FALSE;
#else
  // Open port
  audioPort=ALopenport("qaudioport",
    write?"w":"r",audioConfig);
  if(audioPort)
    return TRUE;
  return FALSE;
#endif
}
void QAudioPort::Close()
{
  if(!audioPort)return;
#if defined(WIN32) || defined(linux)
#else
  ALcloseport(audioPort);
#endif
  audioPort=0;
}

int QAudioPort::GetFD()
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  return ALgetfd(IAP);
#endif
}
int QAudioPort::GetFillable()
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  return (int)ALgetfillable(IAP);
#endif
}
int QAudioPort::GetFilled()
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  return (int)ALgetfilled(IAP);
#endif
}
int QAudioPort::SetFillPoint(int n)
// Sets point at which underflow occurs (select() returns)
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  return ALsetfillpoint(audioPort,n);
#endif
}

/******
* I/O *
******/
int QAudioPort::WriteSamps(void *p,int count)
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  int r;
  r=ALwritesamps(IAP,p,count);
  return r;
#endif
}
int QAudioPort::ReadSamps(void *p,int count)
{
  if(!IAP)return 0;
#if defined(WIN32) || defined(linux)
  return 0;
#else
  int r;
  r=ALreadsamps(IAP,p,count);
  return r;
#endif
}

void QAudioPort::ZeroFrames(int count)
// Write silence directly; more efficient
{
  if(!IAP)return;
#if defined(WIN32) || defined(linux)
#else
  alZeroFrames(IAP,count);
#endif
}

/************************
* CONVENIENCE FUNCTIONS *
************************/
void *QAudioPort::AllocBuffer(int size)
{ if(buffer)FreeBuffer();
  bufferSize=size;
  buffer=qcalloc(size);
  return buffer;
}
void *QAudioPort::GetBuffer()
{ return buffer;
}
void QAudioPort::FreeBuffer()
{ if(buffer)
  { qfree(buffer);
    bufferSize=0;
  }
}
int QAudioPort::GetBufferSize()
{ return bufferSize;
}

//#define TEST
#ifdef  TEST

#include <unistd.h>		// sginap

#define BUFSIZE	10000

QAudioPort *ap;
char *audioBuf;

void initAudio()
{ int i;
  int v,dv;
  short *p;		// 16-bit

  audioBuf=(char*)qcalloc(2*BUFSIZE);
  v=0; dv=1500; p=(short*)audioBuf;
  // Triangle generation
  for(i=0;i<BUFSIZE;i++)
  { *p++=v;
    v+=dv;
    if(v>32000||v<-32000)
    { dv=-dv;
      v+=dv;
    }
  }
}

void main(int argc,char **argv)
{
  printf("QAudioPort test\n");
  ap=new QAudioPort(); 
  if(ap->Open())
  { initAudio();
    ap->WriteSamps(audioBuf,BUFSIZE);
    printf("  written %d samples\n",BUFSIZE);
    while(ap->GetFilled())
    { printf("filled %d\n",ap->GetFilled());
      sginap(1);
    }
  }
  printf("  remove ap\n");
  delete ap;
}

#endif
