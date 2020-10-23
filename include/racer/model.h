// rmodel.h

#ifndef __RACER_MODEL_H
#define __RACER_MODEL_H

#include <d3/geode.h>

class RCar;

class RModel
// A 3D model, used by some car components for graphics
{
 protected:
  DGeode *geode;                        // The 3D model
  RCar   *car;
  qstring fileName;                     // Where is it stored?

 public:
  RModel(RCar *car);
  ~RModel();

  // Attribs
  DGeode *GetGeode(){ return geode; }
  bool    IsLoaded();

  // I/O
  bool Load(QInfo *info,cstring path,cstring modelTag=0);
  bool Save();

  // Painting
  void Paint();
};

#endif
