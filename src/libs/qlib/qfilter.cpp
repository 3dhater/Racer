/*
 * Filters - often used group
 */

#include <qlib/bitmap.h>
#include <qlib/filter.h>
#include <qlib/app.h>
#include <qlib/control.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/***************
* FILTER CLASS *
***************/
static void FilterCB(QControl *c,void *p,int v)
// Control callback; passes message on the the filter callback
{ QFilter *f=(QFilter*)p;
  //qmsg("FilterCB(c=%p,p=%p,v=%d)\n",c,p,v);
  f->EvChange();
}

QFilter::QFilter()
{ next=0;
}
QFilter::~QFilter(){}

void QFilter::OnChange(QFilterCallback cbFunc,void *cp)
{
  cbOnChange=cbFunc;
  pOnChange=cp;
}
void QFilter::EvChange()
// A change happened
{
  if(cbOnChange)
    cbOnChange(this,pOnChange);
}

void QFilter::Do(QBitMap *bm)
{ qerr("QFilter::Do abstract call");
}

/****************************
* OFTEN-USED FILTER CLASSES *
****************************/
QFilterBrightness::QFilterBrightness()
{ strength=new QControl();
  //qmsg("QFBN: strength setcb\n");
  app->GetFXManager()->Add(strength);
  strength->SetCallback(FilterCB,(void*)this);	// OnChange
}
QFilterBrightness::~QFilterBrightness(){}

void QFilterBrightness::Do(QBitMap *bm)
{ QFilter0Brightness(bm,strength->Get());
}

// Fading
QFilterFade::QFilterFade()
{ strength=new QControl();
  //qmsg("QFBN: strength setcb\n");
  app->GetFXManager()->Add(strength);
  strength->SetCallback(FilterCB,(void*)this);	// OnChange
}
QFilterFade::~QFilterFade(){}

void QFilterFade::Do(QBitMap *bm)
{ QFilter0Fade(bm,strength->Get());
}

// Graying out
QFilterGray::QFilterGray()
{ strength=new QControl();
  //qmsg("QFBN: strength setcb\n");
  app->GetFXManager()->Add(strength);
  strength->SetCallback(FilterCB,(void*)this);	// OnChange
}
QFilterGray::~QFilterGray(){}

void QFilterGray::Do(QBitMap *bm)
{ QFilter0Gray(bm,strength->Get());
}

// Colorizing
QFilterColorize::QFilterColorize(QColor *color)
{ strength=new QControl();
  col=0;
  SetColor(color);
  //qmsg("QFBN: strength setcb\n");
  app->GetFXManager()->Add(strength);
  strength->SetCallback(FilterCB,(void*)this);	// OnChange
}
QFilterColorize::~QFilterColorize(){}

void QFilterColorize::SetColor(QColor *color)
{
  if(col)delete col;
  col=new QColor(color->GetR(),color->GetG(),color->GetB());
}
void QFilterColorize::Do(QBitMap *bm)
{ QFilter0Colorize(bm,col,strength->Get());
}

/********************
* FILTER OPERATIONS *
********************/

/** Anti-aliasing **/

