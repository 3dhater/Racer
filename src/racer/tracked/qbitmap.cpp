/*
 * QBitmap - image storage space (Pixmap in X-terms)
 * 01-10-96: Created!
 * ??-??-98: Support for writing RGB images
 * 03-07-98: Support to write TGA images
 * 29-08-01: Added rotation operations (0/90/180/270 degrees).
 * NOTES:
 * - This file should be optimized to the max
 * - Perhaps we should incorporate the readers (RGB/TGA) from QImage here.
 *   (because the writers are also here)
 * FUTURE:
 * - 8-bit bitmaps (grayscale only)
 * BUGS:
 * - ConvertToABGR() actually just swaps byte-orders; it doesn't check
 *   the current format
 * (C) MarketGraph/RVG
 */

#include <qlib/bitmap.h>
#include <qlib/image.h>			// Writing
#include <qlib/filter.h>
#include <qlib/debug.h>
#include <stdio.h>
#ifndef WIN32
#include <bstring.h>
#endif
#include <stdlib.h>
DEBUG_ENABLE

#ifdef WIN32
#define bcopy(s,d,n)  memcpy(d,s,n)
#endif

#define MAX_WID_WR  4096                // Max width to write, RGBA

QBitMap::QBitMap(int _dep,int _wid,int _hgt,int _flags)
  : QDrawable()
{
  flags=_flags;
  dep=_dep;
  wid=_wid;
  hgt=_hgt;
  mem=0;

  if(dep!=0&&wid!=0&&hgt!=0)
    Alloc(dep,wid,hgt,flags);
}
QBitMap::QBitMap(QBitMap *bm)
// Clone a bitmap
{
  flags=bm->GetFlags();
  dep=bm->GetDepth();
  wid=bm->GetWidth();
  hgt=bm->GetHeight();
  mem=0;
  if(dep!=0&&wid!=0&&hgt!=0)
    Alloc(dep,wid,hgt,flags);
}

QBitMap::~QBitMap()
{ Free();
}

/*********
* Memory *
*********/
bool QBitMap::Alloc(int _dep,int _wid,int _hgt,int _flags,void *p)
{ long size;

  QASSERT_0(_dep==24||_dep==32);	// Only 24 or 32 bpl bitmap supported on SGI

  Free();			// Free any currently allocated bitmap
  dep=_dep;
  wid=_wid; hgt=_hgt;
  flags=_flags;

  size=(dep+7)/8;		// Get #bytes per pixel
  size=size*wid*hgt;
  memSize=size;
  if(p)
  { // Already allocated (used by DMbuffer functions to fake QBitMap instances)
    mem=(char*)p;
    flags|=QBMF_USERALLOC;
  } else
  { mem=(char*)qcalloc(memSize);
    if(!mem)qwarn("QBitMap::Alloc() failed on %dx%d, depth %d, flags %d\n",
      dep,wid,hgt,flags);
  }
  if(!mem)return FALSE;
  return TRUE;
}

void QBitMap::Free()
{ if(mem)
  { if(!(flags&QBMF_USERALLOC))
      qfree(mem);
    mem=0; memSize=0;
    wid=hgt=dep=0;
  }
}

/**********
* Attribs *
**********/
int QBitMap::GetWidth()
{ return wid; }
int QBitMap::GetHeight()
{ return hgt; }
char *QBitMap::GetAddr(int x,int y)
// Returns a pointer to the buffer at (x,y)
{
  return (char*)(mem+x*4+y*GetModulo());
}

void QBitMap::Mask(int /*plane*/)
{}
void QBitMap::Clear(int /*plane*/)
{}

/*********************
* BITMAP INTERACTION *
*********************/
QBitMap *QBitMap::CreateClone(bool copyImage)
// Creates a clone instance
{ QBitMap *bm;
  bm=new QBitMap(dep,wid,hgt,flags);
  if(!bm)return bm;
  if(copyImage)
    bm->CopyBitsFrom(this);
  return bm;
}

/***********
* Blitting *
***********/
void QBitMap::CopyBitsFrom(QBitMap *bm)
// Copy entire bitmap from 'bm'
{
  int count;
  // Never overflow the bitmap bytes
  count=GetBufferSize();
  if(bm->GetBufferSize()<count)
    count=bm->GetBufferSize();
  bcopy(bm->GetBuffer(),mem,count);
}
void QBitMap::CopyPixelsFrom(QBitMap *sbm,int sx,int sy,int wid,int hgt,
  int dx,int dy)
