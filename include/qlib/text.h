// qlib/qtext.h - text gel
// Notes:
// - Lesser point: needs gc to paint in (uses funny buffer tricks)

#ifndef __QLIB_QTEXT_H
#define __QLIB_QTEXT_H

#include <qlib/bob.h>
#include <qlib/image.h>
#include <qlib/control.h>
#include <qlib/font.h>

class QText : public QBob
{
 public:
  string ClassName(){ return "text"; }
  // Shortcuts
  enum Shortcuts
  {
    AA=QGELF_AA
  };

 protected:
  QBitMap *bm;		// Bitmap in which text is located
  QCanvas *cv;		// canvas with which to draw the text
  QFont   *font;	// Font used in creation
  cstring  text;	// Supposedly in the bitmap
  int      twid,thgt;	// This text's size in pixels
  QColor  *col,*colg;	// Color and gradient color (gradient=future)

 public:
  QText(QCanvas *cv,QFont *font,cstring text=0,int flags=0);
  ~QText();

  void SetFont(QFont *font);
  void SetColor(QColor *col,QColor *colg=0);

  void    SetText(cstring txt,int aaType=QBM_AA_ALG_LINEAR);
  cstring GetText(){ return text; }
};

void QTextSetMargins(int x,int y);
void QTextSetOffset(int x,int y);
void QTextSetDefaults();

#endif
