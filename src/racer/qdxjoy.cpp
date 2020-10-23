/*
 * QDXJoy - a DirectX joystick
 * 04-09-00: Created! (Win32) (heavily based on the JoyStImm MS SDK sample)
 * 17-08-01: Linux support added (thanks to Dominik Pospisil)
 * 22-10-01: Conversion to DX8 (Windows). What a pain. Number of FF axes
 * reduced to 1 (DInput complained that not all axes where FF-enabled).
 * NOTES:
 * - Perhaps QJoystick would be a better name, but this is tied to DirectX
 * and a more general joystick class might be needed above this one.
 * - Windows: Only attached joysticks will be found
 * - Linux: no FF is supported yet since the Linux joystick drivers
 * don't support it yet. Hopefully this will change in the future.
 * - If you find this source easy to read than you must love
 * DirectX. Changing from DirectX7 to DX8 was an unnecessary pain
 * and waste of time. This concept is a total failure.
 * ASSUMPTIONS:
 * - Expects a QSHELL to exist (Windows only; for exclusive access
 * when our window is current)
 * (C) MarketGraph/Ruud van Gaal
 */

#include <qlib/dxjoy.h>
#include <qlib/app.h>
#include <math.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/************
* JOYSTICKS *
************/
QDXJoy::QDXJoy(int _n)
  : QObject()
// Creates an interface to joystick device #n (n=index of attached controllers)
{
  int i;
qdbg("QDXJoy ctor\n");

  Name("QDXJoy");

  x=y=z=0;
  rx=ry=rz=0;
  for(i=0;i<MAX_BUTTON;i++)
    button[i]=0;
  n=_n;

#ifdef WIN32
  dxJoy=0;
  memset(&diDevCaps,0,sizeof(diDevCaps));

  // Make sure DirectInput is up
  QDXINPUT_OPEN;
  if(!qDXInput->IsOpen())return;

  //Open();
#endif
#ifdef linux
  fd=0;
#endif
}
QDXJoy::~QDXJoy()
{
  Close();
}

#ifdef WIN32
// #joysticks to skip
static int enumSkipCount;
BOOL CALLBACK EnumCallback(const DIDEVICEINSTANCE *pdidInstance,void *ctx)
// Called when a joystick is searched in QDXJoy ctor
{
  QDXJoy *joy;
  HRESULT hr;
#ifdef OBS_DX7
  LPDIRECTINPUTDEVICE2 dxJoyLocal;
#else
  // DX8
  LPDIRECTINPUTDEVICE8 dxJoyLocal;
#endif

qlog(QLOG_INFO,"Enum: controller found; skip %d more",enumSkipCount);

  // Search on for the required controller
  if(enumSkipCount>0)
  {
    enumSkipCount--;
    return DIENUM_CONTINUE;
  }

  joy=(QDXJoy*)ctx;
  // Get an interface to the enumerated joystick
#ifdef OBS_DX7
  hr=qDXInput->GetPDI()->CreateDeviceEx(pdidInstance->guidInstance,
    IID_IDirectInputDevice2,(void**)&dxJoyLocal,NULL);
#else
  // DX8
  hr=qDXInput->GetPDI()->CreateDevice(pdidInstance->guidInstance,
    &dxJoyLocal,NULL);
#endif
  // This may fail, for example, if the user just plugs out the controller
  if(FAILED(hr))
    return DIENUM_CONTINUE;
  // Store in class
  joy->_SetInputDevice(dxJoyLocal);
  // We're done
  return DIENUM_STOP;
}
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE *pdidoi,void *ctx)
// Called for every axis on the controller
{
  QDXJoy *joy;

  joy=(QDXJoy*)ctx;

  DIPROPRANGE diprg;
  diprg.diph.dwSize=sizeof(DIPROPRANGE);
  diprg.diph.dwHeaderSize=sizeof(DIPROPHEADER);
#ifdef OBS_DX7_GRMBL
  diprg.diph.dwHow=DIPH_BYOFFSET;
  diprg.diph.dwObj=pdidoi->dwOfs;   // Specify the enumerated axis
#else
  diprg.diph.dwHow=DIPH_BYID;
  diprg.diph.dwObj=pdidoi->dwType;   // Specify the enumerated axis
#endif
  diprg.lMin=-1000;
  diprg.lMax=1000;

qlog(QLOG_INFO,"QDXJoy; axis %d\n",pdidoi->dwOfs);

  // Set range for the axis
  if(FAILED(joy->GetDXJoy()->SetProperty(DIPROP_RANGE,&diprg.diph)))
  { qwarn("QDXJoy: Can't set axis range (axis=%d)",pdidoi->dwOfs);
    // Just continue nevertheless
    //return DIENUM_STOP;
    return DIENUM_CONTINUE;
  }
  if(FAILED(joy->GetDXJoy()->GetProperty(DIPROP_RANGE,&diprg.diph)))
  {
    qwarn("QDXJoy: Can't get axis range (axis=%d)",pdidoi->dwOfs);
    return DIENUM_STOP;
  }
qlog(QLOG_INFO,"QDXJoy; axis %d, range %d-%d\n",pdidoi->dwOfs,diprg.lMin,diprg.lMax);
  return DIENUM_CONTINUE;
}

