// d3/spline.h - types of splines

#ifndef __D3_SPLINE_H
#define __D3_SPLINE_H

#include <d3/types.h>

//
// Hermite splines
//

// Knot types
#define HMS_DEFAULT	0		// Curve
#define HMS_LINEAR	1

class DHermiteSpline
// Hermite spline interpolation
// Includes tension & bias parameters to control the tangents
// Feed it values and use GetValue() to get curve points.
// Note that if you have 5 control points, you can pass t=0..5
// (it will traverse automatically through the ctrl pts)
{
 public:
  enum Flags
  {
    LOOP_SPLINE=1               // Close spline at end to start?
  };

 protected:
  // Class member variables
  int flags;
  int points;
  int pointsAllocated;
  dfloat *p;          		// Point locations
  int    *pType;		// Types
  dfloat *m;                    // Tangents

  void AllocatePoints(int n);
  
 public:
  DHermiteSpline();
  ~DHermiteSpline();

  // Attribs
  int GetPoints(){ return points; }
  void EnableLoop(){ flags|=LOOP_SPLINE; }
  void DisableLoop(){ flags&=~LOOP_SPLINE; }
  bool IsLoop(){ if(flags&LOOP_SPLINE)return TRUE; return FALSE; }

  void Reset();    
  void AddPoint(dfloat v,int n=-1);
  void RemovePoint(int n);
  void SetPointType(int n,int type);	// HMS_xxx

  dfloat GetValue(dfloat t);          // The whole curve
  dfloat GetPoint(int n);              // Value by index
  int    GetPointType(int n);
};

#endif
