// qlib/progress.h

#ifndef __QLIB_PROGRESS_H
#define __QLIB_PROGRESS_H

#include <qlib/window.h>
#include <qlib/color.h>
#include <qlib/font.h>

class QProgress : public QWindow
{
 public:
  string ClassName(){ return "button"; }
 protected:
  // Flags
  enum Flags
  { NOBORDER=1
  };

  QColor  *colShadow1,*colShadow2;
  QColor  *colText;
  string   text;		// If textual button
  QFont   *font;
  int      align;
  int      cur,total;		// Progress

 public:
  enum Alignment
  { CENTER=0,		// Alignment of text
    LEFT=1, 		// Left align (mostly borderless buttons)
    RIGHT=2
  };

 public:
  QProgress(QWindow *parent,QRect *pos=0,string text=0);
  ~QProgress();

  cstring GetText(){ return text; }
  void SetText(cstring text);
  void SetTextColor(QColor *col);

  void SetProgress(int current,int total,bool paint=TRUE);

  void Paint(QRect *r=0);
};

#endif
