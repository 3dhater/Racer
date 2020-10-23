// qlib/qimagefx.h - effects on image gels (Bobs)

#ifndef __QLIB_QIMAGEFX_H
#define __QLIB_QIMAGEFX_H

#include <qlib/fx.h>
#include <qlib/rect.h>
#include <qlib/point.h>
#include <qlib/bob.h>

class QImageFX : public QObject
{
  string ClassName(){ return "fx"; }
 protected:
  int flags;
  QBob *bob;

 public:
  QImageFX(QBob *bob,int flags=0);
  ~QImageFX();

  // Using
  void Iterate();
  int GetBob(){ return bob; }
};

#endif