// Copy a subrectangle from another bitmaps
// Assumes 32-bit pixels (RGBA or variant)
{ int x,y;
  long *src,*dst;
  // Reverse bitmap coordinates
//CopyBitsFrom(sbm); return;
  sy=sbm->GetHeight()-hgt-sy;
  dy=GetHeight()-hgt-dy;

  // Copy subrect
  for(y=0;y<hgt;y++)
  { src=((long*)sbm->GetBuffer())+(y+sy)*sbm->GetWidth()+sx;
    dst=((long*)mem)+(dy+y)*GetWidth()+dx;
    for(x=0;x<wid;x++)
      *dst++=*src++;
  }
}

//
// Channel operations
//
void QBitMap::CopyChannel(QBitMap *sbm,int srcChannel,int dstChannel,
  int sx,int sy,int wid,int hgt,int dx,int dy)
// Copy a channel from one bitmap into on of our channels
// Default arguments are used for dstChannel/sx/sy etc
// Assumptions: 32-bit only
{ int x,y;
  char *src,*dst;

  if(dep!=32)
  { qwarn("QBitMap::CopyChannel() only supported for 32-bit");
    return;
  }

  // Default args
  if(dstChannel==-1)dstChannel=srcChannel;
  if(wid==-1)wid=sbm->GetWidth();
  if(hgt==-1)hgt=sbm->GetHeight();

  // Check args
  //...

  // Reverse bitmap coordinates
  sy=sbm->GetHeight()-hgt-sy;
  dy=GetHeight()-hgt-dy;

  // Copy subrect
  for(y=0;y<hgt;y++)
  { src=((char*)sbm->GetBuffer())+(y+sy)*sbm->GetWidth()*4+sx*4+srcChannel;
    dst=((char*)mem)+(dy+y)*GetWidth()*4+dx*4+dstChannel;
    for(x=0;x<wid;x++)
    { *dst=*src;
      dst+=4; src+=4;
    }
  }
}
void QBitMap::InvertChannel(int channel,int sx,int sy,int wid,int hgt)
// Invert a channel (possibly just a subrectangle)
// Default arguments are used to invert entire channel
// Assumptions: 32-bit only
{ int x,y;
  char *dst;

  if(dep!=32)
  { qwarn("QBitMap::InvertChannel() only supported for 32-bit");
    return;
  }

  // Default args
  if(wid==-1)wid=GetWidth();
  if(hgt==-1)hgt=GetHeight();

  // Check args
  //...

  // Reverse bitmap coordinates
  //sy=sbm->GetHeight()-hgt-sy;
  sy=GetHeight()-hgt-sy;

  for(y=0;y<hgt;y++)
  { //src=((char*)sbm->GetBuffer())+(y+sy)*sbm->GetWidth()*4+sx*4+srcChannel;
    dst=((char*)mem)+(sy+y)*GetWidth()*4+sx*4+channel;
    for(x=0;x<wid;x++,dst+=4)
    { *dst=~*dst;
    }
  }
}

/***************
* Pixel colors *
***************/
void QBitMap::GetColorAt(int x,int y,QColor *c)
{
  unsigned char *p;
  y=GetHeight()-y-1;
  p=(unsigned char*)(mem+((y*GetWidth())+x)*4);
  c->SetRGBA(p[0],p[1],p[2],p[3]);
} 
void QBitMap::SetColorAt(int x,int y,QColor *c)
{
  unsigned char *p;
  y=GetHeight()-y-1;
  p=(unsigned char*)(mem+((y*GetWidth())+x)*4);
  p[0]=c->GetR();
  p[1]=c->GetG();
  p[2]=c->GetB();
  p[3]=c->GetA();
}

