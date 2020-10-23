/*
 * QGrid - helper class for gridded images
 * 30-11-99: Created!
 * NOTES:
 * - Helps when you have a number of images, divided over a number
 * of rows and columns. Easy fetching of grid blocks.
 * (C) MarketGraph/RvG
 */

#include <qlib/grid.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QGrid"

QGrid::QGrid(QRect *baseRect,int _rows,int _cols,int _maxImage,int _flags)
// Create a grid from a basic rectangle, extended for rows x cols
// Ordering of images is left to right, top to bottom (reading order)
// '_flags' may change ordering to column first, row next (Chinese reading?)
// (QGrid::VERTICAL). Horizontal ordering is preferred.
{
  DBG_C("ctor")
  DBG_ARG_I(_rows);
  DBG_ARG_I(_cols);

  rows=_rows;
  cols=_cols;
  gridInfo=0;
  flags=_flags;

  // Check
  if(rows<=0)rows=1;
  if(cols<=0)cols=1;

  x=baseRect->x;
  y=baseRect->y;
  widInside=baseRect->wid;
  hgtInside=baseRect->hgt;

  // Assume used area is full grid block
  widGrid=widInside;
  hgtGrid=hgtInside;

  // Calculate max image if not given
  maxImage=_maxImage;
  if(maxImage==-1)
    maxImage=rows*cols;
}

QGrid::~QGrid()
{
  DBG_C("dtor")
}

void QGrid::GetRect(int image,QRect *r)
// Retrieve the used area of image 'image'
{
  DBG_C("GetRect")
  int row,col;

  if(image>=maxImage)
  { qwarn("QGrid::GetRect; image (%d) larger than maxImage (%d)",
      image,maxImage);
    image=0;
  }
  if(flags&COLUMN_FIRST)
  { row=image%rows;
    col=image/rows;
  } else
  { // Default; rows first (reading order, lt-rt, up-dn)
    row=image/cols;
    col=image%cols;
  }
  r->x=x+widGrid*col;
  r->y=y+hgtGrid*row;
  r->wid=widInside;
  r->hgt=hgtInside;
}

