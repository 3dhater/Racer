/*
 * QProgressDialog - useful during lengthy operations
 * 07-01-01: Created! (based on qstrdlg.cpp)
 * BUGS:
 * (C) MarketGraph/RvG
 */

#include <qlib/dialog.h>
#include <qlib/app.h>
#include <qlib/button.h>
#include <qlib/keys.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

#define BUTTON_WID	100	// Cancel button
#define BUTTON_HGT	40

#undef  DBG_CLASS
#define DBG_CLASS "QProgressDialog"

/*********
* C/DTOR *
*********/
QProgressDialog::QProgressDialog(QWindow *parent,QRect *r,cstring ititle,
  cstring itext,int iflags)
  : QDialog(parent,r,ititle)
{
  DBG_C("ctor")
  
  QRect rr;
  int y=38;

  pdlgFlags=iflags;
//qdbg("pdlgFlags=%d, NO_CANCEL=%d\n",pdlgFlags,NO_CANCEL);
  // Make sure runtime flags are off
  pdlgFlags&=~(CANCEL_PRESSED);
  
  rr.SetXY(r->wid/2-BUTTON_WID/2,r->hgt-50);
  rr.SetSize(BUTTON_WID,BUTTON_HGT);
  if(!(pdlgFlags&NO_CANCEL))
  {
    bCancel=new QButton(this,&rr,"Cancel");
    bCancel->ShortCut(QK_ESC);
    bCancel->SetKeyPropagation(TRUE);
  } else bCancel=0;
  
  // Explanatory text inside dialog
  rr.SetXY(10,y);
  lText=new QLabel(this,&rr,itext);
  
  // Progress bar
  rr.SetXY(10,y+30);
  rr.SetSize(r->wid-2*10,25);
  prg=new QProgress(this,&rr);
}
QProgressDialog::~QProgressDialog()
{
  DBG_C("dtor")
  if(bCancel)delete bCancel;
  if(lText)delete lText;
  if(prg)delete prg;
}

/*********
* Events *
*********/
bool QProgressDialog::Event(QEvent *e)
{
//qdbg("QPrgDlg: Event %d\n",e->type);
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==bCancel)
    { 
      // User wants to cancel
      pdlgFlags|=CANCEL_PRESSED;
      return TRUE;
    }
  } else if(e->type==QEvent::KEYPRESS)
  {
    if(e->n==QK_ESC)
    {
      // User wants to cancel
      pdlgFlags|=CANCEL_PRESSED;
      return TRUE;
    }
  }
  return QDialog::Event(e);
}

/*********
* Create *
*********/
bool QProgressDialog::Create()
// Returns FALSE if dialog couldn't created.
{
  QWM->SetKeyboardFocus(bCancel);
  // Get it up visually
  app->RunPoll();
  return TRUE;
}

/*********************************************
* Handling events while the progress is busy *
*********************************************/
bool QProgressDialog::Poll()
// Returns TRUE if polling was normal (can continue).
// Returns FALSE if 'Cancel' was pressed (a request to abort)
{
  app->RunPoll();
  // Signal caller if Cancel was pressed
  if(pdlgFlags&CANCEL_PRESSED)return FALSE;
  return TRUE;
}

void QProgressDialog::SetProgress(int cur,int total)
// Update the progress bar
{
  if(prg)
    prg->SetProgress(cur,total,TRUE);
}
void QProgressDialog::SetProgressText(cstring text)
// Update the progress bar text
{
  if(prg)
    prg->SetText(text);
}
