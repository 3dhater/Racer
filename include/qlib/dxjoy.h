// qlib/dxjoy.h - Win32 only Joystick for DirectInput

#ifndef __QLIB_DXJOY_H
#define __QLIB_DXJOY_H

#include <qlib/dxinput.h>

#ifdef linux
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// Force feedback effects
class QDXJoy;

#define QDXFF_MAXFORCE    10000

class QDXFFEffect
{
 protected:
#ifdef WIN32
  QDXJoy *dxJoy;                     // For which joystick?
  LPDIRECTINPUTEFFECT pDIEffect;
  DIEFFECT   diEffect;               // Parameters
  DIENVELOPE diEnvelope;
  union force
  {
    DICUSTOMFORCE   customForce;
    DICONSTANTFORCE constantForce;
    DIPERIODIC      periodicForce;
    DICONDITION     conditionForce;
  } force;
#endif

 public:
  enum PeriodicTypes
  // Types of periodic forces
  {
    TRIANGLE,
    SQUARE,
    SINE
  };

  QDXFFEffect(QDXJoy *joy);
  ~QDXFFEffect();

  void Start();
  void Stop();
  void Release();

  // Convenience creation
  bool SetupConstantForce();
  bool UpdateConstantForce(int fx,int fy);
  bool SetupInertiaForce(int force);
  bool SetupFrictionForce(int force);
  bool SetupPeriodicForce(int type=TRIANGLE);
  bool UpdatePeriodicForce(int magnitude,int period);
};

class QDXJoy : public QObject
{
 protected:
#ifdef WIN32
#ifdef OBS_DX7
  LPDIRECTINPUTDEVICE2 dxJoy;
#else
  // DX8
  LPDIRECTINPUTDEVICE8 dxJoy;
#endif
  DIDEVCAPS diDevCaps;
#endif
#ifdef linux
  int fd;               // File descriptor for joystick device
#endif

  int n;		// Joystick/controller index
 public:
  enum Max
  { MAX_BUTTON=32	// Max of DirectX7a DIJOYSTATE struct
  };

  // P olled state
  int  x,y,z;		// Position
  int  rx,ry,rz;	// Rotation
  int  slider[2];       // Slider values
  int  pov[4];          // 4 point-of-view hats
  char button[MAX_BUTTON];

 public:
  QDXJoy(int n=0);
  ~QDXJoy();

  // Internal creation functions
#ifdef WIN32
#ifdef OBS_DX7
  void _SetInputDevice(LPDIRECTINPUTDEVICE2 joy);
  LPDIRECTINPUTDEVICE2 GetDXJoy(){ return dxJoy; }
#else
  void _SetInputDevice(LPDIRECTINPUTDEVICE8 joy);
  LPDIRECTINPUTDEVICE8 GetDXJoy(){ return dxJoy; }
#endif
#endif
#ifdef linux
  int  GetFD(){ return fd; }
#endif

  // Attribs
  bool IsOpen();

  bool Open();
  void Close();

  bool Acquire();
  bool Unacquire();

  void Poll();

  // Force feedback support
  bool IsForceFeedback();
  bool DisableAutoCenter();
  bool EnableAutoCenter();
};

#endif

