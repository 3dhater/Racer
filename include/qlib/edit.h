// qlib/edit.h

#ifndef __QLIB_EDIT_H
#define __QLIB_EDIT_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

class QEdit : public QWindow
{
 public:
  string ClassName(){ return "edit"; }

 protected:
  enum State
  { INACTIVE=0,
    ACTIVE=1
  };

  enum
  { MAX_VIEW_LINES=100,		// Max number of (virtual) lines you may see
    MAX_FIND_LEN=80             // Max length of find string
  };

 public:
  enum Flags
  { MULTILINE=1,		// Be able to use more than 1 line?
    VIEW_WRAP=2,		// Wrap lines that are too wide for the view?
    SCROLLING=4,		// Use scrolling if text does not fit?
    NO_ENTER_PROPAGATE=8,	// Hold ENTER from propagating to other win's?
    READONLY=16                 // Don't allow editing
  };
  enum EFlags			// Edit flags; are dynamic and change a lot
  { CHANGED=1,			// File has changed?
    AUTOINDENT=2,		// Automatic insertion of spaces?
    IF_BRACE_OUTDENT=4,		// If '}', then outdent a bit
    DRAGGING=8			// Mouse is being dragged with button held
  };
  enum FindFlags		// When calling Find()
  {
    CASE_DEPENDENT=1,		// Case==CASE
    SELECT_AS_RANGE=2,		// Select in range
    FIND_BACKWARDS=4		// Reverse search
  };

 protected:
  // General attribs
  int      state;
  int      flags;		// Multi-line? (behavior flags)
  int      eFlags;		// Edit flags (changed?)
  string   text;		// Contents
  int      maxChar;		// Size of allocated space for the text
  // Viewing
  QFont   *font;
  QColor  *colText;
  string   lineStart[MAX_VIEW_LINES];	// Start of line data per view line
  int      lineLen[MAX_VIEW_LINES];	// Line length in chars
  string   textTop;		// Ptr to text at top of control
  int      linesInView;		// Lines that fit
  // Refreshing
  char     lineRefresh[MAX_VIEW_LINES];
  // Editing
  int      cx,cy;		// Cursor location
  int      cxWanted;		// Try not to cap cx at every line
  //int      cyTop;		// Lines above top
  // Range selection
  string   rangeStart,rangeEnd;	// Ptrs into text
  signed char rangeDirection,
              pad;
  // Markers
  string   marker;
  // Buffer (cut/copy/paste)
  string   buffer;
  int      bufferSize;

  // Movement
  bool CursorUp();
  bool CursorDown();
  bool CursorRight();
  bool CursorLeft();
  void CursorHome();
  void CursorEnd();
  bool ScrollUp();
  bool ScrollDown();
  void Goto(cstring p);

  // Formatting
  void  FormatView();
  void  RefreshView();
  void  RefreshLine(int y);
  void  RefreshSmart();
  void  CheckCX();
  void  CheckCY();
  char  CurrentChar();
  char *CurrentCharPtr();
  int   IndentPoint(cstring s);

  // Ranges
  void RangeClear();
  bool RangeActive();
  void RangeSetStart(char *p);
  void RangeSetEnd(char *p);
  bool RangeDelete();
  bool IsInRange(cstring p);
  char *MouseXYtoCursorXY(int mx,int my,int *x,int *y);

  // Copy buffer
  void Cut();
  void Copy();
  void Paste();

  // Modifications
  bool InsertChar(char c);
  bool InsertString(cstring s,bool reformat=TRUE);
  bool DeleteChar();
  bool DeleteChars(int n);

  void Changed();

 public:
  QEdit(QWindow *parent,QRect *pos=0,int maxlen=80,cstring text=0,int flags=0);
  ~QEdit();

  void   SetFont(QFont *font);
  QFont *GetFont(){ return font; }
  void   SetText(string text);
  string GetText(){ return text; }
  int    GetInt();
  void   SetTextColor(QColor *col);
  void   NoEnterPropagation();

  // Information
  bool   IsChanged();
  bool   IsMultiLine();
  bool   IsReadOnly();
  void   EnableReadOnly(){ flags|=READONLY; }
  void   DisableReadOnly(){ flags&=~READONLY; }
  int    GetCX();
  int    GetCY();			// Full document Y position (not 'cy')
  char*  GetRangeStart(){ return rangeStart; }
  char*  GetRangeEnd()  { return rangeEnd; }

  // Public editing
  void SelectAll();
  bool Find(cstring s,int flags=0);

  // Painting
  cstring PaintLine(int y /*,cstring text*/);
  void    Paint(QRect *r=0);

  // Overrules events
  bool EvButtonPress(int button,int x,int y);
  bool EvButtonRelease(int button,int x,int y);
  bool EvMotionNotify(int x,int y);
  bool EvKeyPress(int key,int x,int y);
  bool EvSetFocus();
  bool EvLoseFocus();
};

#endif
