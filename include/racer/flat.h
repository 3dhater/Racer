// racer/flat.h

#ifndef __RACER_FLAT_H
#define __RACER_FLAT_H

#include <racer/types.h>

cstring RFindFile(cstring fname,cstring prefDir=".",cstring fallBackDir=0);
cstring RFindDir(cstring prefDir);

// Scaling
void   RScaleXY(int x,int y,int *px,int *py);
void   RScaleRect(const QRect *rIn,QRect *rOut);
rfloat RScaleX(int n);
rfloat RScaleY(int n);

#endif
