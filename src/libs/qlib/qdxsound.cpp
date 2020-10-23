/*
 * QDXSound - DirectX sound abstraction
 * 25-09-00: Created! (Win32)
 * FUTURE:
 * - Select/enumerate audio devices
 * NOTES:
 * - Stubbed on anything !Win32 (to fail)
 * (C) MarketGraph/Ruud van Gaal
 */

#include <qlib/dxsound.h>
#include <qlib/app.h>
#include <qlib/debug.h>
// SDL support for Linux sound (for example)
#ifdef Q_USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif
DEBUG_ENABLE

QDXSound *qDXSound;

QDXSound::QDXSound()
{
  // There can be only one
  if(qDXSound)
  { qerr("QDXSound: multiple objects created");
    delete qDXSound;
  }
  qDXSound=this;

#ifdef WIN32
  ds=0;
  listener=0;
#endif

  // Defaults
  frequency=22050;
  channels=2;
  bits=16;
#ifdef Q_USE_SDL
  // No 3D support in SDL (?)
  flags=0;
#else
  flags=USE3D;
#endif
}
QDXSound::~QDXSound()
{
  if(IsOpen())
    Close();
}

bool QDXSound::Open()
// Returns TRUE if DirectSound could be opened with the given settings
{
#ifdef WIN32
  HRESULT hr;
  LPDIRECTSOUNDBUFFER sbPrimary;

  // Initialize COM
  if(hr=CoInitialize(NULL))
  { qerr("QDXSound:Open(); can't initialize COM (%s)",Error2Str(hr));
    return FALSE;
  }

  // Use default device
  if(FAILED(hr=DirectSoundCreate(NULL,&ds,NULL)))
  { qerr("QDXSound:Open(); Can't create DirectSound (%s)",Error2Str(hr));
    return FALSE;
  }

  // Set cooperative level
  if(FAILED(hr=ds->SetCooperativeLevel(QSHELL->GetQXWindow()->GetHWND(),DSSCL_PRIORITY)))
  { qerr("QDXSound:Open(); can't set DSSCL_PRIORITY cooperative level, hr=0x%x",hr);
    return FALSE;
  }

  // Obtain primary buffer
  DSBUFFERDESC dbd;
  ZeroMemory(&dbd,sizeof(dbd));
  dbd.dwSize=sizeof(DSBUFFERDESC);
  dbd.dwFlags=DSBCAPS_PRIMARYBUFFER;
  if(flags&USE3D)
  { //qlog(QLOG_INFO,"3D primary buffer");
    dbd.dwFlags|=DSBCAPS_CTRL3D;
  }
  if(FAILED(hr=ds->CreateSoundBuffer(&dbd,&sbPrimary,NULL)))
  {
    qerr("QDXSound:Open(); Can't create primary sound buffer (%s)",Error2Str(hr));
    return FALSE;
  }

  // Get 3D listener
  if(FAILED(hr=sbPrimary->QueryInterface(IID_IDirectSound3DListener,(void**)&listener)))
  {
    qerr("QDXSound:Open(); Can't query interface for 3D listener, hr=0x%x",hr);
    return FALSE;
  }
  // Get listener params
  listenerParams.dwSize=sizeof(DS3DLISTENER);
  listener->GetAllParameters(&listenerParams);

  // Set primary buffer format (frequency, bits etc)
  WAVEFORMATEX wfx;
  ZeroMemory(&wfx,sizeof(wfx));
  wfx.wFormatTag=WAVE_FORMAT_PCM;
  wfx.nChannels=channels;
  wfx.nSamplesPerSec=frequency;
  wfx.wBitsPerSample=bits;
  wfx.nBlockAlign=wfx.wBitsPerSample/8*wfx.nChannels;
  wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;
  if(FAILED(hr=sbPrimary->SetFormat(&wfx)))
  { qerr("QDXSound:Open(); Can't set format of primary sound buffer, hr=0x%x",hr);
    return FALSE;
  }

  // Release primary buffer (not needed anymore, it seems)
  if(sbPrimary)
  { sbPrimary->Release();
    sbPrimary=0;
  }

  // Phew, we've come through!
  return TRUE;
#else
  // Other OS'es
  return FALSE;
#endif
}
void QDXSound::Close()
{
#ifdef WIN32
  qlog(QLOG_INFO,"QDXSound:Close(); closing DirectSound/COM\n");

  if(listener)listener->Release();
  if(ds)ds->Release();

  // Uninit COM
  CoUninitialize();
#endif
}

bool QDXSound::IsOpen()
{
#ifdef WIN32
  if(ds)return TRUE;
#endif
  return FALSE;
}

/****************
* ERROR STRINGS *
****************/
cstring QDXSound::Error2Str(int hr)
{
  static char buf[40];

#ifdef WIN32
  switch(hr)
  { case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
    case DSERR_ALLOCATED: return "DSERR_ALLOCATED";
    case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
    case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
    case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
    case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
    case DSERR_GENERIC: return "DSERR_GENERIC";
    // Newer .h files?
    //case DSERR_HWUNAVAIL: return "DSERR_HWUNAVAIL";
    case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
    case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
    case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
    case DSERR_NODRIVER: return "DSERR_NODRIVER";
    case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
    case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
    case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
    case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
    case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
    case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
    default: sprintf(buf,"DSERR 0x%x",hr); return buf;
  }
#else
  // Non-Win32
  return "DSERR not supported on this OS";
#endif
}
