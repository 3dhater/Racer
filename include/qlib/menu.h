// qlib/menu.h

#ifndef __QLIB_MENU_H
#define __QLIB_MENU_H

#include <qlib/button.h>
#include <qlib/dialog.h>

#define QMENU_MAXITEM	10

//class QMenu : public QWindow
class QMenu : public QDialog
// Popup window with items
{
 public:
  string ClassName(){ return "menu"; }
  QButton *item[QMENU_MAXITEM];
  int items;
  int mx,my;		// Add location
  int retCode;		// Selected item or -1
  enum RetCode
  {
    NO_ITEM=-1,		// retCode value
    IDLE=-2             // Selecting
  };

 public:
  QMenu(QWindow *parent,int x,int y,int wid);
  ~QMenu();

  void AddLine(string text);
  int  Execute();

  bool Event(QEvent *e);
  void Paint(QRect *r=0);
};

// Convenience
int QMenuPopup(int x,int y,string m[]);

#endif
