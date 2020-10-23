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
// Paints the box in the current transformation
{
//qdbg("DBoundingBox::_Paint\n");
//box.DbgPrint("DBoundingBox::_Paint()");

  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);

  glColor3f(1,1,0);

  glBegin(GL_LINE_STRIP);
    glVertex3f(box.min.x,box.min.y,box.min.z);
    glVertex3f(box.max.x,box.min.y,box.min.z);
    glVertex3f(box.max.x,box.max.y,box.min.z);
    glVertex3f(box.min.x,box.max.y,box.min.z);
    glVertex3f(box.min.x,box.min.y,box.min.z);
    glVertex3f(box.min.x,box.min.y,box.max.z);
    glVertex3f(box.min.x,box.max.y,box.max.z);
    glVertex3f(box.max.x,box.max.y,box.max.z);
    glVertex3f(box.max.x,box.min.y,box.max.z);
    glVertex3f(box.max.x,box.min.y,box.min.z);
  glEnd();
  glBegin(GL_LINES);
    glVertex3f(box.min.x,box.max.y,box.min.z);
    glVertex3f(box.min.x,box.max.y,box.max.z);
    
    glVertex3f(box.max.x,box.max.y,box.min.z);
    glVertex3f(box.max.x,box.max.y,box.max.z);
    
    glVertex3f(box.min.x,box.min.y,box.max.z);
    glVertex3f(box.max.x,box.min.y,box.max.z);
  glEnd();

  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
}
