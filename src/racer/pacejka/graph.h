// graph.h

#ifndef __GRAPH_H
#define __GRAPH_H

#include <racer/pacejka.h>

class Graph
{
 public:

  enum Max
  {
    MAX_VALUE=1000
  };
  enum Params
  {
    PARAM_FX,
    PARAM_FY,
    PARAM_MZ,
    PARAM_LONG_STIFFNESS,
    PARAM_CORN_STIFFNESS,
    MAX_PARAM
  };
  enum Flags
  {
    NO_FOCUS=1,          // Draw soft
    SHOW_COEFF=2         // Display coefficients?
  };

  // Values
  float minX,maxX;
  float minY,maxY;
  qstring xName,yName;

  // Painting
  int   flags;
  float scaleX,scaleY;
  int   wid,hgt;
  int   steps;
  float v[MAX_VALUE];
  int   offX,offY;
  QColor *col;

  // Source
  RPacejka *pacejka;
  int       param;
  
 public:
  Graph(int param);
  ~Graph();

  void LoadFrom(cstring carName);

  cstring GetHorizontalUnit();
  cstring GetVerticalUnit();

  void CalculatePoints();

  void Paint();
};

#endif
