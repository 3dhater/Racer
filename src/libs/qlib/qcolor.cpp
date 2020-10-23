/*
 * QColor - generic color values
 * 24-12-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/color.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QColor::QColor(int r,int g,int b,int a)
{
  rgba=(r<<24)|(g<<16)|(b<<8)|(a);
}
QColor::QColor(ulong irgba)
{ rgba=irgba;
}

QColor::~QColor()
{
}

ulong QColor::GetRGBA() const
{
  return rgba;
}
ulong QColor::GetRGB() const
// Don't return Alpha
{
  return ((rgba>>8)&0xFFFFFF);
}

void QColor::SetRGBA(int r,int g,int b,int a)
{
  rgba=(r<<24)|(g<<16)|(b<<8)|(a);
}
void QColor::Set(const QColor *c)
{
  rgba=c->GetRGBA();
}

