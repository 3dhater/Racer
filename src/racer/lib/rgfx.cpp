/*
 * Racer - graphics help
 * 09-08-00: Created!
 * 05-10-00: Incorporated into Racer.lib (all functions prefixed with R)
 * (C) Dolphinity
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
#include <qlib/canvas.h>
#include <qlib/app.h>
DEBUG_ENABLE

void RGfxText3D(cstring s)
// Paint text at the current 3D location
{
  int len;

  // QASSERT_V(s);

  len=strlen(s);
  glListBase(QSHELL->GetCanvas()->GetFont()->GetListBase());
  glCallLists(len,GL_UNSIGNED_BYTE,s);
}
void RGfxText3D(float x,float y,float z,cstring s)
{
  RGfxGo(x,y,z);
  RGfxText3D(s);
}

void RGfxGo(float x,float y,float z)
{
  glPushMatrix();
  //glScalef(.5,.5,.5);
  glRasterPos3f(x,y,z);
  glPopMatrix();
}

void RGfxArrow(float x,float y,float z,float *v,float scale)
// Paint a vector arrow
// Used in painting forces to clarify things
{
  float len;
  float wid=.05f,		// In meters
        headLen;

  // Calc length of vector
  len=sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  len*=scale;
  // Head of 10cm or relative to force if it isn't big enough
  if(len>.1)headLen=.1;
  else      headLen=len*.3;
  //glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  //glDisable(GL_BLEND);
  glPushMatrix();
  //glScalef(scale,scale,scale);
  // Is the arrow backwards (vector's Z<0)
  // Note that to paint (0,0,-1) we'll have actually paint the reverse
  if(v[2]>0)
  { len=-len;
    headLen=-headLen;
  }
  // Forward arrow
  glBegin(GL_QUADS);
    glVertex3f(x-wid,y,z);
    glVertex3f(x+wid,y,z);
    glVertex3f(x+wid,y,z-len+headLen);
    glVertex3f(x-wid,y,z-len+headLen);
  glEnd();
  // Head
  glBegin(GL_TRIANGLES);
    glVertex3f(x-wid*2,y,z-len+headLen);
    glVertex3f(x+wid*2,y,z-len+headLen);
    glVertex3f(x,y,z-len);
  glEnd();
  glPopMatrix();

  //glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  //glEnable(GL_BLEND);
}

void RGfxAxes(float x,float y,float z,float len)
// Paint an axis system
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glBegin(GL_LINES);
    // X/Y/Z
    glColor3f(0,0,1);
    glVertex3f(x,y,z);
    glVertex3f(x+len,y,z);
    glColor3f(0,1,0);
    glVertex3f(x,y,z);
    glVertex3f(x,y+len,z);
    glColor3f(1,0,0);
    glVertex3f(x,y,z);
    glVertex3f(x,y,z+len);
  glEnd();

  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
}

void RGfxVector(DVector3 *v,float scale,float r,float g,float b)
// Paint a vector (line)
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glColor3f(r,g,b);
  glBegin(GL_LINES);
    // X/Y/Z
    glVertex3f(0,0,0);
    glVertex3f(v->x*scale,v->y*scale,v->z*scale);
  glEnd();

  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
}

