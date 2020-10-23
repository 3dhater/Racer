// d3/batch.h - batch rendering

#ifndef __D3_BATCH_H
#define __D3_BATCH_H

#include <d3/geode.h>

struct DBatch
{
 public:
  enum Max
  {
    MAX_GEOB=1000,
    MAX_MAT=1000
  };
  enum NodeFlags
  {
    IS_TRANSPARENT=1             // Node is transparent
  };
  enum Flags
  {
    HAS_TRANSPARENT_GEOBS=1      // Transparent objects detected
  };

 private:
  struct GeobNode
  {
    GeobNode *next;
    DGeob    *geob;
    short     burst;
    short     flags;
  };

  DMaterial *mat[MAX_MAT];       // Different materials
  DTexture  *tex[MAX_MAT];       // Textures
  GeobNode  *list[MAX_MAT];      // List of geobs for a material
  GeobNode   node[MAX_GEOB];     // Prepare nodes
  int    mats;                   // Also the #lists
  int    geobs;
  int    flags;

  int FindGeobList(DTexture *findTex);

 public:
  DBatch();
  ~DBatch();

  void Reset();
  void Add(DGeode *geode);
  void Flush();
};

#endif
