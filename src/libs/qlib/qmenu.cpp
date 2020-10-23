/*
 * QMenu - popup menu (loose paper)
 * 02-03-98: Created!
 * FUTURE:
 * - Separators
 * - Images as items
 * - QMenuPopup() should support separator lines
 * (C) MG/RVG
 */

#include <qlib/menu.h>
#include <qlib/keys.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define BW	2		// Border width
#define YSPACE	0		// Space between items

static QRect r(0,0,1,1);
static QMenu *curMenu;

QMenu::QMenu(QWindow *parent,int x,int y,int wid)
  : QDialog(parent,&r,"menu")
{
  items=0;
  mx=BW; my=BW;
  Move(x,y);
  Size(wid,8);
}

QMenu::~QMenu()
{
  int i;
  for(i=0;i<items;i++)
    delete item[i];
}

void QMenu::AddLine(string text)
// Add usual menu option
{
  QRect r;
  QRect pos;

//qdbg("QMenu:AddLine()\n");
  GetPos(&pos); pos.x=pos.y=0;
  r.x=mx; r.y=my;
  r.wid=GetWidth()-BW*2;
  r.hgt=app->GetSystemFont()->GetHeight()+YSPACE;
  item[items]=new QButton(this,&r,text);
  item[items]->BorderOff();
  item[items]->SetImageBackground(FALSE);
  //item[items]->Align(QButton::LEFT);
  //r.hgt=item[items]->GetHeight();
  //qdbg("button hgt=%d\n",r.hgt);
  my+=r.hgt+YSPACE;
  items++;
  Size(GetWidth(),my+BW);
//qdbg("QMenu:AddLine() RET\n");
}

bool QMenu::Event(QEvent *e)
{ int i;
  //if(e->type!=6)qdbg("Qmenu:event()\n");
  if(e->type==QEvent::CLICK)
  {
    for(i=0;i<items;i++)
    { if(e->win==item[i])
      { retCode=i;
        return TRUE;
      }
    }
  } else if(e->type==QEvent::KEYPRESS)
  {
   if(e->n==QK_ESC)
   { goto no_choice;
   }
  } else if(e->type==QEvent::BUTTONPRESS)
  { // Clicking outside window?
//qdbg("QM: Click outside?\n");
    if(e->win==0)
    { // Clicked outside the dialog!
     no_choice:
      retCode=NO_ITEM;
      // Push event on
      QEventPush(e);
      return TRUE;
    }
    //if(e->win->IsDescendantOf(this))
    return FALSE;
#ifdef OBS
    for(i=0;i<items;i++)
    { if(e->win==item[i])
      { return FALSE;
      }
    }
    if(e->win==this)return FALSE;
#endif
  }
  return QDialog::Event(e);
}

static bool qmenuEH(QEvent *e)
// Used to get events outside of window
{
#ifdef OBS_DBG
  if(e->type==QEvent::KEYPRESS&&e->n==QK_SPACE)
  { qdbg("invalidate; thisdialog=%p\n",curMenu);
    curMenu->Invalidate();
  }
#endif
  return curMenu->Event(e);
}

int QMenu::Execute()
{
  QAppEventProc oldEP;

  //qdbg("QMenu:Exec\n");
  oldEP=app->GetEventProc();
  app->SetEventProc(qmenuEH);
  curMenu=this;
//Paint();
  retCode=IDLE;
//Invalidate();
  while(retCode==IDLE)app->Run1();
  app->SetEventProc(oldEP);
  return retCode;
}

void QMenu::Paint(QRect *r)
{ Restore();
  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  cv->Outline(pos.x,pos.y,pos.wid,pos.hgt);
}

/**************
* CONVENIENCE *
**************/
int QMenuPopup(int x,int y,string m[])
// Easy does it
// m[] contains string and a 0 string to end it
{ int n,i;
  QMenu *popup;
  //qdbg("QMenuPopup(%p,%d)\n",m,size);
  // Count #strings
  for(n=0;m[n];n++);
  popup=new QMenu(QSHELL,x,y,200);
  for(i=0;i<n;i++)
    popup->AddLine(m[i]);
  i=popup->Execute();
  delete popup;
  return i;
}

