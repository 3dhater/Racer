// racer/filter.h

#ifndef __RACER_FILTER_H
#define __RACER_FILTER_H

#include <racer/types.h>

class RFilter
{
 protected:
  enum Flags
  {
    DAMPING=1                     // Are we damping?
  };

  int    flags;
  rfloat value;                   // The value
  rfloat damping;                 // Sort of the inertia
  rfloat invDamping;              // 1/damping (precalculated)

 public:
  RFilter(rfloat damping);
  ~RFilter();

  rfloat GetValue(){ return value; }

  void EnableDamping();
  void DisableDamping();
  void SetDamping(rfloat damping);

  void Set(rfloat v){ value=v; }
  void MoveTo(rfloat v);
};

#endif

