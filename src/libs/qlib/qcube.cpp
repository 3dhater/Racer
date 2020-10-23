/*
 * QCube - a 3D cube - possibly textured
 * 02-07-97: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/cube.h>
#include <GL/gl.h>
#include <qlib/app.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QCube::QCube(qfloat iwid,qfloat ihgt,qfloat idep)
{ int i;
  wid=iwid;
  hgt=ihgt;
  dep=idep;
  for(i=0;i<6;i++)
    poly[i]=new QPolygon(4);
  wid/=2; hgt/=2; dep/=2;
  // Front
  poly[0]->SetVertex(0,-wid,hgt,-dep);
  poly[0]->SetVertex(1,wid,hgt,-dep);
  poly[0]->SetVertex(2,wid,-hgt,-dep);
  poly[0]->SetVertex(3,-wid,-hgt,-dep);
  // Back
  poly[1]->SetVertex(0,-wid,hgt,dep);
  poly[1]->SetVertex(1,-wid,-hgt,dep);
  poly[1]->SetVertex(2,wid,-hgt,dep);
  poly[1]->SetVertex(3,wid,hgt,dep);
  // Top
  poly[2]->SetVertex(0,-wid,hgt,-dep);
  poly[2]->SetVertex(1,-wid,hgt,dep);
  poly[2]->SetVertex(2,wid,hgt,dep);
  poly[2]->SetVertex(3,wid,hgt,-dep);
  // Side right
  poly[3]->SetVertex(0,wid,hgt,-dep);
  poly[3]->SetVertex(1,wid,hgt,dep);
  poly[3]->SetVertex(2,wid,-hgt,dep);
  poly[3]->SetVertex(3,wid,-hgt,-dep);
  // Side right
  poly[4]->SetVertex(0,-wid,hgt,-dep);
  poly[4]->SetVertex(1,-wid,-hgt,-dep);
  poly[4]->SetVertex(2,-wid,-hgt,dep);
  poly[4]->SetVertex(3,-wid,hgt,dep);
  // Bottom
  poly[5]->SetVertex(0,wid,-hgt,-dep);
  poly[5]->SetVertex(1,wid,-hgt,dep);
  poly[5]->SetVertex(2,-wid,-hgt,dep);
  poly[5]->SetVertex(3,-wid,-hgt,-dep);
  for(i=0;i<6;i++)
  {
  }
  flags=0;
}

QCube::~QCube()
{ int i;
  for(i=0;i<6;i++)
    if(poly[i])delete poly[i];
}

void QCube::DefineTexture(int side,QBitMap *src,QRect *r)
{
  QASSERT_V(side>=0&&side<6);
  poly[side]->DefineTexture(src,r);
}

void QCube::Paint()
// Paints polygon in the current state
{ int i;
  for(i=0;i<6;i++)
    poly[i]->Paint();
}
