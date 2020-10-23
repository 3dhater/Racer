// qlib/dialog.h

#ifndef __QLIB_DIALOG_H
#define __QLIB_DIALOG_H

#include <qlib/object.h>
#include <qlib/color.h>
#include <qlib/image.h>
#include <qlib/window.h>
#include <qlib/shell.h>
#include <qlib/edit.h>
#include <qlib/label.h>
#include <qlib/listbox.h>
#include <qlib/titlebar.h>
#include <qlib/progress.h>
#include <qlib/event.h>
#include <X11/X.h>			// 'Window' type

#ifndef WIN32
// Windows-like returns
#define IDOK		1		// OK button pressed
#define IDCANCEL	2		// Dialog cancelled
#endif

// Base dialog
class QDialog : public QShell
{
 protected:
  //QWMInfo  wmInfo;			// Info to be able to pop up/down
  //QShell  *oldShell;			// Old app shell
  QWindow   *oldKeyboardFocus;		// Who had the keyboard focus?
  QTitleBar *titleBar;			// Default title bar
  bool       bDrag;
  int        dragX,dragY;		// Point of drag

 public:
  QDialog(QWindow *parent,QRect *r,cstring title);
  ~QDialog();

  //QWMInfo *GetQWMInfo(){ return &wmInfo; }

  void Show();
  void Hide();
  virtual int Execute();

  void Paint(QRect *r=0);
  bool EvButtonPress(int n,int x,int y);
  bool EvButtonRelease(int n,int x,int y);
  bool EvMotionNotify(int x,int y);
  bool EvExpose(int x,int y,int wid,int hgt);
  bool Event(QEvent *e);
};

// Simple default dialogs (often used)

//
// File dialog (qfiledlg.cpp)
//
struct QFileDialogInfo
{
  string dir;
  string file;
  string filter;
};

class QFileDialog : public QDialog
{
 public:
  enum Max
  { MAX_DSP_FILES=16,
    MAX_DIR_FILES=256
  };

 public:
  QButton *bFile[MAX_DSP_FILES];
  QButton *bYes,*bNo;
  QFileDialogInfo *fdi;
  QListBox *lbFiles;
  QProp   *pFiles;
  QButton *bUp,*bDown,*bParent;
  QLabel  *lTitle,*lDir,*lFile,*lFilter;
  QEdit   *eDir,*eFile,*eFilter;

  void ShowDir();
  bool ReadDir(string dir);

 public:
  QFileDialog(QWindow *parent,QRect *r,cstring title=0);
  ~QFileDialog();

  QFileDialogInfo *GetFileDialogInfo(){ return fdi; }
  string GetDir(){ return fdi->dir; }
  string GetFile(){ return fdi->file; }
  string GetFilter(){ return fdi->filter; }
  void SetDir(string s);
  void SetFile(string s);
  void SetFilter(string s);

  bool Event(QEvent *e);

  int Execute();
  void Paint(QRect *r=0);
};

// Preferred version which stores entered settings in an info file
bool QDlgFile(cstring title,QInfo *info,cstring infoPath,
  string outpath,string outdir=0,string outfile=0);
// Version which returns filter
bool QDlgFile(cstring title,cstring idir,cstring ifile,cstring ipattern,
  string outpath,string outdir,string outfile,string outfilter,int x,int y);
// Older version without 'outfilter'
bool QDlgFile(string title,string idir,string ifile,string ipattern,
  string outpath,string outdir,string outfile,int x,int y);
// Version without coordinates
bool QDlgFile(string title,string idir,string ifile,string ipattern,
  string outpath,string outdir=0,string outfile=0);

//
// 3D dialog (q3ddlg.cpp)
//
struct Q3DDialogInfo
{ int xa,ya,za;			// Rotation angles
  int x,y,z;			// Position in 3D
};

typedef void (*CB3DDLG)(Q3DDialogInfo *p);

class Q3DDialog : public QDialog
{
 public:
  QButton *bYes,*bNo;
  QProp *pRotation[3],		// XYZ rotation
        *pPos[3];		// Positioning
  QLabel  *lTitle;
  Q3DDialogInfo *di;
  CB3DDLG cbOnChange;		// Modified prop

 public:
  Q3DDialog(QWindow *parent,QRect *r);
  ~Q3DDialog();

  void OnChange(CB3DDLG cb);

  Q3DDialogInfo *GetInfo(){ return di; }

  int Execute();
  void Paint(QRect *r=0);
};

// String dialog (qstrdlg.cpp)

class QStringDialog : public QDialog
{
 public:
  QButton *bYes,*bNo;
  cstring  title,text;
  string   buf;
  QLabel  *lTitle,*lText;
  QEdit   *eString;

 public:
  QStringDialog(QWindow *parent,QRect *r,cstring title,cstring text,string buf,
    int lines=1);
  ~QStringDialog();

  string GetText(){ return eString->GetText(); }

  int Execute();
  void Paint(QRect *r=0);
  //bool Event(QEvent *e);
};

class QTextDialog : public QDialog
// Used for message boxes
{
 public:
  enum Flags
  { OK_ONLY=1             // No cancel
  };

 public:
  QButton *bYes,*bNo;
  cstring  title,text;
  QLabel  *lTitle,*lText;

 public:
  QTextDialog(QWindow *parent,QRect *r,cstring title,cstring text,int flags=0);
  ~QTextDialog();

  void SetButtonText(cstring sYes,cstring sNo);
  int Execute();
  void Paint(QRect *r=0);
};

class QProgressDialog : public QDialog
// Progress dialog; optional Cancel button
{
 public:
  enum Flags
  {
    NO_CANCEL=1,	// Don't show a Cancel button
    CANCEL_PRESSED=2	// Runtime flag to indicate Cancel was pressed
  };
  int        pdlgFlags;
  QButton   *bCancel;
  cstring    text;
  QLabel    *lText;
  QProgress *prg;

 public:
  QProgressDialog(QWindow *parent,QRect *r,cstring title,cstring text,
    int flags=0);
  ~QProgressDialog();

  QProgress *GetProgress(){ return prg; }

  bool Create();
  bool Poll();
  void SetProgress(int current,int total);
  void SetProgressText(cstring text);
  bool Event(QEvent *e);
};

// Convenience functions (flat)
int QDlgString(cstring title,cstring text,string buf,int maxLen,
  int lines=1,QRect *pos=0);
bool QDlgEditFile(cstring fname,int maxLine,cstring title=0,int flags=0);

#define QMB_OK_ONLY	1
int QMessageBox(cstring title,cstring text,int flags=0,QRect *pos=0);

#endif
