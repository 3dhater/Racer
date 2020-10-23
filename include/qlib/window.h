// qlib/qwindow.h
// 20-02-98: Total armageddon; QXWindow is now the base window class
// 20-12-99: Hierarchy is now QObj-QDrawable-QWindow again

#ifndef __QLIB_QWINDOW_H
#define __QLIB_QWINDOW_H

#include <qlib/drawable.h>
#include <qlib/color.h>
#include <qlib/image.h>
#include <qlib/event.h>
#include <qlib/canvas.h>
#include <X11/X.h>			// 'Window' type
#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif

class QCursor;
class QGLContext;

class QXWindow : public QDrawable
// Encapsulating the XWindow (actual X11 regions)
// A QWindow may or not have a real associated QXWindow
// Most low-level X11 window stuff should be done in this class; not
// in QWindow itself. Also, higher level Q stuff should NOT be done here
// (like Q OpenGL contexts, Q event catching etc.)
{
 protected:
 private:
#ifdef WIN32
  // Win32 attributes
  HWND     hWnd;
  HDC      hDC;
  WNDCLASS wc;
#endif
  // X11/GLX/OpenGL related
  Window xwindow;		// Actual XWindow
  XVisualInfo *vi;		// The X visual information
  QXWindow *parent;
  QGLContext *gl;		// OpenGL port
  int xflags;			// Info about whatever

  QRect ipos;			// Initial create position

 public:
  enum XFlags
  { XMANAGED=1,			// Managed by 4Dwm? (PrefWM())
    DEFVISUAL=2,		// Use default visual?
    ZBUFFER=4,			// Request OpenGL Zbuffer
    FULLSCREEN=8		// Switch to fullscreen
  };

 public:
  QXWindow(QXWindow *parent,int x,int y,int wid,int hgt);
  virtual ~QXWindow();

  // QDrawable inheritance
  GLXDrawable GetGLXDrawable();
  Drawable    GetDrawable();

  QGLContext  *GetGLContext(){ return gl; }
  XVisualInfo *GetVisualInfo(){ return vi; }
#ifdef WIN32
  HDC          GetHDC() { return hDC; }
  HWND         GetHWND(){ return hWnd; }
#endif

  // Precreate
  void PrefWM(bool wm=TRUE);	// Use window manager to manage?
  void PrefFullScreen();	// Switch to fullscreen
  void PrefDefaultVisual();	// Use WM default visual?
  void EnableZBuffer();
  void DisableZBuffer();

  // Create the actual XWindow
  bool Create();
  void Destroy();

  // Information
  bool IsX();			// Legacy; warns
  bool IsCreated();		// Valid X11 window?
  int  GetWidth();
  int  GetHeight();
  int  GetDepth();
  void GetPos(QRect *r);	// Convenience
  bool IsDirect();		// Direct or by 4Dwm?
  bool IsFullScreen();		// Fullscreen mode selected?
  QXWindow *GetParent(){ return parent; }

  // X11 basics; in which actual X window do we live?
  Window GetXWindow(){ return xwindow; }
  //void SetXWindow(Window w);	// Why would we?

  // Window manager directed windows
  void SetTitle(string name);
  void SetIconName(string name);

  int  GetX();
  int  GetY();
  void Move(int x,int y);
  void Size(int wid,int hgt);
  void Raise();
  void Lower();

  // Functionality only available for actual X windows
  void SetCursor(QCursor *c);
  void FocusKeys(int time=0);	// Set keyboard focus to this window
  virtual void Swap();		// Available for double-buffered windows
  void Map();
  void Unmap();
};

