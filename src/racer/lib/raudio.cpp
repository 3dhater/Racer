/*
 * RAudio - audio handling for multiple objects/streams
 * 17-08-00: Created!
 * 21-02-01: Finally fixed audio distortion on SGI.
 * (c) Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/info.h>
#ifdef Q_USE_FMOD
#include <fmod/fmod_errors.h>
#endif
DEBUG_ENABLE

// Show all the possible drivers? (FMOD only)
//#define OPT_FMOD_SHOW_DRIVERS

// Test things with some music?
//#define QDEBUG_TEST_MUSIC

#define INI_NAME      "audio.ini"

// Fixed volume for SGI (256 is max volume)
#define FIXEDVOL      40

/********************
* An Audio producer *
********************/
#undef  DBG_CLASS
#define DBG_CLASS "RAudioProducer"

RAudioProducer::RAudioProducer()
{
//qlog(QLOG_INFO,"RAudioProducer dtor\n");
//qdbg("RAudioProducer ctor\n");
  flags=ENABLED;
  smp=0;
  sbuf=0;
  speedSample=speedCurrent=1;
  buffer=0;
  curPos=0;
  delta=0;
  vol=256;
}
RAudioProducer::~RAudioProducer()
{
//qlog(QLOG_INFO,"RAudioProducer dtor\n");
  if(flags&PRIVATE_SMP)
  {
    QDELETE(smp);
  }
  QDELETE(sbuf);
}

void RAudioProducer::SetSampleSpeed(int n)
{
  speedSample=n;
}
void RAudioProducer::SetCurrentSpeed(int n)
// Set speed; it will be played relative to the sample speed
{
  // Check for a sample to be present
  if(!smp)return;

  speedCurrent=n;
#ifdef Q_USE_FMOD
  int orgFreq,curFreq;
  orgFreq=RMGR->audio->GetFrequency();
  // Current frequency
  curFreq=orgFreq*speedCurrent/speedSample;
  if(curFreq<1000)curFreq=1000;
  smp->SetFrequency(curFreq);
#endif

#ifdef Q_USE_DIRECTSOUND
  // Set frequency of playing sample
  if(!sbuf)return;

  // Original frequency
  int orgFreq,curFreq;
  orgFreq=RMGR->audio->GetFrequency();
  // Current frequency
  curFreq=orgFreq*speedCurrent/speedSample;
  if(curFreq<1000)curFreq=1000;
  sbuf->SetFrequency(curFreq);
#endif

}

bool RAudioProducer::LoadSample(cstring fileName)
{
  // Load sample as 3D by default
  smp=new QSample(fileName,QSample::HW3D|QSample::LOOP);

  if(!smp->IsOK())
  {
    qwarn("RAudioProducer: Can't load sample '%s'\n",fileName);
    QDELETE(smp);
    return FALSE;
  }
#ifdef Q_USE_DIRECTSOUND
  sbuf=new QDXSoundBuffer(smp,QDXSoundBuffer::IS3D,
    QDXSoundBuffer::ALG_NO_VIRTUALIZATION);
  if(!sbuf->IsCreated())
    qwarn("RAudioProducer:LoadSample(); couldn't create QDXSoundBuffer");
  sbuf->EnableLoop();
  sbuf->Play();
#endif

#ifdef __sgi
  // SGI
  SetSample(smp);
#endif // SGI

#ifdef Q_USE_FMOD
  SetSample(smp); 
  //  if (smp->GetChannel()==0) // engine sound 
  //loop everything, modulate volume! 
  smp->Loop(TRUE); 
  //smp->Play(); 
#endif // linux 

  flags|=PRIVATE_SMP;
  return TRUE;
}
void RAudioProducer::SetBuffer(void *_buffer,int frames)
// Use audio buffer to pick sample frames from
{
  buffer=_buffer;
  bufferSize=frames;
  curPos=0;
}
void RAudioProducer::SetSample(QSample *smp)
// Use loaded 'smp'
{
  int size;
  size=smp->GetNoofFrames();
  // Assuming 22050Hz, 8-bit, 1 channel
  SetBuffer(smp->GetBuffer(),size);
  
//QQuickSave("sample.dmp",smp->GetBuffer(),size);
}
void RAudioProducer::ProduceFrames(void *dBuffer,int frames)
{
#ifdef Q_USE_DIRECTSOUND
  // DirectSound does producing in a separate (system) thread/process
#endif
#ifdef Q_USE_FMOD
  // FMOD uses its own thread
#endif

#ifdef __sgi
  // SGI simple interpolation mechanism
  int i;
  unsigned char *s;
  signed   char *d;
  int v,vd;
  
  // Any sample present?
  if(!smp)return;

  d=(signed char*)dBuffer;
  s=(unsigned char*)buffer;
  for(i=0;i<frames;i++,d++)
  {
//qdbg("i=%d, curPos=%d, d=%p, s=%p\n",i,curPos,d,s);
    //*d=*d+((s[curPos])*vol/256)^0x80;
    //v=(s[curPos]-128);
    v=s[curPos];
//if(i<10)qdbg("vOrg=%d, vol=%d, v=%d\n",v,vol,v*vol/256);
//if(i<10)qdbg("vOrg=$%x\n",v);
    v-=128;
    v=v*vol/256;
    // Bad sampling can be LOUD!
    v=v*FIXEDVOL/256;
    //v=((v-128)*vol/256)+128;
//if(i<10)qdbg("vol=%d, v=$%x\n",vol,v);
    //if(vol<90)v=0;
//if(i<10)qdbg("*dOrg=$%x\n",*d);
    // Cutoff adding to avoid distortion
    vd=*d;
    if(vd+v>127)*d=127;
    else if(vd+v<-128)*d=-128;
    else
    { // Normal add
      *d=*d+v;
    }
//if(i<10)qdbg("*d=$%x\n",*d);
    delta+=speedCurrent;
    while(delta>speedSample)
    { delta-=speedSample;
      curPos++;
    }
    // Modulo (but faster)
    while(curPos>=bufferSize)
      curPos-=bufferSize;
  }

#endif
}