/********************
* FORMAT CONVERSIONS *
*********************/
static void cvtShorts(ushort *buffer,long n)
// From Paul Haeberli source code
{
#ifdef __sgi
  short i;
  long nshorts = n>>1;
  unsigned short swrd;

  for(i=0; i<nshorts; i++)
  { swrd = *buffer;
    *buffer++ = (swrd>>8) | (swrd<<8);
  }
#endif
}
static void cvtLongs(long *buffer,int n)
// From Paul Haeberli's libimage
// Should be intel.cpp or something
{ //int i;
  unsigned long lwrd;
  //n/=4;
  //for(i=0;i<n;i++)
  for(n/=4;n>0;n--)
  { lwrd=*buffer;
    *buffer++=((lwrd>>24)         |
              (lwrd>>8 & 0xff00)  |
              (lwrd<<8 & 0xff0000)|
              (lwrd<<24)          );
  }
}
void QBitMap::ConvertToABGR()
{ cvtLongs((long*)mem,memSize);
}

/*******************
* Rotating squares *
*******************/
void QBitMap::Rotate90(QRect *s,QBitMap *dbm,QRect *d)
// RGBA 32-bit only
{
  long *pSrc,*pDst;
  int   x,y;
  
  for(y=0;y<s->hgt;y++)
  {
    pSrc=(long*)GetAddr(s->x,s->y+y);
    pDst=(long*)dbm->GetAddr(d->x+y,d->y+s->hgt-1);
    for(x=0;x<s->wid;x++)
    {
      *pDst=*pSrc++;
      pDst-=dbm->GetModulo()/4;
    }
  }
}
void QBitMap::Rotate270(QRect *s,QBitMap *dbm,QRect *d)
// RGBA 32-bit only
{
  long *pSrc,*pDst;
  int   x,y;
  
  for(y=0;y<s->hgt;y++)
  {
    pSrc=(long*)GetAddr(s->x,s->y+y);
    pDst=(long*)dbm->GetAddr(d->x+s->hgt-1-y,d->y);
    for(x=0;x<s->wid;x++)
    {
      *pDst=*pSrc++;
      pDst+=dbm->GetModulo()/4;
    }
  }
}
void QBitMap::Rotate180(QRect *s,QBitMap *dbm,QRect *d)
// RGBA 32-bit only; actually more a mirror
{
  long *pSrc,*pDst;
  int   x,y;
  
  for(y=0;y<s->hgt;y++)
  {
    pSrc=(long*)GetAddr(s->x,s->y+y);
    pDst=(long*)dbm->GetAddr(d->x+s->wid-1,d->y+s->hgt-1-y);
    for(x=0;x<s->wid;x++)
    {
      *pDst=*pSrc++;
      pDst--;
    }
  }
}
void QBitMap::Rotate(int angle,QRect *s,QBitMap *dbm,QRect *d)
// Rotates part of a bitmap into another bitmap.
// Angle=0, 90, 180 or 270. Angle=0 does a regular blit
// From 'd', only d->x and d->y are used, d->wid/d->hgt are ignored.
// Area must be rectangular, except for angle=0 (and angle=180?).
{
  if(angle==0)
  {
    dbm->CopyPixelsFrom(this,s->x,s->y,s->wid,s->hgt,d->x,d->y);
  } else
  {
    // Check rectangularity
    if(s->wid!=s->hgt)
    {
      qwarn("QBitMap:Rotate(); can only do rectangular regions, not %dx%d",
        s->wid,s->hgt);
      return;
    }
    switch(angle)
    {
      case  90: Rotate90(s,dbm,d); break;
      case 180: Rotate180(s,dbm,d); break;
      case 270: Rotate270(s,dbm,d); break;
      default: qwarn("QBitMap:Rotate(); angle %d not supported",angle); break;
    }
  }
}

