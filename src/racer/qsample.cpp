/*
 * QSample - samples wrapping
 * 05-07-96: Created!
 * 16-12-96: Revised completely to allow for streaming audio (disk & mem)
 * Hardcoded to use 16-bit samples, 2 channels (stereo) always (!).
 * 19-04-99: QChannel usage for affecting soundplay WHILE playing
 * 14-09-99: Some major changes; Play() now needs start volume (needed
 * for fading in loops etc). SetVolume() has been renamed to Volumize() to
 * indicate better that it modifies the sample. Dynamic volume changes must
 * use the QChannel interface.
 * 09-08-01: Support for SDL (Linux for example) and FMOD (www.fmod.org)
 * BUGS:
 * - All samples are expected to be 16-bit, stereo (4 bytes/frame, 2chn)
 * - Looping is done on BYTES_PER_FRAME segments; clicks may occur at loop(!)
 * FUTURE:
 * - Perhaps kill the audioprocess and associated ports/configs somewhere.
 * - Use QThread() to start process? (prctl SETEXITSIG!)
 * NOTES:
 * - After opening the audioport, the ALsetchannels() may not be modified
 *   (setwidth() IS dynamic)
 * - poll() is used to be multitasking-friendly. However, poll() returns
 * immediately if the audio is not constantly kept busy. Hope it doesn't
 * cost much writing zeroes all the time.
 * (C) MarketGraph/RVG 1996
 */

#include <dmedia/audio.h>
#include <dmedia/audiofile.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#endif
#ifdef __sgi
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <limits.h>
#include <unistd.h>		// sginap()
#include <sys/prctl.h>
#include <sys/schedctl.h>
#include <signal.h>
#include <ulocks.h>
#include <bstring.h>
#endif
#include <qlib/sample.h>
#include <qlib/debug.h>

#ifdef Q_USE_FMOD
#include <fmod/fmod_errors.h>
#endif

#ifdef Q_USE_SDL // use SDL for Linux sound
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif

DEBUG_ENABLE

// Default stream buffer size increased from 2048 to 4096 on 19-4-99
// because of FX like volume
#define DEFAULT_STREAMSIZE	4096
//#define DEFAULT_STREAMSIZE	2048

// Default sample offset to match video output
#define DEFAULT_OFFSET		5500

#define MAX_CHANNEL	4	// Indy has 4, I believe
#define QUEUESIZE	70000	// Used audio queue (samples; /2~STREAM_BUFSIZE)
#define STREAM_BUFSIZE	pSBufSize // Streaming buffer (read-ahead) in frames
#define BYTES_PER_FRAME	4	// For this class; 16-bit stereo

#define CMD_PLAY	0	// 'cmd' values for comm. with AudioProcess()
#define CMD_STOP	1

//// shared data used by the audio process and the main thread

static bool     procStarted;
//static struct __APdata ad[MAX_CHANNEL];
static QChannel *ad[MAX_CHANNEL];
#ifdef __sgi
static usema_t *sendSema,*rcvSema;	// Command processing
#endif

static int       cmd;			// Command to execute
static QSample  *smp;			// Sample which the command uses
static int       startVolume;		// Volume at which to begin play
static QChannel *playChannel;		// Which channel was used to play


//// end shared data

#ifndef WIN32
static pid_t    procID;			// Process ID of sample process
#endif
static int apRR;				// Audioport selection
static int pSBufSize=DEFAULT_STREAMSIZE;	// Stream buffer size
static int smpOffset=DEFAULT_OFFSET;		// Sample offset
#ifdef TEST
proftime_t t;
#endif

#ifdef __sgi

/***********
* QCHANNEL *
***********/
#undef  DBG_CLASS
#define DBG_CLASS "QChannel"

QChannel::QChannel()
{
  DBG_C("ctor")
  
  streamBuf=0;
  streamBufSize=0;
  streamBufSizeFrames=0;
  queuesize=0;
  frames=0;
  curFrame=0;
  port=0;
  volume=256;			// Full
  //audioConfig=0;
  //audioPort=0;
  port=new QAudioPort();
  busy=FALSE;
  smp=0;
#ifdef Q_USE_SDL
  mixchunk=0;
#endif 
  // FX
  volFrom=volTo=0;
  volCur=0; volTotal=0;
}
QChannel::~QChannel()
{
  DBG_C("dtor")
}
void QChannel::SetVolume(int v,int iterations)
{
  DBG_C("SetVolume")
  if(v<0)v=0; else if(v>256)v=256;
  if(iterations==0)
  { volume=v;
  } else
  { // Slowly go there
    volFrom=volume;
    volTo=v;
    volCur=0;
    volTotal=iterations;		// Thread will see this
  }
}
bool QChannel::IsTransitioning()
// Returns TRUE if this channel is doing a volume transition
{
  if(volTotal)return TRUE;
  return FALSE;
}

#undef  DBG_CLASS
#define DBG_CLASS "QSample"