static void aa4Point(ubyte *data,int wid,int hgt)
// Uses 4-point sampling and averaging. This should require that the
// actual bitmap is shifted by 1,1 (right and down), because it will
// actually move the bitmap by 1/4th of a pixel to the upperleft.
// The bitmap should ideally have a full range of zeroes as a border.
// ......
// .dddd.  .=zero, d=data
// .dddd.
// ......
{ int x,y,n;
  ubyte *curRow,*nextRow,*p;

  //printf("aa4Point\n");
  for(y=0;y<hgt-1;y++)
  { p=data+y*wid*4;
    curRow=p+3;
    nextRow=curRow+wid*4;
    for(x=0;x<wid-1;x++,curRow+=4,nextRow+=4,p+=4)
    { //if(!*curRow)continue;
      n=*curRow+*(curRow+4)+*nextRow+*(nextRow+4);
      n/=4;
      if(n)
      { *p=255; *(p+1)=255; *(p+2)=255;
        //*p=n; *(p+1)=n; *(p+2)=n;
      }
      *curRow=n;
    }
  }
}
static void aa9Point(ubyte *data,int wid,int hgt)
// Uses 9-point sampling and averaging. This should require that the
// actual bitmap is shifted by 2,2 (right and down), because it will
// actually move the bitmap by 1 1/3rd of a pixel to the upperleft.
// The bitmap should ideally have 2 full ranges of zeroes as a border.
// ......
// .dddd.  .=2*zero, d=data
// .dddd.
// ......
// This looks much like avgAlpha, BUT, avgAlpha seems to render using
// its own results, and it doesn't create new gray pixels as it renders
// (it just touches alpha). This algorithm does create new pixels.
{ int x,y,n;
  ubyte *curRow,*nextRow,*nnRow,*p;

  //printf("aa9Point\n");
  for(y=0;y<hgt-2;y++)
  { p=data+y*wid*4;
    curRow=p+3;
    nextRow=curRow+wid*4;
    nnRow=nextRow+wid*4;
    for(x=0;x<wid-2;x++,curRow+=4,nextRow+=4,nnRow+=4,p+=4)
    { //if(!*curRow)continue;
      n=*curRow+*(curRow+4)+*(curRow+8);
      n+=*nextRow+*(nextRow+4)+*(nextRow+8);
      n+=*nnRow+*(nnRow+4)+*(nnRow+8);
      n/=9;
      if(n)
      { // Create a pixel
        *p=255; *(p+1)=255; *(p+2)=255;
        //*p=n; *(p+1)=n; *(p+2)=n;
      }
      *curRow=n;
    }
  }
}
static void aa16Point(ubyte *data,int wid,int hgt)
// Uses 16-point sampling and averaging. This should require that the
// actual bitmap is shifted by 4,4 (right and down), because it will
// actually move the bitmap by 2 1/4th of a pixel to the upperleft.
// The bitmap should ideally have 3 full ranges of zeroes as a border.
// ......
// .dddd.  .=3*zero, d=data
// .dddd.
// ......
{ int x,y,n;
  ubyte *curRow,*nextRow,*nnRow,*nnnRow,*p;

  //printf("aa16Point\n");
  for(y=0;y<hgt-3;y++)
  { p=data+y*wid*4;
    curRow=p+3;
    nextRow=curRow+wid*4;
    nnRow=nextRow+wid*4;
    nnnRow=nnRow+wid*4;
    for(x=0;x<wid-3;x++,curRow+=4,nextRow+=4,nnRow+=4,nnnRow+=4,p+=4)
    { //if(!*curRow)continue;
      n=*curRow+*(curRow+4)+*(curRow+8)+*(curRow+12);
      n+=*nextRow+*(nextRow+4)+*(nextRow+8)+*(nextRow+12);
      n+=*nnRow+*(nnRow+4)+*(nnRow+8)+*(nnRow+12);
      n+=*nnnRow+*(nnnRow+4)+*(nnnRow+8)+*(nnnRow+12);
      n/=16;
      if(n)
      { // Create a pixel
        *p=255; *(p+1)=255; *(p+2)=255;
        //*p=n; *(p+1)=n; *(p+2)=n;
      }
      *curRow=n;
    }
  }
}

void QFilterAntiAlias(QBitMap *sbm,int nPoints)
// Anti-aliasing, using n-point averaging
// nPoints may be: 4, 9 or 16
// In-bitmap poking
// Image is shifted, but not darkened (total brightness is kept)
// Bugs:
// - Makes it white
{ QASSERT_V(nPoints==4||nPoints==9||nPoints==16);
  if(nPoints==4)
    aa4Point((ubyte*)sbm->GetBuffer(),sbm->GetWidth(),sbm->GetHeight());
  else if(nPoints==9)
    aa9Point((ubyte*)sbm->GetBuffer(),sbm->GetWidth(),sbm->GetHeight());
  else if(nPoints==16)
    aa16Point((ubyte*)sbm->GetBuffer(),sbm->GetWidth(),sbm->GetHeight());
}

/** Blending **/

void QFilterBlendBurn(QBitMap *bm)
// This filter burns the alpha channel into the color channels,
// making the alpha obsolete.
// (used by QScroller to generate unblended anti-aliased texts for example)
// Assumptions:
// - The blended color is current black (0,0,0)
// - The color pixels are all white
// - The entire bitmap is burned
// - RGBA 8bpp
// Bugs:
// - Should blend the color values instead of assuming white
{ int i,n;
  char *p=bm->GetBuffer(),*pAlpha;
  n=bm->GetBufferSize()/4;
  pAlpha=p+3;
  for(i=0;i<n;i++,pAlpha+=4)
  { if(!*pAlpha)
    { *(long*)p=0; p+=4;
      //*p++=0; *p++=0; *p++=0;
    } else
    { // Apply alpha to color pixel
      *p++=*pAlpha; *p++=*pAlpha; *p++=*pAlpha;
      p++;
    }
  }
}