/************************
* HIGH-LEVEL OPERATIONS *
************************/
void QBitMap::CreateTransparency(QColor *transpCol)
// Generate alpha information such that every pixel that has color 'transpCol'
// is transparent. The rest is filled with alpha=255 (1.0)
// Use by QText for example to generate transparent text bobs.
{ ubyte *pAlpha;
  long  *pCol;
  int    n;
  long   tCol;

  //printf("QBitMap:: Create transp\n");
  pCol=(long*)mem;
  pAlpha=(ubyte*)mem+3;				// Alpha byte
  tCol=transpCol->GetRGBA()&0xFFFFFF00;	// RGB, alpha=don't care
  n=GetWidth()*GetHeight();		// #pixels
  //printf("n=%d, tCol=%d\n",n,tCol);
  int cnt=0;
  for(;n>0;n--)
  { if((*pCol&0xFFFFFF00)==tCol){ *pAlpha=0; cnt++; }
    else                        *pAlpha=255;
    pCol++; pAlpha+=4;
  }
  //printf("pixels transp=%d, non-transp=%d\n",cnt,GetWidth()*GetHeight()-cnt);
}
void QBitMap::SetAlpha(int v)
// Set entire alpha channel to a specific value
{
  ubyte *pAlpha;
  ubyte alpha=(ubyte)v;
  int n;

  pAlpha=(ubyte*)mem+3;
  n=GetWidth()*GetHeight();             // #pixels
  for(;n>0;n--)
  { *pAlpha=alpha;
    pAlpha+=4;
  }
}
int QBitMap::KeyPurple()
// For Racer BMP files, often purple is used as the key color
// This function modifies the purple pixels to full alpha
// Returns #purple pixels found (so you know it's meant to use transparency)
{ 
  //long  *pCol;
  unsigned char *pCol;
  int    n,count=0;

  pCol=(unsigned char*)mem;
  n=GetWidth()*GetHeight();		// #pixels
  for(;n>0;n--)
  { //if((*pCol==0xFF00FFFF))
    if(pCol[0]==0xFF&&pCol[1]==0&&pCol[2]==0xFF)
    {
      pCol[0]=0;
      pCol[1]=0;
      pCol[2]=0;
      pCol[3]=0;
      count++;
#ifdef ND_DO_NOT_HARDCODE_OPAQUE
    } else
    {
      // Not keyed; fill alpha (SGI retrieves alpha values)
      pCol[3]=0xFF;
#endif
    }
    pCol+=4;
  }
  return count;
}

/****************
* ANTI-ALIASING *
****************/
static void aaLinear(ubyte *data,int wid,int hgt)
// Smooths out the edges a bit
{ int x,y;
  ubyte *prevRow,*curRow,*nextRow;	// 3 lines
  int cnt;
  int alpha9way[]=			// 3x3 matrix resulting alpha value
  { 0,28,57,55,60,70,80,98,110,255 };

  //printf("aaLinear\n");
  for(y=1;y<hgt-1;y++)
  { prevRow=data+(y-1)*wid*4+3;
    curRow =data+y*wid*4+3;
    nextRow=data+(y+1)*wid*4+3;
    // Start at col 1
    prevRow+=4;
    curRow +=4;
    nextRow+=4;
    for(x=1;x<wid-1;x++,prevRow+=4,curRow+=4,nextRow+=4)
    { // Pixels with alpha==0 are not painted, so why bother
      if(*curRow==0)continue;
      cnt=1;			// The pixel itself
      if(*(prevRow-4))cnt++;
      if(*prevRow)cnt++;
      if(*(prevRow+4))cnt++;
      if(*(curRow-4))cnt++;
      //if(*curRow)cnt++;
      if(*(curRow+4))cnt++;
      if(*(nextRow-4))cnt++;
      if(*nextRow)cnt++;
      if(*(nextRow+4))cnt++;
      *curRow=alpha9way[cnt];	// New alpha value
      /*if(cnt==9)*curRow=255;
      else      *curRow=(*curRow)/2;*/
    }
  }
}
static void aaAvgAlpha(ubyte *data,int wid,int hgt)
// Creates true average; calling this multiple times will result in smoother
// edges each time.
{
  int    x,y;
  ubyte *prevRow,*curRow,*nextRow;	// 3 lines
  int    sumAlpha;

  //printf("aaAvgAlpha\n");
  for(y=1;y<hgt-1;y++)
  { prevRow=data+(y-1)*wid*4+3;
    curRow =data+y*wid*4+3;
    nextRow=data+(y+1)*wid*4+3;
    // Start at col 1
    prevRow+=4;
    curRow +=4;
    nextRow+=4;
    for(x=1;x<wid-1;x++,prevRow+=4,curRow+=4,nextRow+=4)
    { // Pixels with alpha==0 are not painted, so why bother
      if(*curRow==0)continue;
      sumAlpha=*curRow;			// The pixel itself
      //printf("pixel alpha=%d;",*curRow);
      sumAlpha+=*(prevRow-4); //printf("%d;",*(prevRow-4));
      sumAlpha+=*prevRow; //printf("%d;",*(prevRow));
      sumAlpha+=*(prevRow+4); //printf("%d;",*(prevRow+4));
      sumAlpha+=*(curRow-4); //printf("%d;",*(curRow-4));
      sumAlpha+=*(curRow+4); //printf("%d;",*(curRow+4));
      sumAlpha+=*(nextRow-4); //printf("%d;",*(nextRow-4));
      sumAlpha+=*nextRow; //printf("%d;",*nextRow);
      sumAlpha+=*(nextRow+4); //printf("%d;",*(nextRow+4));
      /*if(sumAlpha==9*255)printf("[%d]",sumAlpha);
      else               printf("sum%d\n",sumAlpha);*/
      *curRow=sumAlpha/9;
    }
  }
}

