/*
 * QDrawable - base class for drawable objects
 * (windows/pbuffers/bitmaps/pixmaps)
 * 28-02-99: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/drawable.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDrawable::QDrawable()
{
  cv=0;
}
QDrawable::~QDrawable()
{}

/********
* STUBS *
********/
GLXDrawable QDrawable::GetGLXDrawable()
{
  // Default is to NOT have an associated GLXDrawable
  return 0;
}
Drawable QDrawable::GetDrawable()
{
  // Default is to NOT have an associated Drawable
  return 0;
}

bool QDrawable::Create()
{
  qerr("QDrawable::Create() base class called");
  return TRUE;
}
void QDrawable::Destroy()
{
  qerr("QDrawable::Destroy() base class called");
}

#ifdef OBS_VIRTUAL_BASE_CLASS
int QDrawable::GetWidth()
{ return 0; }
int QDrawable::GetHeight()
{ return 0; }
#endif
