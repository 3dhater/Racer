// qlib/qmsg.h - Message window

#ifndef __QLIB_MSG_H
#define __QLIB_MSG_H

#include <qlib/window.h>
#include <qlib/canvas.h>
#include <qlib/font.h>

class QMsg : public QWindow
// A message window for all kinds of messages
{
 public:
  string ClassName(){ return "msg"; }
  enum CaptureFlags
  { CP_ERROR=1,		// Capture flags
    CP_WARNING=2,
    CP_DEBUG=4,
    CP_MESSAGE=8,
    CP_ALL=CP_ERROR|CP_WARNING|CP_DEBUG|CP_MESSAGE
  };

 protected:
  int flags;
  QFont *font;			// Non-proportional
  int cx,cy;			// Cursor position
  int maxCX,maxCY;		// Size in chars
  int cwid,chgt;		// Char size
  QColor *col;			// Inside color

  void ScrollUp();

 public:
  QMsg(QWindow *parent,int x,int y,int wid,int hgt,int captMask=0);
  ~QMsg();

  // Attribs
  QFont *GetFont(){ return font; }
  void   SetFont(QFont *font);

  void Capture(int msgMask);	// Capture all kinds of messages
  void Paint(QRect *r=0);

  // Output
  void Out(cstring s);
  void printf(cstring fmt,...);
};

#endif
