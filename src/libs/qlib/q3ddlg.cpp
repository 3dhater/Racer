/*
 * Q3DDialog - providing X/Y/Z rotation and X/Y/Z positioning props
 * 18-07-97: Created!
 * NOTES:
 * - For Lingo cube positioning (first use); developed in opengl/texture/
 * (C) MG/RVG
 */

#include <qlib/dialog.h>
#include <qlib/app.h>
#include <qlib/button.h>
#include <qlib/dir.h>
#include <bstring.h>
#include <string.h>
#include <X11/Xlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DLG_WID		400
#define DLG_HGT		400

#define MAX_POS_RANGE	1000

#define FILES_X		12
#define FILES_Y		35
#define FILE_WID	350
#define FILE_HGT	20

#define EDIT_WID	260
#define EDIT_HGT	20
#define EDIT_X		100
#define EDIT_YSPACE	26

#define YN_WID		100	// OK/Cancel buttons (Yes/No)
#define YN_HGT		40

static int orgX,orgY;		// Drag original location

Q3DDialog::Q3DDialog(QWindow *parent,QRect *r)
  : QDialog(parent,r,"3D")
{ int i;
  QRect rr;
  qdbg("Q3DDialog ctor; this=%p\n",this);

  rr.SetSize(FILE_WID,FILE_HGT);
  //rr.SetXY(r->x+10,r->y+r->hgt-50);
  rr.SetXY(10,r->hgt-50);
  rr.SetSize(YN_WID,YN_HGT);
  bYes=new QButton(this,&rr,"Accept");
  //rr.SetXY(r->x+r->wid-YN_WID-10,r->y+r->hgt-YN_HGT-10);
  rr.SetXY(r->wid-YN_WID-10,r->hgt-YN_HGT-10);
  bNo=new QButton(this,&rr,"Cancel");

  //rr.SetXY(r->wid/2-YN_WID/2,r->hgt-YN_HGT-10);
  //bParent=new QButton(this,&rr,"Parent");

  //bUp=0;
  //bDown=0;
  //rr.SetXY(r->wid-30,FILES_Y-2);
  //rr.SetSize(20,MAX_DSP_FILES*FILE_HGT+4);
  for(i=0;i<3;i++)
  {
    rr.SetXY(20+i*30,40);
    rr.SetSize(25,360);
    pRotation[i]=new QProp(this,&rr,QProp::VERTICAL);
    pRotation[i]->SetRange(0,360);
    pRotation[i]->SetPosition(0);
    pRotation[i]->SetJump(1);
    rr.SetXY(20+(i+4)*30,40);
    rr.SetSize(25,360);
    pPos[i]=new QProp(this,&rr,QProp::VERTICAL);
    pPos[i]->SetRange(0,MAX_POS_RANGE);
    pPos[i]->SetPosition(MAX_POS_RANGE/2);
    pPos[i]->SetJump(1);
  }

  rr.SetXY(10,10);
  lTitle=new QLabel(this,&rr,"Select 3D location and orientation");

  // Dir/file/filter
/*
  int y=FILES_Y+MAX_DSP_FILES*FILE_HGT+10;
  rr.SetXY(10,y+2); rr.SetSize(EDIT_WID,EDIT_HGT);
  lDir=new QLabel(this,&rr,"Directory");
  rr.SetXY(EDIT_X,y);
  eDir=new QEdit(this,&rr,256);
  y+=EDIT_YSPACE;

  rr.SetXY(10,y+2);
  lFilter=new QLabel(this,&rr,"Filter");
  rr.SetXY(EDIT_X,y);
  eFilter=new QEdit(this,&rr,256);
  y+=EDIT_YSPACE;

  rr.SetXY(10,y+2);
  lFile=new QLabel(this,&rr,"File");
  rr.SetXY(EDIT_X,y);
  eFile=new QEdit(this,&rr,256);
*/
  // Dialog info (information about selection)
  di=(Q3DDialogInfo*)qcalloc(sizeof(*di));
  di->xa=0;
  di->ya=0;
  di->za=0;
  di->x=0;
  di->y=0;
  di->z=0;

  cbOnChange=0;
  qdbg("Q3DDialog ctor RET\n");
}
Q3DDialog::~Q3DDialog()
{ int i;
  //qdbg("Q3DDialog dtor\n");
  delete bYes;
  //qdbg("1\n");
  delete bNo;
  //qdbg("2\n");
  for(i=0;i<3;i++)
  { delete pRotation[i];
    delete pPos[i];
  }
  //delete pFiles;
  //qdbg("3\n");
  delete lTitle;
  //qdbg("4\n");
  //app->GetWindowManager()->RemoveWindow(this);
  //app->GetWindowManager()->HideDialog(this);
  //qdbg("7\n");
 //skip:
  if(di)
  {
    //...
    qfree(di);
  }
  qdbg("Q3DDialog dtor RET\n");
}

void Q3DDialog::OnChange(CB3DDLG _cb)
{
  cbOnChange=_cb;
}

static int retCode;
static Q3DDialog *fd;
//static QButton *lastBut;		// For dblclicks

static bool fdEH(QEvent *e)
{
  int i;
  if(e->type!=QEVENT_MOTIONNOTIFY)
    qdbg("fdEH event %d; win %p\n",e->type,e->win);
  /*if(e->type==QEVENT_EXPOSE&&e->win==fd)
  { qdbg("Expose dialog (%p)\n",fd);
    app->GetWindowManager()->Paint();
    fd->bNo->Paint();
  }*/
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==fd->bYes)
    { retCode=0;
    } else if(e->win==fd->bNo)retCode=1;
    for(i=0;i<3;i++)
    { if(e->win==fd->pRotation[i]||e->win==fd->pPos[i])
      { // Fill in info structure
        fd->di->xa=fd->pRotation[0]->GetPosition();
        fd->di->ya=fd->pRotation[1]->GetPosition();
        fd->di->za=fd->pRotation[2]->GetPosition();
        fd->di->x=fd->pPos[0]->GetPosition()-MAX_POS_RANGE/2;
        fd->di->y=fd->pPos[1]->GetPosition()-MAX_POS_RANGE/2;
        fd->di->z=fd->pPos[2]->GetPosition()-MAX_POS_RANGE/2;
        //qdbg("change: xyz=%d,%d,%d, pos=%d,%d,%d\n",
        if(fd->cbOnChange)
          fd->cbOnChange(fd->di);
      }
    }
  }
  return FALSE;
}

int Q3DDialog::Execute()
// Returns: 0=file selected
//          1=canceled
{
  QState *state,*oldState;

  qdbg("QFileDlg: execute\n");
  //Show();
  //ReadDir("BACKUP");
  // Switch events to go to our state
  //app->GetWindowManager()->Paint();
  oldState=app->GetStateManager()->GetCurrentState();
  state=new QState("3Ddlg",fdEH);
  app->GetStateManager()->GotoState(state);
  retCode=-1;
  fd=this;
  while(retCode==-1)
  { app->Run1();
    //qdbg("QFD:Events; retCode=%d\n",retCode);
  }
  app->GetStateManager()->GotoState(oldState);
  // Remove state from the manager and delete it
  delete state;
  return retCode;
}

void Q3DDialog::Paint(QRect *r)
// Gets called when expose happens (QDialog)
// and when WindowManager->Paint() is called
{ int i;
  qdbg("QFileDlg:Paint\n");
  bYes->Paint();
  bNo->Paint();
  lTitle->Paint();
  for(i=0;i<3;i++)
  { pRotation[i]->Paint();
    pPos[i]->Paint();
  }
  //app->GetWindowManager()->Paint();
}