void RAudioProducer::SetVolume(int n)
{
#if defined (Q_USE_SDL) || defined (Q_USE_FMOD)
  // fprintf (stderr, "Setting volume to %d\n", n); 
  smp->SetVolume(n); 
  return; 
#endif 
 
  vol=n;
#ifdef Q_USE_DIRECTSOUND
  if(sbuf)
  {
    int v;
    // My system seems to reduce volumes to 0
    // already at -5000, so I boost up the volume
    // here. A bit of a hack, but blame DirectSound.
#ifdef ND_10000_IS_CRAP
    v=-10000+40*n;
    if(v<-10000)v=-10000;
    else if(v>0)v=0;
#endif
    v=-5120+20*n;
    if(v<-5000)v=-5000;
    else if(v>0)v=0;
    sbuf->SetVolume(v);
  }
#endif
 

}

/**************************
* The entire audio system *
**************************/
#undef  DBG_CLASS
#define DBG_CLASS "RAudio"

RAudio::RAudio()
{
  producers=0;
  port=0;
  frequency=0;
  bits=0;
  channels=0;
  enable=TRUE;
  Load(INI_NAME);
}
RAudio::~RAudio()
{
//qdbg("RAudio dtor\n");
  if(producers)
  {
    // Not all producers were removed yet; may lead to trouble
    // on OSes like Win98
    qwarn("RAudio:dtor(); not all producers were removed yet");
  }
#ifdef Q_USE_DIRECTSOUND
  QDELETE(dxSound);
#endif
}

bool RAudio::Load(cstring fileName)
{
  //QInfo info(RFindFile(fileName));
  frequency=RMGR->GetMainInfo()->GetInt("audio.frequency");
  bits=RMGR->GetMainInfo()->GetInt("audio.bits");
#ifdef __sgi
  // Only support for 8 bits
  bits=8;
#endif
  channels=RMGR->GetMainInfo()->GetInt("audio.channels");
  enable=RMGR->GetMainInfo()->GetInt("audio.enable");
  return Create();
}