/************************
* SPROC'd AUDIO PROCESS *
************************/
static void ProcessAudioCommand()
{ long pvbuf[4];
  int  i;
  static ALconfig audioconfig;

  DBG_C_FLAT("ProcessAudioCommand");
  
  //printf("processaudiocmd %d\n",cmd);
  if(!audioconfig)
    audioconfig=ALnewconfig();   
  switch(cmd)
  { case CMD_PLAY:
      // Select an audioport that is not busy
      for(i=0;i<MAX_CHANNEL;i++)
        if(!ad[i]->busy)
        { apRR=i; goto found; }
      // No-one available; overrule a used port
      //printf("QSample: overrule\n");
      for(i=0;i<MAX_CHANNEL;i++)
      { // Avoid streaming samples; uninterrupted background music mostly
        if(ad[i]->smp->flags&QSAMPLE_STREAM)continue;
        // Avoid looping samples
        if(ad[i]->smp->flags&QSAMPLE_LOOP)continue;
        // Kick out currently playing sample
        ad[i]->smp->busy=FALSE;
        apRR=i; goto found;
      }
      // Really no ports available; forget it
      apRR=(apRR+1)%MAX_CHANNEL;
      return;
     found:
      //printf("  (audio) play sample\n");
#ifdef TEST
      profStart(&t);
#endif
      playChannel=ad[apRR];
//qdbg("CMD_PLAY: playChannel=%p\n",playChannel);
      ad[apRR]->busy=TRUE;
      ad[apRR]->smp=smp;
      ad[apRR]->curFrame=0;
      ad[apRR]->SetVolume(startVolume);
      ad[apRR]->frames=smp->frames;

      // Set output rate
      pvbuf[0]=AL_OUTPUT_RATE; pvbuf[1]=(long)ad[apRR]->smp->file_rate;
      ALsetparams(AL_DEFAULT_DEVICE,pvbuf,2);
      // Set config (x-bits, st/mo)
      ALsetwidth(audioconfig,ad[apRR]->smp->sampwidth/8);
      ALsetchannels(audioconfig,ad[apRR]->smp->samps_per_frame);
      ALsetqueuesize(audioconfig,QUEUESIZE);
      ALsetconfig(ad[apRR]->port->GetAudioPort(),audioconfig);
      //ALfreeconfig(audioconfig);
#ifdef TEST
      profReport(&t,"ProcessAudioCommand audio setup");
#endif
      break;
  }
}

