/*
 * QFXList - a container full of QFX
 * 05-10-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/fx.h>
#include <stdio.h>
#include <stdlib.h>

QFXList::QFXList()
{ head=0;
}
QFXList::~QFXList()
{
}

bool QFXList::IsDone()
{ QFX *i;
  for(i=head;i;i=0)
  { if(!i->IsDone())
      return FALSE;
  }
  return TRUE;
}

void QFXList::Add(QFX *fx)
{
  head=fx;
}

void QFXList::Iterate()
{ QFX *i;
  for(i=head;i;i=0)
    i->Iterate();
}

void QFXList::Run()
{
  printf("QFXList::Run\n");
  while(!IsDone())
  { Iterate();
  }
}

