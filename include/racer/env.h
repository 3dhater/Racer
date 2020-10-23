// renv.h - Environment

#ifndef __RENV_H
#define __RENV_H

#include <qlib/types.h>

class REnvironment
// The environment in which the cars are located
{
  float airDensity;			// Density
  float gravity;

 public:
  REnvironment();
  ~REnvironment();

  bool Load(cstring fileName);

  float GetAirDensity(){ return airDensity; }
  float GetGravity(){ return gravity; }
};

#endif