static void Volumize(short *smp,int vol,int n)
{
  //DBG_C("Volumize")
  for(;n>=0;n--)
  { *smp=(*smp*vol)>>8;
    smp++;
  }
}
static void AudioProcess(void *)
// This thread feeds the audio hardware continuously
// Should be run with root priority
{
  int i;
  struct pollfd pollFD[MAX_CHANNEL+1];	// One for semaphore

  DBG_C_FLAT("AudioProcess")

  // Set high priority execution

//qdbg("QSample: AudioProcess() running\n");
#define LINGO
#define PRI_SAMPLE	32		// Before app itself
#ifdef LINGO
  // Boost thread priority
  if(setreuid(getuid(),0)==-1)
  { qdbg("QSample: can't setreuid(getuid,0) to become root\n");
  }
  schedctl(NDPRI,0,PRI_SAMPLE);
  // Continue as user that has run the program (file permissions)
  setreuid(getuid(),getuid());
#else
  if(!QSetThreadPriority(PRI_MEDIUM))
  { qwarn("Couldn't set audio thread priority; run as root");
  }
#endif

  // Open the audio ports
  for(i=0;i<MAX_CHANNEL;i++)
  { ad[i]=new QChannel();
  }

  for(i=0;i<MAX_CHANNEL;i++)
  {
    ad[i]->port->SetQueueSize(QUEUESIZE);
    if(!ad[i]->port->Open())
      qerr("QSample: Can't open audioport");
    // Init stream buffer
    ad[i]->streamBufSizeFrames=STREAM_BUFSIZE;
    ad[i]->streamBufSize=ad[i]->streamBufSizeFrames*BYTES_PER_FRAME;
    ad[i]->streamBuf=(char*)calloc(1,ad[i]->streamBufSize);
    ad[i]->queuesize=ad[i]->port->GetQueueSize();
  }

  /** Prepare to poll on the channels **/
  pollFD[0].fd=usopenpollsema(sendSema,0777);
  pollFD[0].events=POLLIN;
  for(i=0;i<MAX_CHANNEL;i++)
  {
    pollFD[i+1].fd=ad[i]->port->GetFD();
    pollFD[i+1].events=POLLOUT;
  }

  /** Wait for first event to happen **/
  uspsema(sendSema);

  // Signal parent thread that we are active
  procStarted=1;

  /** Continually wait for channel draining and audio commands **/
  while(1)
  {
    long fillPoint;
    for(i=0;i<MAX_CHANNEL;i++)
    { fillPoint=ad[i]->queuesize-ad[i]->streamBufSizeFrames/4;
      ad[i]->port->SetFillPoint(fillPoint);
    }

    poll(pollFD,MAX_CHANNEL+1,-1);
    // Check for audio command
    if(pollFD[0].revents&POLLIN)
    { ProcessAudioCommand();
      uspsema(sendSema);
      usvsema(rcvSema);		// Let parent thread continue
      //printf("  (audio) cmd processed\n");
    }
    for(i=0;i<MAX_CHANNEL;i++)
    { if(!(pollFD[i+1].revents&POLLOUT))
        continue;
//qdbg("AProc: port[%d]: busy=%d\n",i,ad[i]->busy);
      if(!ad[i]->busy)
      { // Write something to keep the audio busy for a while
//qdbg("> not busy; keep it doing something\n");
        //ad[i]->port->WriteSamps(ad[i]->streamBuf,ad[i]->streamBufSizeFrames);
        ad[i]->port->ZeroFrames(ad[i]->streamBufSizeFrames);
        continue;
      }
#ifdef TEST
      //profReport(&t,"  AP pollout");
#endif

      // Get sample data, be it from disk or from mem
      int toPlay;
     redo_loop:
      toPlay=ad[i]->streamBufSizeFrames;
      if(toPlay+ad[i]->curFrame>ad[i]->frames)
      { toPlay=ad[i]->frames-ad[i]->curFrame;
      }
      //printf("toPlay=%d\n",toPlay);

      // Handle volume transitions
      if(ad[i]->volTotal)
      {
//qdbg("voltrans: filled=%d\n",ad[i]->port->GetFilled());
        if(++ad[i]->volCur==ad[i]->volTotal)
        { ad[i]->volume=ad[i]->volTo;
          ad[i]->volTotal=0;
        } else
        { ad[i]->volume=ad[i]->volFrom+
            ad[i]->volCur*(ad[i]->volTo-ad[i]->volFrom)/ad[i]->volTotal;
//qdbg("volume transition; from/to %d/%d, cur=%d, interpol %d/%d\n",ad[i]->volFrom,ad[i]->volTo,ad[i]->volume,ad[i]->volCur,ad[i]->volTotal);
        }
      }

      if(toPlay>BYTES_PER_FRAME-1)
      { if(ad[i]->smp->flags&QSAMPLE_STREAM)
        { // Stream from disk
//printf("> stream from disk\n");
          AFseekframe(ad[i]->smp->afh,AF_DEFAULT_TRACK,ad[i]->curFrame);
//printf("> read %d frames @%d\n",toPlay,ad[i]->curFrame);
          AFreadframes(ad[i]->smp->afh,AF_DEFAULT_TRACK,ad[i]->streamBuf,
            toPlay);

          // Play it volumized
          if(ad[i]->volume==0)
          { // Don't waste time on setting volume to 0; write silence
            ad[i]->port->ZeroFrames(toPlay);
          } else if(ad[i]->volume!=256)
          { // Bring sample down to right volume
            Volumize((short*)ad[i]->streamBuf,ad[i]->volume,toPlay*2);
            ad[i]->port->WriteSamps(ad[i]->streamBuf,toPlay*2);
          } else
          { // Just output original sample
            ad[i]->port->WriteSamps(ad[i]->streamBuf,toPlay*2);
          }

        } else
        { // Stream directly from memory
          char *curMem;
//printf("> direct from mem\n");
          curMem=(char*)ad[i]->smp->mem;
          curMem+=ad[i]->curFrame*BYTES_PER_FRAME;
          if(ad[i]->volume==0)
          { // Don't waste time on setting volume to 0; write silence
            ad[i]->port->ZeroFrames(toPlay);
          } else if(ad[i]->volume!=256)
          { // Bring sample down to right volume
            bcopy(curMem,ad[i]->streamBuf,toPlay*2*2);
            Volumize((short*)ad[i]->streamBuf,ad[i]->volume,toPlay*2);
//qdbg("volumize: filled=%d\n",ad[i]->port->GetFilled());
            // Play FX-ed sample
            curMem=ad[i]->streamBuf;
//qdbg("(filled=%d)\n",ad[i]->port->GetFilled());
            ad[i]->port->WriteSamps(curMem,toPlay*2);
          } else
          { // Just output original sample
            ad[i]->port->WriteSamps(curMem,toPlay*2);
          }
        }
        ad[i]->curFrame+=toPlay;
      } else
      { // Check for looping sample
        if(ad[i]->smp->flags&QSAMPLE_LOOP)
        { ad[i]->curFrame=0;
          // Avoid endless loop for zero-sized samples
          if(ad[i]->frames>=BYTES_PER_FRAME)
          { //printf("QSample: loop\n");
            goto redo_loop;
          }
        }
        ad[i]->busy=0;
        ad[i]->smp->busy=FALSE;
      }
    }
  }
}

