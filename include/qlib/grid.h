// qlib/grid.h

#ifndef __QLIB_GRID_H
#define __QLIB_GRID_H

#include <qlib/point.h>

struct QGridInfo
{
  int x,y;
  int wid,hgt;
};

class QGrid
{
  int rows,cols;
  QGridInfo **gridInfo;
  int x,y;
  int widInside,hgtInside;	// Used area inside grid block
  int widGrid,hgtGrid;		// Grid size
  int maxImage;
  int flags;

 public:
  enum Flags
  { ROW_FIRST=0,		// Default ordering; rows first
    COLUMN_FIRST=1 };		// Vertical ordering; columns first

 public:
  QGrid(QRect *baseRect,int rows,int cols,int maxImage=-1,int flags=0);
  ~QGrid();

  void GetRect(int image,QRect *r);
  int  GetMaxImage(){ return maxImage; }
};

#endif
