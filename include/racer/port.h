// racer/port.h - Rendering ports

#ifndef __RACER_PORT_H
#define __RACER_PORT_H

#include <qlib/types.h>
#include <d3/culler.h>

class RPort
{
 protected:
  QRect rect;                        // Location and size of port
  DCullerSphereList *culler;         // Points to an external culler

 public:
  RPort();
  ~RPort();

  // Attribs
  DCullerSphereList *GetCuller(){ return culler; }
  QRect             *GetRect(){ return &rect; }
  void SetCuller(DCullerSphereList *culler);

  void Paint();
};

#endif