void RunAP()
// Start up the audio process if not already started
{
  static short int signum=9;

  // Spawn audio process if not done already
  if(!procStarted)
  { // Initialize audio process spawn
    usptr_t *arena;
    // Don't let other processes access the arena
    usconfig(CONF_ARENATYPE,US_SHAREDONLY);
    arena=usinit(tmpnam(0));
    sendSema=usnewpollsema(arena,0);
    rcvSema =usnewsema(arena,1);
    prctl(PR_SETEXITSIG,(void*)signum);
    //printf("QSample: sproc()\n");
    procID=sproc(AudioProcess,PR_SALL);
    // Must wait until audioprocess has done uspsema(sendSema)
    while(!procStarted)
      sginap(1);
  }
}
void QSampleKillProcess()
{
  if(!procStarted)return;
  kill(procID,SIGKILL);
  procStarted=FALSE;
}

#endif
// !Win32

/*****************
* SAMPLE LOADERS *
*****************/
#if defined(WIN32)
//-----------------------------------------------------------------------------
// File: WavRead.h
//
// Desc: Support for loading and playing Wave files using DirectSound sound
//       buffers.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

//-----------------------------------------------------------------------------
// Name: class CWaveSoundRead
// Desc: A class to read in sound data from a Wave file
//-----------------------------------------------------------------------------
class CWaveSoundRead
{
public:
    WAVEFORMATEX* m_pwfx;        // Pointer to WAVEFORMATEX structure
    HMMIO         m_hmmioIn;     // MM I/O handle for the WAVE
    MMCKINFO      m_ckIn;        // Multimedia RIFF chunk
    MMCKINFO      m_ckInRiff;    // Use in opening a WAVE file

public:
    CWaveSoundRead();
    ~CWaveSoundRead();
  
    HRESULT Open( CHAR* strFilename );
    HRESULT Reset();
    HRESULT Read( UINT nSizeToRead, BYTE* pbData, UINT* pnSizeRead );
    HRESULT Close();

};
#endif

/******************
* QSAMPLE MEMBERS *
******************/
#if defined(Q_USE_SDL) || defined(Q_USE_FMOD)
int QSample::mix_channel=0; 
#endif

QSample::QSample(cstring fname,int _flags)
{ 
  DBG_C("ctor")
  DBG_ARG_S(fname);
  DBG_ARG_I(_flags);

#ifdef __sgi
  // Make sure audio process is running
  RunAP();
#endif

  fd=0;
  mem=0;
  frames=0;
  flags=_flags;
  // Default format
  file_rate=44100.0;
  sampwidth=16;
  samps_per_frame=2;
#if defined(__sgi)
  afh=0;
#endif
#if defined(Q_USE_SDL)
  channel=mix_channel; 
  // Potential bug here; when new-ing and deleting
  // samples in random order, the mix_channel
  // won't give the correct channel id.
  QSample::mix_channel++; 
qdbg("QSample ctor: mix_channel now %d\n", mix_channel); 
#endif
#if defined(Q_USE_FMOD)
  channel=mix_channel;
  QSample::mix_channel++;
  sound=0;
#endif

  busy=FALSE;
  Load(fname);
}

QSample::~QSample()
{ 
  DBG_C("dtor")
  if(mem)qfree(mem);
#if defined(__sgi)
  if(afh)
    AFclosefile(afh);
#endif

#ifdef Q_USE_SDL
  if (mixchunk) 
    Mix_FreeChunk(mixchunk);
  //fprintf (stderr, "QSample::DTOR mixchunk freed.\n"); 
  mix_channel--;  
#endif

#ifdef Q_USE_FMOD
  if (sound) 
    FSOUND_Sample_Free(sound);
  //fprintf (stderr, "QSample::DTOR sound freed.\n"); 
  mix_channel--;  
#endif

}
#ifdef Q_USE_SDL
void QSample::SetVolume(int n)
// Set volume from 0..255 (SDL)
{
  // Note that SDL supports only 0..127
  Mix_VolumeChunk(mixchunk,n/2); 
}
#endif 

#ifdef Q_USE_FMOD
void QSample::SetVolume(int n)
// Set volume from 0..255 (FMOD)
{
  QASSERT_V(sound);

  FSOUND_SetVolume(channel,n); 
}

void QSample::SetFrequency(int n)
// Set playing frequency of the sample
{
  QASSERT_V(sound);

qdbg("QSample:SetFreq(%d)\n",n);

  FSOUND_SetFrequency(channel, n); 
  return; 
}
#endif 

bool QSample::IsOK()
// Returns TRUE if the sample was succesfully loaded, or the file is open
// (in case it is a streaming sample)
// Don't use GetBuffer() for this, because that doesn't work for streaming
// samples.
{
#ifdef Q_USE_SDL
  if (mixchunk) return TRUE; 
#endif 

#ifdef Q_USE_FMOD
  if (sound) return TRUE; 
#endif 

  DBG_C("IsOK")
  if(mem)return TRUE;
#if defined(__sgi)
  if(afh)return TRUE;
#endif
  // Not loaded, and no file open; sample is NOT ok
  return FALSE;
}

#if defined(__sgi)

int QSample::SampleWidth(void)
// Returns sample frame width in #bits (!)
{
  DBG_C("SampleWidth")
  int sampfmt,sampwidth;
  
  AFgetsampfmt(afh,AF_DEFAULT_TRACK,&sampfmt,&sampwidth);
  return (int)sampwidth;
}
 
