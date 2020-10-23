// qlib/listbox.h

#ifndef __QLIB_LISTBOX_H
#define __QLIB_LISTBOX_H

#include <qlib/scrollbar.h>

struct QLBItemInfo
{
  enum QLBFlags
  {
    SELECTED=1
  };
  cstring text;
  int     flags;
  int     indent;
  int     icon[2];
};

class QListBox : public QWindow
// A list of items
// Flexible
{
 public:
  string ClassName(){ return "listbox"; }

  enum Flags
  { MULTI_SELECT=1,		// Enable selecting more than 1 item
    SORT_CASE_INDEPENDENT=2	// Don't look at case when sorting items
  };

 protected:
  QScrollBar *barHor,*barVert;
  int    lbFlags;		// The flags
  int    firstItem,
         itemsVisible;
  QLBItemInfo *itemInfo;
  //char **itemString;
  int    items,
         itemsAllocated;
  QFont *font;

 public:
  QListBox(QWindow *parent,QRect *pos,int flags=0);
  ~QListBox();

  // Item handling
  void    FreeItems();
  virtual void GetItemSize(int *wid,int *hgt);
  void    GetItemRect(int item,QRect *r);
  bool    IsItemVisible(int item);
  int     GetNoofItems(){ return items; }
  int     GetSelectedItem();
  cstring GetItemText(int item);

  // Items insertions
  void Empty();
  bool AddString(cstring s);
  bool AddDir(cstring dirName,cstring filter=0,int flags=0);
  void Sort();

  void Select(int item);
  void DeselectAll();

  // Painting
  void Paint(QRect *r=0);
  virtual void PaintItem(int n);

  bool Event(QEvent *e);
};

#endif
