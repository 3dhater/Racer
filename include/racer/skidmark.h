// racer/skidmark.h

#ifndef __RACER_SKIDMARK_H
#define __RACER_SKIDMARK_H

class RSkidMarkManager
{
 public:
  enum Max
  {
    MAX_STRIP=100           // Max. number of separate strips
  };

 protected:
  int maxPoint,
      maxStrip;
  //DVector3 *point[MAX_STRIP];     // Array of key points
  DVector3  stripLastV[MAX_STRIP];
  int       stripStart[MAX_STRIP],
            stripSize[MAX_STRIP];
  float    *stripV[MAX_STRIP];
  char      stripInUse[MAX_STRIP];  // Being built?
  int       strips;
  int       nextStrip;
  DTexture *texture;

 public:
  RSkidMarkManager();
  ~RSkidMarkManager();

  void Clear();
  int  StartStrip();
  void StartStripForced(int stripNr);
  int  AddPoint(int strip,DVector3 *v);
  void EndStrip(int strip);
  void EndAllStrips();

  void Paint();
};

#endif

