/*
 * QScrollBar - combines often used prop+up/down button
 * 23-02-00: Created!
 * NOTES:
 * - Often used itself as a container within a listbox, for example.
 * (C) MarketGraph/RVG
 */

#include <qlib/scrollbar.h>
#include <qlib/app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// Default up/down left/right button size
#define DEFAULT_BUTTON_WID  17
#define DEFAULT_BUTTON_HGT  16
  
QScrollBar::QScrollBar(QWindow *parent,QRect *ipos,int dir)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  /*c*/ string txt;
  
  Create();

  // 'Less' button
  QRect r;
  int bWid,bHgt;           // Button size

  bWid=bHgt=0;
  r.x=ipos->x; r.y=ipos->y;
  if(dir==VERTICAL)
  { r.wid=ipos->wid;
    bHgt=DEFAULT_BUTTON_HGT;
    r.hgt=bHgt;
    txt="$UP";
  } else
  { 
    bWid=DEFAULT_BUTTON_WID;
    r.wid=bWid;
    r.hgt=ipos->hgt;
    txt="$LEFT";
  }
  butLess=new QButton(this,&r,txt);
  butLess->DisableShadow();
  butLess->NoTabStop();

  // 'More' button
  if(dir==VERTICAL)
  {
    r.x=ipos->x;
    r.y=ipos->y+ipos->hgt-r.hgt;
    txt="$DOWN";
  } else
  {
    r.x=ipos->x+ipos->wid-r.wid;
    r.y=ipos->y;
    txt="$RIGHT";
  }
  butMore=new QButton(this,&r,txt);
  butMore->NoTabStop();
  butMore->DisableShadow();
  //Catch(CF_BUTTONPRESS);
  
  // Prop window inbetween
  if(dir==VERTICAL)
  {
    r.x=ipos->x;
    r.y=ipos->y+r.hgt;
    r.hgt=ipos->hgt-2*bHgt;
  } else
  {
    r.x=ipos->x+bWid;
    r.y=ipos->y;
    r.wid=ipos->wid-2*bWid;
  }
  prop=new QProp(this,&r,dir==VERTICAL?QProp::VERTICAL:QProp::HORIZONTAL);
  
  // Make windows members of this
  butLess->SetCompoundWindow(this);
  butMore->SetCompoundWindow(this);
  prop->SetCompoundWindow(this);
}

QScrollBar::~QScrollBar()
{
  if(butLess)delete butLess;
  if(butMore)delete butMore;
  if(prop)delete prop;
}

#ifdef OBS
void QScrollBar::Paint()
{
  return;
  if(!IsVisible())return;

  Restore();
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;
  cv->Insides(pos.x,pos.y,pos.wid,pos.hgt);
}
#endif

bool QScrollBar::Event(QEvent *e)
// Events arrive here because contained windows pass on
// generated events to us
{
//qdbg("qsb:event(%d) n=%d, this=%p, win=%p\n",e->type,e->n,this,e->win);
  //return QWindow::Event(e);
  if(e->type==QEvent::CHANGE)
  {
    if(e->win==prop)
    { // Prop is being dragged, pass on event as if it is from us
     pass_on:
      e->win=this;
      PushEvent(e);
      return TRUE;
    }
  } else if(e->type==QEvent::CLICK)
  {
    if(e->win==butLess)
    { prop->SetPosition(prop->GetPosition()-1);
      goto pass_on;
    } else if(e->win==butMore)
    { prop->SetPosition(prop->GetPosition()+1);
      goto pass_on;
    } else if(e->win==prop)
    { goto pass_on;
    }
  }
  // No actual scrollbar events happen, so no QWindow::Event(e)
  return FALSE;
}

