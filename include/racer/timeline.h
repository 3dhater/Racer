// racer/timeline.h

#ifndef __RACER_TIMELINE_H
#define __RACER_TIMELINE_H

class RTimeLine
// A line which marks the time when driven through
{
 public:
  DVector3 from,to;          // Start, end point of line
  DPlane3 *plane;            // A plane going through 'from' and 'to'
                             // The plane is directed right up/down (along Y)
  rfloat   distanceSquared;  // From 'from' to 'to'

 public:
  RTimeLine(const DVector3 *from,const DVector3 *to);
  ~RTimeLine();

  // Attribs
  const DVector3 *GetFrom(){ return &from; }
  const DVector3 *GetTo(){ return &to; }
  const DPlane3  *GetPlane(){ return plane; }

  void Define(const DVector3 *from,const DVector3 *to);
  bool CrossesPlane(const DVector3 *v1,const DVector3 *v2) const;
};

#endif
