/*
 * QListBox - a user interface label (may be image or text)
 * 11-04-97: Created!
 * 04-06-97: Now allocated memory to hold text
 * 21-09-99: Multiline support.
 * BUGS:
 * - Paint() doesn't clip all the time
 * (C) MarketGraph/RVG
 */

#include <qlib/listbox.h>
#include <qlib/scrollbar.h>
#include <qlib/app.h>
#include <qlib/dir.h>
#include <qlib/image.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

#define DEFAULT_BAR_WID  17
#define DEFAULT_BAR_HGT  16

// Number of item strings to allocate in advance
#define DEF_ALLOCATE     10

#define BORDER_WID       4

QListBox::QListBox(QWindow *parent,QRect *ipos,int flags)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  barVert=0;
  barHor=0;
  firstItem=0;
  items=0;
  lbFlags=flags;
  itemsAllocated=DEF_ALLOCATE;
  //itemString=(char**)qcalloc(sizeof(string)*itemsAllocated);
  itemInfo=(QLBItemInfo*)qcalloc(sizeof(QLBItemInfo)*itemsAllocated);
  font=app->GetSystemFont();
  
  int w,h;
  GetItemSize(&w,&h);
  if(!h)itemsVisible=0;
  else  itemsVisible=(ipos->hgt-2*BORDER_WID)/h;
  
  Create();

  // Create scroll bars
  QRect r;
  int   bWid,bHgt;
  bWid=DEFAULT_BAR_WID;
  r.x=ipos->x+ipos->wid-bWid-2;
  r.y=ipos->y+2;
  r.wid=bWid;
  r.hgt=ipos->hgt-2*2;
  barVert=new QScrollBar(this,&r,QScrollBar::VERTICAL);
  barVert->SetCompoundWindow(this);
  
  // A label doesn't catch anything
  //Catch(CF_BUTTONPRESS);
}

QListBox::~QListBox()
{
  FreeItems();
  // Free itemInfo array itself
  qfree(itemInfo);
  if(barHor)delete barHor;
  if(barVert)delete barVert;
}

/********
* ITEMS *
********/
void QListBox::FreeItems()
// De-allocate entire itemInfo structure
{
  int i;

  if(!itemInfo)return;
  for(i=0;i<itemsAllocated;i++)
  {
    if(itemInfo[i].text)
    { qfree((void*)itemInfo[i].text);
      itemInfo[i].text=0;
    }
  }
}

void QListBox::GetItemSize(int *wid,int *hgt)
// Returns size of an item in 'wid' and 'hgt'
// Override this virtual function in derived classes to modify the item size
// (like thumbnail list views)
{
  QRect r;
  GetPos(&r);
  // Mind the scrollbars
  if(barVert)r.wid-=barVert->GetWidth();
  *wid=r.wid-2*BORDER_WID;
  *hgt=font->GetHeight();
}
void QListBox::GetItemRect(int item,QRect *r)
// Puts item bounding box in 'r'
// This is the area where PaintItem() should paint in.
// Don't use this function on invisible items (items not in view)
{
  int w,h;
  QRect pos;
  int relItem;            // Relative to firstItem

  GetPos(&pos); pos.x=pos.y=0;
  GetItemSize(&r->wid,&r->hgt);
  relItem=item-firstItem;
  r->x=pos.x+BORDER_WID;
  r->y=pos.y+BORDER_WID+relItem*r->hgt;
}
bool QListBox::IsItemVisible(int n)
// Returns TRUE if item 'n' is onscreen
{
  if(n<0||n>=items)return FALSE;
  if(n>=firstItem&&n<firstItem+itemsVisible)return TRUE;
  return FALSE;
}

/********
* PAINT *
********/
void QListBox::Paint(QRect *r)
{
  int i,w,h;
  int bvWid,bhHgt;
//qdbg("qlb:Paint\n");
  if(!IsVisible())return;

  //Restore();
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;
  //if(barVert)pos.wid-=barVert->GetWidth()-2;
  // Take into account any scrollbars
  if(barVert)bvWid=barVert->GetWidth();
  else       bvWid=0;
  bhHgt=0;
  cv->Inline(pos.x,pos.y,pos.wid,pos.hgt);
  cv->SetColor(QApp::PX_WHITE);
  if(items<firstItem+itemsVisible)
  { // Clear more than normal because items are missing
    cv->Rectfill(pos.x+2,pos.y+2,pos.wid-barVert->GetWidth()-2*2,pos.hgt-2*2);
  }
  // Take care when drawing outline to minimize flashing
  cv->Rectfill(pos.x+2,pos.y+2,pos.wid-bvWid-2*2,2);
  cv->Rectfill(pos.x+2,pos.y+2,2,pos.hgt-2*2);
  cv->Rectfill(pos.x+pos.wid-bvWid-2*2,pos.y+2,2,pos.hgt-bhHgt-2*2);
  GetItemSize(&w,&h);
  cv->Rectfill(pos.x+2,pos.y+pos.hgt-bhHgt-2-h,pos.wid-bvWid-2*2,
    h);
  for(i=firstItem;i<firstItem+itemsVisible&&i<items;i++)
  {
    PaintItem(i);
  }
}

