// qlib/install.h

#ifndef __QLIB_INSTALL_H
#define __QLIB_INSTALL_H

#include <qlib/archive.h>
#include <qlib/progress.h>
#include <qlib/msg.h>
#include <qlib/types.h>

class QInstallVar
{
 public:
  qstring name;
  qstring val;
};

class QInstall : public QObject
// Install class that helps installing .ins/.ar files
// Scripting includes simple variables
{
 //public:
  enum { MAX_IVAR=10 };
 private:
  qstring    name;
  QArchive  *ar;
  QFile     *fIns;
  // Possible UI elements to show status
  QProgress *prg;
  QMsg      *msg;
  int        timing;
  // Script variables
  QInstallVar *ivar[MAX_IVAR];

 public:
  QInstall();
  ~QInstall();

  bool Open(cstring name,cstring iname,cstring arName=0);
  bool Close();

  bool Install();
  bool CopyFile(cstring fname,cstring destDir);
  bool MakeDir(cstring dir);
  bool InstallFont(qstring& fname,qstring& xFontName);

  // Variables
  QInstallVar *FindVar(cstring name);
  QInstallVar *FindFreeVar();
  void ExpandVars(qstring& s);

  // UI feedback
  void DefineProgress(QProgress *prg);
  void DefineMsg(QMsg *msg);
  void UpdateProgress(int cur,int total);
};

#endif
