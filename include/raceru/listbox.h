// raceru/listbox.h

#ifndef __RACERU_LISTBOX_H
#define __RACERU_LISTBOX_H

#include <qlib/listbox.h>
#include <raceru/window.h>
#include <d3/texfont.h>

#ifdef ND_QLIB
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
#endif

class RListBox : public QListBox
// A list of items
// Flexible
{
 public:
  string ClassName(){ return "rlistbox"; }

  // Attribs
  DTexFont *tfont;
  QColor *colNormal,*colHilite,*colEdge;

  // Animation
  RAnimTimer *aList,           // Displaying the list
             *aRest;           // All the rest

 public:
  RListBox(QWindow *parent,QRect *pos,int flags=0,DTexFont *font=0);
  ~RListBox();

  void GetItemSize(int *wid,int *hgt);

  // Painting
  void Paint(QRect *r=0);
  void PaintItem(int n);

  // Animation
  void AnimIn(int t,int delay=0);
};

#endif
