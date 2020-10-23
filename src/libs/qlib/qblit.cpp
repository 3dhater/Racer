/*
 * QBlit
 * 23-04-98: Created!
 * NOTES:
 * - 32-bit blitters for linear bitmaps
 * (C) MG/RVG
 */

#include <qlib/blit.h>
#include <qlib/debug.h>
DEBUG_ENABLE

typedef int rgba;

/******************
* 32-bit blitting *
******************/
static void BlitCopy(QBitMap *sbm,int sx,int sy,QBitMap *dbm,int dx,int dy,
  int wid,int hgt)
// Copies a rectangular area
// No blending
{ 
  rgba *s,*d;
  int   modSrc,modDst;
  int   x,y;

  // Address of pixels
  s=(rgba*)sbm->GetBuffer();
  d=(rgba*)dbm->GetBuffer();
  s+=sx+sy*sbm->GetWidth();
  d+=dx+dy*dbm->GetWidth();
  modSrc=sbm->GetWidth()-wid;
  modDst=dbm->GetWidth()-wid;
  for(y=0;y<hgt;y++)
  { for(x=0;x<wid;x++)
    { *d++=*s++;
    }
    s+=modSrc;
    d+=modDst;
  }
}

void QBlit(QBitMap *sbm,int sx,int sy,QBitMap *dbm,int dx,int dy,
  int wid,int hgt,int flags)
{
  // Check parameters
  if(wid<=0||hgt<=0)return;

  //qdbg("QBlit: s=%d/%d, d=%d/%d %dx%d\n",sx,sy,dx,dy,wid,hgt);
  BlitCopy(sbm,sx,sy,dbm,dx,dy,wid,hgt);
}

