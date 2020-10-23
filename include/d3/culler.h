// d3/culler.h - culling base class

#ifndef __D3_CULLER_H
#define __D3_CULLER_H

#include <d3/matrix.h>

class DCuller
// A cull interface class with some basic operations
{
 public:
  enum Planes
  // A number of planes symbols to indicate which direction it is for
  {
    RIGHT,LEFT,BOTTOM,TOP,PFAR,PNEAR
  };
  enum Classifications
  {
    OUTSIDE=-1,			// Bounding volume outside frustum
    INTERSECTING=0,		// BV intersecting frustum
    INSIDE=1			// Completely inside
  };

 protected:
  DPlane3 frustumPlane[6];	// The 6 planes which bound the frustum
  DMatrix4 matModelView,	// Matrices of modelview and projection
           matProjection,
           matFrustum;		// PROJECTION*MODELVIEW

 public:
  DCuller();
  virtual ~DCuller();

  // Attribs
  DPlane3 *GetFrustumPlane(int n){ return &frustumPlane[n]; }

  // Operations
  void CalcFrustumEquations();
  virtual void Paint(){}

  // Classifying bounding volumes
  int  PointInFrustum(const DVector3 *p) const;
  int  SphereInFrustum(const DVector3 *center,dfloat radius) const;

  // Debugging
  void DbgPrint(cstring s);
};

/***************
* SPHERE LISTS *
***************/
struct DCSLNode
{
  DGeode  *geode;		// The model
  DVector3 center;		// Center of sphere
  dfloat   radius;		// Size of sphere
};
class DCullerSphereList : public DCuller
// A culling class based on sphere bounding volumes and a list
{
  enum Max
  { MAX_NODE=500
  };
 protected:
  int nodes;
  DCSLNode *node;
 public:
  DCullerSphereList();
  ~DCullerSphereList();

  // Attribs
  int GetNodes(){ return nodes; }
  DCSLNode *GetNode(int n){ return &node[n]; }

  void Destroy();

  // Operations
  bool AddGeode(DGeode *geode);
  void Paint();
};

#endif
