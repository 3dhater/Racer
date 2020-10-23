/*
 * Intersections - hitting the objects
 * 14-01-2001: Created! (22:57:04)
 * NOTES:
 * - May be incorporated into rtrackv.cpp for example
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// Define PICK_ALL_NODES to test for hits with ALL objects,
// not only those in the culler (sky and decal objects stay out of
// the culler)
#define PICK_ALL_NODES

/********************
* Manual projection *
********************/
void Unproject(DVector3 *vWin,DVector3 *vObj)
// Unprojects a window coordinate (vWin) into a 3D coordinate (vObj)
// vWin->z defines the depth into the display
// Is not that fast, since it retrieves matrix state information,
// and it inverts the model*proj matrix.
// For some easy picking, it will do though.
// ?BUG; it doesn't use gluUnproject, but gluProject. So the caller,
// below (PickTest) may have a bug and should call Project().
{
  double mModel[16],mPrj[16];
  int    viewport[4];
  GLdouble wx,wy,wz;
  GLdouble ox,oy,oz;
  int i;
  
//qdbg("Unprj: glCtx: %p\n",glXGetCurrentContext());
  // Retrieve matrices and viewport settings
  glGetDoublev(GL_MODELVIEW_MATRIX,mModel);
  glGetDoublev(GL_PROJECTION_MATRIX,mPrj);
  glGetIntegerv(GL_VIEWPORT,viewport);

#ifdef OBS
  for(i=0;i<16;i++)
  {
    qdbg("mModel[%d]=%g\n",i,mModel[i]);
  }  
  for(i=0;i<16;i++)
  {
    qdbg("mPrj[%d]=%g\n",i,mPrj[i]);
  }
#endif
  
  // Reverse projection
  wx=vWin->x;
  wy=vWin->y;
  wz=vWin->z;
  if(!gluUnProject(wx,wy,wz,mModel,mPrj,viewport,&ox,&oy,&oz))
  {
    qwarn("Unproject failed");
    vObj->SetToZero();
    return;
  }
  
  vObj->x=(dfloat)ox;
  vObj->y=(dfloat)oy;
  vObj->z=(dfloat)oz;
  
  if(!gluProject(ox,oy,oz,mModel,mPrj,viewport,&wx,&wy,&wz))
  {
    qwarn("Project failed");
  }
//qdbg("gluProject: %f,%f,%f\n",wx,wy,wz);
}
void Project(DVector3 *vObj,DVector3 *vWin)
// !! Text must be updated
// Projects a window coordinate (vWin) into a 3D coordinate (vObj)
// vWin->z defines the depth into the display
// Is not that fast, since it retrieves matrix state information,
// and it inverts the model*proj matrix.
// For some easy picking, it will do though.
{
  double mModel[16],mPrj[16];
  int    viewport[4],i;
  GLdouble wx,wy,wz;
  GLdouble ox,oy,oz;
  
  // Retrieve matrices and viewport settings
  glGetDoublev(GL_MODELVIEW_MATRIX,mModel);
  glGetDoublev(GL_PROJECTION_MATRIX,mPrj);
  glGetIntegerv(GL_VIEWPORT,viewport);

#ifdef OBS
  for(i=0;i<16;i++)
  {
    qdbg("mModel[%d]=%g\n",i,mModel[i]);
  }  
  for(i=0;i<16;i++)
  {
    qdbg("mPrj[%d]=%g\n",i,mPrj[i]);
  }
#endif
  
  // Reverse projection
  ox=vObj->x;
  oy=vObj->y;
  oz=vObj->z;
  if(!gluProject(ox,oy,oz,mModel,mPrj,viewport,&wx,&wy,&wz))
  {
    qwarn("gluProject failed");
    vWin->SetToZero();
    return;
  }
  
  vWin->x=(dfloat)wx;
  vWin->y=(dfloat)wy;
  vWin->z=(dfloat)wz;
}

/****************************
* RAY - SPHERE Intersection *
****************************/
bool RaySphereIntersect(DVector3 *origin,DVector3 *dir,
  DVector3 *center,dfloat radius,DVector3 *intersect)