#endif
// !Win32

int QSample::GetNoofFrames()
// Returns #frames (frames = samples * channels!)
{

#ifdef Q_USE_SDL
  fprintf(stderr, "QSample::getnooframes called\n");
  return mixchunk->alen;
#endif   

#ifdef Q_USE_FMOD
//qdbg("QSample::GetNoofFrames()\n");
  return FSOUND_Sample_GetLength(sound); 
#endif   

  DBG_C("GetNoofFrames")
  return frames;
}

bool QSample::Load(cstring fname)
// Read a sample.
// Returns FALSE in case of failure.
{
  DBG_C("Load")
  DBG_ARG_S(fname);

  int offset=smpOffset;
  if(!(flags&QSAMPLE_OFFSET))
    offset=0;

#if defined(Q_USE_DIRECTSOUND)
  // Only .wav is supported under Win32 using WAVREAD.CPP from the DirectX SDK
  CWaveSoundRead *wsr=0;
  HRESULT hr;
  WAVEFORMATEX *fmt;
  int nBytes;

  wsr=new CWaveSoundRead();
  if(FAILED(hr=wsr->Open((string)fname)))
  {
    qwarn("QSample:Load(); can't open '%s'",fname);
fail:
    if(wsr)delete wsr;
    return FALSE;
  }
  // Extract sample format
  fmt=wsr->m_pwfx;
  if(fmt->wFormatTag!=WAVE_FORMAT_PCM)
  { qwarn("QSample:Load(); sample now WAVE_FORMAT_PCM (formatTag=%d)",fmt->wFormatTag);
    goto fail;
  }
  file_rate=fmt->nSamplesPerSec;
  samps_per_frame=fmt->nChannels;
  sampwidth=fmt->wBitsPerSample;
  frames=wsr->m_ckIn.cksize/(samps_per_frame*(sampwidth/8));
  // SGI vars
  fd=0;
  compression=0;

  // Read sample data
  nBytes=(sampwidth/8)*frames*samps_per_frame;
  mem=qcalloc(nBytes);
  if(!mem)
  { qwarn("QSample:Load(%s); no mem",fname);
    goto fail;
  }
  UINT dummySize;
  wsr->Read(nBytes,(unsigned char*)mem,&dummySize);
  delete wsr;

#ifdef OBS_DEV
  // Dump file (DEV)
  { FILE *fw;
    fw=fopen("sample.dmp","wb");
    fprintf(fw,"Sample: %s\n",fname);
    fprintf(fw,"Rate: %f, samps_per_frame(channels)=%d, sampwidth=%d, frames=%d\n",
      file_rate,samps_per_frame,sampwidth,frames);
    fprintf(fw,"Dummsize: %d\n",dummySize);
    fwrite(mem,1,frames*samps_per_frame*(sampwidth/8),fw);
    fclose(fw);
  }
#endif
  return TRUE;
#endif
#ifdef __sgi
  // SGI AL loader; support .wav, .aiff and lots of others
  //printf("QSample::Load(%s)\n",fname);
  fd=open(QExpandFilename(fname),O_RDONLY);
  if(fd<0)return FALSE;			// Can't open file
  if(AFidentifyfd(fd)<0)return FALSE;	// Not AIFF-C or AIFF
  afh=AFopenfd(fd,"r",AF_NULL_FILESETUP);
  if(afh==AF_NULL_FILEHANDLE)return FALSE;	// Failed to open

  file_rate=AFgetrate(afh,AF_DEFAULT_TRACK);
  compression=AFgetcompression(afh,AF_DEFAULT_TRACK);
  samps_per_frame=AFgetchannels(afh,AF_DEFAULT_TRACK);
  filefmt=AFgetfilefmt(afh,&vers);

  // Read sample into memory
  sampwidth=SampleWidth();
  frames=AFgetframecnt(afh,AF_DEFAULT_TRACK);
  frames+=offset;
  //printf("QSample::Load: frames=%d, sampwidth=%d\n",frames,sampwidth);

  if(flags&QSAMPLE_STREAM)
  { // Use streaming instead of loading up in memory
    //printf("> using diskstreaming for sample\n");
    mem=0;
    // Keep the file open to do disk streaming
  } else
  { mem=qcalloc((sampwidth/8)*frames*samps_per_frame);
    if(!mem)
    { printf("QSample::Load(%s): no mem\n",fname);
      return FALSE;
    }
    AFseekframe(afh,AF_DEFAULT_TRACK,0); 
    short *smp=(short*)mem;
    AFreadframes(afh,AF_DEFAULT_TRACK,smp+offset*2,frames);
    AFclosefile(afh);
    afh=0;
  }
  return FALSE;
#endif

#ifdef Q_USE_SDL
  //#define DEBUG_SDL 
//printf ("Qsample::Load (linux) passed filename %s\n", fname); 
  mixchunk = Mix_LoadWAV(fname); 
  if (mixchunk) {  
    fprintf (stderr, "Qsample::Load (linux) %s loaded ok\n", fname); 
    fprintf (stderr, "Sample Length: %d\n", 
	     mixchunk->alen);  
    fprintf (stderr, "Sample Channel: %d\n", 
	     channel);  
    
#ifdef DEBUG_SDL
    // play it once just to make sure
    Mix_PlayChannel(0, mixchunk, 1);   
    // wait for sound to finish
    while (Mix_Playing(0)) {
      SDL_Delay(10);
    }
#endif //DEBUG_SDL

  } else {
    printf ("Qsample::Load (linux) %s load failed: %s\n", 
	    fname, Mix_GetError()); 
    return FALSE; 
  }
  return TRUE; 
#endif  //Q_USE_SDL

#ifdef Q_USE_FMOD
  int fmodFlags=0;

  if(flags&HW3D)fmodFlags|=FSOUND_HW3D;
  if(flags&FORCE_2D)fmodFlags|=FSOUND_2D;
  if(flags&LOOP)fmodFlags|=FSOUND_LOOP_NORMAL;

//qdbg("Qsample::Load (linux) passed filename %s\n", fname); 
  sound=FSOUND_Sample_Load(channel,fname,fmodFlags,0); 
  
  if (sound)
  {  
#ifdef OBS
    qdbg("Qsample::Load (linux) %s loaded ok\n", fname); 
    qdbg("Sample Length: %d\n", 
	     FSOUND_Sample_GetLength(sound));  
    qdbg("Sample Channel: %d\n",channel);  
#endif
  } else
  {
    qerr("Can't load sample: %s",FMOD_ErrorString(FSOUND_GetError()));
    return FALSE; 
  }
  return TRUE; 
#endif  //Q_USE_FMOD
}
bool QSample::IsPlaying()
{
#ifdef Q_USE_SDL
  if (Mix_Playing(channel))
      return busy; 
#else
  DBG_C("IsPlaying")
  return busy;
#endif //Q_USE_SDL
}
  

