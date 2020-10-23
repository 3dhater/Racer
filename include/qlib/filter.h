// qlib/qfilter.h - Filter operations and classes

#ifndef __QLIB_QFILTER_H
#define __QLIB_QFILTER_H

#include <qlib/fx.h>

//
// Filters affect bitmaps (and perhaps sound one day)
// They are used by QFilter classes to do the dirty work.
//

// qfilter.cpp

// x-point averaging (shifts image!)
void QFilterAntiAlias(QBitMap *bm,int nPoints);

// Burning the alpha channel into the color values, based on black
void QFilterBlendBurn(QBitMap *bm);

// Shadows
void QFilterShadow(QBitMap *bm,int dx,int dy,QColor *col);
void QFilter0Outline(QBitMap *bm,QColor *col);

// Shading
void QFilter0Brightness(QBitMap *bm,int strength);	// -100..100
void QFilter0Fade(QBitMap *bm,int strength);
void QFilter0Gray(QBitMap *bm,int strength);		// 0..100; kills color
void QFilter0Colorize(QBitMap *bm,QColor *col,int strength);

class QFilter;
class QControl;
typedef void (*QFilterCallback)(QFilter *f,void *p);

// Automatic filters (FX)
class QFilter : public QObject
{
 public:
  QFilter *next;			// Linked list
  QFilterCallback cbOnChange;
  void *pOnChange;

 public:
  QFilter();
  ~QFilter();

  // Callbacks
  void OnChange(QFilterCallback cbFunc,void *clientPtr);

  void EvChange();

  virtual void Do(QBitMap *bm);
};

// Often-used filter classes
class QFilterBrightness : public QFilter
{public:
  QControl *strength;
 public:
  QFilterBrightness();
  ~QFilterBrightness();

  void Do(QBitMap *bm);
};
class QFilterFade : public QFilter
// Fade out alpha; negative numbers boost up alpha values (blending off)
{public:
  QControl *strength;
 public:
  QFilterFade();
  ~QFilterFade();

  void Do(QBitMap *bm);
};
class QFilterGray : public QFilter
// Gray out colors
{public:
  QControl *strength;
 public:
  QFilterGray();
  ~QFilterGray();

  void Do(QBitMap *bm);
};
class QFilterColorize : public QFilter
// Colorize; Amiga spread
{public:
  QControl *strength;
  QColor   *col;
 public:
  QFilterColorize(QColor *col);
  ~QFilterColorize();

  void SetColor(QColor *color);
  void Do(QBitMap *bm);
};

#endif
