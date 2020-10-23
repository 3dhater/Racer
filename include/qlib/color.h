// qlib/qcolor - 32-bit color values

#ifndef __QLIB_COLOR_H
#define __QLIB_COLOR_H

#include <qlib/object.h>

class QColor : public QObject
{
 protected:
  ulong rgba;			// RGBA 8-bit per channel
 public:
  QColor(ulong rgba=0);
  QColor(int r,int g,int b,int a=255);
  ~QColor();

  ulong GetRGBA() const;
  ulong GetRGB() const;
  int   GetR() const { return (int)(rgba>>24); }
  int   GetG() const { return (int)((rgba>>16)&0xFF); }
  int   GetB() const { return (int)((rgba>>8)&0xFF); }
  int   GetA() const { return (int)(rgba&0xFF); }

  void SetRGBA(int r,int g,int b,int a=255);
  void Set(const QColor *color);
};

#endif