/** Shading **/
//#define AL
void QFilterShadow(QBitMap *bm,int dx,int dy,QColor *col)
// Create a shadow of the desired color
{ ulong rgba=col->GetRGBA();
  ulong *pSrc,*pDst;
  int x,y;
  int wid,hgt;
  int dAlpha;

  // OpenGL bitmaps flip
  //dy=-dy;
  //printf("QFilterShadow: bm=%p, d=%d/%d, col=%d\n",bm,dx,dy,col->GetRGBA());
  wid=bm->GetWidth();
  hgt=bm->GetHeight();
  //for(y=hgt-1-dy;y>=0;y--)
  for(y=0;y<hgt-1-dy;y++)
  { // Take care; bitmap is flipped in memory
    pSrc=(ulong*)(bm->GetBuffer())+(y+dy)*wid+wid-1-dx;
    pDst=(ulong*)(bm->GetBuffer())+y*wid+wid-1;
    for(x=0;x<wid-dx;x++,pSrc--,pDst--)
    { // Only do pixels with alpha!=0
      dAlpha=*pDst&0xFF;
      if(dAlpha==0xFF)continue;	// Don't overwrite ex. pixels
      if((*pSrc&0xFF)==0xFF)
      { *pDst=rgba;
      }
    }
  }
  //printf("QFS RET\n");
}

void QFilter0Brightness(QBitMap *bm,int strength)
// strength=-100..100, 0=original, -100=black, 100=white
// Does not touch alpha information
{
  int  i,count;
  ubyte *p;
  ubyte table[256];
  //qdbg("QFilterBrightness:Do\n");
  count=bm->GetWidth()*bm->GetHeight();
  if(strength<-100)strength=-100;
  else if(strength>100)strength=100;
  if(strength>0)
  { // Lookup table for white
    for(i=0;i<256;i++) 
      table[i]=i+(255-i)*strength/100;
  } else if(strength<0)
  { // Black
    strength=100+strength;
    for(i=0;i<256;i++) 
      table[i]=i*strength/100;
  } else return;
  /*for(i=0;i<256;i++)
  { printf(" %3d",table[i]);
    if((i&15)==15)printf("\n");
  }*/
  for(p=(ubyte*)bm->GetBuffer();count>0;count--)
  { *p=table[*p]; p++;		// R
    *p=table[*p]; p++;
    *p=table[*p];		// B
    p+=2;			// Skip A
  }
}

void QFilter0Fade(QBitMap *bm,int strength)
// strength=-100..100, 0=original, negative=boost alpha (0 becomes 255)
// positive=fadeout (100=fully gone)
{ int i;
  int  count;
  ubyte *p;
  ubyte table[256];
  count=bm->GetWidth()*bm->GetHeight();
  if(strength<-100)strength=-100;
  else if(strength>100)strength=100;
  //printf("strength fade=%d\n",strength);
  if(strength>0)
  { // Fade out
    strength=100-strength;
    for(i=0;i<256;i++) 
      table[i]=i*strength/100;
  } else if(strength<0)
  { // Fade in (fade out alpha blend)
    strength=-strength;
    for(i=0;i<256;i++) 
      table[i]=i+(255-i)*strength/100;
  } else return;
  /*for(i=0;i<256;i++)
  { printf(" %3d",table[i]);
    if((i&15)==15)printf("\n");
  }*/
  for(p=(ubyte*)bm->GetBuffer()+3;count>0;count--,p+=4)
  { *p=table[*p];
  }
}

