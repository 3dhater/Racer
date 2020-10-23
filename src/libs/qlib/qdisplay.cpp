/*
 * QDisplay.cpp - a connection to a display server (QDisplayServer?)
 * 04-10-96: Created!
 * 28-04-97: X11
 * NOTES:
 * - Multiple displays can be opened by 1 app.
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/display.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <qlib/debug.h>
DEBUG_ENABLE

class QDisplayInternal
{public:
  Display *xdisplay;
};

QDisplay::QDisplay(cstring _name)
{
  name=0;
  //name=qstrdup(_name);

  internal=(QDisplayInternal*)qcalloc(sizeof(*internal));
  QASSERT_V(internal);
#ifdef WIN32
  internal->xdisplay=(Display*)1234;
#else
  // X11
  internal->xdisplay=XOpenDisplay(_name);
#endif
}
QDisplay::~QDisplay()
{
  if(internal)qfree(internal);
  if(name)qfree(name);
}

int QDisplay::GetNoofScreens()
// Should return the number of actual screens connected
{
  return 1;
}

Display *QDisplay::GetXDisplay()
{
  QASSERT_0(internal);
  return internal->xdisplay;
}

