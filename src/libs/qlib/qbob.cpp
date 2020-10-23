/*
 * QBob.cpp - image gel
 * 04-10-96: Created!
 * 29-04-97: X11/Canvas
 * 16-05-97: Filters. Operate on smaller clone bitmaps.
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/bob.h>
#include <qlib/app.h>
#include <GL/gl.h>
#include <qlib/debug.h>
DEBUG_ENABLE

static void BobCB(QControl *c,void *p,int n)
{ QBob *b=(QBob*)p;
  //printf("bobcb\n");
  b->SetControl(c->GetControlID(),n);
}
static void BobImageCB(QControl *c,void *p,int n)
// Image control changed
{ QBob *b=(QBob*)p;
  // Re-apply filters only if filters are used
  if(b->filterList)
    b->MarkDirtyFilter();
  else
    b->MarkDirty();
}
static void BobFilterCB(QFilter *f,void *p)
// A filter changed, p=the bob
{ QBob *b=(QBob*)p;
  b->MarkDirtyFilter();
}

QBob::QBob(QBitMap *bm,int _flags,
  int _sx,int _sy,int _wid,int _hgt)
  : QGel(_flags|QGELF_DBLBUF)
{
  //printf("QBob ctor\n");
  sbm=bm;
  // Initially, no filters apply
  bmScaled=0;
  bmFiltered=0;
  filterList=0;
  zoomX=zoomY=1.0;
  //flags=_flags;
  sx=_sx; sy=_sy;
  if(sbm)
  { if(_wid==-1)swid=bm->GetWidth();
    else        swid=_wid;
    if(_hgt==-1)shgt=bm->GetHeight();
    else        shgt=_hgt;
  } else swid=shgt=0;
  // Allocate controls
  x=new QControl(CONTROL_X); x->SetCallback(BobCB,(void*)this);
  y=new QControl(CONTROL_Y); y->SetCallback(BobCB,(void*)this);
  wid=new QControl();
  hgt=new QControl();
  scale_x=new QControl();
  scale_y=new QControl();
  image=new QControl(); image->SetCallback(BobImageCB,(void*)this);
  //curImage=0;
  // Automatically add controls to FX manager
  app->GetFXManager()->Add(x);
  app->GetFXManager()->Add(y);
  app->GetFXManager()->Add(image);
  //printf("  QBob ctor RET\n");
}
QBob::~QBob()
{ // Delete controls
  if(x){ app->GetFXManager()->Remove(x); delete x; }
  if(y){ app->GetFXManager()->Remove(y); delete y; }
  if(wid){ /*app->GetFXManager()->Remove(wid);*/ delete wid; }
  if(hgt){ /*app->GetFXManager()->Remove(hgt);*/ delete hgt; }
  if(scale_x){ /*app->GetFXManager()->Remove(scale_x);*/ delete scale_x; }
  if(scale_y){ /*app->GetFXManager()->Remove(scale_y);*/ delete scale_y; }
  if(image){ app->GetFXManager()->Remove(image); delete image; }
  if(bmScaled!=sbm)delete bmScaled;
  if(bmFiltered)delete bmFiltered;
}

void QBob::MarkDirtyScale()
{ flags|=QGELF_DIRTYSCALE;
}
void QBob::MarkDirtyFilter()
{ flags|=QGELF_DIRTYFILTER;
  MarkDirty();			// Bob is dirty in itself
}

void QBob::SetSourceBitMap(QBitMap *bm)
{
  sbm=bm;
  MarkDirtyFilter();
}
void QBob::SetSourceLocation(int x,int y,int wid,int hgt)
{
  if(wid!=-1)
  { swid=wid; size.wid=wid; }
  if(hgt!=-1)
  { shgt=hgt;
    size.hgt=hgt;		// For gellist updates
  }
  sx=x;
  sy=y;
  MarkDirtyFilter();
}
void QBob::GetSourceLocation(QRect *r)
{
  r->x=sx; r->y=sy;
  r->wid=swid; r->hgt=shgt;
}

void QBob::SetSourceSize(int wid,int hgt)
{ size.wid=swid=wid;
  size.hgt=shgt=hgt;
  MarkDirtyFilter();
}

void QBob::Paint()
{
  PaintAt(loc.x,loc.y);
}