#ifdef Q_USE_SDL
QChannel *QSample::Play(int vol) {
  // Play the sample; beginning at (dynamic) volume 'vol'
  fprintf(stderr, "SDL Playing channel: %d\n", channel); 
  // Check for looping sample
  if(flags&QSAMPLE_LOOP)
    Mix_PlayChannel(channel, mixchunk, -1); 
  else // play once
    Mix_PlayChannel(channel, mixchunk, 1); 

  return (QChannel *) NULL; 
}

void QSample::Stop()
// Stop (looping) sample; also for one-shot samples
{
  Mix_HaltChannel(channel); 
  return; 
}

void QSample::Loop(bool yn)
{
  DBG_C("Loop");

//qdbg(stderr, "Setting sample loop\n");
  if(yn)flags|=QSAMPLE_LOOP;
  else  flags&=~QSAMPLE_LOOP;
}

#endif // Q_USE_SDL


#ifdef Q_USE_FMOD
QChannel *QSample::Play(int vol)
// FMOD doesn't return a channel
{
  if(!IsOK())
  {
    qwarn("QSample:Play(); sample is not ok");
    return 0;
  }
  // Play the sample; beginning at (dynamic) volume 'vol'
qdbg("FMOD Playing channel: %d\n",channel); 
  // Check for looping sample
  if(flags&QSAMPLE_LOOP)
  {
    // 5-10-01; PlaySound should not force channel 0, but let FMod select
    // a channel, since hardware channels start up >>0.
    // This probably caused the lack of sound on some systems (with acc sound).
    //FSOUND_PlaySound(channel, sound); 
    channel=FSOUND_PlaySound(FSOUND_FREE,sound); 
  }
  return (QChannel *) NULL; 
}

void QSample::Stop()
// Stop (looping) sample; also for one-shot samples
{
  FSOUND_StopSound(channel); 
  return; 
}

void QSample::Loop(bool yn)
{
qdbg("Setting sample loop\n");
  DBG_C("Loop");
  if(yn)flags|=QSAMPLE_LOOP;
  else  flags&=~QSAMPLE_LOOP;
}

#endif // Q_USE_FMOD

#ifdef __sgi

QChannel *QSample::Play(int vol)
// Play the sample; beginning at (dynamic) volume 'vol'
{
  DBG_C("Play")
//printf("QSample:Play this=%p\n",this);

  // Leave message for audio process (set .mem last!)
  cmd=CMD_PLAY;
  smp=this;
  startVolume=vol;

  // Wakeup audio process
  usvsema(sendSema);
  // Wait for response
  uspsema(rcvSema);
  busy=TRUE;
  while(!playChannel)
  { //printf("playChn==0\n");
    QNap(1);
  }

  // 'playChannel' was set by AudioProcess() to indicate which channel was used
  return playChannel;
}
void QSample::Stop()
// Stop (looping) sample; also for one-shot samples
{
  DBG_C("Stop")
  int i;
  busy=FALSE;
  // Find port on which this sample is playing
  for(i=0;i<MAX_CHANNEL;i++)
  { //printf("QSmp:Stop: port[%d].busy=%d\n",i,ad[i].busy);
    if(ad[i]->smp==this)
    { // Stop port
      //printf("  qsmp:stop: stop port %d\n",i);
      ad[i]->busy=FALSE;
      //ad[i].smp=0;
    }
  }
  /*for(i=0;i<MAX_CHANNEL;i++)
    printf("QSmp:Stop: port[%d].busy=%d\n",i,ad[i].busy);*/
}

