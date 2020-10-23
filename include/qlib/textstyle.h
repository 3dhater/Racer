// qlib/textstyle.h - text layering

#ifndef __QLIB_TEXTSTYLE_H
#define __QLIB_TEXTSTYLE_H

#include <qlib/color.h>

struct QTextStyleLayer
{
  int    x,y;		// Offset
  QColor color;		// RGB + alpha
};

class QTextStyle : public QObject
{
 public:
  string ClassName(){ return "textstyle"; }
  enum
  { MAX_LAYER=10
  };

 protected:
  QTextStyleLayer layer[MAX_LAYER];
  int             layers;

 public:
  QTextStyle();
  ~QTextStyle();

  int              GetLayers(){ return layers; }
  QTextStyleLayer *GetLayer(int n);

  bool AddLayer(const QColor *col,int x,int y);
};

#endif
