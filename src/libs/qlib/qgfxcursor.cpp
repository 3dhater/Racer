/*
 * QGFXCursor - graphical cursor that replaces the X11 cursor
 * 10-06-99: Created!
 * NOTES:
 * - Uses a QBob; may not be appropriate for some apps.
 * (C) MarketGraph/RVG
 */

#include <qlib/gfxcursor.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QGFXCursor::QGFXCursor(string fname)
{
  img=new QImage(fname);
  x=y=0;
  bob=new QBob(img,QGELF_BLEND,0,0,img->GetWidth(),img->GetHeight());
  bob->SetPriority(99999);		// On top
}
QGFXCursor::~QGFXCursor()
{
  delete bob;
  delete img;
}

void QGFXCursor::Move(int x,int y)
{
  bob->x->Set(x);
  bob->y->Set(y);
}

void QGFXCursor::HandleEvent(QEvent *e)
{
  if(e->type==QEvent::MOTIONNOTIFY)
  { Move(e->xRoot,e->yRoot);
  }
}

