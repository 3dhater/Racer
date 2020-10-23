// racer/trackf.h

#ifndef __RACER_TRACKF_H
#define __RACER_TRACKF_H

#include <qlib/vector.h>
#include <racer/types.h>
#include <racer/track.h>
#include <d3/texture.h>

struct RTF_Sector
{
  char type,
       pad[3];
  int    steps;                         // Subdivisions
  rfloat length;			// Length of sector (longitudinal)
  rfloat angle,				// Angle for curve
         innerRadiusStart,              // Radius of inner turn
         outerRadiusStart,              // Radius of outer turn
         innerRadiusEnd,                // Radii at end of curve
         outerRadiusEnd;
  rfloat minLatStart,maxLatStart;	// Width at start
  rfloat minLatEnd,maxLatEnd;		// Width at end of sector
};
struct RTF_SectorGfx
// Associated graphics data
{
  int    vertices;
  float *fVertex;			// Vertex positions
  float *fTexCoord;                     // Texture coordinates
  float *tverticles;
  float *tnormals;
};

class RTrackFlat : public RTrack
{
 public:
  enum Max
  {
    MAX_SECTOR=100
  };

  enum SurfaceType
  {
    SRF_NULL,                            // No surface
    SRF_ROAD,
    SRF_GRASS,
    MAX_TEXTURE                          // #textures
  };
  
  enum SectorType
  {
    SECTOR_STRAIGHT,
    SECTOR_CURVE
  };

  RTF_Sector    *sector[MAX_SECTOR];
  RTF_SectorGfx *sectorGfx[MAX_SECTOR];
  int            sectors;

  // Textures for track itself
  DBitMapTexture *tex[MAX_TEXTURE];
  
  // Basic creation
  DVector3 createPos;
  float    createHeading;

 public:
  RTrackFlat();
  ~RTrackFlat();

  // Creation
  bool CreateTextures();
  // Override loading from RTrack base
  bool Load();

  // Basic building
 protected:
  void ToWorld(float longi,float lat,DVector3 *vWorld);

 public:
  bool AddSector(const RTF_Sector *newSector);

  // Graphics
  void PaintSector(int n);
  void Paint();
};

#endif
