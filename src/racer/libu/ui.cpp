/*
 * RacerU - standard UI (user interface) aid
 * 27-02-01: Created!
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <raceru/all.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/image.h>
#include <qlib/app.h>
#include <d3/mesh.h>
DEBUG_ENABLE

bool rrFullScreenTexture(cstring fname)
// Load & display a fullscreen texture image
// Returns FALSE in case of failure:
// - the image could not be loaded
{
  QImage *img;
  DBitMapTexture *tex;
  DMesh *mesh;
  int    w,h;
  QDraw *draw;

  // Load the image
  img=new QImage(fname);
  if(!img->IsRead())
  { QDELETE(img);
    return FALSE;
  }
  // Create texture from image
  tex=new DBitMapTexture(img);

  // Create quad for display
  mesh=new DMesh();
  if(Q_BC)draw=Q_BC;
  else    draw=QSHELL;
  w=draw->GetWidth();
  h=draw->GetHeight();
  mesh->DefineFlat2D(w,h);
  QRect r(0,0,img->GetWidth(),img->GetHeight());
  mesh->GetPoly(0)->DefineTexture(tex,&r);

  // Display quad
  QCV->Clear();
  mesh->Paint();
  draw->Swap();
  // Equalize
  //QCV->Clear();
  mesh->Paint();
  //draw->Swap();

  // Cleanup
  QDELETE(img);
  QDELETE(mesh);
  QDELETE(tex);
  return TRUE;
}