#ifdef OBS_DX7
void QDXJoy::_SetInputDevice(LPDIRECTINPUTDEVICE2 joy)
#else
void QDXJoy::_SetInputDevice(LPDIRECTINPUTDEVICE8 joy)
#endif
// Used in creating the joystick device (called from EnumCallback)
{
  dxJoy=joy;
}
#endif

/**********
* ATTRIBS *
**********/
bool QDXJoy::IsOpen()
// Returns TRUE if a joystick device has been opened
{
#ifdef WIN32
  if(dxJoy)return TRUE;
#endif
#ifdef linux
  if(fd>0)return TRUE;
#endif
  return FALSE;
}

bool QDXJoy::Open()
// Open the joystick
{
#ifdef WIN32
  // Look for the controller (that is currently attached)
  HRESULT hr;

qlog(QLOG_INFO,"QDXJoy:Open()\n");

  // Make sure you skip controllers until the indexed one
  enumSkipCount=n;
  // Find all controllers, and pick out #n
#ifdef OBS_DX7
  hr=qDXInput->GetPDI()->EnumDevices(DIDEVTYPE_JOYSTICK,EnumCallback,this,
    DIEDFL_ATTACHEDONLY /*|DIEDFL_FORCEFEEDBACK*/);
#else
  hr=qDXInput->GetPDI()->EnumDevices(DI8DEVCLASS_GAMECTRL,EnumCallback,this,
    DIEDFL_ATTACHEDONLY /*|DIEDFL_FORCEFEEDBACK*/);
#endif
  if(FAILED(hr)||dxJoy==0)
  { qwarn("QDXJoy: can't create/find joystick #%d",n);
    return FALSE;
  }

  // Set data format to "simple joystick"
  hr=dxJoy->SetDataFormat(&c_dfDIJoystick);
  if(FAILED(hr))
  { qwarn("QDXJoy: can't set data format for joystick #%d",n);
    return FALSE;
  }

  // Set cooperative level
  // Currently we want exclusive access for our main window
  hr=dxJoy->SetCooperativeLevel(QSHELL->GetQXWindow()->GetHWND(),
    //DISCL_EXCLUSIVE|DISCL_BACKGROUND);
    DISCL_EXCLUSIVE|DISCL_FOREGROUND);
  if(FAILED(hr))
  { qwarn("QDXJoy: can't set cooperative level for joystick #%d (%s)",
      n,qDXInput->Err2Str(hr));
    qlog(QLOG_INFO,"hdc=0x%x, err_invalidparam=%x, notinitialized=%x, e_handle=%x\n",
      QSHELL->GetQXWindow()->GetHDC(),DIERR_INVALIDPARAM,DIERR_NOTINITIALIZED,E_HANDLE);
    return FALSE;
  } else
  {
    qlog(QLOG_INFO,"hr=%x (%s)\n",hr,qDXInput->Err2Str(hr));
  }

  // Determine how many axes the joystick has
  diDevCaps.dwSize=sizeof(DIDEVCAPS);
  hr=dxJoy->GetCapabilities(&diDevCaps);
  if(FAILED(hr))
  { qwarn("QDXJoy: can't get capabilities for joystick #%d",n);
    return FALSE;
  }
qlog(QLOG_INFO,"Controller #%d found, FF=%d\n",n,IsForceFeedback());
//qdbg("Right after ***\n");

  // Enumerate the axes and set the range of all axes to match something decent
  dxJoy->EnumObjects(EnumAxesCallback,(void*)this,DIDFT_AXIS);
#endif

#ifdef linux
qdbg("QDXJoy: opening joystick dev");
  char devName[50];
  // Single device for now
  strcpy(devName,"/dev/js0");
qdbg("  n=%d, devName='%s'\n",n,devName);
  fd=open(devName,O_RDONLY|O_NONBLOCK);
  if(fd<0)
  {
    qwarn("QDXJoy: can't create/find joystick #%d",n);
    return FALSE;
  }
  char joyName[128];
  if(ioctl(fd,JSIOCGNAME(sizeof(joyName)),joyName)<0)
    strcpy(joyName,"<unknown>");
qdbg("  joystick name: %s\n",joyName);
#endif

  return TRUE;
}

