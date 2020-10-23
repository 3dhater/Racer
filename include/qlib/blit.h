// blit.h

#ifndef __BLIT_H
#define __BLIT_H

#include <qlib/bitmap.h>

const int QBLITF_BLEND=1;

void QBlit(QBitMap *sbm,int sx,int sy,QBitMap *dbm,int dx,int dy,
  int wid,int hgt,int flags);

#endif
