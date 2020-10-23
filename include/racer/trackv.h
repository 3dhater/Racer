// racer/trackv.h

#ifndef __RACER_TRACKV_H
#define __RACER_TRACKV_H

#include <qlib/vector.h>
#include <racer/types.h>
#include <racer/track.h>
#include <d3/texture.h>
#include <d3/culler.h>

struct RTV_Node
// A part of the track
{
  enum Flags
  {
    F_SKY=1,                    // This node is part of the sky
    F_COLLIDE=2,                // May collide with this
    F_SURFACE=4,                // Is a surface
    F_RELATIVE_POS=8,           // Moves with the car or camera (horizons)
    F_INVISIBLE=16,             // Not a rendered object
    F_NO_CULLING=32,            // Render all faces (NYI)
    F_NO_ZBUFFER=64,            // Disable Zbuffer read/write
    F_DECAL=128,                // Drawn on top (driving line, static skids)
    F_XXX
  };

  char type,
       flags,
       pad[2];
  qstring fileName;		// Name of the model file
  DGeode *geode;		// The graphics
  // Culling properties (copies)
  DVector3 center;
  dfloat   radius;
};

class RTrackVRML : public RTrack
// A track class based on VRML-style nodes
// Based on the SCGT (ISI's Sports Car GT) track model a bit
{
 public:
  enum Max
  {
    MAX_NODE=1500		// Max #parts
  };
  enum Flags
  {
    CULLER_READY=1		// Culling structure prepared?
  };

  int flags;
  RTV_Node *node[MAX_NODE];
  int       nodes;
  DCullerSphereList *culler;

 public:
  RTrackVRML();
  ~RTrackVRML();

  // Override loading from RTrack base
  bool Load();
  bool Save();

  // Attribs
  DCullerSphereList *GetCuller(){ return culler; }

  // Basic building
  int       GetNodes(){ return nodes; }
  RTV_Node *GetNode(int n){ return node[n]; }
  bool      AddNode(const RTV_Node *newNode);
  void CreateAABBTree();

  // Graphics
  void BuildCuller();
  void DestroyCuller();
  void PaintNode(int n);
  void Paint();
  void PaintHidden();
  void PaintHiddenCams();
  void PaintHiddenWayPoints();
  void PaintHiddenRoadVertices();
  void PaintHiddenSplineTris();

  // Surface detection
  void GetSurfaceInfo(DVector3 *pos,DVector3 *dir,RSurfaceInfo *si);

  // Collision detection
  bool CollisionOBB(DOBB *obb,DVector3 *intersection,DVector3 *normal);
};

#endif
