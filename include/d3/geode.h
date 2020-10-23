// d3/geode.h - Geometric object (lots of polygons)

#ifndef __D3_GEODE_H
#define __D3_GEODE_H

#include <d3/object.h>
#include <d3/poly.h>
#include <d3/material.h>

class DGeode;

typedef unsigned short dindex;		// Index format

class DGeob
// A geometric object, consisting of a single attached mesh
// Several of these may exist in a single DGeode
// When importing .ASE files, every GEOMOBJECT is 1 DGeob
{
 public:
  enum MaxThings
  {
    MAX_FACE_BURST=100          // Face ranges that share the same MTLID
  };
  enum PaintFlags
  {
    SIMPLE_TEXTURE=1		// Just a texture, no lighting or material
  };
  enum Flags
  // Generic flags; none used yet (future)
  {
    PASS_TRANSPARENT=1          // Only paint transparent geob bursts
  };

  int flags,paintFlags;

  // OpenGL vertex arrays
  dfloat *vertex;
  dfloat *face_v,*face_nrm;
  // Indexed face sets
  dindex *index;
  int     indices;
  dfloat *normal;		// The indexed version of 'face_nrm'
  int     normals;
  // Texturing
  dfloat *tvertex;
  dfloat *tface_v;
  // Coloring
  dfloat *vcolor;
  // Loading arrays
  dfloat *nrmVector;
  int     nrmVectors;

  // Primitives
  int vertices,
      faces;
  int tvertices,tfaces,vcolors;
  // Allocated sizes (not all polygons have to be triangles)
  int face_v_elements,
      face_nrm_elements,
      tface_v_elements;
  
  // Dividing rendered faces into groups of equal material (bursts
  // of faces that look the same)
  int burstStart[MAX_FACE_BURST];
  int burstCount[MAX_FACE_BURST];
  int burstMtlID[MAX_FACE_BURST];
  int burstVperP[MAX_FACE_BURST];	// Vertices per polygon
  int bursts;

  int materialRef;            // Which material is used (submat's are used!)

  GLuint list;			// OpenGL display list

  // Our master
  DGeode *geode;

 public:
  DGeob(DGeode *geode);
  DGeob(DGeob *master,int flags);
  ~DGeob();

  void Destroy();
  void DestroyList();
  void DestroyNormals();	// Get rid of normal information

  // I/O
  int  GetBurstForFace(int n);
  bool ImportDOF(QFile *f);
  bool ExportDOF(QFile *f);

  // Attribs
  DMaterial *GetMaterialForID(int id);
  void GetBoundingBox(DBox *box);

  // Painting
  void Paint();
  void PaintOptimized(int burst,int flags);

  // Modeling operations
  void ScaleVertices(float x,float y,float z);
  void Translate(float x,float y,float z);
  void FlipFaces(int burst=-1);
};

class DGeode : public DObject
// A collection of connected geobs, mostly just loaded from disk files
// Contains the materials, the actual polygons themselves are stored
// in DGeobs
{
 public:
  enum MaxThings
  {
    MAX_MATERIAL=250,           // Different materials
    MAX_GEOB=150		// DGeob count
  };
  enum CloneFlags
  { CLONE_COPY_MATERIAL=1,	// Make a separate copy of the materials
    CLONE_COPY_GEOMETRY=2	// Copy geometry
  };
  enum Flags
  {
    VIEW_BURSTS=1		// Show bursts by color, not textured
  };
  
 public:
  // Materials
  qstring    pathMap;              // Images path
  DMaterial *material[MAX_MATERIAL];
  int        materials;
  int        flags;

 public:
  // GeomObjects
  DGeob *geob[MAX_GEOB];
  int    geobs;

 public:
  DGeode(cstring fname);
  DGeode(DGeode *master,int flags);
  ~DGeode();

  void Destroy();
  void DestroyLists();
  void DestroyNormals();	// Get rid of all normal information

  // Attributes
  DMaterial *GetMaterial(int n);
  DMaterial *FindMaterial(cstring name);
  int        GetNoofGeobs(){ return geobs; }
  void       SetViewBursts(bool yn);
  bool       IsViewBurstsEnabled()
  { if(flags&VIEW_BURSTS)return TRUE; return FALSE; }
  void       GetBoundingBox(DBox *box);

  // I/O
  void    SetMapPath(cstring path);
  cstring GetMapPath(){ return pathMap; }
  void    FindMapFile(cstring fname,string d);
  //bool    ImportASE(cstring fname);
  //bool    ImportVRML(cstring fname);
  bool    ImportDOF(QFile *f);
  bool    ExportDOF(QFile *f);
 
  // Painting
  void _Paint();

  // Operations
  void ScaleVertices(float x,float y,float z);
  void Translate(float x,float y,float z);
  //void Triangulate();
  //void RethinkNormals();
  //void OptimizeIndices();
};

// Support modules - separated for clarity and smaller linking
// dgeode_opt.cpp
int  DGeodeOptimizeBursts(DGeode *g);
int  DGeodeOptimizeIndices(DGeode *g);
int  DGeodePackIndices(DGeode *g);
// dgeode_op.cpp
void DGeodeTriangulate(DGeode *g);
void DGeodeRethinkNormals(DGeode *g);
// dgeode_ase.cpp
bool DGeodeImportASE(DGeode *g,cstring fname);
// dgeode_vrml.cpp
bool DGeodeImportVRML(DGeode *g,cstring fname);
// dgeode_ewrl.cpp
bool DGeodeExportVRML(DGeode *g,cstring fname);
// dgeode_3do.cpp
bool DGeodeImport3DO(DGeode *g,cstring fname);
// dgeode_wave.cpp
bool DGeodeImportWAVE(DGeode *g,cstring fname);

#endif