void QSample::Loop(bool yn)
{

  fprintf(stderr, "Setting sample loop\n");
  DBG_C("Loop");
  if(yn)flags|=QSAMPLE_LOOP;
  else  flags&=~QSAMPLE_LOOP;
}

void QSample::Volumize(int perc)
// Modify sample to match new volume percentage (was called SetVolume() once)
// Doesn't work for streaming audio
// Bug: assumes 16-bit stereo
{
  DBG_C("Volumize")
  short *p;
  int i,v;

  if(!mem)
  { qwarn("QSample::Volumize() called on streaming sample");
    return;
  }

  p=(short*)mem;
  if(sampwidth!=16)
  { qwarn("QSample::SetVolume for non-16-bit samples NYI");
    return;
  }
  //qdbg("QSample::SetVol: samps_per_frame=%d\n",samps_per_frame);
  for(i=0;i<frames*samps_per_frame;i++)
  { v=(int)*p;
    v=v*perc/100;
    //if(v>65535)v=65535;
    //if(v<0)v=0;
    if(v>32767)v=32767;
    else if(v<-32768)v=-32768;
    *p=v;
    p++;
  }
}
#endif
// __sgi

/*************
* 3D Support *
*************/
void QSample::Set3DAttributes(float *pos,float *vel)
// Set position and velocity (in meters and m/s)
{
#ifdef Q_USE_FMOD
  if(!sound)return;
  // Check channel...
  FSOUND_3D_SetAttributes(channel,pos,vel);
#endif
}

/*****************
* Flat functions *
*****************/
void QSampleSetStreamSize(int size)
// Before playing any samples, you may set the stream buffer size
{
  DBG_C_FLAT("SetStreamSize")
  QASSERT_V(size>100&&size<=32768);
  if(procStarted)
  { qerr("Attempt to set sample stream buffer size after samples were played.");
    return;
  }
  pSBufSize=size;
}

void QSampleSetOffset(int off)
{
  DBG_C_FLAT("SetOffset")
  smpOffset=off;
}


/********************
* WIN32 WAVE LOADER *
********************/
#ifdef WIN32
//-----------------------------------------------------------------------------
// File: WavRead.cpp
//
// Desc: Wave file support for loading and playing Wave files using DirectSound 
//       buffers.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

