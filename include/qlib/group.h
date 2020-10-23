// qlib/group.h

#ifndef __QLIB_GROUP_H
#define __QLIB_GROUP_H

#include <qlib/window.h>
#include <qlib/font.h>

class QGroup : public QWindow
// A simple group rectangle (possibly with text) to group user interface
// elements.
{
 public:
  string ClassName(){ return "group"; }
 protected:
  QColor  *colText;
  string   text;		// If textual button
  QFont   *font;

 public:
  QGroup(QWindow *parent,QRect *pos=0,string text=0);
  ~QGroup();

  void SetText(string text);
  void Paint(QRect *r);
};

#endif
