// qlib/dxinput.h - Win32 only

#ifndef __QLIB_DXINPUT_H
#define __QLIB_DXINPUT_H

#ifdef WIN32
// From a tip of Kankris, DX7 behavior is forced using
// the following define. Since DX8 is now used, it is
// no longer needed.
//#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#endif

#include <qlib/types.h>
#include <qlib/object.h>

// Globals
class QDXInput;
extern QDXInput *qDXInput;

class QDXInput : public QObject
{
#ifdef WIN32
  //LPDIRECTINPUT7 pdi;
  LPDIRECTINPUT8 pdi;
#endif
 public:
  QDXInput();
  ~QDXInput();

#ifdef WIN32
  LPDIRECTINPUT8 GetPDI(){ return pdi; }
#endif

  bool Open();
  void Close();
  bool IsOpen();

  // General functions
  cstring Err2Str(int errnr);
};

// Handy macros to make sure the manager is there and not
#define QDXINPUT_OPEN {if(!qDXInput)new QDXInput();}
#define QDXINPUT_CLOSE {if(qDXInput)delete qDXInput;}

#endif