void QBitMap::CreateAA(int alg)
// Creates an anti-aliased border at the edges of the bitmap.
// Note the object will thus seem thinner when painted with blending.
// Some assumptions are made (as this is mostly used for aa-ing text!):
// - Pixels which have their alpha set at 0 are skipped (weren't visible)
// - Full bitmap is aa-ed
// - Calling this function twice will result in no change (existing alpha
//   values are not taken into consideration)
// - A pixel is seen as painted if its alpha value is !0.
// - As a result of the previous rule, the aa algorithm is fully based
//   on alpha values only (it doesn't even look at the color values)
// - 32-bit bitmaps only
// - It uses a 3x3 matrices mostly.
// - Only pixels that are 1 pixel inside the bitmap are processed currently
//   (for speed); future will change that with special begin/end loops to
//   also process the border pixels (as this will often be used).
{
  int wid,hgt;
  ubyte *data;

  wid=GetWidth();
  hgt=GetHeight();
  data=(ubyte*)GetBuffer();

  switch(alg)
  { case QBM_AA_ALG_LINEAR:    aaLinear(data,wid,hgt); break;
    //case QBM_AA_ALG_AVG_ALPHA: aaAvgAlpha(data,wid,hgt); break;
    case QBM_AA_ALG_AVG_ALPHA:
      //aaAvgAlpha(data,wid,hgt);
      //aaAvgAlpha(data,wid,hgt);
      aaAvgAlpha(data,wid,hgt); break;
    case QBM_AA_ALG_4POINT:
      //aa4Point(data,wid,hgt); break;
      QFilterAntiAlias(this,4); break;
    case QBM_AA_ALG_9POINT:
      //aa9Point(data,wid,hgt); break;
      QFilterAntiAlias(this,9); break;
    case QBM_AA_ALG_16POINT:
      //aa16Point(data,wid,hgt); break;
      QFilterAntiAlias(this,16); break;
  }
}

/**********
* Lutting *
**********/
void QBitMap::Lut(int minV,bool doAlpha)
// Lights bitmap at dark spots to have at least 'minV' brightness.
// minV=0..255
// If 'doAlpha'==TRUE, then alpha places except where fully transparent
// are processed as well
// If doAlpha==FALSE, then only full opaque pixels are changed
{
  int i,n,v;
  char *p=GetBuffer();
  n=GetBufferSize()/4;

  // More accuracy
  minV*=100;
  for(i=0;i<n;i++,p+=4)
  {
    // Don't modify full transparent pixels
    if(doAlpha)
    { if(p[3]==0)continue;
    } else
    { if(p[3]<254)continue;
    }

    // Calculate brightness 0..255
    v=(p[0]*29+p[1]*58+p[2]*13);
    // Below minimum?
    if(v<minV)
    {
      // Scale
      if(v==0)
      { // Pure black => make gray
        p[0]=p[1]=p[2]=minV/100;
      } else
      {
        // Scale to match minimal brightness
        p[0]=minV*p[0]/v;
        p[1]=minV*p[1]/v;
        p[2]=minV*p[2]/v;
      }
    }
  }
}