// Determines whether a ray intersects the sphere
// From 'Real-Time Rendering', page 299.
{
  DVector3 l;
  dfloat   d,lSquared,mSquared;
  dfloat   rSquared=radius*radius;
  
#ifdef OBS
qdbg("origin %f,%f,%f\n",origin->x,origin->y,origin->z);
qdbg("dir %f,%f,%f\n",dir->x,dir->y,dir->z);
#endif
  // Calculate line from origin to center
  l=*center-*origin;
//qdbg("line=%f,%f,%f\n",l.x,l.y,l.z);
  // Calculate length of projection of direction onto that line
  d=l.Dot(*dir);
//qdbg("  proj. d=%f\n",d);
  lSquared=l.DotSelf();
//qdbg("  l^2=%f, radius=%f (^2=%f)\n",lSquared,radius,rSquared);
  if(d<0&&lSquared>rSquared)
  {
    // No intersection
    return FALSE;
  }
  
  // Check for how far we are off the center
  mSquared=lSquared-d*d;
  if(mSquared>rSquared)return FALSE;
  
  // Calculate intersection point
  if(!intersect)return TRUE;
  dfloat q,t;
  q=sqrt(rSquared-mSquared);
  if(lSquared>rSquared)t=d-q;
  else                 t=d+q;
//qdbg("t=%f\n",t);
  *intersect=*origin;
  // Hand-written t*(*dir)
  intersect->x+=t*dir->x;
  intersect->y+=t*dir->y;
  intersect->z+=t*dir->z;
  return TRUE;
}

/******************************
* RAY - TRIANGLE Intersection *
******************************/
#define EPSILON 0.00001
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2)\
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 

int RayTriangleIntersect(dfloat orig[3],dfloat dir[3],
  dfloat vert0[3],dfloat vert1[3],dfloat vert2[3],
  dfloat *t,dfloat *u,dfloat *v)
