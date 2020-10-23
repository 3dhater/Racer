/*
 * DlgProp
 * 06-12-2000: Created! (15:10:23)
 * NOTES:
 * - Property dialog for CarLab
 * BUGS:
 * - Not reentrant
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include "dlgprop.h"
#include <qlib/debug.h>
DEBUG_ENABLE

#define FILES_Y         18

// Location of prop edit controls
#define EDIT_X          170
#define EDIT_YSPACE	26
#define EDIT_WID        300
#define EDIT_HGT        20

#define MSG_HGT         50

#define YN_WID		100	// OK/Cancel buttons (Yes/No)
#define YN_HGT		40

/*******
* Ctor *
*******/
DlgProp::DlgProp(QWindow *parent,QRect *r,cstring ititle,
  cstring *elts,cstring *help,QInfo **_info,cstring *infoPath,int _props)
  : QDialog(parent,r,ititle)
{
  QRect rr;
  int   i;
  int   y=FILES_Y+10;
  qstring s;
  
  props=_props;
  sHelp=help;
  sInfoPath=infoPath;
  info=_info;
  
  // OK/Cancel
  rr.SetXY(10,r->hgt-50);
  rr.SetSize(YN_WID,YN_HGT);
  bYes=new QButton(this,&rr,"OK");
  bYes->ShortCut(QK_ENTER);
  //rr.SetXY(r->x+r->wid-YN_WID-10,r->y+r->hgt-YN_HGT-10);
  rr.SetXY(r->wid-YN_WID-10,r->hgt-YN_HGT-10);
  bNo=new QButton(this,&rr,"Cancel");
  bNo->ShortCut(QK_ESC);
  
  // Props
  y+=EDIT_YSPACE;
  for(i=0;i<props;i++)
  {
    rr.SetXY(EDIT_X-155,y);
    labProp[i]=new QLabel(this,&rr,elts[i]);
    rr.SetXY(EDIT_X,y-2);
    rr.SetSize(EDIT_WID,EDIT_HGT);
    edProp[i]=new QEdit(this,&rr,256);
    
    // Get setting from info file
    info[i]->GetString(infoPath[i],s);
    edProp[i]->SetText(((string)((cstring)s)));
    
    y+=EDIT_YSPACE;
  }
  y+=25;
  msgHelp=new QMsg(this,10,y,r->wid-20,MSG_HGT);
}
DlgProp::~DlgProp()
{
  int i;
  
  for(i=0;i<props;i++)
  {
    if(labProp[i])delete labProp[i];
    if(edProp[i])delete edProp[i];
  }
  delete msgHelp;
  delete bYes;
  delete bNo;
}

/*********
* Events *
*********/
static int retCode;
static DlgProp *dlg;

static bool fdEH(QEvent *e)
{
  int i;
//qdbg("fdEH(%d)\n",e->type);
  if(e->type==QEvent::SETFOCUS)
  {
    // Set help text
    for(i=0;i<dlg->props;i++)
    {
      if(e->win==dlg->edProp[i])
      {
        dlg->msgHelp->printf("\n\n\n");
        dlg->msgHelp->printf(dlg->sHelp[i]);
      }
    }
  }
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==dlg->bYes)
    {
      retCode=TRUE;
      dlg->Commit();
    } else if(e->win==dlg->bNo)
    {
      retCode=FALSE;
    }
  }
  return FALSE;
}

#ifdef OBS
bool DlgProp::Event(QEvent *e)
{
  qdbg("QStrDlg: Event %d\n",e->type);
  fdEH(e);
  return QDialog::Event(e);
}
#endif

/*********
* Commit *
*********/
void DlgProp::Commit()
{
  int i;
  
  // Write the edit data to the file
  for(i=0;i<props;i++)
  {
    // Put setting into info file
    info[i]->SetString(sInfoPath[i],edProp[i]->GetText());
  }
  // Write results; write won't happen when the file is not dirty
  // anymore.
  for(i=0;i<props;i++)
  {
    info[i]->Write();
  }
}

/**********
* Execute *
**********/
int DlgProp::Execute()
// Returns: 0=file selected
//          1=canceled
{
  QAppEventProc oldEP;
  dlg=this;
  retCode=-1;
  oldEP=app->GetEventProc();
  app->SetEventProc(fdEH);
  // Get dialog up before setting focus (otherwise the help
  // text will disappear because of the painting)
  app->RunPoll();
  app->GetWindowManager()->SetKeyboardFocus(edProp[0]);
  while(retCode==-1)app->Run1();
  app->SetEventProc(oldEP);
  return retCode;
}

void DlgProp::Paint(QRect *r)
{
}

/************************
* CONVENIENCE FUNCTIONS *
************************/
bool RDlgProps(cstring title,cstring *elts,cstring *help,
  QInfo **info,cstring *infoPath,int props)
// Returns TRUE if dialog was OK-ed
{
  DlgProp *dlg;
  bool     ret;
  QRect    r;

  //strcpy(buf,"5");
  r.SetXY(100,150);
  r.SetSize(500,160+props*EDIT_YSPACE+MSG_HGT);
  dlg=new DlgProp(app->GetShell(),&r,title,elts,help,info,infoPath,props);
  ret=(bool)dlg->Execute();
  delete dlg;
  return ret;
}
