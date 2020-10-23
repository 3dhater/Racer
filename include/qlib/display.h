// qlib/qdisplay.h
// A connection to a display server

#ifndef __QLIB_QDISPLAY_H
#define __QLIB_QDISPLAY_H

#include <qlib/object.h>
//#include <X11/Xutil.h>

class QDisplayInternal;
// Display definition
struct _XDisplay;
typedef struct _XDisplay Display;

class QDisplay : public QObject
{
 protected:
  QDisplayInternal *internal;
  // X11
  //Display *xDisplay;

  string name;
  //QApp *app;			// This display's app

 public:
  QDisplay(cstring name=0);
  ~QDisplay();

  // Info
  Display *GetXDisplay();

  // Attributes
  //QApp *GetApp(){ return app; }
  int GetNoofScreens();
};

#endif

