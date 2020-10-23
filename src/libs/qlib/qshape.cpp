/*
 * QShape.cpp - algorithmic gels (rectangles, cursor etc)
 * 09-06-97: Created!
 * NOTES:
 * - For example: a cursor for the Text Generator (Mach6)
 * BUGS:
 * - Sits in the middle of a blit queue and flushes it when painted
 * This means blits are not as optimized as possible (but it works correctly)
 * - It does NOT work correctly; doubly blitted areas. Fortunately restores
 *   work still.
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/shape.h>
#include <qlib/app.h>
#include <qlib/opengl.h>
#include <GL/gl.h>
#include <qlib/debug.h>
DEBUG_ENABLE

static void BobCB(QControl *c,void *p,int n)
{ QShape *b=(QShape*)p;
  //printf("bobcb\n");
  b->SetControl(c->GetControlID(),n);
}

QShape::QShape(QRect *r,int itype)
  : QGel(QGELF_DBLBUF)
{
  //printf("QShape ctor\n");
  type=itype;
  
  // Allocate controls
  x=new QControl(CONTROL_X); x->SetCallback(BobCB,(void*)this);
  y=new QControl(CONTROL_Y); y->SetCallback(BobCB,(void*)this);
  wid=new QControl(); wid->SetCallback(BobCB,(void*)this);
  hgt=new QControl(); hgt->SetCallback(BobCB,(void*)this);
  x->Set(r->x);
  y->Set(r->y);
  wid->Set(r->wid);
  hgt->Set(r->hgt);
  size.wid=r->wid;
  size.hgt=r->hgt;
  // Automatically add controls to FX manager
  app->GetFXManager()->Add(x);
  app->GetFXManager()->Add(y);
  app->GetFXManager()->Add(wid);
  app->GetFXManager()->Add(hgt);
  //printf("  QShape ctor RET\n");
}
QShape::~QShape()
{ // Delete controls
  if(x)delete x;
  if(y)delete y;
  if(wid)delete wid;
  if(hgt)delete hgt;
}

extern QImage *img;
void QShape::Paint()
{
  if(flags&QGELF_INVISIBLE)return;
  //printf("QShape paint cv=%p\n",cv);

  if(cv==0)return;
  
  // Highly non-optimized! Should use BlitQ
  cv->Flush();

  cv->Blend(TRUE);
  cv->Select();
  //glDisable(GL_BLEND);
  cv->SetColor(255,255,255,40);
  //printf("  shape %d,%d %dx%d\n",x->Get(),y->Get(),wid->Get(),hgt->Get());
  //cv->Rectfill(x->Get(),y->Get(),wid->Get(),hgt->Get());
  //cv->Blit(img,10,10);
  //cv->Blit(img,x->Get(),y->Get());
  cv->Rectfill(loc.x,loc.y,wid->Get(),hgt->Get());
  //cv->GetGLContext()->Swap(); QNap(5);
  //cv->GetGLContext()->Swap();
  //printf("QShape::Paint RET\n");
}

void QShape::PaintPart(QRect *r)
// Paint relative to start of bob's normal image
{
  if(flags&QGELF_INVISIBLE)return;

  //printf("QShape::PaintPart(%d,%d,%dx%d)\n",r->x,r->y,r->wid,r->hgt);
  if(cv==0)return;
  cv->Flush();
  cv->Blend(TRUE);
  cv->SetColor(255,255,0);
  cv->SetColor(255,255,255,40);
  //cv->Rectfill(x->Get(),y->Get(),wid->Get(),hgt->Get());
  cv->Rectfill(loc.x,loc.y,wid->Get(),hgt->Get());
  //printf("QShape::PaintPart RET\n");
}