void QDXJoy::Close()
// Close the joystick device
{
#ifdef WIN32
  if(dxJoy)
  {
    Unacquire();
    dxJoy->Release();
    dxJoy=0;
  }
#endif
#ifdef linux
  if(fd>0)
  {
    close(fd);
    fd=0;
  }
#endif
}

/**********
* ACQUIRE *
**********/
bool QDXJoy::Acquire()
// Acquire the joystick for use
{
#ifdef WIN32
  if(dxJoy)
  { dxJoy->Acquire();
  } else return FALSE;
#endif
  return TRUE;
}
bool QDXJoy::Unacquire()
// Unacquire the joystick; somebody else may have it
{
#ifdef WIN32
  if(dxJoy)
  { dxJoy->Unacquire();
  } else return FALSE;
#endif
  return TRUE;
}

/**********
* POLLING *
**********/
void QDXJoy::Poll()
{
#ifdef WIN32
  DIJOYSTATE js;
  HRESULT hr;

  if(!dxJoy)return;

  // Hm, does this ever end the loop?
  do
  {
    hr=dxJoy->Poll();
    if(FAILED(hr))
    {
      static int count;
      // Don't overdue error reporting
      if(count<5)
      { qwarn("QDXJoy:Poll(); can't poll controller (%s)",qDXInput->Err2Str(hr));
        if(++count==5)
          qwarn("QDXJoy: too many poll failures; no more reporting");
      }
      //return;
    }

    // Get the device's state
    hr=dxJoy->GetDeviceState(sizeof(DIJOYSTATE),&js);
    if(hr==DIERR_INPUTLOST)
    {
      // Input stream has been interrupted
      // Reacquire and try to move on
      hr=dxJoy->Acquire();
      if(FAILED(hr))
      { qwarn("QDXJoy:Poll(); input lost and can't reacquire, err=%x",hr);
        return;
      }
    }
  } while(hr==DIERR_INPUTLOST);

  if(FAILED(hr))
  { 
    qwarn("QDXJoy: Can't poll controller");
    return;
  }
  // Take over joystick state
  x=js.lX;
  y=js.lY;
  z=js.lZ;
  rx=js.lRx;
  ry=js.lRy;
  rz=js.lRz;
  slider[0]=js.rglSlider[0];
  slider[1]=js.rglSlider[1];
  for(int i=0;i<MAX_BUTTON;i++)
    button[i]=(js.rgbButtons[i]&0x80)>>7;
  // There are still 2 axes and the POV which we may use...
#endif

#ifdef linux
  js_event e;
  while(read(fd,&e,sizeof(e))>0)
  {
    e.type&=~JS_EVENT_INIT;
    switch(e.type)
    {
      case JS_EVENT_BUTTON: button[e.number]=e.value; break;
      case JS_EVENT_AXIS:
        switch(e.number)
        { case 0: x=((int)e.value)*1000/32767; break;
          case 1: y=((int)e.value)*1000/32767; break;
          case 2: z=((int)e.value)*1000/32767; break;
          case 3: rx=((int)e.value)*1000/32767; break;
          case 4: ry=((int)e.value)*1000/32767; break;
          case 5: rz=((int)e.value)*1000/32767; break;
        }
        break;
     }
  }
#endif
}

