// qlib/titlebar.h - in QShells mostly

#ifndef __QLIB_TITLEBAR_H
#define __QLIB_TITLEBAR_H

#include <qlib/button.h>
#include <qlib/prop.h>

class QTitleBar : public QWindow
// A proportional slider + buttons (container), horizontal or vertical
{
 public:
  string ClassName(){ return "titlebar"; }

 protected:
  QButton *butClose,*butMinimize,*butMaximize;
  QFont   *font;
  cstring  title;

 public:
  QTitleBar(QWindow *parent,cstring title,QFont *font=0);
  ~QTitleBar();

  QButton *GetCloseButton(){ return butClose; }
  QButton *GetMinimizeButton(){ return butMinimize; }
  QButton *GetMaximizeButton(){ return butMaximize; }
  QFont   *GetFont(){ return font; }

  void SetTitle(cstring text);

  void Paint(QRect *r=0);

  bool Event(QEvent *e);
};

#endif