class QWindow : public QDrawable
// A Q Window; may optionally be an actual X window, or just live
// in a parent QWindow which IS an X window. This is much faster for OpenGL
// rendering. (less context switching)
{
 public:
  string ClassName(){ return "window"; }
 protected:

  // Creation
  QRect     rCreation;

  // Where are we in the hierarchy
  QXWindow *qxwin;			// Where we actually live
  QWindow  *parent;			// Direct window parent
  QWindow  *compoundWindow;		// Are we part of a compound window?
  int flags;
  int catchFlags;			// Event grabbing
  int   tabStop;			// Tab Stop index; -1=none, >=0: used
  void *userData;			// Associated user data

 public:
  // Window flags
  enum WinFlags
  { HIDDEN=1,				// Not displayed (mapped)
    DISABLED=2,				// Not functional (no events)
    ISX=4, 				// Actual X window
    CREATED=8,				// Window has been created
    COMPRESS_MOTION=0x10,		// Compress motion events
    COMPRESS_EXPOSE=0x20,		// Compress expose events
    NO_IMG_BGR=0x40,			// Don't use image to restore bgr
    XMANAGED=0x80,			// For QXWindow (only real X windows)
    FULLSCREEN=0x100			// Switch to fullscreen?
  };

  // Catch flags (events are eaten by zwindows that catch these)
  enum CatchFlags
  {
	CF_BUTTONPRESS=1,
	CF_BUTTONRELEASE=2,
	CF_MOTIONNOTIFY=4,
	CF_KEYPRESS=8,
	CF_KEYRELEASE=16,
	CF_EXPOSE=32
  };

 public:
  QWindow(QWindow *parent,int x,int y,int wid,int hgt);
  virtual ~QWindow();

  // Actually create the window system stuff
  void MakeRealX();			// Use real OS window
  void MakeSoftWindow();		// Use encapsulated window
  void PrefWM(bool yn=TRUE);		// Use X window mgr?
  void PrefFullScreen();		// Switch to fullscreen
  bool Create();
  void Destroy();

  // Attributes
  QWindow *GetParent(){ return parent; }
  void  Disable(){ flags|=DISABLED; }
  void  Enable(){ flags&=~DISABLED; }
  void *GetUserData(){ return userData; }
  void  SetUserData(void *p);
  //QCanvas *GetCanvas(){ return cv; }

  // Position
  void  GetPos(QRect *r);	// Logical pos relative to parent QWindow
  void  GetXPos(QRect *r);	// Pos relative to parent QXWindow (!)
  void  GetRootPos(QRect *r);	// Pos relative to root X window (!)
  int   GetX();
  int   GetY();
  int   GetWidth();
  int   GetHeight();
  bool  Move(int x,int y);
  bool  Size(int wid,int hgt);

  bool IsVisible();		// IsMapped for X11
  bool IsEnabled();
  bool IsDescendantOf(QWindow *root);
  bool IsFocus();		// Is the input focus currently?

  // Compound window support
  void     SetCompoundWindow(QWindow *w);
  QWindow *GetCompoundWindow();

  // X11 interaction
  QXWindow *GetQXWindow();
  bool  IsX();

  // Modifying
  void Hide();
  void Show();
  void SetImageBackground(bool yn);
  bool HasImageBackground();

  // Tab stops
  void SetTabStop(int n=-2);
  int  GetTabStop(){ return tabStop; }
  void NoTabStop();

  // Painting
  void Invalidate(QRect *r=0);
  void Restore();
  virtual void Paint(QRect *r=0);

  // Q events (Catch() is old functionality; is mostly ignored 11-3-00)
  void Catch(int catchFlags);
  bool Catches(int catchFlag);

  void CompressExpose();
  bool CompressesExpose();
  void CompressMotion();
  bool CompressesMotion();

  void PushEvent(QEvent *e);

  // Events (return: FALSE=not processed, TRUE=processed)
  virtual bool Event(QEvent *e);
  // Broken down events
  virtual bool EvButtonPress(int button,int x,int y);
  virtual bool EvButtonRelease(int button,int x,int y);
  virtual bool EvKeyPress(int key,int x,int y);
  virtual bool EvKeyRelease(int key,int x,int y);
  virtual bool EvMotionNotify(int x,int y);
  virtual bool EvExpose(int x,int y,int wid,int hgt);
  virtual bool EvResize(int wid,int hgt);

  // Keyboard focus
  virtual bool EvSetFocus();
  virtual bool EvLoseFocus();

  // Entering/leaving window (fly-over)
  virtual bool EvEnter();
  virtual bool EvLeave();
};

/*******************
* Q Window Manager *
*******************/

struct QWMInfo
// Information used to be able to popup modal dialogs
{ QWindow **winList;
  int       activeWins;
  int       maxWindow;
  QWindow  *keyboardFocus;
};
class QDialog;

class QWindowManager : public QObject
{
 protected:
  // A list of windows
  QWindow **winList;
  int       activeWins;                 // Number of active windows
  int       maxWindow;
  // Focus
  QWindow  *keyboardFocus;              // Which windows has the keyboard focus
  QWindow  *mouseCapture;		// Mouse capture window
  // Background information
  QColor   *colBG;
  QImage   *imgBgr;
  int       flags;

  enum Flags
  { HANDLE_EXPOSE=1,			// Obsolete
    NO_TAB_CONTROL=2			// Don't use TAB to focus
  };

 public:
  QWindowManager();
  ~QWindowManager();

  // Attribs
  void DisableTABControl();
  void EnableTABControl();
  bool IsTABControlEnabled();

  // Event handling
  bool ProcessEvent(QEvent *e);
  void PushEvent(QEvent *e);            // Generate your own user events

  void WaitForExposure();               // Make sure we are visible
  bool AddWindow(QWindow *win);
  bool RemoveWindow(QWindow *win);
  bool WindowExists(QWindow *win);	// Still in the list?


  void SetBackground(string fileName);
  void SetBackgroundColor(QColor *col); // Even background
  void RestoreBackground(QWindow *w,QRect *r=0);

  // Input focus
  QWindow *GetKeyboardFocus(){ return keyboardFocus; }
  bool     SetKeyboardFocus(QWindow *w);
  void     FocusFirst();
  void     FocusLast();
  void     FocusNext();
  void     FocusPrev();
  int      GetMaxTabStop();

  // Going through the list of windows
  int      GetNoofWindows();
  QWindow *GetWindowN(int n);

  // Finding windows
  QWindow *WhichWindowAt(int x,int y,int cFlag,Window xw=0);

  // New FindWindow()'s for event decomposition
  // Note that a name conflict arises with FindWindow(), which is #define'd to something
  // else (FindWindowA()/FindWindowW()) on Win32
  QWindow  *FindOSWindow(Window w,int x,int y);	// Find (child) QWindow
  QWindow  *FindXWindow(Window w);

 public:
  void Paint();                         // Repaint entire interface
  void Run();

  // Mouse capturing
  void BeginMouseCapture(QWindow *w);
  void EndMouseCapture();
  QWindow *GetMouseCapture(){ return mouseCapture; }

  // Dialog support
  void ShowDialog(QDialog *d);
  void HideDialog(QDialog *d);
};

#endif
