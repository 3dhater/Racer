// qlib/drawable.h
// 28-02-99: Created!
// NOTES
// - QDrawable is the base class for windows, pixmaps, pbuffers and bitmaps.
// - Every QDrawable can be drawn to using its QCanvas.

#ifndef __QLIB_DRAWABLE_H
#define __QLIB_DRAWABLE_H

#include <qlib/object.h>
#include <GL/glx.h>

class QCanvas;

class QDrawable : public QObject
// Something you can draw on (windows/pbuffers/bitmaps etc)
// Some may provide a GLXDrawable; that is incorporated here (windows/pbuffers)
// (no subclass QGLXDrawable was created)
// Some may provide a Drawable (X11); also incorporated here (windows/pixmaps)
{
 public:
  string className(){ return "drawable"; }

 protected:
  QCanvas *cv;			// Each drawable can be drawn on
 public:
  QDrawable();
  virtual ~QDrawable();

  QCanvas *GetCanvas(){ return cv; }

  virtual GLXDrawable GetGLXDrawable();
  virtual Drawable    GetDrawable();

  virtual bool Create();
  virtual void Destroy();

  // Functionality for EVERY drawable
  virtual int GetWidth()=0;
  virtual int GetHeight()=0;
};

#endif