/***************
* File writing *
***************/
bool QBitMap::WriteRGB(cstring name,int format)
// Hardcoded for dim=3, zsize=4 (RGBA)
{ FILE *fw;
  SGIHeader hdr;
  char bufLine[MAX_WID_WR*4];
  int c,i,j,wid,dep;
  char *p;
  static char z[512];

  if(GetWidth()>MAX_WID_WR)
  { qerr("QBitMap:Write; image too wide");
    return FALSE;
  }
  fw=fopen(name,"wb");
  if(!fw)return FALSE;
  memset(&hdr,0,sizeof(hdr));
  hdr.imagic=RGB_IMAGIC;                // Motorola order
  hdr.type=RGB_TYPE_VERBATIM|1;
  hdr.dim=3;
  hdr.xsize=GetWidth();
  hdr.ysize=GetHeight();
  hdr.zsize=4;                          // RGBA
  hdr.min=0;
  hdr.max=255;
  hdr.wasteBytes=0;
  strncpy(hdr.name,name,79);
  hdr.colormap=0;
  fwrite(&hdr,1,sizeof(hdr),fw);
  fwrite(z,1,512-sizeof(hdr),fw);
  // Write pixels
  wid=hdr.xsize;
  dep=32;
  for(c=0;c<4;c++)
  { for(i=0;i<hdr.ysize;i++)
    { p=mem+i*wid*(dep/8);
      p+=c;
      for(j=0;j<hdr.xsize;j++)
      { bufLine[j]=*p;
        p+=4;
      }
      fwrite(bufLine,1,hdr.xsize,fw);
    }
  }
  fclose(fw);
  return TRUE;
}
bool QBitMap::WriteTGA(cstring name,int format)
// TGA color & grayscale images
{
  FILE *fw;
  TGAHeader hdr;
  char bufLine[MAX_WID_WR*4];
  int x,y;
  //int c,x,y,wid,hgt,dep;
  char *d,*s;
  //static char z[512];

  if(GetWidth()>MAX_WID_WR)
  { qerr("QBitMap:WriteTGA(); image too wide");
    return FALSE;
  }
  fw=fopen(name,"wb");
  if(!fw)return FALSE;
  memset(&hdr,0,sizeof(hdr));
  if(format==QBMFORMAT_TGAGRAY)
  { hdr.imageType=3;
    hdr.pixDepth=8;
    hdr.imgDesc=8;          // 8 alpha bits, hmm, Photoshop does this
  } else
  { hdr.imageType=2;
    hdr.pixDepth=32;
    hdr.imgDesc=8;          // Hmm, not too sure
  }
  hdr.width=wid;
  hdr.height=hgt;
  // Write header (careful; structure padding!)
  // Reverse shorts for little-Endian platform (SGI=LE)
  cvtShorts((ushort*)&hdr.cmFirstEntry,4);
  cvtShorts((ushort*)&hdr.xOrg,8);
  fwrite(&hdr,1,3,fw);
  fwrite(&hdr.cmFirstEntry,1,5,fw);
  fwrite(&hdr.xOrg,1,10,fw);
  //fwrite(&hdr,1,sizeof(hdr),fw);
  // Write pixels
  if(format==QBMFORMAT_TGAGRAY)
  { // 8-bit grayscale
    for(y=0;y<hgt;y++)
    { s=mem+y*wid*(dep/8);
      d=bufLine;
      for(x=0;x<wid;x++)
      { *d++=*s;            // Take R for gray value
        s+=4;
      }
      fwrite(bufLine,1,wid,fw);
    }
  } else if(format==QBMFORMAT_TGA)
  { // 32-bit color (TrueColor)
    //qerr("QBitMap:WriteTGA(); no truecolor support");
    for(y=0;y<hgt;y++)
    { s=mem+y*wid*(dep/8);
      d=bufLine;
      for(x=0;x<wid;x++)
      { *d++=s[2];
        *d++=s[1];
        *d++=s[0];
        *d++=s[3];
        s+=4;
      }
      fwrite(bufLine,1,wid*4,fw);
    }
  } else
  { qerr("QBitMap::WriteTGA() unsupported format %d",format);
  }
  fclose(fw);
  return TRUE;
}

bool QBitMap::Write(cstring name,int format)
{ cstring fname;
  fname=QExpandFilename(name);
  switch(format)
  { case QBMFORMAT_RGB: return WriteRGB(fname,format);
    case QBMFORMAT_TGAGRAY: return WriteTGA(fname,format);
    case QBMFORMAT_TGA: return WriteTGA(fname,format);
    default:
#ifdef DEBUG
      qerr("QBitMap:Write(); unsupported format %d to write '%s'",format,name);
#endif
      return FALSE;
  }
  //return TRUE;         // We never get here
}
