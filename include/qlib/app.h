// qlib/qapp.h

#ifndef __QLIB_QAPP_H
#define __QLIB_QAPP_H

#ifdef WIN32
// Make sure winsock.h isn't used
#include <winsock.h>
#include <windows.h>
#endif


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <qlib/display.h>
#include <qlib/shell.h>
#include <qlib/font.h>
#include <qlib/state.h>
#include <qlib/fxman.h>
#include <qlib/gel.h>
#include <qlib/bc.h>
#include <qlib/video.h>

// Handy macros

// X display server
#define XDSP	(app->GetDisplay()->GetXDisplay())
// The user interface shell
#define QSHELL  (app->GetShell())
// The broadcast window
#define Q_BC	(app->GetBC())
// Default canvas of broadcast window or 0 if none
#define QCV     (app->GetBC()?app->GetBC()->GetCanvas():QSHELL->GetCanvas())
// The video server
#define QVS     (app->GetVideoServer())
// The window manager
#define QWM     (app->GetWindowManager())
// The state manager
#define QSM     (app->GetStateManager())

#ifdef WIN32
// Win32 global variables
extern HINSTANCE qHInstance;	// Program instance
#endif

class QScreen;
class QEvent;

typedef void (*QAppIdleProc)(void);
typedef bool (*QAppEventProc)(QEvent *e);
typedef void (*QAppExitProc)(void);

struct QFullScreenSettings
// A structure to set/get the screen resolution, dimensions etc
// Used in Win32 for example for fullscreen operation
{
  int width,
      height,
      bits;
};

class QApp : public QObject
// Encapsulates the things a generic application will almost always need
{
 public:
  // Creation flags
  enum CreateFlags
  { NO_UI=1,			// Don't create the basic shell
    NO_BC=2,			// Don't create a broadcast dblbuf window
    NO_SM=4,			// Don't create a statemanager
    PREF_WM=8,			// Use window manager to handle resizing etc
    FULLSCREEN=16		// Go fullscreen
  };

  enum PixelValues
  { PX_WHITE,
    PX_BLACK,
    PX_LTGRAY,PX_MDGRAY,PX_DKGRAY,
    PX_RED
  };

  // Logical pixels
  enum LogicalPixels
  { PX_UIBGR=PX_MDGRAY
  };
#ifdef OBS
  const int PX_UIBGR=PX_MDGRAY;
#endif

 protected:
  // Creation
  QRect *rUI,*rBC;
  int    createFlags;

  // UI support
  Pixel uiPixel[10];		// enum PixelValues

  string appName;		// This application's name
  QAppIdleProc idleProc;	// Idle proc
  QAppEventProc evProc;		// Event handler
  QAppExitProc  exitProc;	// Just before exiting
  QDisplay *dsp;
  QScreen  *scr;
  QFullScreenSettings fss;	// What to do when going fullscreen
  //QXWindow  **qxwin;		// Array of X windows
  QWindowManager *winmgr;	// The UI manager (1 QShell)
  QFont   *sysFont;		// System font for the UI
  QFont   *sysFontNP;		// Non-proportional font
  QShell  *shell;		// UI shell
  QCursor *cursShell;		// Cursor for shell
  QStateManager *sm;		// QApps are mostly state driven
  QEvent      event;		// Current event being processed
  int         modFlags;		// SHIFT/CTRL memorization
  int         mouseX,mouseY;    // Last seen mouse location
  int         mouseButtons;     // Mask of pressed buttons
  // Gel support
  QBC        *bc;		// Current gel container
  QCursor    *cursBC;		// Cursor for BC
  QFXManager *fm;		// Gel coordinates/filters etc.
  QFXManager *vfm;		// VideoFX manager
  QFXManager *ffm;		// FreeFX manager (FUTURE)
  QGelList *gl;
  // Digital media support (video)
  QVideoServer *vs;

  QWindow *FindQWindow(Window w);

 public:
  QApp(cstring name="qapp",cstring displayName=0);
  // Destruction
  ~QApp();
  void Destroy();		// So QApp can destroy itself at exit

  // Creation options
  void Create();		// Create most default things for you
  void PrefUIRect(QRect *r);	// Where to create the UI window
  void PrefBCRect(QRect *r);	// Where to create a gel window
  void PrefNoBC();		// Use own BC?
  void PrefNoUI();		// Don't create a shell
  void PrefNoStates();		// No VCR support and statemanager
  void PrefWM(bool yn=TRUE);	// Listen to 4Dwm?
  void PrefFullScreen();	// Use fullscreen shell? (Win32 only)

  // Info
  Pixel GetPixel(int col);
  QFullScreenSettings *GetFullScreenSettings(){ return &fss; }
  cstring              GetAppName(){ return appName; }
  QDisplay            *GetDisplay(){ return dsp; }
  QShell              *GetShell(){ return shell; }
  QWindowManager      *GetWindowManager(){ return winmgr; }
  QFont               *GetSystemFont(){ return sysFont; }
  QFont               *GetSystemFontNP(){ return sysFontNP; }
  QStateManager       *GetStateManager(){ return sm; }
  QFXManager          *GetFXManager(){ return fm; }
  QFXManager          *GetVideoFXManager(){ return vfm; }
  QFXManager          *GetFreeFXManager(){ return ffm; }
  QGelList            *GetGelList(){ return gl; }
  QBC                 *GetBC(){ return bc; }
  QEvent              *GetCurrentEvent(){ return &event; }
  int                  GetEventFD();
  QVideoServer        *GetVideoServer();

  // Pointer/cursor info
  QCursor *GetShellCursor(){ return cursShell; }
  QCursor *GetBCCursor(){ return cursBC; }
  void     GetCursorPosition(int *x,int *y);

  int GetWidth(){ return shell->GetWidth(); }
  int GetHeight(){ return shell->GetHeight(); }

  int GetVTraceCount();

  int  GetKeyModifiers();
  void GetMouseXY(int *x,int *y){ *x=mouseX; *y=mouseY; }
  int  GetMouseButtons(){ return mouseButtons; }

  // Window support for 'current' entities
  void CreateSysFonts();
  void SetShell(QShell *sh);
  void SetBC(QBC *bc);

  // Running
  void SetIdleProc(QAppIdleProc func);
  void SetEventProc(QAppEventProc func);
  QAppEventProc GetEventProc(){ return evProc; }
  void SetExitProc(QAppExitProc func);
  void Exit(int rc=0);			// Safe exit
  // Timers
  int  AddTimer(int msecs);		// Millisecs; returns timer ID
  void RemoveTimer(int id);

  // Event handling
  void Sync();				// Synchronize animation
  // Run one event tops (return: 0=event eaten, 1=event; not eaten, 2=idle)
  int  Run1();
  // Run indefinitely
  void Run(QState *stateStart=0,QState *stateGlobal=0);
  // Run until no events are left
  void RunPoll(QState *stateStart=0,QState *stateGlobal=0);

  // Shortcuts to data member functionality
  void GotoState(QState *newState);
};

// qapp.cpp global current app
extern QApp *app;

#endif

