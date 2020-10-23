/*
 * QDXSBuf - DirectSound buffer (2D/3D) (Win32 only)
 * 26-09-00: Created!
 * NOTES:
 * - May be emulated on other OSes (using OpenAL or just 2D audio)
 * (C) MarketGraph/RvG
 */

#include <qlib/dxsbuf.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDXSoundBuffer::QDXSoundBuffer(QSample *_smp,int _flags,int _alg)
// Create DX sound buffer based on sample
// It takes over the sample's size, so preferably use only this sample in this buffer
{
#ifdef WIN32
  dsBuf=0;
  dsBuf3D=0;
#endif
  flags=_flags;
  alg=_alg;
  smp=_smp;

  if(!(flags&IS3D))
  { qwarn("QDXSoundBuffer; only 3D buffers are supported");
    flags|=IS3D;
  }
  // Create for convenience of this constructor
  Create();
}

bool QDXSoundBuffer::Create()
// Create a soundbuffer
// Assumes a sample (QSample) is used to define the buffer size.
// In case of failure, FALSE is returned.
// After creation, you MAY delete your sample, as it is copied into this buffer, or you
// may hold on to it for other buffers or for other uses.
{
#ifdef WIN32
  // Setup (secondary) buffer creation information
  DSBUFFERDESC dbd;
  WAVEFORMATEX fmt;
  HRESULT hr;
  int nBytes;

  ZeroMemory(&fmt,sizeof(fmt));
  fmt.wFormatTag=WAVE_FORMAT_PCM;
  fmt.nChannels=smp->samps_per_frame;
  fmt.nSamplesPerSec=smp->file_rate;
  fmt.wBitsPerSample=smp->sampwidth;
  fmt.nBlockAlign=(fmt.wBitsPerSample/8)*fmt.nChannels;
  fmt.nAvgBytesPerSec=fmt.nSamplesPerSec*fmt.nBlockAlign;
  fmt.cbSize=0;

  ZeroMemory(&dbd,sizeof(dbd));
  dbd.dwSize=sizeof(DSBUFFERDESC);
  dbd.dwFlags=DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
  if(flags&IS3D)
  { //qlog(QLOG_INFO,"3D secondary buffer");
    dbd.dwFlags|=DSBCAPS_CTRL3D;
  }
  nBytes=smp->frames*smp->samps_per_frame*(smp->sampwidth/8);
  dbd.dwBufferBytes=nBytes;
  dbd.lpwfxFormat=&fmt;
  switch(alg)
  {
    case ALG_NO_VIRTUALIZATION: dbd.guid3DAlgorithm=DS3DALG_NO_VIRTUALIZATION; break;
    case ALG_HRTF_FULL        : dbd.guid3DAlgorithm=DS3DALG_HRTF_FULL; break;
    case ALG_HRTF_LIGHT       : dbd.guid3DAlgorithm=DS3DALG_HRTF_LIGHT; break;
  }
  // Create buffer
  if(FAILED(hr=qDXSound->GetDS()->CreateSoundBuffer(&dbd,&dsBuf,NULL)))
  {
    qerr("QDXSoundBuffer:ctor(smp); Can't create secondary sound buffer (%s)",
      qDXSound->Error2Str(hr));
    return FALSE;
  }
  if(hr==DS_NO_VIRTUALIZATION)
  {
    qwarn("QDXSoundBuffer: 3D algorithm requested is not supported on this OS.");
  }

  // Get 3D buffer from the secondary buffer
  if(FAILED(hr=dsBuf->QueryInterface(IID_IDirectSound3DBuffer,(void**)&dsBuf3D)))
  {
    qerr("QDXSoundBuffer: failed to query interface for 3D buffer (%s)",
      qDXSound->Error2Str(hr));
    return FALSE;
  }

  // Get 3D params
  params.dwSize=sizeof(DS3DBUFFER);
  dsBuf3D->GetAllParameters(&params);

  // Set new 3D params
  params.dwMode=DS3DMODE_HEADRELATIVE;
  dsBuf3D->SetAllParameters(&params,DS3D_IMMEDIATE);

  // Lock buffer
  void *data1,*data2;
  DWORD len1,len2;
  if(FAILED(hr=dsBuf->Lock(0,nBytes,&data1,&len1,&data2,&len2,0)))
  {
    qwarn("QDXSoundBuffer: can't lock buffer to copy sample (%s)",
      qDXSound->Error2Str(hr));
    return FALSE;
  }

  // Copy memory into the buffer
  memcpy(data1,smp->GetBuffer(),nBytes);

  // Unlock buffer
  dsBuf->Unlock(data1,nBytes,NULL,0);

  return TRUE;
#else
  // Not supported on non-Win32 platforms
  return FALSE;
#endif
}

