/*
 * QFileDialog - file selection
 * 24-06-97: Created!
 * 18-03-00: Updated; title bar, listbox
 * NOTES:
 * - Basic file requester for Lingo/Mach6
 * FUTURE:
 * - Columnized, tabstopped listbox (info on file)
 * - Thumbnailing, networking etc etc etc
 * BUGS:
 * - Not reentrant (2 file reqs)
 * - Paint() method doesn't paint ALL window
 * - Does not free all strings upon delete
 * (C) MG/RVG
 */

#include <qlib/dialog.h>
#include <qlib/app.h>
#include <qlib/button.h>
#pragma hdrstop
#ifdef WIN32
// getcwd()
#include <direct.h>
#endif
#ifdef linux
// getcwd()
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DLG_WID		400
#define DLG_HGT		400

#define FILES_X		12
#define FILES_Y		35
#define FILE_WID        (DLG_WID-2*FILES_X)
#define FILE_HGT	20

#define EDIT_WID        286
#define EDIT_HGT	20
#define EDIT_X          105
#define EDIT_YSPACE	26

#define YN_WID		100	// OK/Cancel buttons (Yes/No)
#define YN_HGT		40

QFileDialog::QFileDialog(QWindow *parent,QRect *r,cstring title)
  : QDialog(parent,r,title)
{ int i;
  QRect rr;
  
//qdbg("QFileDialog ctor; this=%p\n",this);

  rr.SetSize(FILE_WID,FILE_HGT*MAX_DSP_FILES);
  rr.SetXY(FILES_X,FILES_Y);
  lbFiles=new QListBox(this,&rr,0);
  
  // Ok/Cancel
  rr.SetXY(10,r->hgt-50);
  rr.SetSize(YN_WID,YN_HGT);
  bYes=new QButton(this,&rr,"Open");

  // Extra functions
  rr.SetXY(r->wid/2-YN_WID/2,r->hgt-YN_HGT-10);
  bParent=new QButton(this,&rr,"Parent");

  //rr.SetXY(r->x+r->wid-YN_WID-10,r->y+r->hgt-YN_HGT-10);
  rr.SetXY(r->wid-YN_WID-10,r->hgt-YN_HGT-10);
  bNo=new QButton(this,&rr,"Cancel");

  // Dir/file/filter entry
  int y=FILES_Y+MAX_DSP_FILES*FILE_HGT+10;
  rr.SetXY(12,y+2); rr.SetSize(EDIT_WID,EDIT_HGT);
  lDir=new QLabel(this,&rr,"Directory");
  rr.SetXY(EDIT_X,y);
  eDir=new QEdit(this,&rr,256);
  y+=EDIT_YSPACE;

  rr.SetXY(12,y+2);
  lFilter=new QLabel(this,&rr,"Filter");
  rr.SetXY(EDIT_X,y);
  eFilter=new QEdit(this,&rr,256);
  y+=EDIT_YSPACE;

  rr.SetXY(12,y+2);
  lFile=new QLabel(this,&rr,"File");
  rr.SetXY(EDIT_X,y);
  eFile=new QEdit(this,&rr,256);

  // Dialog info (information about selection)
  fdi=(QFileDialogInfo*)qcalloc(sizeof(*fdi));
  fdi->filter=qstrdup("*");

//qdbg("QFileDialog ctor RET\n");
}
QFileDialog::~QFileDialog()
{
//qdbg("QFileDialog dtor\n");
  delete bYes;
  delete bNo;
  delete bParent;
  delete lbFiles;
  delete lFile; delete eFile;
  delete lFilter; delete eFilter;
  delete lDir; delete eDir;
  if(fdi)
  { if(fdi->dir){ qfree(fdi->dir); fdi->dir=0; }
    if(fdi->file){ qfree(fdi->file); fdi->file=0; }
    if(fdi->filter){ qfree(fdi->filter); fdi->filter=0; }
    qfree(fdi);
  }
//qdbg("QFileDialog dtor RET\n");
}

void QFileDialog::SetDir(string s)
{
//qdbg("QFileDlg:SetDir(%s)\n",s);
  char buf[FILENAME_MAX];
  
  if(fdi->dir)qfree(fdi->dir);
  if(!strcmp(s,".")||s[0]==0)
  { // Replace this by the full path
    s=getcwd(buf,sizeof(buf));
  }
  fdi->dir=qstrdup(s);
//qdbg("  set to '%s'\n",fdi->dir);
}
void QFileDialog::SetFile(string s)
{
  if(fdi->file)qfree(fdi->file);
  fdi->file=qstrdup(s);
}
void QFileDialog::SetFilter(string s)
{
  if(fdi->filter)qfree(fdi->filter);
  fdi->filter=qstrdup(s);
}