//-----------------------------------------------------------------------------
// Name: ReadMMIO()
// Desc: Support function for reading from a multimedia I/O stream
//-----------------------------------------------------------------------------
HRESULT ReadMMIO( HMMIO hmmioIn, MMCKINFO* pckInRIFF, WAVEFORMATEX** ppwfxInfo )
{
    MMCKINFO        ckIn;           // chunk info. for general use.
    PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       

    *ppwfxInfo = NULL;

    if( ( 0 != mmioDescend( hmmioIn, pckInRIFF, NULL, 0 ) ) )
        return E_FAIL;

    if( (pckInRIFF->ckid != FOURCC_RIFF) ||
        (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E') ) )
        return E_FAIL;

    // Search the input file for for the 'fmt ' chunk.
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if( 0 != mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK) )
        return E_FAIL;

    // Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    // if there are extra parameters at the end, we'll ignore them
       if( ckIn.cksize < (LONG) sizeof(PCMWAVEFORMAT) )
           return E_FAIL;

    // Read the 'fmt ' chunk into <pcmWaveFormat>.
    if( mmioRead( hmmioIn, (HPSTR) &pcmWaveFormat, 
                  sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat) )
        return E_FAIL;

    // Allocate the waveformatex, but if its not pcm format, read the next
    // word, and thats how many extra bytes to allocate.
    if( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM )
    {
        if( NULL == ( *ppwfxInfo = new WAVEFORMATEX ) )
            return E_FAIL;

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = 0;
    }
    else
    {
        // Read in length of extra bytes.
        WORD cbExtraBytes = 0L;
        if( mmioRead( hmmioIn, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD) )
            return E_FAIL;

        *ppwfxInfo = (WAVEFORMATEX*)new CHAR[ sizeof(WAVEFORMATEX) + cbExtraBytes ];
        if( NULL == *ppwfxInfo )
            return E_FAIL;

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
        (*ppwfxInfo)->cbSize = cbExtraBytes;

        // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
        if( mmioRead( hmmioIn, (CHAR*)(((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(WORD)),
                      cbExtraBytes ) != cbExtraBytes )
        {
            delete *ppwfxInfo;
            *ppwfxInfo = NULL;
            return E_FAIL;
        }
    }

    // Ascend the input file out of the 'fmt ' chunk.
    if( 0 != mmioAscend( hmmioIn, &ckIn, 0 ) )
    {
        delete *ppwfxInfo;
        *ppwfxInfo = NULL;
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveOpenFile()
// Desc: This function will open a wave input file and prepare it for reading,
//       so the data can be easily read with WaveReadFile. Returns 0 if
//       successful, the error code if not.
//-----------------------------------------------------------------------------
HRESULT WaveOpenFile( CHAR* strFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
                  MMCKINFO* pckInRIFF )
{
    HRESULT hr;
    HMMIO   hmmioIn = NULL;
    
    if( NULL == ( hmmioIn = mmioOpen( strFileName, NULL, MMIO_ALLOCBUF|MMIO_READ ) ) )
        return E_FAIL;

    if( FAILED( hr = ReadMMIO( hmmioIn, pckInRIFF, ppwfxInfo ) ) )
    {
        mmioClose( hmmioIn, 0 );
        return hr;
    }

    *phmmioIn = hmmioIn;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveStartDataRead()
// Desc: Routine has to be called before WaveReadFile as it searches for the
//       chunk to descend into for reading, that is, the 'data' chunk.  For
//       simplicity, this used to be in the open routine, but was taken out and
//       moved to a separate routine so there was more control on the chunks
//       that are before the data chunk, such as 'fact', etc...
//-----------------------------------------------------------------------------
HRESULT WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn,
                           MMCKINFO* pckInRIFF )
{
    // Seek to the data
    if( -1 == mmioSeek( *phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC),
                        SEEK_SET ) )
        return E_FAIL;

    // Search the input file for for the 'data' chunk.
    pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
    if( 0 != mmioDescend( *phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK ) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveReadFile()
// Desc: Reads wave data from the wave file. Make sure we're descended into
//       the data chunk before calling this function.
//          hmmioIn      - Handle to mmio.
//          cbRead       - # of bytes to read.   
//          pbDest       - Destination buffer to put bytes.
//          cbActualRead - # of bytes actually read.
//-----------------------------------------------------------------------------
HRESULT WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
                      MMCKINFO* pckIn, UINT* cbActualRead )
{
    MMIOINFO mmioinfoIn;         // current status of <hmmioIn>

    *cbActualRead = 0;

    if( 0 != mmioGetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;
                
    UINT cbDataIn = cbRead;
    if( cbDataIn > pckIn->cksize ) 
        cbDataIn = pckIn->cksize;       

    pckIn->cksize -= cbDataIn;
    
    for( DWORD cT = 0; cT < cbDataIn; cT++ )
    {
        // Copy the bytes from the io to the buffer.
        if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
        {
            if( 0 != mmioAdvance( hmmioIn, &mmioinfoIn, MMIO_READ ) )
                return E_FAIL;

            if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
                return E_FAIL;
        }

        // Actual copy.
        *((BYTE*)pbDest+cT) = *((BYTE*)mmioinfoIn.pchNext);
        mmioinfoIn.pchNext++;
    }

    if( 0 != mmioSetInfo( hmmioIn, &mmioinfoIn, 0 ) )
        return E_FAIL;

    *cbActualRead = cbDataIn;
    return S_OK;
}



  
//-----------------------------------------------------------------------------
// Name: CWaveSoundRead()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CWaveSoundRead::CWaveSoundRead()
{
    m_pwfx   = NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CWaveSoundRead()
// Desc: Destructs the class
//-----------------------------------------------------------------------------
CWaveSoundRead::~CWaveSoundRead()
{
    Close();
    SAFE_DELETE( m_pwfx );
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Opens a wave file for reading
//-----------------------------------------------------------------------------
HRESULT CWaveSoundRead::Open( CHAR* strFilename )
{
    SAFE_DELETE( m_pwfx );

    HRESULT  hr;
    
    if( FAILED( hr = WaveOpenFile( strFilename, &m_hmmioIn, &m_pwfx, &m_ckInRiff ) ) )
        return hr;

    if( FAILED( hr = Reset() ) )
        return hr;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: Reset()
// Desc: Resets the internal m_ckIn pointer so reading starts from the 
//       beginning of the file again 
//-----------------------------------------------------------------------------
HRESULT CWaveSoundRead::Reset()
{
    return WaveStartDataRead( &m_hmmioIn, &m_ckIn, &m_ckInRiff );
}




//-----------------------------------------------------------------------------
// Name: Read()
// Desc: Reads a wave file into a pointer and returns how much read
//       using m_ckIn to determine where to start reading from
//-----------------------------------------------------------------------------
HRESULT CWaveSoundRead::Read( UINT nSizeToRead, BYTE* pbData, UINT* pnSizeRead )
{
    return WaveReadFile( m_hmmioIn, nSizeToRead, pbData, &m_ckIn, pnSizeRead );
}




//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Closes an open wave file 
//-----------------------------------------------------------------------------
HRESULT CWaveSoundRead::Close()
{
    mmioClose( m_hmmioIn, 0 );
    return S_OK;
}

#endif
// WIN32