QDXSoundBuffer::~QDXSoundBuffer()
// Destructor
{
  Destroy();
}

void QDXSoundBuffer::Destroy()
// Destroy buffer
{
#ifdef WIN32
  if(dsBuf3D){ dsBuf3D->Release(); dsBuf3D=0; }
  if(dsBuf){ dsBuf->Release(); dsBuf=0; }
#endif
}

bool QDXSoundBuffer::IsCreated()
// Returns TRUE if buffer is created
{
#ifdef WIN32
  if(dsBuf)return TRUE;
#endif
  return FALSE;
}

void QDXSoundBuffer::EnableLoop()
{
  flags|=LOOP;
}
void QDXSoundBuffer::DisableLoop()
{
  flags&=~LOOP;
}

/***********
* PLAYBACK *
***********/
bool QDXSoundBuffer::Play()
// Playback buffer
{
#ifdef WIN32
  HRESULT hr;
  DWORD status;

  if(!dsBuf)return FALSE;

  Restore();

  if(FAILED(hr=dsBuf->GetStatus(&status)))
  { qwarn("QDXSoundBuffer:Play(); can't get status");
    return FALSE;
  }
  if(status&DSBSTATUS_PLAYING)
  {
    // Already playing; just restart
    dsBuf->SetCurrentPosition(0);
  } else
  {
    // Play buffer
    DWORD bLoop;
    if(flags&LOOP)bLoop=DSBPLAY_LOOPING;
    else          bLoop=0;
    if(FAILED(hr=dsBuf->Play(0,0,bLoop)))
    {
      qwarn("QDXSoundBuffer::Play(); failed to play (%s)",
        qDXSound->Error2Str(hr));
      return FALSE;
    }
  }
  return TRUE;
#else
  // NYI on other OSes
  return FALSE;
#endif
}

void QDXSoundBuffer::Restore()
// Restore buffers if lost
{
#ifdef WIN32
  if(!dsBuf)return;
  HRESULT hr;
  DWORD status;
  if(FAILED(hr=dsBuf->GetStatus(&status)))
  { qwarn("QDXSoundBuffer:Restore(); can't get status");
    return;
  }
  if(status&DSBSTATUS_BUFFERLOST)
  {
    qwarn("QDXSoundBuffer:Restore(); buffer lost and no way to fill");
    return;
  }
#endif
}
void QDXSoundBuffer::Stop()
// Stop playback of buffer
{
#ifdef WIN32
  if(!dsBuf)return;
  dsBuf->Stop();
  dsBuf->SetCurrentPosition(0);
#endif
}

/**********************
* PLAYBACK PARAMETERS *
**********************/
void QDXSoundBuffer::SetFrequency(int freq)
// Set 'freq' hz as playback frequency
// Depends on the original frequency with which this buffer was created to what
// it actually does
{
#ifdef WIN32
  if(!dsBuf)return;
  dsBuf->SetFrequency(freq);
#endif
}
void QDXSoundBuffer::SetVolume(int vol)
// Set volume
// Max is vol==0, min (silence) is vol=-10000
{
#ifdef WIN32
  if(!dsBuf)return;
  dsBuf->SetVolume(vol);
#endif
}

