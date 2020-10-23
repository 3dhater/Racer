// qlib/curve.h - declaration

#ifndef __QLIB_CURVE_H
#define __QLIB_CURVE_H

#include <qlib/types.h>
#include <qlib/info.h>

typedef float qcfloat;

// Knot types
#define CMS_DEFAULT	0		// Curve
#define CMS_LINEAR	1

struct QCurvePoint
// A point on the curve
{
  enum
  { T_CURVED=0,				// Smooth curve
    T_LINEAR=1				// Hard linear knot
  };

  qcfloat x,y;
  int     type;
};

class QCurve
// QCurve does 2D curve approximation
// Feed it keypoints and use GetValue() to get curve points.
{
 public:
  // Axes
  qstring xName,yName;
  int xSteps,ySteps;
  int xMin,yMin,xMax,yMax;
  QCurvePoint *p;

 protected:
  // Class member variables
  int points;
  int pointsAllocated;

  void AllocatePoints(int n);
  
 public:
  QCurve();
  ~QCurve();

  int GetPoints(){ return points; }

  bool Load(QInfo *info,cstring path);
  bool Save(QInfo *info,cstring path);

  void Reset();    

  int FindRightPoint(qcfloat x);

  QCurvePoint *GetPoint(int n);
  int  AddPoint(qcfloat x,qcfloat y,int type=QCurvePoint::T_LINEAR);
  void ModifyPoint(int n,qcfloat x,qcfloat y);
  void RemovePoint(int n);
  void SetPointType(int n,int type);
  int  GetPointType(int n);

  qcfloat GetValue(qcfloat x);          // Approximate y for f(x)
};

#endif
