// qlib/scrollbar.h

#ifndef __QLIB_SCROLLBAR_H
#define __QLIB_SCROLLBAR_H

#include <qlib/button.h>
#include <qlib/prop.h>

class QScrollBar : public QWindow
// A proportional slider + buttons (container), horizontal or vertical
{
 public:
  string ClassName(){ return "scrollbar"; }

  enum Direction
  { HORIZONTAL,
    VERTICAL
  };

 protected:
  int      dir;
  QButton *butLess,*butMore;
  QProp   *prop;

 public:
  QScrollBar(QWindow *parent,QRect *pos,int dir);
  ~QScrollBar();

  QButton *GetLessButton(){ return butLess; }
  QButton *GetMoreButton(){ return butMore; }
  QProp   *GetProp(){ return prop; }

  bool Event(QEvent *e);
};

#endif