/*****************
* READ DIRECTORY *
*****************/
void QFileDialog::ShowDir()
{ int i,base;
  eDir->SetText(fdi->dir);
  eFile->SetText(fdi->file);
  eFilter->SetText(fdi->filter);
}
bool QFileDialog::ReadDir(string dir)
{
  lbFiles->AddDir(dir);
  ShowDir();
  return TRUE;
}

static int retCode;
static QFileDialog *fd;

bool QFileDialog::Event(QEvent *e)
{
//qdbg("QFileDialog:Event(%d)\n",e->type);
  return QDialog::Event(e);
}
static bool fdEH(QEvent *e)
{ int i,n;
  char buf[FILENAME_MAX];
  char selDir[FILENAME_MAX];
  
#ifdef OBS
  if(e->type!=QEVENT_MOTIONNOTIFY)
    qdbg("fdEH event %d; win %p; dir=%s\n",e->type,e->win,fd->fdi->dir);
#endif
  if(e->type==QEvent::CLICK)
  {
    if(e->win==fd->lbFiles)
    {
      // A file/dir was selected
      n=fd->lbFiles->GetSelectedItem();
      if(n<0)return FALSE;
      // Dir or file?
      sprintf(selDir,"%s",fd->lbFiles->GetItemText(n));
     enter_dir:
      sprintf(buf,"%s/%s",fd->GetDir(),selDir);
      if(QIsDir(buf))
      {
        // Directory
        char newdir[FILENAME_MAX];
        QFollowDir(fd->GetDir(),selDir,newdir);
        fd->SetDir(newdir);
        fd->ReadDir(fd->fdi->dir);
      } else
      { // File
        fd->eFile->SetText((string)selDir);
        fd->eFile->Paint();
      }
      return TRUE;
    }
  }
  if(e->type==QEvent::DBL_CLICK)
  { if(e->win==fd->lbFiles)
    { // Double clicked file; check if it is not a directory
      sprintf(buf,"%s/%s",fd->GetDir(),
        fd->lbFiles->GetItemText(fd->lbFiles->GetSelectedItem()));
      if(!QIsDir(buf))
      { 
//qdbg("'%s' accepted as file\n",buf);
        goto do_yes;
      }
    }
  }
  if(e->type==QEvent::CLOSE)
  { // Same as Cancel
    retCode=1;
    return TRUE;
  }
  if(e->type==QEVENT_CLICK)
  { 
    if(e->win==fd->bYes)
    {do_yes:
      retCode=0;
      // Use the file in the edit window
      if(fd->fdi->file)qfree(fd->fdi->file);
      fd->fdi->file=qstrdup(fd->eFile->GetText());
      return TRUE;
    } else if(e->win==fd->bNo)retCode=1;
    else if(e->win==fd->bParent)
    { sprintf(selDir,"..");
      goto enter_dir;
    }
    //else if(e->win==fd->pFiles)fd->ShowDir();
    else if(e->win==fd->eFile)
    { // Accept this file if ENTER was used
      goto do_yes;
    } else if(e->win==fd->eDir)
    { // Use this dir
      fd->SetDir(fd->eDir->GetText());
      fd->ReadDir(fd->fdi->dir);
      QWM->SetKeyboardFocus(fd->eFile);
      return TRUE;
    }
  }
  //return QDialog::Event(e);
  return FALSE;
}

int QFileDialog::Execute()
// Returns: 0=file selected
//          1=canceled
{
  QAppEventProc oldEP;

  string s=qstrdup(GetDir());
  ReadDir(s);
  qfree(s);

  oldEP=app->GetEventProc();
  app->SetEventProc(fdEH);

  retCode=-1;
  fd=this;
  QWM->SetKeyboardFocus(eFile);
  while(retCode==-1)app->Run1();
  app->SetEventProc(oldEP);

  return retCode;
}

void QFileDialog::Paint(QRect *r)
// Gets called when expose happens (QDialog)
// and when WindowManager->Paint() is called
{
}

/**************
* CONVENIENCE *
**************/
bool QDlgFile(string title,string idir,string ifile,string ipattern,
  string outpath,string outdir,string outfile,string outfilter,int x,int y)
