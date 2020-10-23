/*
 * QTitleBar - a titlebar for dialog and normal QShells
 * 18-03-00: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/titlebar.h>
#include <qlib/app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

#define DEFAULT_FONT (app->GetSystemFont())
#define EXTRA_HGT    4

// Default up/down left/right button size
#define DEFAULT_BUTTON_WID  16
#define DEFAULT_BUTTON_HGT  14
  
QTitleBar::QTitleBar(QWindow *parent,cstring text,QFont *_font)
  : QWindow(parent,2,2,parent->GetWidth()-2*2,
      _font?_font->GetHeight()+EXTRA_HGT:DEFAULT_FONT->GetHeight()+EXTRA_HGT)
{
  /*c*/ string txt;
  
  title=qstrdup(text);
  font=_font;
  if(!font)font=DEFAULT_FONT;
  CompressMotion();
  Create();

  // Close button
  QRect r;
  int bWid,bHgt;           // Button size
  
  r.wid=DEFAULT_BUTTON_WID;
  r.hgt=DEFAULT_BUTTON_HGT;
  r.x=rCreation.x+rCreation.wid-r.wid-2;
  r.y=rCreation.y+(rCreation.hgt-r.hgt)/2+1;
  txt="x";
  butClose=new QButton(this,&r,txt);
  butClose->DisableShadow();
  butClose->NoTabStop();
  
  // Make windows members of this
  butClose->SetCompoundWindow(this);
  
  Catch(CF_BUTTONPRESS|CF_BUTTONRELEASE|CF_MOTIONNOTIFY);
}

QTitleBar::~QTitleBar()
{
  delete butClose;
  if(title)qfree((void*)title);
}

void QTitleBar::SetTitle(cstring text)
{
  if(title)qfree((void*)title);
  title=qstrdup(text);
}

void QTitleBar::Paint(QRect *r)
{
  if(!IsVisible())return;

  //Restore();
  QRect pos;
  QColor colLeft(10,36,106),colRight(166,202,240);
  GetPos(&pos);
  pos.x=pos.y=0;
  //cv->SetColor(0,0,128);
  //cv->Rectfill(pos.x,pos.y,pos.wid,pos.hgt);
  cv->Rectfill(&pos,&colLeft,&colRight,&colRight,&colLeft);
//qdbg("pnt %dx%d\n",pos.wid,pos.hgt);
  //cv->Insides(pos.x,pos.y,pos.wid,pos.hgt);
  if(title)
  { cv->SetColor(QApp::PX_WHITE);
    cv->SetFont(font);
    cv->Text(title,pos.x+4,pos.y+EXTRA_HGT/2);
  }
  // Painted over the buttons
  butClose->Invalidate();
}

bool QTitleBar::Event(QEvent *e)
// Events arrive here because contained windows pass on
// generated events to us
{
//qdbg("qtb:event(%d) n=%d, this=%p, win=%p\n",e->type,e->n,this,e->win);
  if(e->type==QEvent::CLICK)
  {
    if(e->win==butClose)
    {
      // Request to close the parent (not the titlebar itself)
      e->type=QEvent::CLOSE;
      //e->win=this;
      e->win=parent;
      PushEvent(e);
      return TRUE;
    }
  }
  return QWindow::Event(e);
}