// From from Real-Time Rendering, page 305
{
  dfloat edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
  dfloat det,inv_det;

   /* find vectors for two edges sharing vert0 */
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   /* begin calculating determinant - also used to calculate U parameter */
   CROSS(pvec, dir, edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = DOT(edge1, pvec);

#ifdef TEST_CULL           /* define TEST_CULL if culling is desired */
   if (det < EPSILON)
      return 0;

   /* calculate distance from vert0 to ray origin */
   SUB(tvec, orig, vert0);

   /* calculate U parameter and test bounds */
   *u = DOT(tvec, pvec);
   if (*u < 0.0 || *u > det)
      return 0;

   /* prepare to test V parameter */
   CROSS(qvec, tvec, edge1);

    /* calculate V parameter and test bounds */
   *v = DOT(dir, qvec);
   if (*v < 0.0 || *u + *v > det)
      return 0;

   /* calculate t, scale parameters, ray intersects triangle */
   *t = DOT(edge2, qvec);
   inv_det = 1.0 / det;
   *t *= inv_det;
   *u *= inv_det;
   *v *= inv_det;
#else                    /* the non-culling branch */
   if (det > -EPSILON && det < EPSILON)
     return 0;
   inv_det = 1.0 / det;

   /* calculate distance from vert0 to ray origin */
   SUB(tvec, orig, vert0);

   /* calculate U parameter and test bounds */
   *u = DOT(tvec, pvec) * inv_det;
   if (*u < 0.0 || *u > 1.0)
     return 0;

   /* prepare to test V parameter */
   CROSS(qvec, tvec, edge1);

   /* calculate V parameter and test bounds */
   *v = DOT(dir, qvec) * inv_det;
   if (*v < 0.0 || *u + *v > 1.0)
     return 0;

   /* calculate t, ray intersects triangle */
   *t = DOT(edge2, qvec) * inv_det;
#endif
   return 1;
}

/********************************
* Finding a triangle in a geode *
********************************/
static int FindGeodeTriangle(DVector3 *origin,DVector3 *dir,DGeode *geode,
  DVector3 *inter,dfloat *tDistance)
// The ray is traced and tested if it hits any triangle in 'geode'.
// If a hit is found, it returns 1.
// In 'tDistance' also the distance to the triangle is stored.
// If no hit is found, 0 is returned.
{
  int g,tri,nTris,n;
  dfloat *pVertex;
  dindex *pIndex;
  DGeob *geob;
  dfloat v0[3],v1[3],v2[3];
  dfloat t,u,v;
  // Closest hit
  dfloat tMin=99999.0f;
  bool   isFound=FALSE;

  for(g=0;g<geode->geobs;g++)
  {
    geob=geode->geob[g];
    pVertex=geob->vertex;
    if(!pVertex)continue;
    nTris=geob->indices/3;
//qdbg("geob %d; nTris=%d\n",g,nTris);
    for(tri=0;tri<nTris;tri++)
    {
      n=geob->index[tri*3+0];
      pVertex=&geob->vertex[n*3];
      v0[0]=pVertex[0];
      v0[1]=pVertex[1];
      v0[2]=pVertex[2];
      
      n=geob->index[tri*3+1];
      pVertex=&geob->vertex[n*3];
      v1[0]=pVertex[0];
      v1[1]=pVertex[1];
      v1[2]=pVertex[2];
      
      n=geob->index[tri*3+2];
      pVertex=&geob->vertex[n*3];
      v2[0]=pVertex[0];
      v2[1]=pVertex[1];
      v2[2]=pVertex[2];
#ifdef OBS
qdbg("v0=(%.2f,%.2f,%.2f) v1=(%.2f,%.2f,%.2f) v2=(%.2f,%.2f,%.2f)\n",
v0[0],v0[1],v0[2],v1[0],v1[1],v1[2],v2[0],v2[1],v2[2]);
#endif
      if(RayTriangleIntersect(&origin->x,&dir->x,v0,v1,v2,&t,&u,&v))
      {
//qdbg("Intersect TRI! t=%.2f, u=%.2f, v=%.2f\n",t,u,v);
        // Is this the closest?
        if(t>=tMin)
          continue;
        tMin=t;
        // Return distance to triangle
        *tDistance=t;

        // Calculate intersection point
        // 2 ways are possible:
        // - follow the ray: origin+t*direction
        // - interpolate vertices; u and v are barycentric coordinates
        //   which we can average and thus get the weighted average
        //   of the 3 vertices that make up the triangle
        inter->x=origin->x+t*dir->x;
        inter->y=origin->y+t*dir->y;
        inter->z=origin->z+t*dir->z;
        if(mode==MODE_SPLINE)
        {
          // Find out which vertex was closest
          DVector3 v;
          rfloat   minDist=1e10;   // Distance^2 to vertex
          dfloat  *va;             // Ptr to closest vertex XYZ
          
          v.x=v0[0]; v.y=v0[1]; v.z=v0[2];
          minDist=v.SquaredDistanceTo(inter);
          va=v0;
          
          v.x=v1[0]; v.y=v1[1]; v.z=v1[2];
          if(v.SquaredDistanceTo(inter)<minDist)
          {
            minDist=v.SquaredDistanceTo(inter);
            va=v1;
          }
          
          v.x=v2[0]; v.y=v2[1]; v.z=v2[2];
          if(v.SquaredDistanceTo(inter)<minDist)
          {
            minDist=v.SquaredDistanceTo(inter);
            va=v2;
          }
          
          // Snap to closest vertex
          inter->x=va[0];
          inter->y=va[1];
          inter->z=va[2];
        }
        // Search on for possibly closer triangles
        isFound=TRUE;
        //return 1;
      }
    }
  }
  if(isFound)return 1;
  return 0;
}

/************
* Pick test *
************/
static RTV_Node *FindTrackNode(DCSLNode *cullerNode)
// From a given culler node, find the track node.
// Returns 0 in case it wasn't found
{
  int i;
  RTV_Node *trackNode;
  for(i=0;i<track->GetNodes();i++)
  {
    trackNode=track->GetNode(i);
    if(trackNode->geode==cullerNode->geode)
    {
      // Found the node! Store the results
      nodePropIndex=i;
      nodeProp=trackNode;
      return trackNode;
    }
  }
  // Not found
  return 0;
}
void PickTest(int x,int y,DTransformation *tf,DVector3 *pt)
// User picks window location (x,y)
// See where it ends up, puts it in 'pt'
// If a triangle is hit, the intersection is in 'pt'.
// If only a bounding sphere is hit, that's in 'pt' (the nearest hit)
// If nothing is hit, pt is untouched.
// A raytracing-type algorithm.
{
  DVector3 vWin;
  DVector3 vOrigin,vDir,vPrj,vIntersect;
  int i,n;
  //DCSLNode *node;
  RTV_Node *node;
  // Closest polygon
#ifdef OBS_NOT_VOLUME
  dfloat    curVolume,minVolume;
#endif
  dfloat    t,                     // Distance to closest tri
            tMin;

  QCV->Select();
  
  vWin.x=x; vWin.y=y;
 
  // Origin is camera position
  vOrigin.x=tf->x;
  vOrigin.y=tf->y;
  vOrigin.z=tf->z;
  
  // Get the point at the near plane where the mouse points to
  vWin.z=0.1f;
  Unproject(&vWin,&vPrj);
//qdbg("%d,%d -> obj %.2f,%.2f,%.2f\n",x,y,vPrj.x,vPrj.y,vPrj.z);
  // Get the ray from camera to near-plane intersection
  // (it's much like raytracing)
  vDir=vPrj-vOrigin;
//qdbg("Resulting dir: %.2f,%.2f,%.2f\n",vDir.x,vDir.y,vDir.z);
  //*pt=vDir;

  vDir.Normalize();
  
  // See if it hits a sphere in the track
  // Then check the triangles for a hit, and select the object
  // with the smallest volume
  //minVolume=999999.0f;
  tMin=999999.0f;
//qdbg("track=%p, culler=%p\n",track,track->GetCuller());
#ifdef PICK_ALL_NODES
  n=track->GetNodes();
//qdbg("%d nodes\n",n);
#else
  n=track->GetCuller()->GetNodes();
#endif
  for(i=0;i<n;i++)
  {
#ifdef PICK_ALL_NODES
    node=track->GetNode(i);
#else
    node=track->GetCuller()->GetNode(i);
#endif
    
#ifdef OBS
    DVector3 v;
    Project(&node->center,&v);
qdbg("Projected node center: %f,%f,%f\n",v.x,v.y,v.z);
#endif

#ifdef OBS
DVector3 v=node->center;
qdbg("Sphere center: %.2f,%.2f,%.2f, r=%.f\n",v.x,v.y,v.z,node->radius);
#endif
    if(RaySphereIntersect(&vOrigin,&vDir,&node->center,node->radius,
      &vIntersect))
    { 
#ifdef OBS_NOT_VOLUME
      // Check if this node has the smallest volume
      DBox box;
      node->geode->GetBoundingBox(&box);
      curVolume=box.Volume();
      if(curVolume>=minVolume)
      {
        // This object is bigger, don't hit it
        continue;
      }
#endif
      
//qdbg("** Intersect %d\n",i);
      //*pt=vIntersect;
#ifdef OBS
qdbg("intersection point=(%.2f,%.2f,%.2f)\n",
vIntersect.x,vIntersect.y,vIntersect.z);
#endif
      if(FindGeodeTriangle(&vOrigin,&vDir,node->geode,&vIntersect,&t))
      {
        // Check for a closer tri
        if(t<=0||t>=tMin)
          continue;
        tMin=t;
//qdbg("hit closer t=%f\n",t);

        // We've got a better hit than any before
        // vIntersect contains 3D coordinate of triangle intersection
        *pt=vIntersect;
//qdbg("intersection point=(%.2f,%.2f,%.2f)\n",
//vIntersect.x,vIntersect.y,vIntersect.z);
        // Remember which node was hit, and its size
#ifdef OBS_NOT_VOLUME
        minVolume=curVolume;
#endif
        if(mode==MODE_PROPS)
        {
#ifdef PICK_ALL_NODES
          nodeProp=node;
#else
          nodeProp=FindTrackNode(node);
#endif
        }
        // Search on for a possibly closer or better hit
        //break;
      }
    }
  }
}
