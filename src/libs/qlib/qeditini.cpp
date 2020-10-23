/*
 * QEditIni - edit INI setting
 * 14-06-99: Created!
 * (C) MG/RVG
 */

#include <qlib/editini.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DEFAULT_WID	200
#define DEFAULT_HGT	20

#define DEFAULT_XSPACING	250
#define DEFAULT_YSPACING	25

QEditIni::QEditIni(QEditIniGroup *grp,string sLabel,QInfo *_info,
  string iniPath,int x,int y)
{
  QRect r;
  r.x=x; r.y=y;
  r.wid=DEFAULT_WID;
  r.hgt=DEFAULT_HGT;
  label=new QLabel(app->GetShell(),&r,sLabel);
  info=_info;
  r.x+=DEFAULT_XSPACING; r.y=y;
  edit=new QEdit(app->GetShell(),&r,256,info->GetStringDirect(iniPath));
  path=qstrdup(iniPath);
}

QEditIni::~QEditIni()
{
  delete label;
  delete edit;
  qfree(path);
}

/****************
* EDITINI GROUP *
****************/
QEditIniGroup::QEditIniGroup(int _x,int _y,string _name)
{
  x=_x; y=_y;
  name=qstrdup(_name);
  wid=DEFAULT_WID+DEFAULT_XSPACING+50;
  hgt=50;
  group=0;
  
  // Deduced
  cx=x+25; cy=y+25;
}
QEditIniGroup::~QEditIniGroup()
{ if(group)delete group;
  qfree(name);
}

int QEditIniGroup::GetX()
{ return cx;
}
int QEditIniGroup::GetY()
{ return cy;
}

void QEditIniGroup::Advance()
{ int dy=DEFAULT_YSPACING;
  cy+=dy;
  hgt+=dy;
}

void QEditIniGroup::Create()
{ QRect r;
  if(group)
  { qerr("QEditIniGroup::Create(): multiply called");
  }
  r.x=x; r.y=y; r.wid=wid; r.hgt=hgt;
  group=new QGroup(app->GetShell(),&r,name);
  //group->Lower();
}
