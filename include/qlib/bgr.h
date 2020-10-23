// qlib/qbgr.h - background gel

#ifndef __QLIB_QBGR_H
#define __QLIB_QBGR_H

#include <qlib/gel.h>
#include <qlib/image.h>

class QBgr : public QGel
{
 public:
  string ClassName(){ return "bgr"; }
 protected:
  QImage *sbm;

 public:
  QBgr(string name);
  ~QBgr();

  QImage *GetImage(){ return sbm; }
  //int GetWidth(){ return wid; }
  //int GetHeight(){ return hgt; }
  void LoadImage(string fname);

  void Paint();
  void PaintPart(QRect *r);
};


#endif
