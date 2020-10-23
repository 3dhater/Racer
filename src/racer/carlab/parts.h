// parts.h

#ifndef __PARTS_H
#define __PARTS_H

// Max #parts
#define MAX_PART    8

enum Parts
{
  PART_CAR,
  PART_BODY,
  PART_STEER,
  PART_ENGINE,
  PART_GEARS,
  PART_WHEEL,
  PART_SUSP,
  PART_CAM
};

extern cstring partName[MAX_PART];
extern int curPart;

void PartsSetup();
bool PartsEvent(QEvent *e);

#endif
