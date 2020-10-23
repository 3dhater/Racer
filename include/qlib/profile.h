/*
 * Profiling support
 * 30-12-96: Created!
 * 31-12-96: Now using SGI's hardware counter; no link with dmedia req'd
 */

#ifndef __QLIB_PROFILE_H
#define __QLIB_PROFILE_H

#include <qlib/timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USE_SYSSGI		// Define to use syssgi system counter
//#define USE_UST               // Define to use dmedia nanoseconds timer

//#define IO4_TIMER_IS_64BIT    // Define for Challenge/ONYX family (64-bit cnt)

#ifdef USE_SYSSGI
typedef unsigned int proftime_t;
#else
// UST
typedef uint64 proftime_t;
#endif

void profStart(proftime_t *t);
void profReport(proftime_t *t,char *s);
void profReport0(proftime_t *t,char *s);	// Restarts timer

#ifdef __cplusplus
}
#endif

// Profiling classes

class QProfileSection;

class QProfiler
{
 public:
  enum Max
  {
    MAX_SECTION=100
  };
  enum Flags
  {
  };

  QTimer *tmr;
  QProfileSection *section[MAX_SECTION];
  int sections;
  // Runtime tracking of call depth
  int curDepth;

 public:
  QProfiler(){ sections=0; curDepth=0; tmr=new QTimer(); }
  ~QProfiler(){ if(tmr)delete tmr; }

  void Reset();
  void Start();
  void Stop();
  void Add(QProfileSection *section);

  void Report(int flags=0);
};

extern QProfiler qprofiler;

class QProfileSection
{
 public:
  int sectionNr;
  int count;
  int enterTime,totalTime;
  int totalDepth;
  cstring name;

 public:
  QProfileSection(cstring s)
  {
    qprofiler.Add(this);
    count=0;
    totalTime=0;
    totalDepth=0;
    name=s;
  }
  ~QProfileSection(){}

  void Enter();
  void Leave();
};
class QProfileHit
{
  QProfileSection *section;
 public:
  QProfileHit(QProfileSection *_section)
  { section=_section;
    section->Enter();
  }
  ~QProfileHit()
  { section->Leave();
  }
};

#ifdef Q_ENABLE_PROFILING
// Static section for improved performance, and an auto object for timing
//#define QPROF(s) static QProfileSection __qprfs(s);
#define QPROF(s) static QProfileSection __qps(s);QProfileHit __qphit(&__qps);
#else
// Don't do anything
#define QPROF(s)
#endif

#endif

