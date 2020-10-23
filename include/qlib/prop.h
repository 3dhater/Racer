// qlib/prop.h - Proportional gadget

#ifndef __QLIB_PROP_H
#define __QLIB_PROP_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

class QProp : public QWindow
{
 public:
  string ClassName(){ return "prop"; }
 protected:
  enum State
  { IDLE,
    DRAGGING
  };

 public:
  // Prop type
  enum Type
  { HORIZONTAL,
    VERTICAL,
    FREE            // Hor/vert free movement
  };

  enum PropFlags
  { SHOW_VALUE=1		// Show value in slider?
  };

 protected:
  int      propFlags;
  int      state;
  int      type;		// Hor/vert/free
  int      min[2],max[2];	// Ranges to drag
  int      dsp[2];		// Displayed range
  int      jmp[2];		// Jump speed
  int      cur[2];		// Position in min..max (not the pixel x/y!)
  int      dragX,dragY;		// While dragging
  qstring  text;                // Contents

  int GetDir(int dir);
  void CalcWindowBox(QRect *r);

 public:
  QProp(QWindow *parent,QRect *pos=0,int type=HORIZONTAL,int propFlags=0);
  ~QProp();

  // Attribs
  int  GetPosition(int dir=-1);
  void SetRange(int min,int max,int dir=-1);
  void SetDisplayed(int n,int outOf,int dir=-1);	// n/outOf
  void SetJump(int n,int dir=-1);
  void SetPosition(int pos,int dir=-1);
  void DisableShowValue();
  void EnableShowValue();
  cstring GetText(){ return text.cstr(); }
  void    SetText(cstring s){ text=s; }

  void Paint(QRect *r=0);

  // Overrules events
  bool EvButtonPress(int button,int x,int y);
  bool EvButtonRelease(int button,int x,int y);
  bool EvMotionNotify(int x,int y);
};

#endif
