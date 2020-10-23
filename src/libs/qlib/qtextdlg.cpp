/*
 * QTextDialog - MessageBox thingy
 * 08-09-97: Created!
 * NOTES:
 * - Basic file requester for Lingo/Mach6
 * BUGS:
 * - Not reentrant (2 file reqs)
 * - Paint() method doesn't paint ALL window
 * - Does not free all strings upon delete
 * (C) MG/RVG
 */

#include <qlib/dialog.h>
#include <qlib/app.h>
#include <qlib/button.h>
#include <qlib/dir.h>
#include <qlib/keys.h>
#include <bstring.h>
#include <string.h>
#include <X11/Xlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#ifdef OBS
//#define MAX_DSP_FILES	10	// Files on display
//#define MAX_DIR_FILES	256	// Max #files in a single dir

#define DLG_WID		400
#define DLG_HGT		400

#define FILES_X		12
#define FILES_Y		38
#define FILE_WID	350
#define FILE_HGT	20

#define EDIT_WID	260
#define EDIT_HGT	20
#define EDIT_X		100
#define EDIT_YSPACE	26
#endif

#define FILES_Y		38

#define YN_WID		100	// OK/Cancel buttons (Yes/No)
#define YN_HGT		35

QTextDialog::QTextDialog(QWindow *parent,QRect *r,cstring ititle,
  cstring itext,int flags)
  : QDialog(parent,r,ititle)
{ int i;
  QRect rr;
//qdbg("QTextDialog ctor; this=%p\n",this);

  rr.y=r->hgt-40;
  if(flags&OK_ONLY)
    rr.x=r->wid/2-YN_WID/2;		// Center
  else
    rr.x=12;
  rr.SetSize(YN_WID,YN_HGT);
  bYes=new QButton(this,&rr,"OK");
  bYes->ShortCut(QK_ENTER);
  //rr.SetXY(r->x+r->wid-YN_WID-10,r->y+r->hgt-YN_HGT-10);
  if(!(flags&OK_ONLY))
  { //rr.SetXY(r->wid-YN_WID-10,r->hgt-YN_HGT-10);
    rr.x=r->wid-YN_WID-10;
    bNo =new QButton(this,&rr,"Cancel");
    bNo->ShortCut(QK_ESC);
  } else bNo=0;

  //rr.SetXY(10,10);
  //lTitle=new QLabel(this,&rr,ititle);

  // Dir/file/filter
  int y=FILES_Y+10;
  // Label resizes itself to match text (in the Y direction)
  rr.SetXY(10,y);
  lText=new QLabel(this,&rr,itext);
//qdbg("QTextDialog ctor RET\n");
}
QTextDialog::~QTextDialog()
{
//qdbg("QTextDialog dtor\n");
  delete bYes;
  if(bNo)delete bNo;
  //delete lTitle;
  delete lText;
  //delete eString;
//qdbg("QTextDialog dtor RET\n");
}

static int retCode;
static QTextDialog *fd;

void QTextDialog::SetButtonText(cstring sYes,cstring sNo)
{
  bYes->SetText(sYes);
  if(bNo!=0&&sNo!=0)bNo->SetText(sNo);
}

static bool fdEH(QEvent *e)
{ int i,n;
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==fd->bYes)
    {do_ok:
      retCode=IDOK;
    } else if(e->win==fd->bNo)retCode=IDCANCEL;
  } else if(e->type==QEvent::KEYPRESS)
  {
    if(e->n==QK_ENTER||e->n==QK_KP_ENTER)
      goto do_ok;
  } else if(e->type==QEvent::CLOSE)
  { retCode=IDCANCEL;
  }
  return FALSE;
}

int QTextDialog::Execute()
// Returns: 0=file selected
//          1=canceled
{
  QState *state=0,*oldState=0;
  QAppEventProc oldEvProc=0;

//qdbg("QStrDlg: execute\n");
  if(app->GetStateManager())
  { // Switch events to go to our state
    oldState=app->GetStateManager()->GetCurrentState();
    state=new QState("textDlg",fdEH);
    app->GetStateManager()->GotoState(state,TRUE);
  } else
  { // Add event handler
    oldEvProc=app->GetEventProc();
    app->SetEventProc(fdEH);
  }
  retCode=-1;
  fd=this;
  //eString->Focus(TRUE);
  //eString->EvButtonPress(1,0,0);
  app->GetWindowManager()->SetKeyboardFocus(bYes);	// Hmmm?
  while(retCode==-1)
  { app->Run1();
//qdbg("QFD:Events; retCode=%d\n",retCode);
  }
  if(app->GetStateManager())
  { // Restore old state
    app->GetStateManager()->GotoState(oldState,TRUE);
    // Get rid of state
    //app->GetStateManager()->VCR_FrameBack();
    // Remove state from the manager and delete it
    delete state;
  } else
  { // Set old event handler
    app->SetEventProc(oldEvProc);
  }
  return retCode;
}

void QTextDialog::Paint(QRect *r)
// Gets called when expose happens (QDialog)
// and when WindowManager->Paint() is called
{
#ifdef OBS
 int i;
  qdbg("QFileDlg:Paint\n");
  //for(i=0;i<MAX_DSP_FILES;i++)
    //bFile[i]->Paint();
  bYes->Paint();
  if(bNo)bNo->Paint();
  //eString->Paint();
  lTitle->Paint();
  lText->Paint();
  // Etc...
  QRect pos;
  GetPos(&pos);

  cv->Separator(lTitle->GetX(),lTitle->GetY()+lTitle->GetHeight()+4,
    pos.wid-20 /*,lTitle->GetWidth() */);
  //cv->Outline(FILES_X-2,FILES_Y-2,FILE_WID+4,MAX_DSP_FILES*FILE_HGT+4);
#endif
}

/************************
* CONVENIENCE FUNCTIONS *
************************/
int QMessageBox(cstring title,cstring text,int flags,QRect *pos)
// Returns: 0=OK selected
//          1=canceled
{
  QTextDialog *sdlg;
  int ret;
  QRect r;

  if(pos==0)
  { pos=&r;
    r.SetXY(100,150);
    r.SetSize(300,140);
  }
  sdlg=new QTextDialog(app->GetShell(),pos,title,text,flags);
  ret=sdlg->Execute();
  delete sdlg;
  return ret;
}

