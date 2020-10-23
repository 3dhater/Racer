// qlib/dbfield.h

#ifndef __QLIB_DBFIELD_H
#define __QLIB_DBFIELD_H

#include <qlib/edit.h>
#include <qlib/label.h>
#include <qlib/database.h>

class QDBField : public QObject
{
 public:
  string ClassName(){ return "dbfield"; }
 protected:
  QEdit *edit;
  string labStr;
  QLabel *label;
 public:
  QDBField(int x,int y,int wid,int hgt,int maxLen,string lab,int lines=1);
  ~QDBField();

  QEdit *GetEdit(){ return edit; }
  QLabel *GetLabel(){ return label; }

  void SetLabel(string s);
  void SetText(string text);
};

#endif
