/*
 * QDXInput - DirectX input abstraction
 * 04-09-00: Created! (Win32)
 * NOTES:
 * - Stubbed on anything !Win32 (to succeed)
 * (C) MarketGraph/Ruud van Gaal
 */

#include <qlib/dxinput.h>
#include <qlib/app.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Global DirectInput manager
QDXInput *qDXInput;

QDXInput::QDXInput()
  : QObject()
{
  Name("QDXInput");

  // There can be only one
  if(qDXInput)
  { qwarn("QDXInput: multiple objects created");
    delete qDXInput;
  }
  qDXInput=this;
#ifdef WIN32
  pdi=0;
#endif
  Open();
}

QDXInput::~QDXInput()
{
  Close();
  // Let programs now we're gone
  qDXInput=0;
}

bool QDXInput::Open()
// Creates the manager
{
#ifdef WIN32
  // Create DirectInput manager
  HRESULT hr;

#ifdef OBS_DX7
  // Register with DInput subsystem, use the latest version (compile time define)
  hr=DirectInputCreateEx(qHInstance,DIRECTINPUT_VERSION,IID_IDirectInput7,
    (LPVOID*)&pdi,NULL);
#endif
  // DirectX8
  hr=DirectInput8Create(qHInstance,DIRECTINPUT_VERSION,IID_IDirectInput8,
    (VOID**)&pdi,0);
  if(FAILED(hr))
  { pdi=0;
    qerr("Can't open DirectInput");
    return FALSE;
  }
#endif
  return TRUE;
}

bool QDXInput::IsOpen()
// Returns TRUE if DirectInput manager is open
{
#ifdef WIN32
  if(pdi)return TRUE;
#endif
  return FALSE;
}

void QDXInput::Close()
{
#ifdef WIN32
  if(pdi)
  {
    pdi->Release();
    pdi=0;
  }
#endif
}

cstring QDXInput::Err2Str(int n)
// Converts error to string
{
#ifdef WIN32
  static char buf[40];
  switch(n)
  { 
    case DIERR_DEVICENOTREG: return "DIERR_DEVICENOTREG";
    case DIERR_DEVICEFULL: return "DIERR_DEVICEFULL";
    case DIERR_INVALIDPARAM: return "DIERR_INVALIDPARAM";
    case DIERR_NOTINITIALIZED: return "DIERR_NOTINITIALIZED";
    case DIERR_INCOMPLETEEFFECT: return "DIERR_INCOMPLETEEFFECT";
    case DIERR_INPUTLOST: return "DIERR_INPUTLOST";
    case DIERR_EFFECTPLAYING: return "DIERR_EFFECTPLAYING";
    case DIERR_NOTEXCLUSIVEACQUIRED: return "DIERR_NOTEXCLUSIVEACQUIRED";
    case DIERR_UNSUPPORTED: return "DIERR_UNSUPPORTED";
    case DIERR_OBJECTNOTFOUND: return "DIERR_OBJECTNOTFOUND";
    // Non-errors
    case DI_OK: return "DI_OK";
    case DI_EFFECTRESTARTED: return "DI_EFFECTRESTARTED";
    case DI_DOWNLOADSKIPPED: return "DI_DOWNLOADSKIPPED";
    case DI_TRUNCATED: return "DI_TRUNCATED";
    case DI_PROPNOEFFECT: return "DI_PROPNOEFFECT";
    case DI_TRUNCATEDANDRESTARTED: return "DI_TRUNCATEDANDRESTARTED";
    // Others
    default:
      sprintf(buf,"DIERR_%x",n); return buf;
  }
#else
  return "DInput not support on this OS";
#endif
}