// Convencience file dialog function
// 'out*' vars may be 0 (no output generated for that parameter)
{ QFileDialog *fdlg;
  bool rc=FALSE;
  QRect r(x,y,400,510);
  
//qdbg("QDlgFile()\n");
  fdlg=new QFileDialog(app->GetShell(),&r,title);
  fdlg->SetDir(idir);
  fdlg->SetFile(ifile);
  if(fdlg->Execute()==0)
  {
//qdbg("QDlgFile: '%s' in '%s' selected\n",fdlg->GetFile(),fdlg->GetDir());
    if(outdir)strcpy(outdir,fdlg->GetDir());
    if(outfile)strcpy(outfile,fdlg->GetFile());
    if(outfilter)strcpy(outfilter,fdlg->GetFilter());
    if(outpath)
    { sprintf(outpath,"%s/%s",fdlg->GetDir(),fdlg->GetFile());
    }
    rc=TRUE;
  }// else qdbg("FileDialog canceled\n");
  delete fdlg;
  return rc;
}
bool QDlgFile(string title,string idir,string ifile,string ipattern,
  string outpath,string outdir,string outfile,int x,int y)
// Older version which doesn't supply an outfile
{
  return QDlgFile(title,idir,ifile,ipattern,outpath,outdir,outfile,0,x,y);
}
bool QDlgFile(string title,string idir,string ifile,string ipattern,
  string outpath,string outdir,string outfile)
{
  return QDlgFile(title,idir,ifile,ipattern,outpath,outdir,outfile,100,150);
}
bool QDlgFile(cstring title,QInfo *info,cstring infoPath,
  string outpath,string outdir,string outfile)
// Get a file, using last saved settings in the specified 'info' file
// 'outpath', 'outdir' and 'outfile' may be 0
// This is preferred over explicit functions, so the entered values
// get stored in the info file for later retrieval.
{
  char idir[1024],ifile[1024],ifilter[1024];
  char odir[1024],ofile[1024],ofilter[1024];
  char buf[256];
  int  x,y;
  bool result;

  // Retrieve last-used settings
  sprintf(buf,"qfiledlg.%s.x",infoPath);
  x=info->GetInt(buf);
  sprintf(buf,"qfiledlg.%s.y",infoPath);
  y=info->GetInt(buf);
  sprintf(buf,"qfiledlg.%s.dir",infoPath);
  info->GetString(buf,idir);
  sprintf(buf,"qfiledlg.%s.file",infoPath);
  info->GetString(buf,ifile);
  sprintf(buf,"qfiledlg.%s.filter",infoPath);
  info->GetString(buf,ifilter);

#ifdef WIN32
  // On Win32, rather directly use Win32's file dialog
  // That dialog is not bad at all and much more common
  OPENFILENAME ofn;
  char szFileName[MAX_PATH];

  ZeroMemory(&ofn, sizeof(ofn));
  szFileName[0] = 0;

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = 0; //hwnd;
  //ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
  ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
  ofn.lpstrFile = szFileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_EXPLORER | /*OFN_FILEMUSTEXIST |*/ OFN_HIDEREADONLY;
  ofn.lpstrDefExt = 0; //"txt";

  if(GetOpenFileName(&ofn)){
     //MessageBox(hwnd, szFileName, "You chose...", MB_OK);
    qdbg("You chose: '%s'\n",szFileName);
    result=TRUE;
    if(outpath)
    {
      strcpy(outpath,szFileName);
    }
  } else
  {
    result=FALSE;
  }
#else
  // SGI/IRIX version
  // Request file (explicit)
  QFileDialog *fdlg;
  QRect        r(x,y,400,510);
 
  fdlg=new QFileDialog(app->GetShell(),&r,title);
  fdlg->SetDir(idir);
  fdlg->SetFile(ifile);
  fdlg->SetFilter(ifilter);
  if(fdlg->Execute()==0)
  {
    // File selected
    result=TRUE;
    if(outdir)strcpy(outdir,fdlg->GetDir());
    if(outfile)strcpy(outfile,fdlg->GetFile());
    //if(outfilter)strcpy(outfilter,fdlg->GetFilter());
    if(outpath)
    { sprintf(outpath,"%s/%s",fdlg->GetDir(),fdlg->GetFile());
    }

    // Store settings in QInfo file
    sprintf(buf,"qfiledlg.%s.x",infoPath);
    info->SetInt(buf,fdlg->GetX());
    sprintf(buf,"qfiledlg.%s.y",infoPath);
    info->SetInt(buf,fdlg->GetY());
    sprintf(buf,"qfiledlg.%s.dir",infoPath);
    info->SetString(buf,fdlg->GetDir());
    sprintf(buf,"qfiledlg.%s.file",infoPath);
    info->SetString(buf,fdlg->GetFile());
    sprintf(buf,"qfiledlg.%s.filter",infoPath);
    info->SetString(buf,fdlg->GetFilter());
    // Flush
    if(!info->Write())
    { qwarn("QFileDlg: can't save settings for '%s'",infoPath);
    }
  } else result=FALSE;
  delete fdlg;
#endif

  return result;
}

