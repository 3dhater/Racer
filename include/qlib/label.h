// qlib/label.h

#ifndef __QLIB_LABEL_H
#define __QLIB_LABEL_H

#include <qlib/window.h>
#include <qlib/font.h>

class QBitMap;

class QLabel : public QWindow
// A text or bitmap without further decorations
{
 public:
  string ClassName(){ return "label"; }
  enum
  { IMAGE=1,
    IMAGE_NOBLEND=2       // Do not use alpha
  };

 protected:
  QColor  *colText;
  string   text;
  QBitMap *bm;		// Bitmap to use (mostly ==img)
  QImage  *img;		// If image was given
  QFont   *font;

 public:
  QLabel(QWindow *parent,QRect *pos,cstring text,int flags=0);
  ~QLabel();

  void SetFont(QFont *font);
  void SetText(cstring text);
  string GetText(){ return text; }
  void Paint(QRect *r=0);

  bool EvButtonPress(int button,int x,int y);
};

#endif
