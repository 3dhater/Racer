// d3/mesh.h - declaration

#ifndef __D3_DMESH_H
#define __D3_DMESH_H

#include <d3/object.h>
#include <d3/poly.h>

class DMesh : public DObject
// A collection of connected polygons
{
 protected:
  // Class member variables
  DPoly **poly;
  int   polys,apolys;		// In use, allocated

 public:
  DMesh();
  ~DMesh();

  // Info
  DPoly *GetPoly(int n);

  // Primitives
  void Define(int polys);
  // Shortcut for primitives
  void DefineFlat2D(int w,int h,int xOff=0,int yOff=0);
  void DefineDS(int w,int h,int xOff=0,int yOff=0);

  // Painting
  void _Paint();
};

#endif
