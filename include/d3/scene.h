// d3/scene.h - A scene with a collection of DGeodes

#ifndef __D3_SCENE_H
#define __D3_SCENE_H

#include <d3/geode.h>

class DScene
// A collection of models
{
 public:
  enum Max
  { MAX_GEODE=500
  };

 protected:
  int geodes;
  DGeode *geode[MAX_GEODE];

 public:
  DScene();
  ~DScene();

  // Attribs
  int     GetGeodes(){ return geodes; }
  DGeode *GetGeode(int n){ return geode[n]; }

  // Operations
  bool AddGeode(DGeode *geode);
  //bool FindGeode(DGeode *geode);
};

#endif
