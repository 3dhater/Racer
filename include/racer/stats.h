// racer/stats.h

#ifndef __RACER_STATS_H
#define __RACER_STATS_H

#ifndef __QLIB_QINFO_H
#include <qlib/info.h>
#endif

class RCar;

class RStats
// Keep statistics on best laps
{
  // Statistics
 public:
  enum Max
  {
    MAX_BEST_LAP=10
  };
  enum Flags
  {
    PRACTICE=1                  // Lap done in practice session
  };

 public:
  // Best laps
  int lapTime[MAX_BEST_LAP];
  int date[MAX_BEST_LAP];       // Seconds since 1-1-1970
  qstring car[MAX_BEST_LAP];    // Driven in which car
  int flags[MAX_BEST_LAP];      // Practice?

  bool fDirty;

 public:
  RStats();
  ~RStats();

  bool Load(QInfo *info,cstring path);
  bool Save(QInfo *info,cstring path);

  // Setting new statistics
  void CheckTime(int lapTime,RCar *car);
  void Update(QInfo *info,cstring path);
};

#endif
