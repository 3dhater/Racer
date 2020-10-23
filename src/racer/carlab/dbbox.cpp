/*
 * DBoundingBox - a bounding box 3D graphical object
 * 12-12-00: Created!
 * NOTES:
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
DEBUG_ENABLE

DBoundingBox::DBoundingBox()
  : DObject()
{
}
DBoundingBox::DBoundingBox(DBox *_box)
  : DObject()
{
  box=*_box;
}

DBoundingBox::~DBoundingBox()
{
}

void DBoundingBox::_Paint()
{
  dfloat w,h,d;

//qdbg("DBoundingBox::_Paint\n");

  glPushMatrix();

  glTranslatef(box.center.x,box.center.y,box.center.z);

  w=box.size.x;
  h=box.size.y;
  d=box.size.z;

  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);

  glColor3f(1,1,0);

  // A few parts to get a full cube
  glBegin(GL_LINE_STRIP);
    glVertex3f(-w/2,h/2,d/2);
    glVertex3f(w/2,h/2,d/2);
    glVertex3f(w/2,-h/2,d/2);
    glVertex3f(-w/2,-h/2,d/2);
    glVertex3f(-w/2,h/2,d/2);
    glVertex3f(-w/2,h/2,-d/2);
    glVertex3f(-w/2,-h/2,-d/2);
    glVertex3f(w/2,-h/2,-d/2);
    glVertex3f(w/2,h/2,-d/2);
    glVertex3f(-w/2,h/2,-d/2);
  glEnd();
  glBegin(GL_LINES);
    glVertex3f(-w/2,-h/2,d/2);
    glVertex3f(-w/2,-h/2,-d/2);

    glVertex3f(w/2,-h/2,d/2);
    glVertex3f(w/2,-h/2,-d/2);

    glVertex3f(w/2,h/2,d/2);
    glVertex3f(w/2,h/2,-d/2);
  glEnd();

  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);

  glPopMatrix();
}

