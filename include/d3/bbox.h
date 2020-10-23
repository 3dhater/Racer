// d3/bbox.h - declaration

#ifndef __D3_BBOX_H
#define __D3_BBOX_H

#include <d3/object.h>
#include <d3/types.h>

class DBoundingBox : public DObject
// A bounding box mesh; for debugging purposes mostly
{
 protected:
  DBox box;

 public:
  DBoundingBox();
  DBoundingBox(DBox *box);
  ~DBoundingBox();

  DBox *GetBox(){ return &box; }

  // Primitives
  void SetBox(DBox *box);

  // Painting
  void _Paint();
};

#endif