void QFilter0Gray(QBitMap *bm,int strength)
// Grays out a bitmap (removes color)
// Currently strength=0..128 (-100 would mean more color, but don't know how)
// 0=normal color, 128=totally gray
// Benchmark: no tables : 4400 microsecs for 60x50us
//            RGB avg   : 3800 microsecs
//            static RGB: 3600 microsecs
//            -128..128 : 1440 ms (!) <<7
{ int i,avg,count;
  ubyte *p;
  static ubyte tableR[256],tableG[256],tableB[256];
  // ^^ sorry, but for speed this is handy
  char table[256];

  //proftime_t t;
  //profStart(&t);
  count=bm->GetWidth()*bm->GetHeight();
  if(strength<0)strength=0;
  else if(strength>128)strength=128;
  if(tableR[255]==0)
  { // Precalculate RGB averaging (this affects the precision, since /100
    // is done earlier than originally)
    for(i=0;i<256;i++) 
    { tableR[i]=29*i/100;
      tableG[i]=58*i/100;
      tableB[i]=13*i/100;
    }
  }
  // Precalculate delta (  (avg-currentValue)*strength/128   )
  // Could do 256*256 table, but may take more time calculating the table
  // then using it. Making it static would solve that, but it is a time/space
  // tradeoff.
  for(i=0;i<256;i++)
    table[i]=(i-128)*strength/128;
  for(p=(ubyte*)bm->GetBuffer();count>0;count--)
  {
    // Calc gray pixel
#ifdef OBS
    avg=(29*p[0]+58*p[1]+13*p[2])/100;
#else
    // Less precise, but much faster
    avg=tableR[p[0]]+tableG[p[1]]+tableB[p[2]];
#endif
    // Pull RGB to the gray level according to the strength value
//#ifdef OBS
    *p=*p+(avg-*p)*strength/128; p++;
    *p=*p+(avg-*p)*strength/128; p++;
    *p=*p+(avg-*p)*strength/128;
    p+=2;
/*
#else
    *p=*p+table[avg-*p+128];
    printf("  table[%d]\n",avg-*p+128);
    p++;
    *p=*p+table[avg-*p+128]; p++;
    *p=*p+table[avg-*p+128]; p+=2;
#endif
*/
  }
  //profReport(&t,"QFilter0Gray");
}

void QFilter0Colorize(QBitMap *bm,QColor *col,int strength)
// Strength=0..128
// Sort of spread (RGB, not HSV)
{ int i,avg,count;
  int graySrc,grayCol;
  ubyte *p;
  char table[256];
  ubyte r,g,b;

  qdbg("QFilter0Colorize\n");
  if(strength==0)return;
  // Calc luminance of color
  r=col->GetR();
  g=col->GetG();
  b=col->GetB();
  grayCol=(29*r+58*g+13*b)/128;

  count=bm->GetWidth()*bm->GetHeight();
  if(strength<0)strength=0;
  else if(strength>128)strength=128;
  //qwarn("QFilter0Colorize NYI");
  for(p=(ubyte*)bm->GetBuffer();count>0;count--)
  {
    graySrc=(29*p[0]+58*p[1]+13*p[2])/100;
    //*p=*p+strength*(r-*p)/128; p++;
    //*p=*p+strength*(g-*p)/128; p++;
    //*p=*p+strength*(b-*p)/128; p+=2;
    *p=r*graySrc/255; p++;
    *p=g*graySrc/255; p++;
    *p=b*graySrc/255;
    p+=2;
  }
}

void QFilter0Outline(QBitMap *bm,QColor *col)
{ ulong rgba=col->GetRGBA();
  ulong *pSrc,*pDst;
  int x,y,wid,hgt;
  QBitMap *bmOrg;

  //qdbg("Filter outline\n");
  // Divide, but slow
  bmOrg=bm->CreateClone(TRUE);

  // Left
  wid=bm->GetWidth();
  hgt=bm->GetHeight();
  pSrc=((ulong*)bmOrg->GetBuffer())+0*wid+1;
  pDst=((ulong*)bm->GetBuffer())+0*wid;
  for(y=0;y<hgt;y++)
  { for(x=1;x<wid;x++,pSrc++,pDst++)
    { if(*pDst)continue;		// No filled pixels
      if(*pSrc&0xFF)
        *pDst=rgba;
    }
    pSrc++; pDst++;			// Skip last pixel
  }
  // Right
  pSrc=((ulong*)bmOrg->GetBuffer())+0*wid;
  pDst=((ulong*)bm->GetBuffer())+0*wid+1;
  for(y=0;y<hgt;y++)
  { for(x=1;x<wid;x++,pSrc++,pDst++)
    { if(*pDst)continue;		// No filled pixels
      if(*pSrc&0xFF)
        *pDst=rgba;
    }
    pSrc++; pDst++;			// Skip last pixel
  }
  // Top
  pSrc=((ulong*)bmOrg->GetBuffer())+1*wid;
  pDst=((ulong*)bm->GetBuffer())+0*wid;
  for(y=1;y<hgt;y++)
  { for(x=0;x<wid;x++,pSrc++,pDst++)
    { if(*pDst)continue;		// No filled pixels
      if(*pSrc&0xFF)
        *pDst=rgba;
    }
  }
  // Bottom
  pSrc=((ulong*)bmOrg->GetBuffer())+0*wid;
  pDst=((ulong*)bm->GetBuffer())+1*wid;
  for(y=1;y<hgt;y++)
  { for(x=0;x<wid;x++,pSrc++,pDst++)
    { if(*pDst)continue;		// No filled pixels
      if(*pSrc&0xFF)
        *pDst=rgba;
    }
  }
  delete bmOrg;
}

