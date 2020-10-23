// qlib/button.h

#ifndef __QLIB_CHECK_H
#define __QLIB_CHECK_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

class QCheck : public QWindow
// On/off (grayed?) check button
{
 public:
  string ClassName(){ return "check"; }

 protected:
  enum State
  { ARMED=1,
    DISARMED=0
  };

#ifdef OBS
  QColor  *colUL,*colLR;
  QColor  *ciUL,*ciUR,		// Colors of inside rectangle
          *ciLR,*ciLL;
  QColor  *colShadow1,*colShadow2;
#endif
  QColor  *colText;
  int      state;
  int      chkState;		// 0, 1 or 2 (grayed)
  string   text;		// If textual button
  QFont   *font;
  int      scKey,scMod;		// Shortcut key
  int      eventType;		// Generated event

 public:
  QCheck(QWindow *parent,QRect *pos=0,string text=0);
  ~QCheck();

  int  GetState(){ return chkState; }
  void SetState(bool onOff);            // On/off

  void SetText(string text);
  void SetTextColor(QColor *col);
  void Paint(QRect *r=0);

  void ShortCut(int key,int modX=0);	// Define shortcut key
  void SetEventType(int eType);		// User events

  // Overrules events
  bool EvButtonPress(int button,int x,int y);
  bool EvButtonRelease(int button,int x,int y);
  bool EvKeyPress(int key,int x,int y);
};

#endif
