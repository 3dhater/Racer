// dlgprop.h

#ifndef __DLGPROP_H
#define __DLGPROP_H

#include <qlib/dialog.h>

class DlgProp : public QDialog
{
  enum Max
  {
    MAX_PROP=10
  };

 public:
  QButton *bYes,*bNo;
  cstring  title,text;
  string   buf;
  QLabel  *lTitle,*lText;
  QLabel  *labProp[MAX_PROP];
  QEdit   *edProp[MAX_PROP];
  int      props;
  QMsg    *msgHelp;
  // Property data
  cstring *sHelp,*sInfoPath;
  QInfo  **info;

 public:
  DlgProp(QWindow *parent,QRect *r,cstring title,
    cstring *elts,cstring *help,QInfo **info,cstring *infoPath,int props);
  ~DlgProp();

  int  Execute();
  void Paint(QRect *r=0);
  //bool Event(QEvent *e);

  void Commit();
};

bool RDlgProps(cstring title,cstring *elts,cstring *help,
  QInfo **info,cstring *infoPath,int props);

#endif