void QBob::PaintPart(QRect *r)
// Paint relative to start of bob's normal image
{ int isx,isy;
  if(flags&QGELF_INVISIBLE)return;
  if(flags&QGELF_DIRTYFILTER)ApplyFilters();
  //printf("QBob::PaintPart(%d,%d,%dx%d)\n",r->x,r->y,r->wid,r->hgt);
  if(cv==0)return;
  if(flags&QGELF_BLEND)cv->Blend(TRUE);
  else                 cv->Blend(FALSE);
  QBitMap *bm;
  cv->Select(); glPixelZoom(zoomX,zoomY);
  if(bmFiltered)
  { bm=bmFiltered;
    cv->Blit(bm,loc.x+r->x,loc.y+r->y,r->wid,r->hgt,r->x,r->y);
  } else
  //else if(bmScaled)bm=bmScaled;
  { bm=sbm;
    isx=sx;
    isy=sy+image->Get()*shgt;
    cv->Blit(bm,loc.x+r->x,loc.y+r->y,r->wid,r->hgt,isx+r->x,isy+r->y);
  }
  if(zoomX!=1.0||zoomY!=1.0){ cv->Flush(); glPixelZoom(1,1); }
glPixelZoom(1,1);
  //printf("QBob::PaintPart RET\n");
}

void QBob::PaintAt(int x,int y)
// For lots of special effects, it is handy to be able to plot
// a bob at any position you choose, without using the FX manager
{ int isx,isy;                  // Image corrected
  if(flags&QGELF_INVISIBLE)return;
  if(flags&QGELF_DIRTYFILTER)ApplyFilters();
  //qdbg("Qbob paint cv=%p; img=%d\n",cv,image->Get());
  if(cv==0)return;
  if(flags&QGELF_BLEND)cv->Blend(TRUE);
  else                 cv->Blend(FALSE);
  QBitMap *bm;
  cv->Select(); glPixelZoom(zoomX,zoomY);
  if(bmFiltered)
  { bm=bmFiltered;
    //printf("  blit %d,%d %dx%d\n",loc.x,loc.y,swid,shgt);
    cv->Blit(bm,x,y,swid,shgt,0,0);
  } else
  //else if(bmScaled)bm=bmScaled;
  { bm=sbm;
    isx=sx;
    isy=sy+image->Get()*shgt;
    cv->Blit(bm,x,y,swid,shgt,isx,isy);
  }
  if(zoomX!=1.0||zoomY!=1.0){ cv->Flush(); glPixelZoom(1,1); }
glPixelZoom(1,1);
  //printf("QBob::Paint RET\n");
}

/**********
* FILTERS *
**********/
void QBob::AddFilter(QFilter *f)
{ f->next=filterList;
  filterList=f;
  MarkDirtyFilter();		// Use it
  // To avoid later swapping, allocate the filtered bitmap now
  ApplyFilters();
  // Connect filter to bob (change events)
  f->OnChange(BobFilterCB,(void*)this);
}

void QBob::RemoveFilter(QFilter *f)
{
  qerr("QBob::RemoveFilter NYI");
}

void QBob::ApplyFilters()
{ QFilter *f;
  int isx,isy;
  if(!filterList)goto skip;
  //qdbg("QBob::ApplyFilters\n");
  // Scale original if dirty
  //...
  // Create bitmap to modify
  //qdbg("  bmFiltered=%p, sbm=%p\n",bmFiltered,sbm);
  isx=sx;
  isy=sy+image->Get()*shgt;
  if(!bmFiltered)
  { // Create clone
    //bmFiltered=sbm->CreateClone(TRUE);
    bmFiltered=new QBitMap(sbm->GetDepth(),swid,shgt);
    //qdbg("  clone bitmap=%dx%d from %d,%d\n",swid,shgt,sx,sy);
    bmFiltered->CopyPixelsFrom(sbm,isx,isy,swid,shgt,0,0);
  } else
  { // Copy original image
    //if(bmScaled)bmFiltered->CopyBitsFrom(bmScaled);
    //else        bmFiltered->CopyBitsFrom(sbm);
    bmFiltered->CopyPixelsFrom(sbm,isx,isy,swid,shgt,0,0);
  }
  for(f=filterList;f;f=f->next)
    f->Do(bmFiltered);
 skip:
  flags&=~QGELF_DIRTYFILTER;
}

