// racer/types.h

#ifndef __RACER_TYPES_H
#define __RACER_TYPES_H

#include <d3/types.h>

typedef float rfloat;

struct RCarPos
// 2 positions to indicate a car position and direction
{
  DVector3 from,to;
};

#endif