bool RAudio::Create()
{
//qdbg("RAudio:Create()\n");

#ifdef Q_USE_DIRECTSOUND
  dxSound=new QDXSound();
  if(!dxSound->Open())
  { qerr("Can't open DirectSound");
    return FALSE;
  }
  return TRUE;
#endif

#ifdef __sgi
  // SGI; use AL (audio library)
  port=new QAudioPort();
  port->SetWidth(bits/8);
  port->SetChannels(channels);
  port->SetQueueSize(100000);
  if(!port->Open())
  { qwarn("RAudio:Create(); no more SGI ports");
    return FALSE;
  }
  return TRUE;
#endif

#ifdef Q_USE_FMOD
  // FMOD support on Windows & Linux
  // (though Win32 uses DirectSound by default; this may
  // change in the future)

qdbg("FMOD initializing\n");
  if(FSOUND_GetVersion()<FMOD_VERSION) {
    qerr("FMod; old library! You should be using at least v%.02f"
      " (see www.fmod.org)\n",
      FMOD_VERSION);
    exit(1);
  }

  // Select output type (or autodetect)
  qstring s;
  RMGR->GetMainInfo()->GetString("audio.type",s);
  // Optionally don't use sound at all
  if(!enable)
  {
qdbg("Disabling FSOUND output (NOSOUND)\n");
    FSOUND_SetOutput(FSOUND_OUTPUT_NOSOUND);
  } else
  {
#ifdef WIN32
  if(s=="dsound")FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
  else if(s=="winmm")FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
  else if(s=="a3d")FSOUND_SetOutput(FSOUND_OUTPUT_A3D);
  else if(s=="");
  else qwarn("Audio output '%s' not recognized",s.cstr());
#endif
#ifdef linux
  // Select one of Linux's sound systems
  if(s=="oss")FSOUND_SetOutput(FSOUND_OUTPUT_OSS);
  else if(s=="alsa")FSOUND_SetOutput(FSOUND_OUTPUT_ALSA);
  else if(s=="esd")FSOUND_SetOutput(FSOUND_OUTPUT_ESD);
  else if(s=="");
  else qwarn("Audio output '%s' not recognized",s.cstr());
#endif
  }

#ifdef OPT_FMOD_SHOW_DRIVERS
  int i,n;
  n=FSOUND_GetNumDrivers();
  qdbg("FMOD: %d drivers found\n",n);
  for(i=0;i<n;i++)
  {
    qdbg("Driver %d: '%s'\n",i,FSOUND_GetDriverName(i));
  }
#endif

  //FSOUND_SetMixer(FSOUND_MIXER_BLENDMODE);

//qdbg("FSOUND Init\n");
  //if(!FSOUND_Init(44100,32,FSOUND_INIT_GLOBALFOCUS))
  // DirectSound only (FSOUND_INIT_GLOBALFOCUS)
  //if(!FSOUND_Init(frequency,32,FSOUND_INIT_GLOBALFOCUS))
  if(!FSOUND_Init(frequency,32,0))
  {
    qerr("Can't init FMOD: %s\n",FMOD_ErrorString(FSOUND_GetError()));
    exit(1);
  }
//qdbg("  Max #audio channels: %d\n",FSOUND_GetMaxChannels());

#ifdef QDEBUG_TEST_MUSIC
  // Test music
  FSOUND_STREAM *stream;
  stream=FSOUND_Stream_OpenFile("data/music/mc_level1.mp3",
    FSOUND_LOOP_NORMAL,0);
  if(!stream)qerr("Can't open test music");
  else
  {
    FSOUND_Stream_Play(FSOUND_FREE,stream);
  }
#endif

  return TRUE;
#endif //FMOD
  
}

bool RAudio::AddProducer(RAudioProducer *prod)
{
//qdbg("RAudio::AddProducer(%p)\n",prod);
  if(producers==MAX_PRODUCER)
  {
    qwarn("RAudio:AddProducer(); max (%d) reached",MAX_PRODUCER);
    return FALSE;
  }
  producer[producers]=prod;
  producers++;
//qdbg("  producers=%d\n",producers);
  return TRUE;
}
bool RAudio::RemoveProducer(RAudioProducer *prod)
// Removes a producer from the audio system
// Returns TRUE if producer was found & removed
{
  int i;
//qdbg("RAudio::RemoveProducer(%p)\n",prod);
  for(i=0;i<producers;i++)
  {
    if(prod==producer[i])
    {
//qdbg("  found at i=%d, producers=%d\n",i,producers);
      // Shift array
      for(;i<producers-1;i++)
      {
//qdbg("shift %d to %d\n",i+1,i);
        producer[i]=producer[i+1];
      }
      producers--;
      return TRUE;
    }
  }
  // Producer was not found
qdbg("  NOT found\n");
  return FALSE;
}


void RAudio::Run(float time)
{
#ifdef Q_USE_DIRECTSOUND
  // DirectSound is generating audio
#endif
#ifdef Q_USE_FMOD
  // FMOD is generating audio
#endif

#ifdef __sgi
  static void *bufAudio;
  int i,nFrames;
  int maxFrames=10000;
  
  if(!bufAudio)
  { bufAudio=qcalloc(maxFrames);
  }
  // Let producers drop their audio in the buffer
  nFrames=(int)(time*(float)frequency);
//nFrames*=2;
  if(nFrames>maxFrames)
    nFrames=maxFrames;
//qdbg("time=%f, freq=%d => frames=%d\n",time,frequency,nFrames);

  memset(bufAudio,0,nFrames);         // Channels, bits etc
  for(i=0;i<producers;i++)
  {
    producer[i]->ProduceFrames(bufAudio,nFrames);
  }
  // Pass mixed audio onto OS audio system
  // Note: Samps!=Frames (so this call is bad)
  if(port->GetFilled()==0)
  { //qdbg("** Underflow in RAudio\n");
  }
//qdbg("RAudio: filled=%d, fillable=%d\n",port->GetFilled(),port->GetFillable());
  port->WriteSamps(bufAudio,nFrames);
  
  // Assume a framerate of about 20fps
  int minFrames=frequency/20;
  if(port->GetFilled()<minFrames)
  { // Getting low on fill here; try not to get drops
    // Don't fill in more than we've got
    if(nFrames<minFrames)
    { minFrames=nFrames;
    }
    // Repeat some audio; it will glitch, but less than a drop
    port->WriteSamps(bufAudio,minFrames);
  }
#endif // __sgi
}