void QListBox::PaintItem(int n)
// Paint only item 'n'
// Override this function if you do your specialized listbox
// Default behavior is to paint the associated string
{
  QRect r;
  QLBItemInfo *ii;
  
  if(n<0||n>=items)return;
  
  ii=&itemInfo[n];
  GetItemRect(n,&r);
//qdbg("item '%d'='%s'\n",n,itemString[n]);
//qdbg("  @%d,%d %dx%d\n",r.x,r.y,r.wid,r.hgt);
  if(ii->flags&QLBItemInfo::SELECTED)
    cv->SetColor(0,0,128);
  else
    cv->SetColor(QApp::PX_WHITE);
  cv->Rectfill(&r);
  cv->SetFont(font);
  if(ii->flags&QLBItemInfo::SELECTED)
    cv->SetColor(QApp::PX_WHITE);
  else
    cv->SetColor(QApp::PX_BLACK);
  cv->Text(itemInfo[n].text,r.x+itemInfo[n].indent,r.y);
}

/*********
* ADDING *
*********/
void QListBox::Empty()
// Clear all items
{
  FreeItems();
  items=0;
  firstItem=0;
  barVert->GetProp()->SetRange(0,0);
  barVert->GetProp()->SetDisplayed(0,0);
  barVert->GetProp()->SetPosition(0);
  Invalidate();
}
bool QListBox::AddString(cstring s)
{
  if(items==itemsAllocated)
  {
    int newAlloc;
    QLBItemInfo *newItemInfo;
    int i;
    
//qdbg("need re-alloc");
    newAlloc=itemsAllocated+DEF_ALLOCATE;
    newItemInfo=(QLBItemInfo*)qcalloc(sizeof(QLBItemInfo)*newAlloc);
    if(newItemInfo)
    {
      itemsAllocated=newAlloc;
      // Copy old items over
      for(i=0;i<items;i++)
        newItemInfo[i]=itemInfo[i];
      qfree(itemInfo);
      itemInfo=newItemInfo;
    } else
    { qerr("QListBox: no memory to add string");
      return FALSE;
    }
  }
  itemInfo[items].text=qstrdup(s);
  itemInfo[items].flags=0;
  itemInfo[items].indent=0;
  items++;
  barVert->GetProp()->SetRange(0,items-itemsVisible);
  barVert->GetProp()->SetDisplayed(itemsVisible,items);
  return TRUE;
}
bool QListBox::AddDir(cstring dirName,cstring filter,int flags)
// Convenience function to add the files of a directory
{
  QDirectory *dir;
  QFileInfo   fi;
  char fileName[FILENAME_MAX];
  
  Empty();
  dir=new QDirectory(dirName);
  if(!dir)return FALSE;
  while(dir->ReadNext(fileName,&fi))
  {
    if(!strcmp(fileName,"."))continue;
    if(!AddString(fileName))break;
  }
  delete dir;
  Sort();
  return TRUE;
}
void QListBox::Sort()
// After-insertion sort
{ int i,j;
  QLBItemInfo temp;
  
  for(i=1;i<items;i++)
  {
    for(j=0;j<i;j++)
    {
      if(strcmp(itemInfo[i].text,itemInfo[j].text)<0)
      { // Swap
        temp=itemInfo[i];
        itemInfo[i]=itemInfo[j];
        itemInfo[j]=temp;
      }
    }
  }
}

/************
* SELECTION *
************/
int QListBox::GetSelectedItem()
// Returns selected item or first selected item in case of MULTI_SELECT
// If none is selected, -1 is returned
{ int i;
  for(i=0;i<items;i++)
  { if(itemInfo[i].flags&QLBItemInfo::SELECTED)
    { return i;
    }
  }
  return -1;
}
cstring QListBox::GetItemText(int item)
// Returns text associated with item
{
  // Check range
  if(item<0||item>=items)return "";
  return itemInfo[item].text;
}

void QListBox::DeselectAll()
// Deselect all items
{ int i;
  for(i=0;i<items;i++)
  { if(itemInfo[i].flags&QLBItemInfo::SELECTED)
    { itemInfo[i].flags&=~QLBItemInfo::SELECTED;
#ifdef ND_FUTURE
      // Nicely update only item?
      if(IsItemVisible(i))
      { GetItemRect(i,
      }
#endif
    }
  }
  Invalidate();
}

void QListBox::Select(int item)
// Select an item
{
  QEvent e;
  
  // Check range
  if(item<0||item>=items)return;
  
  // Unselect previous item
  if(!(lbFlags&MULTI_SELECT))
    DeselectAll();
  // Select new item
  itemInfo[item].flags|=QLBItemInfo::SELECTED;
//qdbg("select item %d\n",item);
  // Send event notifying the selection
  e.type=QEvent::CLICK;
  e.win=this;
  PushEvent(&e);
  Invalidate();
}

/*********
* EVENTS *
*********/
bool QListBox::Event(QEvent *e)
{
  int i;
  QRect r;
  
//if(e->type!=6)qdbg("qlb:event(%d)\n",e->type);
  // Check for scrollbar events
  if(e->win==barVert)
  {
    if(e->type==QEvent::CHANGE||e->type==QEvent::CLICK)
    {
      firstItem=barVert->GetProp()->GetPosition();
//qdbg("firstitem=%d\n",firstItem);
      Invalidate();
      return TRUE;
    }
  } else if(e->type==QEvent::BUTTONPRESS)
  { // Any button will do
    // Find any clicked items
    for(i=firstItem;i<firstItem+itemsVisible&&i<items;i++)
    { GetItemRect(i,&r);
//qdbg("@%d,%d; in %d,%d %dx%d?\n",e->x,e->y,r.x,r.y,r.wid,r.hgt);
      if(r.Contains(e->x,e->y))
      { Select(i);
        break;
      }
    }
  } else
  { // Dispatch events
    return QWindow::Event(e);
  }
  return FALSE;
}