/*************************
* FORCE FEEDBACK SUPPORT *
*************************/
bool QDXJoy::IsForceFeedback()
// Returns TRUE if the joystick is capable of force feedback
{
#if defined(linux) || defined(__sgi)
  // No force feedback is provided yet. Hope somebody can add
  // that to the joystick/joy-sidewinder etc. drivers.
  return FALSE;
#endif

#ifdef WIN32
  if(!dxJoy)return FALSE;
  // Capabilities are already reported in diDevCaps;
  if(diDevCaps.dwFlags&DIDC_FORCEFEEDBACK)
    return TRUE;
  return FALSE;
#endif
}

bool QDXJoy::DisableAutoCenter()
// Turn off auto-centering for joystick
// Returns TRUE if succesful
{
#ifdef WIN32
  DIPROPDWORD dw;
  HRESULT hr;

  dw.diph.dwSize=sizeof(DIPROPDWORD);
  dw.diph.dwHeaderSize=sizeof(DIPROPHEADER);
  dw.diph.dwObj=0;
  dw.diph.dwHow=DIPH_DEVICE;
  dw.dwData=DIPROPAUTOCENTER_OFF;
  hr=dxJoy->SetProperty(DIPROP_AUTOCENTER,&dw.diph);
  if(FAILED(hr))
  { qwarn("QDXJoy:DisableAutoCenter() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  return FALSE;
#endif
}
bool QDXJoy::EnableAutoCenter()
// Turn on auto-centering for joystick
// Returns TRUE if succesful
{
#ifdef WIN32
  DIPROPDWORD dw;
  HRESULT hr;

  dw.diph.dwSize=sizeof(DIPROPDWORD);
  dw.diph.dwHeaderSize=sizeof(DIPROPHEADER);
  dw.diph.dwObj=0;
  dw.diph.dwHow=DIPH_DEVICE;
  dw.dwData=DIPROPAUTOCENTER_ON;
  hr=dxJoy->SetProperty(DIPROP_AUTOCENTER,&dw.diph);
  if(FAILED(hr))
  { qwarn("QDXJoy:DisableAutoCenter() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  return FALSE;
#endif
}

/*************************
* FORCE FEEDBACK EFFECTS *
*************************/
QDXFFEffect::QDXFFEffect(QDXJoy *joy)
{
#ifdef WIN32
  dxJoy=joy;
  pDIEffect=0;
  memset(&force,0,sizeof(force));
#endif
}
QDXFFEffect::~QDXFFEffect()
{
#ifdef WIN32
  Release();
#endif
}

void QDXFFEffect::Start()
{
#ifdef WIN32
  if(pDIEffect)
  { HRESULT hr;
    hr=pDIEffect->Start(1,0);
    if(FAILED(hr))
    { qwarn("Can't start FF effect (%s)",qDXInput->Err2Str(hr));
    }
  }
#endif
}
void QDXFFEffect::Stop()
{
#ifdef WIN32
  if(pDIEffect)
    pDIEffect->Stop();
#endif
}
void QDXFFEffect::Release()
{
#ifdef WIN32
  if(pDIEffect)
  { pDIEffect->Release();
    pDIEffect=0;
  }
#endif
}

bool QDXFFEffect::SetupConstantForce()
// Create an effect for a constant force. Returns TRUE if ok.
{
#ifdef WIN32
  HRESULT hr;
  DWORD rgdwAxes[2]={ DIJOFS_X,DIJOFS_Y };
  LONG  rglDirection[2]={ 0,0 };

  ZeroMemory(&diEffect,sizeof(diEffect));
  force.constantForce.lMagnitude=0;
//force.constantForce.lMagnitude=5000;
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration=INFINITE;
  diEffect.dwSamplePeriod=0;
  diEffect.dwGain=DI_FFNOMINALMAX;
  diEffect.dwTriggerButton=DIEB_NOTRIGGER;
  diEffect.dwTriggerRepeatInterval=0;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rgdwAxes=rgdwAxes;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DICONSTANTFORCE);
  diEffect.lpvTypeSpecificParams=&force.constantForce;
  diEffect.dwStartDelay=0;

  hr=dxJoy->GetDXJoy()->CreateEffect(GUID_ConstantForce,&diEffect,&pDIEffect,0);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:SetupConstantForce() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}

bool QDXFFEffect::UpdateConstantForce(int fx,int fy)
// Update the constant force effect
{
#ifdef WIN32
  HRESULT hr;
  LONG  rglDirection[2]={ fx,fy };
  int len;

  if(!pDIEffect)return FALSE;

  ZeroMemory(&diEffect,sizeof(diEffect));
  len=sqrt((float)(fx*fx+fy*fy));
  // For DX8, if only 1 FF axis is present, apply only
  // one direction and keep the direction at zero.
  // This, for now, is always (just 1 FF axis used).
  // This is different from DX7 behavior (!).
  if(1)
  {
    force.constantForce.lMagnitude=fx;
    rglDirection[0]=0;
  }
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DICONSTANTFORCE);
  diEffect.lpvTypeSpecificParams=&force.constantForce;
  diEffect.dwStartDelay=0;

//qlog(QLOG_INFO,"UpdCF; fx=%d, fy=%d, mag=%d\n",fx,fy,len);
  hr=pDIEffect->SetParameters(&diEffect,
    DIEP_DIRECTION|DIEP_TYPESPECIFICPARAMS|DIEP_NORESTART);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:UpdateConstantForce() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}

bool QDXFFEffect::SetupFrictionForce(int f)
// Create an effect for a friction force. Returns TRUE if ok.
{
#ifdef WIN32
  HRESULT hr;
  DWORD rgdwAxes[2]={ DIJOFS_X,DIJOFS_Y };
  LONG  rglDirection[2]={ 0,0 };

  ZeroMemory(&diEffect,sizeof(diEffect));
  force.conditionForce.lOffset=0;
  force.conditionForce.lPositiveCoefficient=f;
  force.conditionForce.lNegativeCoefficient=f;
  force.conditionForce.dwPositiveSaturation=0;
  force.conditionForce.dwNegativeSaturation=0;
  force.conditionForce.lDeadBand=0;
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration=INFINITE;
  diEffect.dwSamplePeriod=0;
  diEffect.dwGain=DI_FFNOMINALMAX;
  diEffect.dwTriggerButton=DIEB_NOTRIGGER;
  diEffect.dwTriggerRepeatInterval=0;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rgdwAxes=rgdwAxes;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DICONDITION);
  diEffect.lpvTypeSpecificParams=&force.conditionForce;
  diEffect.dwStartDelay=0;

  hr=dxJoy->GetDXJoy()->CreateEffect(GUID_Friction,&diEffect,&pDIEffect,0);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:SetupFrictionForce() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}

bool QDXFFEffect::SetupInertiaForce(int f)
// Create an effect for an inertia force. Returns TRUE if ok.
{
#ifdef WIN32
  HRESULT hr;
  DWORD rgdwAxes[2]={ DIJOFS_X,DIJOFS_Y };
  LONG  rglDirection[2]={ 0,0 };

  ZeroMemory(&diEffect,sizeof(diEffect));
  force.constantForce.lMagnitude=f;
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration=INFINITE;
  diEffect.dwSamplePeriod=0;
  diEffect.dwGain=DI_FFNOMINALMAX;
  diEffect.dwTriggerButton=DIEB_NOTRIGGER;
  diEffect.dwTriggerRepeatInterval=0;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rgdwAxes=rgdwAxes;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DICONSTANTFORCE);
  diEffect.lpvTypeSpecificParams=&force.constantForce;
  diEffect.dwStartDelay=0;

  hr=dxJoy->GetDXJoy()->CreateEffect(GUID_Inertia,&diEffect,&pDIEffect,0);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:SetupInertiaForce() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}

