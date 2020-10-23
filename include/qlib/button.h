// qlib/button.h

#ifndef __QLIB_BUTTON_H
#define __QLIB_BUTTON_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

// Magic bitmap names
#define QB_MAGIC_PLAY      "$PLAY"
#define QB_MAGIC_REVERSE   "$REVERSE"
#define QB_MAGIC_FORWARD   "$FORWARD"
#define QB_MAGIC_REWIND    "$REWIND"

class QButton : public QWindow
{
 public:
  string ClassName(){ return "button"; }
 protected:
  enum State
  { ARMED=1,
    DISARMED=0,
    TRACKING=2
  };

  // Flags
  enum BFlags
  { IMAGE=1,			// Text is an image
    NOBORDER=2,			// Don't paint border
    PROPAGATE_KEY=4,		// Pass on key events instead of eating it
    NO_SHADOW=8			// No button shadow
  };

#ifdef OBS
  QColor  *colUL,*colLR;
  QColor  *ciUL,*ciUR,		// Colors of inside rectangle
          *ciLR,*ciLL;
#endif
  QColor  *colShadow1,*colShadow2;
  QColor  *colText;
  int      state;
  int      align;
  int      bflags;
  string   text;		// If textual button
  QFont   *font;
  int      scKey,scMod;		// Shortcut key
  int      eventType;		// Generated event
  // Bitmap buttons
  QBitMap *bmDisarmed,		// Use bitmap inside button
          *bmArmed;
  QRect   *bmSrcDA,		// Disarmed bitmap rectangle
          *bmSrcA;		// Armed

 public:
  enum Alignment
  { CENTER,
    LEFT,               // Mostly borderless buttons
    RIGHT
  };

 public:
  QButton(QWindow *parent,QRect *pos=0,cstring text=0);
  ~QButton();

  cstring GetText(){ return text; }
  void    SetText(cstring text);
  void    SetTextColor(QColor *col);
  void    Paint(QRect *r=0);

  void    SetBitMap(QBitMap *bmda,QRect *r);

  void    BorderOff();
  void    EnableShadow();
  void    DisableShadow();
  void    Align(int a);
  int     GetShortCut(int *modX=0);	// Get shortcut key
  void    ShortCut(int key,int modX=0);	// Define shortcut key
  void    SetEventType(int eType);		// User events
  void    SetKeyPropagation(bool yn=TRUE);	// Pass on key events?

  // Overrules events
  bool EvButtonPress(int button,int x,int y);
  bool EvButtonRelease(int button,int x,int y);
  bool EvMotionNotify(int x,int y);
  bool EvKeyPress(int key,int x,int y);
  bool EvEnter();
  bool EvExit();
};

#endif
