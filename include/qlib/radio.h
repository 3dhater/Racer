// qlib/radio.h

#ifndef __QLIB_RADIO_H
#define __QLIB_RADIO_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

class QRadio : public QWindow
// On/off (grayed?) check button
{
 public:
  string ClassName(){ return "radio"; }

 protected:
  enum State
  { ARMED=1,
    DISARMED=0
  };

  QColor  *colText;
  int      state;
  int      chkState;		// 0, 1 or 2 (grayed)
  string   text;		// If textual button
  QFont   *font;
  int      scKey,scMod;		// Shortcut key
  int      eventType;		// Generated event
  int      group;		// Group related buttons

 public:
  QRadio(QWindow *parent,QRect *pos=0,string text=0,int group=0);
  ~QRadio();

  int  GetState(){ return chkState; }
  void SetState(bool onOff);		// On/off
  void SetText(string text);
  void SetTextColor(QColor *col);
  void SetGroup(int n){ group=n; }
  int  GetGroup(){ return group; }
  void Paint(QRect *r=0);

  void ShortCut(int key,int modX=0);	// Define shortcut key
  void SetEventType(int eType);		// User events

  // Overrules events
  bool EvButtonPress(int button,int x,int y);
  bool EvButtonRelease(int button,int x,int y);
  bool EvKeyPress(int key,int x,int y);
};

#endif
