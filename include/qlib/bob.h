// qlib/qbob.h - image gel

#ifndef __QLIB_QBOB_H
#define __QLIB_QBOB_H

#include <qlib/gel.h>
#include <qlib/image.h>
#include <qlib/control.h>
#include <qlib/filter.h>

class QBob : public QGel
{
 public:
  string ClassName(){ return "bob"; }
 protected:
  QBitMap *sbm;		// Original picture
  int sx,sy;		// Source location in sbm
  int swid,shgt;	// Source brush size
  //int curImage;		// Image control
  // Modifying the bob's bitmap
  QBitMap *bmScaled;	// 1 time scaling (scaled original)
  QBitMap *bmFiltered;	// After applying the filters
 public:
  QFilter *filterList;	// List of applicable filters
 public:
  // Dynamic controls
  QControl *x,*y,*wid,*hgt;
  QControl *scale_x,*scale_y;
  QControl *image;		// Standard issue image flipping
  double    zoomX,zoomY;	// Pixel zoom

 public:
  QBob(QBitMap *bm,int flags=0,int sx=0,int sy=0,int wid=-1,int hgt=-1);
  ~QBob();

  QBitMap *GetSourceBitMap(){ return sbm; }
  void GetSourceLocation(QRect *r);
  void SetSourceBitMap(QBitMap *bm);
  void SetSourceLocation(int x,int y,int wid=-1,int hgt=-1);
  void SetSourceSize(int wid,int hgt);
  
  void MarkDirtyScale();
  void MarkDirtyFilter();

  int GetWidth(){ return (int)(swid*zoomX); }
  int GetHeight(){ return (int)(shgt*zoomY); }

  // Real-time modifications
  void AddFilter(QFilter *f);
  void RemoveFilter(QFilter *f);

  virtual void Paint();
  virtual void PaintAt(int x,int y);
  virtual void PaintPart(QRect *r);

  void ApplyFilters();
};


#endif
