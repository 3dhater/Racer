// racer/track.h
// 26-08-01; Splines extended from 1D to 3D (full surface patch)

#ifndef __RACER_TRACK_H
#define __RACER_TRACK_H

#include <racer/types.h>
#include <racer/ai.h>
#include <racer/sun.h>
#ifndef __RACER_STATS_H
#include <racer/stats.h>
#endif
#include <d3/types.h>
#include <d3/obb.h>
#include <d3/spline.h>
#include <d3/aabbtree.h>
//#include <qlib/archive.h>

class QArchive;

/***********
* Surfaces *
***********/
struct RSurfaceType;

struct RSurfaceInfo
// Describes the state of a point on the track
// Is requested from the track for all wheels
{
  // Input
  DAABBNode *startNode;         // Where to start searching in the tree

  // Position
  rfloat x,y,z;			// Y contains height (altitude)
  rfloat surfaceDistance;       // Distance to actual surface
  // Rotation
  rfloat grade,			// Pitch of track
         bank;			// Roll of track
  // Surface
  rfloat friction;		// Coefficient of friction for the surface
  // Other useful info
  rfloat t,u,v;                 // Intersection barycentric coordinates
  // Pointer to surface?
  DMaterial *material;          // To detect surface
  RSurfaceType *surfaceType;    // Surface type detected (or 0)
  // Caching info (RTrackVRML)
  int lastNode;			// Last sphere node (geode) that was hit
  int lastGeob;			// Last geob that was hit in the node
  int lastTri;			// Last triangle that was hit
  DAABBPolyInfo *lastPI;        // Cache for the tree polygon

  // Ctor
  RSurfaceInfo()
  { lastNode=lastGeob=lastTri=-1;
    lastPI=0;
    material=0; surfaceType=0;
    t=u=v=0;
    x=y=z=0;
    surfaceDistance=0;
    // Unused vars?
    friction=0; grade=bank=0;
  }
};


struct RSurfaceType
// A type of surface, based on a basic type, and refined
// with friction factors
{
 public:
  enum SurfaceTypes
  {
    ROAD,               // Regular road
    GRASS,
    GRAVEL,
    STONE,
    WATER,
    KERB,
    ICE,
    SNOW,
    MUD,
    SAND,
    MAX
  };
 //protected:
  char baseType,
       subType;
  rfloat frictionFactor,
         rollingResistanceFactor;

 public:
  RSurfaceType()
  { baseType=ROAD;
    subType=0;
    frictionFactor=rollingResistanceFactor=1;
  }
};

class RCar;

class RTrackCam
{
 public:
  enum RTrackCamFlags
  {
    ZOOMDOT=1,                  // Use projection along camera line to decide
                                // zooming factor. Otherwise the distance
                                // to the camera is used.
    FIXED=2,                    // Doesn't follow car
    HILITE=4,                   // In TrackEd, hilite the camera (selected)
    DOLLY=8                     // Move camera from pos to pos2
  };
  int      flags;
  // Static properties
  DVector3 pos,                 // Location
           posDir,              // Gives general direction of driving
           posDolly;            // Move-to point
  DVector3 rot,                 // Orientation for pos
           rot2;                // 2nd orientation
  int      group;               // Multiple camera groups are possible
  rfloat   radius;              // When entering the radius, select the cam
  rfloat   zoomEdge,
           zoomClose,
           zoomFar;             // Used in case of RTCF_ZOOMDOT
  // Preview cubes
  DVector3 cubePreview[3];      // Edge, close, far cubes for previewing

  // State (dynamic)
  RCar    *car;                 // Which car we're tracking
  rfloat   normalizedDistance;  // -1..1 The distance of the car
  rfloat   zoom;                // Current zoom value
  DVector3 curPos;              // Current position, including dolly effects

  RTrackCam();
  ~RTrackCam();

  DVector3 *GetCurPos(){ return &curPos; }

  bool Load(QInfo *info,cstring path);

  // Car
  RCar *GetCar(){ return car; }
  void  SetCar(RCar *car);

  // Distances
  void CalcState();
  bool IsInRange();
  bool IsFarAway();
  rfloat ProjectedDistance();

  // Camera specifics
  rfloat GetZoom(){ return zoom; }
  void   GetOrigin(DVector3 *v);
  void   Go(DVector3 *target);
  void   Go();
};

/**********
* Splines *
**********/
struct RSplineLine
// Spline definition along the track (single lateral line)
{
  char type,
       flags;
  DVector3 cp[2];          // Control points; 0=left, 1=right (side of track)
};
struct RSplineTri
// Triangle built from the line definitions
{
  DVector3 v[3];
};
class RSplineRep
// Spline representation of a track
{
 public:
  enum Max
  {
    MAX_TRACE=2           // Lateral traces through the track
  };

 protected:
  RSplineLine *line;
  int          lines,linesAllocated;
  RSplineTri  *tri;
  int          tris,trisAllocated;
  DHermiteSpline *splineHM[MAX_TRACE][3];   // XYZ

 public:
  RSplineRep();
  ~RSplineRep();

  // Attribs
  RSplineLine *GetLine(int n){ return &line[n]; }
  int          GetLines(){ return lines; }
  RSplineTri  *GetTri(int n){ return &tri[n]; }
  int          GetTris(){ return tris; }
  DHermiteSpline *GetSplineHM(int trace,int index)
  { return splineHM[trace][index]; }

  // Memory
  bool Reserve(int lines);

  // Building
  void AddLine(RSplineLine *line);
  void RemoveLine();
  void BuildTriangles();
  void BuildSpline();

  // I/O
  void Load(QInfo *info);
  void Save(QInfo *info);
};

