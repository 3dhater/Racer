// qlib/canvas.h
// 28-02-99: Now uses QDrawable instead of QWindow
// NOTES:
// - QCanvas implements multiple types of drawing, so you don't have to
//   worry. Still, a lot of functionality may not be implemented for non-OpenGL
//   targets.

#ifndef __QLIB_CANVAS_H
#define __QLIB_CANVAS_H

#include <qlib/window.h>
#include <qlib/bitmap.h>
#include <qlib/point.h>
#include <qlib/textstyle.h>

// QCanvas encapsulates painting of mostly 2D objects

#ifdef OBSOLETE
const int QCANVASF_BLEND=1;		// Use alpha information
const int QCANVASF_STATAREA=2;		// Count the pixels
const int QCANVASF_CLIP=4;		// Use (rect) clipping
const int QCANVASF_OPTBLIT=8;		// Optimize blits
const int QCANVASF_USEX=16;		// Use X GC instead of OpenGL (!)

const int QCANVAS_SINGLEBUF=0;	// Type of buffering (drawbuffer)
const int QCANVAS_DOUBLEBUF=1;

// QCanvas::TextML() alignment flags
const int QCV_ALIGN_CENTERH=1;
const int QCV_ALIGN_CENTERV=2;
#endif

class QBlitQueue;		// Blit optimizer
class QGLContext;
class QGC;
class QFont;

class QCanvas : public QObject
{
 public:
  enum Flags
  { //BLEND=1,				// OBSOLETE (is now 'blendMode')
    STATAREA=2,				// Debug count of glDrawPixels
    CLIP=4,				// Use clipping?
    OPTBLIT=8,				// Optimize blitting?
    USEX=16,				// Use X canvas?
    USEBM=32,				// Use QBitMap painting?
    IS3D=64,				// Canvas is in 2D mode (otherwise 2d)?
    NO_FLIP=128                         // Don't flip coordinates
  };
  enum BufferModes			// Buffering of canvas
  { SINGLEBUF=0,
    DOUBLEBUF=1
  };
  enum BlendModes
  { BLEND_OFF=0,			// Don't blend
    BLEND_SRC_ALPHA=1,			// Most common blend; use alpha
    BLEND_CONSTANT=2,			// Use constant color
    BLEND_COMPOSITE=3			// For TVPaint-like blend
  };
  enum TextAlign			// TextML() alignment flags
  { ALIGN_CENTERH=1,
    ALIGN_CENTERV=2
  };

 protected:
  QDrawable  *drw;
  int         mode;			// Single/double buffered
  int         flags;
  QFont      *font;
  int         offsetx,offsety;
  int         clipx,clipy,clipwid,cliphgt;
  QGLContext *gl;			// The actual OpenGL context
  QGC        *gc;			// X GC if applicable
  QBitMap    *bm;			// Destination bitmap if applicable
  QBlitQueue *bq;			// Used when optimizing blits
  QColor     *color;			// Default fgpen
  QColor      blendColor;		// For blending with single color
  int         blendMode;		// enum BlendModes
  QTextStyle *textStyle;		// Current text style or 0

  void InstallBlendMode();

 public:
  QCanvas(QDrawable *drw=0);
  QCanvas(QCanvas *cv);			// Clone from another canvas
  ~QCanvas();

  //void SetWindow(QDrawable *win);
  QDrawable *GetDrawable(){ return drw; }

  void SetOffset(int ox,int oy);
  void SetClipRect(int x,int y,int wid,int hgt);
  void SetClipRect(QRect *r);
  void GetClipRect(QRect *r);

  QColor *GetColor(){ return color; }
  void    SetColor(QColor *col);
  void    SetColor(int r,int g,int b,int a=255);
  void    SetColor(int pixel);		// QApp::PX_xxx

  void   SetFont(QFont *font);
  QFont *GetFont();

  int GetWidth();
  int GetHeight();

  // Drawing destination type (GLXContext/direct bitmap/X11 GC)
  void SetGLContext(QGLContext *gl);
  QGLContext *GetGLContext(){ return gl; }
  void UseX();
  void UseGL();
  void UseBM(QBitMap *dbm);

  bool UsesX();
  bool UsesGL();
  bool UsesBM();

  void Enable(int newFlags);
  void Disable(int clrFlags);
  bool IsEnabled(int flag);

  // Blending
  void Blend(bool yn);			// Obsolete call; kept for convenience
  void SetBlendColor(QColor *blendCol);
  void SetBlending(int type,QColor *blendCol=0);
  int  GetBlending(){ return blendMode; }

  void SetMode(int mode);		// QCANVAS_SINGLE/DOUBLE BUF
  int  GetMode(){ return mode; }

  // 2D/3D modes
  void Set2D();
  void Set3D();
  bool Is2D();
  bool Is3D();
  bool IsNoFlip(){ if(flags&NO_FLIP)return TRUE; return FALSE; }
  void Map2Dto3D(int *px,int *py);
  void Map3Dto2D(int *px,int *py,int *pz=0);

  void Select();		// Select OpenGL context of canvas

  // Painting
  void DrawPixels(QBitMap *srcBM,int dx=0,int dy=0,int wid=-1,int hgt=-1,
            int sx=-1,int sy=-1);
  void Blit(QBitMap *srcBM,int dx=0,int dy=0,int wid=-1,int hgt=-1,
            int sx=-1,int sy=-1);

  void BlitQueue(QBlitQueue *bq);	// Optimized operation
  void Flush();			// Flush any pending paint operations

  // Getting images
  void ReadPixels(QBitMap *bmDst,int sx=0,int sy=0,int wid=-1,int hgt=-1,
                  int dx=0,int dy=0);
  void CopyPixels(int sx,int sy,int wid,int hgt,int dx=0,int dy=0);

  // Rectangle
  void Rectangle(QRect *r);
  void Rectangle(int x,int y,int wid,int hgt);
  void Rectline(int x,int y,int wid,int hgt);
  void Rectfill(QRect *rz,QColor *colUL,QColor *colUR,QColor *colLR,
    QColor *colLL);
  void Rectfill(int x,int y,int wid,int hgt);
  void Rectfill(QRect *r);

  // Clearing
  void Clear();
  void Clear(QRect *r);

  // Lines (stippling also applies for rectangles)
  void Line(int x1,int y1,int x2,int y2);
  void LineStipple(unsigned short pattern,int factor=1);
  void DisableLineStipple();

  // Triangles
  void Triangle(int x1,int y1,int x2,int y2,int x3,int y3,
    QColor *col1=0,QColor *col2=0,QColor *col3=0);

  // Filling
  void Fill(long pixel,int x,int y,int wid,int hgt,QBitMap *dbm);
  void FillAlpha(int alpha,int x,int y,int wid,int hgt,QBitMap *dbm);

#ifdef NOTDEF
  // Drawing
  void Rectangle(QRect *r);
#endif

  // UI support
  void Outline(int x,int y,int wid,int hgt);
  void Inline(int x,int y,int wid,int hgt);
  void Shade(int x,int y,int wid,int hgt);
  void Insides(int x,int y,int wid,int hgt,bool shade=TRUE);
  void Separator(int x,int y,int wid);
  void StippleRect(int x,int y,int wid,int hgt);

  // Text
  QTextStyle *GetTextStyle(){ return textStyle; }
  void        SetTextStyle(QTextStyle *style);
  void Text(cstring txt,int dx=0,int dy=0,int len=-1);
  void TextML(cstring txt,QRect *r,int align=0);
};

#endif
