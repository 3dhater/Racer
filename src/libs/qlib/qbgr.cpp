/*
 * QBgr.cpp - background image for window (gels on top of this gel)
 * 04-10-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/bgr.h>
#include <GL/gl.h>
#include <stdio.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QBgr::QBgr(string name)
 : QGel(QGELF_DBLBUF)
{
  sbm=new QImage(name);
  //flags=0;
  size.wid=sbm->GetWidth();
  size.hgt=sbm->GetHeight();
}
QBgr::~QBgr()
{ if(sbm)delete sbm;
}

void QBgr::Paint()
{
  if(flags&QGELF_INVISIBLE)return;
  //printf("QBgr::Paint cv %p\n",cv);
  if(cv==0)return;
  //glDisable(GL_BLEND);
  //cv->Disable(QCANVASF_BLEND);
  cv->Blend(FALSE);
  cv->Blit(sbm);
}

void QBgr::PaintPart(QRect *r)
{
  if(flags&QGELF_INVISIBLE)return;
  //printf("QBgr::PaintPart %d,%d %dx%d\n",r->x,r->y,r->wid,r->hgt);
  if(cv==0)return;
  //glDisable(GL_BLEND);
  //cv->Disable(QCANVASF_BLEND);
  cv->Blend(FALSE);
  //gc->Blit(sbm);
  //if(flags&QGELF_BLEND)gc->Enable(QCANVASF_BLEND);
  //else                 gc->Disable(QCANVASF_BLEND);
  cv->Blit(sbm,r->x,r->y,r->wid,r->hgt,r->x,r->y);
}

void QBgr::LoadImage(string fname)
{
  if(sbm)delete sbm;
  sbm=new QImage(fname);
  // Resize
  size.wid=sbm->GetWidth();
  size.hgt=sbm->GetHeight();
  MarkDirty();
}