// Callback function for lengthy loads
typedef bool (*RTrackLoadCB)(int cur,int total,cstring text);

/************
* Tree info *
************/
struct RTreePolyInfo
// Extra info on polygons in the AABB tree, except for the vertex data
// itself, which is already in class DAABBPolyInfo.
{
  DMaterial *mat;
  int        splineTri;          // If >=0, the spline triangle index
};

/********
* Track *
********/
class RTrack
// Base class for Racer tracks
// Works tightly actually though with RTrackVRML
{
 protected:
  // Generic information
  qstring name,creator,length;	// Full name and other info
  qstring type,file;
  qstring trackName,trackDir;	// Short name and directory
  RTrackLoadCB cbLoad;		// Loading callback function
  QInfo *infoSpecial;           // special.ini file
  QArchive *archive;            // Packed file

 public:
  enum Max
  {
    MAX_TIMELINE=20,            // Max #timelines (checkpoint lines)
    MAX_GRID_POS=50,            // Max #grid positions
    MAX_SURFACE_TYPE=50,        // Different surfaces
    MAX_WAYPOINT=400,           // AI waypoints
    MAX_TRACKCAM=200,           // Track camers
    MAX_PIT_POS=50              // Max #pit positions
  };

  // Timing
  RTimeLine *timeLine[MAX_TIMELINE];
  int        timeLines;

  // Statistics
  RStats stats;
  QInfo *infoStats;

  // Positions
  RCarPos gridPos[MAX_GRID_POS],
          pitPos[MAX_PIT_POS];
  int     gridPosCount,pitPosCount;

  // std::vector gave uninit'd memory hits in Purity at ctor time;
  // replacing by array to be sure
  RTrackCam *trackCam[MAX_TRACKCAM];
  int        trackCams;
  RWayPoint *wayPoint[MAX_WAYPOINT];
  int        wayPoints;
  //std::vector<RTrackCam*> trackCam;
  //std::vector<RWayPoint*> wayPoint;

  // Track surface
  RSplineRep    splineRep;
  RSurfaceType *surfaceType[MAX_SURFACE_TYPE];
  int           surfaceTypes;
  DAABBTree    *trackTree;      // Tree of all geometry+splines in the track

  // Graphics
  dfloat  clearColor[4];        // Color of clear sky
  RSun   *sun;                  // Simulating a shining sun

 public:
  RTrack();
  virtual ~RTrack();

  // Attributes
  void    SetName(cstring trackName);
  cstring GetName(){ return trackName; }
  cstring GetTrackDir(){ return trackDir; }
  cstring GetFullName(){ return name; }
  cstring GetType(){ return type; }
  RStats *GetStats() { return &stats; }

  RSplineRep *GetSplineRep(){ return &splineRep; }
  DAABBTree  *GetTrackTree(){ return trackTree; }

  int        GetTimeLines(){ return timeLines; }
  RTimeLine *GetTimeLine(int n);

  // Car positions
  int      GetGridPosCount(){ return gridPosCount; }
  RCarPos *GetGridPos(int n);
  int      GetPitPosCount(){ return pitPosCount; }
  RCarPos *GetPitPos(int n);
  //int        GetTrackCamCount(){ return trackCam.size(); }
  int        GetTrackCamCount(){ return trackCams; }
  RTrackCam *GetTrackCam(int n);
  bool       AddTrackCam(RTrackCam *cam);
  //int        GetWayPointCount(){ return wayPoint.size(); }
  int        GetWayPointCount(){ return wayPoints; }
  bool       AddWayPoint(RWayPoint *wp);

  // Surface info for physics engine
  virtual void GetSurfaceInfo(DVector3 *pos,DVector3 *dir,RSurfaceInfo *si);

  // Collision detection
  virtual bool CollisionOBB(DOBB *obb,DVector3 *intersection,DVector3 *normal);

  // I/O
  RTrackLoadCB GetLoadCallback(){ return cbLoad; }
  void         SetLoadCallback(RTrackLoadCB f){ cbLoad=f; }
  bool         LoadSpecial();
  bool         LoadSurfaceTypes();
  virtual bool Load();
  bool         SaveSpecial();
  virtual bool Save();

  // Painting
  virtual void Paint();
  virtual void PaintHidden();
  void PaintHiddenTCamCubes(int n);

  // Idle processing
  void Update();
};

#endif
