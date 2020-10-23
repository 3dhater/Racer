// qlib/editini.h

#ifndef __QLIB_EDITINI_H
#define __QLIB_EDITINI_H

#include <qlib/label.h>
#include <qlib/edit.h>
#include <qlib/info.h>
#include <qlib/group.h>

class QEditIniGroup;

class QEditIni
// UI elements to edit a value from an INI file
{
 protected:
  QLabel *label;
  QEdit  *edit;
  QInfo  *info;
  string  path;

 public:
  QEditIni(QEditIniGroup *grp,string sLabel,QInfo *_info,string iniPath,int x,int y);
  ~QEditIni();
};

class QEditIniGroup
// Grouped editini's
{
  int x,y;
  int wid,hgt;
  int cx,cy;			// Current generation coordinates
  string name;
  QGroup *group;

 public:
  QEditIniGroup(int x,int y,string name);
  ~QEditIniGroup();

  int GetX();
  int GetY();
  void Advance();
  void Create();
};

#endif