/******************
* Periodic forces *
******************/
bool QDXFFEffect::SetupPeriodicForce(int type)
// Create an effect for a periodic force. Returns TRUE if ok.
// type=TRIANGLE, SQUARE etc.
{
#ifdef WIN32
  HRESULT hr;
  DWORD rgdwAxes[2]={ DIJOFS_X,DIJOFS_Y };
  LONG  rglDirection[2]={ 0,0 };

  ZeroMemory(&diEffect,sizeof(diEffect));
  //force.periodicForce.dwMagnitude=10000;
  force.periodicForce.dwMagnitude=0;
  force.periodicForce.lOffset=0;
  force.periodicForce.dwPhase=0;
  force.periodicForce.dwPeriod=100000;
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration=INFINITE;
  diEffect.dwSamplePeriod=0;
  diEffect.dwGain=DI_FFNOMINALMAX;
  diEffect.dwTriggerButton=DIEB_NOTRIGGER;
  diEffect.dwTriggerRepeatInterval=0;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rgdwAxes=rgdwAxes;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DIPERIODIC);
  diEffect.lpvTypeSpecificParams=&force.periodicForce;
  diEffect.dwStartDelay=0;

  // Select GUID based on type
  //switch(){ case TRIANGLE... }

  // Create actual effect
  hr=dxJoy->GetDXJoy()->CreateEffect(GUID_Triangle,&diEffect,&pDIEffect,0);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:SetupInertiaForce() failed (%s)",qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}
