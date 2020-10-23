/*
 * QStrDialog - string entering
 * 21-08-97: Created!
 * 27-02-98: Updated. Event handling modifed (no VCR state)
 * 18-12-00: Return codes modified! Returns TRUE or FALSE now.
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

#define DEFAULT_TEXT_SIZE 100000

#define FILES_Y		28

#define EDIT_X		100
#define EDIT_YSPACE	26

#define YN_WID		100	// OK/Cancel buttons (Yes/No)
#define YN_HGT		40

QStringDialog::QStringDialog(QWindow *parent,QRect *r,cstring ititle,
  cstring itext,string ibuf,int lines)
  : QDialog(parent,r,ititle)
{
  QRect rr;
  //qdbg("QStringDialog ctor; this=%p\n",this);

  rr.SetXY(10,r->hgt-50);
  rr.SetSize(YN_WID,YN_HGT);
  bYes=new QButton(this,&rr,"OK");
  if(lines==1)bYes->ShortCut(QK_ENTER);
  else        bYes->ShortCut(QK_ALT|QK_O);
  //rr.SetXY(r->x+r->wid-YN_WID-10,r->y+r->hgt-YN_HGT-10);
  rr.SetXY(r->wid-YN_WID-10,r->hgt-YN_HGT-10);
  bNo=new QButton(this,&rr,"Cancel");
  bNo->ShortCut(QK_ESC);

  //rr.SetXY(r->wid/2-YN_WID/2,r->hgt-YN_HGT-10);
  //bParent=new QButton(this,&rr,"Parent");

  //rr.SetXY(10,10);
  //lTitle=new QLabel(this,&rr,ititle);

  // Dir/file/filter
  int y=FILES_Y+10;
  rr.SetXY(10,y);
  lText=new QLabel(this,&rr,itext);
  y+=EDIT_YSPACE;
  rr.SetXY(EDIT_X,y);
  rr.SetXY(10,y);
  rr.SetSize(r->wid-20,app->GetSystemFont()->GetHeight()*lines+8);
  int flags;
  if(lines>1)flags=QEdit::MULTILINE|QEdit::SCROLLING;
  else       flags=0;
  buf=ibuf;
  eString=new QEdit(this,&rr,DEFAULT_TEXT_SIZE,buf,flags);
  y+=EDIT_YSPACE;

  //qdbg("QStringDialog ctor RET\n");
}
QStringDialog::~QStringDialog()
{
  //qdbg("QStringDialog dtor\n");
  delete bYes;
  delete bNo;
  //delete lTitle;
  delete lText;
  delete eString;
  //qdbg("QStringDialog dtor RET\n");
}

static int retCode;
static QStringDialog *fd;

static bool fdEH(QEvent *e)
{ //int i,n;
//qdbg("fdEH\n");
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==fd->bYes)
    { retCode=TRUE;
      // Use the file in the edit window
     do_ok:
      strcpy(fd->buf,fd->eString->GetText());
    } else if(e->win==fd->bNo)retCode=FALSE;
  }
  return FALSE;
}

#ifdef OBS
bool QStringDialog::Event(QEvent *e)
{
  qdbg("QStrDlg: Event %d\n",e->type);
  fdEH(e);
  return QDialog::Event(e);
}
#endif

int QStringDialog::Execute()
// Returns: 0=file selected
//          1=canceled
{
  QAppEventProc oldEP;
  fd=this;
  retCode=-1;
  oldEP=app->GetEventProc();
  app->SetEventProc(fdEH);
  //eString->Focus(TRUE);
  //eString->EvButtonPress(1,0,0);
  if(!eString->IsMultiLine())
    eString->SelectAll();
  app->GetWindowManager()->SetKeyboardFocus(eString);
#ifdef linux
  // nVidia driver trouble; doesn't flush
  app->RunPoll();
  glFlush();
#endif
  while(retCode==-1)app->Run1();
  app->SetEventProc(oldEP);
  return retCode;
}

void QStringDialog::Paint(QRect *r)
// Gets called when expose happens (QDialog)
// and when WindowManager->Paint() is called
{
#ifdef OBS
 int i;
  //qdbg("QFileDlg:Paint\n");
  //for(i=0;i<MAX_DSP_FILES;i++)
    //bFile[i]->Paint();
#ifdef OBS
  bYes->Paint();
  bNo->Paint();
  eString->Paint();
  lTitle->Paint();
  lText->Paint();
  // Etc...
#endif

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
int QDlgString(cstring title,cstring text,string buf,int maxLen,int lines,
  QRect *pos)
{
  QStringDialog *sdlg;
  int ret;
  QRect r;

  //strcpy(buf,"5");
  if(pos==0)
  { pos=&r;
    r.SetXY(100,150);
    r.SetSize(300,160+lines*app->GetSystemFont()->GetHeight());
  }
  sdlg=new QStringDialog(app->GetShell(),pos,title,text,buf,lines);
  //sdlg=new QStringDialog(0,pos,title,text,buf,lines);
  //qdbg("  dialog created\n");
  // Lingo hack; use nonprop font
  if(lines>1)
  { sdlg->eString->SetFont(app->GetSystemFontNP());
  }
  ret=sdlg->Execute();
  delete sdlg;
  return ret;
}
