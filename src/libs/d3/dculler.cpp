/*
 * DCuller - (view frustum) culling base class
 * 04-01-2001: Created! (19:29:18)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/culler.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DCuller"

DCuller::DCuller()
{
  DBG_C("ctor")
}
DCuller::~DCuller()
{
  DBG_C("dtor")
}

/*********************************
* Calculating the frustum planes *
*********************************/
void DCuller::CalcFrustumEquations()
// From the current OpenGL modelview and projection matrices,
// calculate the frustum plane equations (Ax+By+Cz+D=0, n=(A,B,C))
// The equations can then be used to see on which side points are.
{
  DBG_C("CalcFrustumEquations")
  
  dfloat *m;
  DPlane3 *p;
  int i;
 
  // This piece of code taken from:
  // http://www.markmorley.com/opengl/frustumculling.html
  // Modified quite a bit to suit the D3 classes.
  
  QTRACE_PRINT("Getting projection matrices\n");
  
  // Retrieve matrices from OpenGL
  glGetFloatv(GL_MODELVIEW_MATRIX,matModelView.GetM());
//matModelView.DbgPrint("ModelView");
  glGetFloatv(GL_PROJECTION_MATRIX,matProjection.GetM());
//matProjection.DbgPrint("Projection");
  
  // Combine into 1 matrix
  QTRACE_PRINT("Combine modelview+prj\n");
  glGetFloatv(GL_PROJECTION_MATRIX,matFrustum.GetM());
  matFrustum.Multiply(&matModelView);
//matFrustum.DbgPrint("Combined (frustum)");
  
  QTRACE_PRINT("Deduce planes\n");
  
  // Get plane parameters
  m=matFrustum.GetM();
  
  p=&frustumPlane[RIGHT];
  p->n.x=m[3]-m[0];
  p->n.y=m[7]-m[4];
  p->n.z=m[11]-m[8];
  p->d=m[15]-m[12];
  
  p=&frustumPlane[LEFT];
  p->n.x=m[3]+m[0];
  p->n.y=m[7]+m[4];
  p->n.z=m[11]+m[8];
  p->d=m[15]+m[12];
  
  p=&frustumPlane[BOTTOM];
  p->n.x=m[3]+m[1];
  p->n.y=m[7]+m[5];
  p->n.z=m[11]+m[9];
  p->d=m[15]+m[13];
  
  p=&frustumPlane[TOP];
  p->n.x=m[3]-m[1];
  p->n.y=m[7]-m[5];
  p->n.z=m[11]-m[9];
  p->d=m[15]-m[13];
  
  p=&frustumPlane[PFAR];
  p->n.x=m[3]-m[2];
  p->n.y=m[7]-m[6];
  p->n.z=m[11]-m[10];
  p->d=m[15]-m[14];
  
  p=&frustumPlane[PNEAR];
  p->n.x=m[3]+m[2];
  p->n.y=m[7]+m[6];
  p->n.z=m[11]+m[10];
  p->d=m[15]+m[14];
  
  // Normalize all plane normals
  QTRACE_PRINT("Normalize planes\n");
  for(i=0;i<6;i++)
    frustumPlane[i].Normalize();

#ifdef OBS
  // Check
  DMatrix4 m;
  glMatrixMode(GL_PROJECTION);
  glMultMatrixf(matModelView.GetM());
  glGetFloatv(GL_PROJECTION_MATRIX,m.GetM());
  m.DbgPrint("Check OpenGL glMultMatrix");
  glMatrixMode(GL_MODELVIEW);
#endif
}

/**************
* Classifying *
**************/
int DCuller::PointInFrustum(const DVector3 *point) const
{
  return INSIDE;
}

int DCuller::SphereInFrustum(const DVector3 *center,dfloat radius) const
// Returns classification (INSIDE/INTERSECTING/OUTSIDE)
{
  int i;
  const DPlane3 *p;
  
  for(i=0;i<6;i++)
  {
    p=&frustumPlane[i];
#ifdef OBS_DEV
    {
      float d=p->n.x*center->x+p->n.y*center->y+p->n.z*center->z+p->d;
      qdbg("SphereIn(%f,%f,%f) radius %f => distance %f\n",
        center->x,center->y,center->z,radius,d);
    }
#endif
    if(p->n.x*center->x+p->n.y*center->y+p->n.z*center->z+p->d <= -radius)
      return OUTSIDE;
  }
  // Decide: Inside or intersecting
  return INTERSECTING;
}

/************
* Debugging *
************/
void DCuller::DbgPrint(cstring s)
{
  qdbg("DCuller: [%s]\n",s);
  qdbg("  "); frustumPlane[RIGHT].DbgPrint("right");
  qdbg("  "); frustumPlane[LEFT].DbgPrint("left");
  qdbg("  "); frustumPlane[TOP].DbgPrint("top");
  qdbg("  "); frustumPlane[BOTTOM].DbgPrint("bottom");
  qdbg("  "); frustumPlane[PFAR].DbgPrint("far");
  qdbg("  "); frustumPlane[PNEAR].DbgPrint("near");
}