bool QDXFFEffect::UpdatePeriodicForce(int magnitude,int period)
// Adjust periodic force
{
#ifdef WIN32
  HRESULT hr;
  LONG  rglDirection[2]={ 0,0 };

  ZeroMemory(&diEffect,sizeof(diEffect));
  // Pass through effect-specific seems to have a problem
  // that the offset and phase are sent, so the effect
  // continually (effectively) restarts.
  force.periodicForce.dwMagnitude=magnitude;
  //force.periodicForce.lOffset=0;
  //force.periodicForce.dwPhase=0;
  force.periodicForce.dwPeriod=period;
  diEffect.dwSize=sizeof(DIEFFECT);
  diEffect.dwFlags=DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
  diEffect.cAxes=2;
diEffect.cAxes=1;
  diEffect.rglDirection=rglDirection;
  diEffect.lpEnvelope=0;
  diEffect.cbTypeSpecificParams=sizeof(DIPERIODIC);
  diEffect.lpvTypeSpecificParams=&force.periodicForce;
  diEffect.dwStartDelay=0;

  diEffect.dwGain=magnitude;
  diEffect.dwSamplePeriod=period;

  hr=pDIEffect->SetParameters(&diEffect,
    //DIEP_GAIN|DIEP_SAMPLEPERIOD|DIEP_NORESTART);
    DIEP_TYPESPECIFICPARAMS|DIEP_NORESTART);
  if(FAILED(hr))
  {
    qwarn("QDXFFEffect:UpdatePeriodicForce() failed (%s)",
      qDXInput->Err2Str(hr));
    return FALSE;
  }
  return TRUE;
#else
  // IRIX/Linux
  return FALSE;
#endif
}

