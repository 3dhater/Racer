// racer/time.h

#ifndef __RACER_TIME_H
#define __RACER_TIME_H

class RTime
// A notion of simulation time
{
 protected:
  rfloat span;		// Time of integration (in seconds)
  int    spanMS;        // Time of integration in milliseconds
  QTimer *tmr;		// Actual real timer
  int curRealTime;	// Last recorded REAL time in msecs
  int curSimTime;	// Time calculated in the sim
  int lastSimTime;      // Last point of simulation

 public:
  RTime();
  ~RTime();

  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  // Attribs
  int  GetRealTime(){ return curRealTime; }
  int  GetSimTime(){ return curSimTime; }
  int  GetLastSimTime(){ return lastSimTime; }
  int  GetSpanMS(){ return spanMS; }
  inline rfloat GetSpan(){ return span; }

  void AddSimTime(int msecs);
  void SetLastSimTime(){ lastSimTime=curSimTime; }
  void SetSpan(int ms);

  // Methods
  void Start();
  void Stop();
  void Reset();
  void Update();
};

#endif
