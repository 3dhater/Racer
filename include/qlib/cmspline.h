// CMSpline.h - declaration

#ifndef __CMSPLINE_H
#define __CMSPLINE_H

typedef float cmfloat;

// Knot types
#define CMS_DEFAULT	0		// Curve
#define CMS_LINEAR	1

class CMSpline
// CMSpline does Catmull-Rom spline interpolation
// Feed it values and use GetValue() to get curve points.
// Note that if you have 5 control points, you can pass t=0..5
// (it will traverse automatically through the ctrl pts)
{
 protected:
  // Class member variables
  int points;
  int pointsAllocated;
  cmfloat *p;          		// Point locations
  int     *pType;		// Types

  void AllocatePoints(int n);
  
 public:
  CMSpline();
  ~CMSpline();

  int GetPoints(){ return points; }

  void Reset();    
  void AddPoint(cmfloat v,int n=-1);
  void RemovePoint(int n);
  void SetPointType(int n,int type);	// CMS_xxx

  cmfloat GetValue(cmfloat t);          // The whole curve
  cmfloat GetPoint(int n);              // Value by index
  int     GetPointType(int n);
};

#endif
