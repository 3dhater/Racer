// racer/driver.h

#ifndef __RACER_DRIVER_H
#define __RACER_DRIVER_H

#ifndef __QLIB_QINFO_H
#include <qlib/info.h>
#endif
#ifndef __RACER_STATS_H
#include <racer/stats.h>
#endif

class RCar;

class RDriver
{
 protected:
  qstring dirName;              // Directory
  qstring name,                 // Real name
          country;
  int     age;
  QInfo  *info;                 // Detail file
  qstring trackName;            // Which track is current?

  // Statistics
  RStats stats;

 public:
  RDriver(cstring name);
  ~RDriver();

  // Attribs
  RStats *GetStats(){ return &stats; }
  cstring GetTrackName(){ return trackName; }
  void SetTrackName(cstring tName);

  bool Load();
  bool Save();

  // Setting new statistics
  void CheckTime(int lapTime,RCar *car);
  void Update();
};

#endif
