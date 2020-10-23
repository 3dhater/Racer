/*
 * QFX - fx base
 * 05-10-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/fx.h>
#include <stdio.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QFX::QFX(int _flags)
{ flags=_flags;
  next=0;
  //qdbg("QFX ctor this=%p\n",this);
  Done(TRUE);
}
QFX::~QFX()
{
  //qdbg("QFX dtor (this=%p)\n",this);
}

void QFX::Iterate()
{ Done(TRUE);
}

