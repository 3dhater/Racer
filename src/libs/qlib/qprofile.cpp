/*
 * QProfile - profiling classes
 * 12-07-2001: Created! (13:53:06)
 * NOTES:
 * USAGE:
 * --- example.cpp
 *
 *   QProfiler qprofiler;
 *   void f()
 *   {
 *     QPROF("f")
 *     for(int i=0;i<100000;i++);
 *   }
 *   void main()
 *   {
 *     qprofiler.Start();
 *     f();
 *     qprofiler.Report();
 *   }
 *
 * (C) MarketGraph/RvG
 */

#include <qlib/profile.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QProfiler"

/*********
* Adding *
*********/
void QProfiler::Add(QProfileSection *sect)
{
  if(sections==MAX_SECTION)
  {
    qwarn("QProfile:Add(); can't add any more sections");
    return;
  }
  section[sections]=sect;
  sections++;
}

/*************
* Start/stop *
*************/
void QProfiler::Reset()
{
  tmr->Reset();
}
void QProfiler::Start()
{
  tmr->Start();
}
void QProfiler::Stop()
{
  tmr->Stop();
}

/*********
* Report *
*********/
void QProfiler::Report(int flags)
{
  int i;
  QProfileSection *s;
  int avg;
  float avgDepth;
  
  qdbg("QProfiler report (times in ms):\n");
  qdbg("Section                   Count      Total time    Avg time "
    "  Avg depth\n");
  qdbg("------------------------  --------   ----------    ---------"
    "  ---------\n");
  for(i=0;i<sections;i++)
  {
    s=section[i];
    if(s->count)
    {
      avg=s->totalTime/s->count;
      avgDepth=((float)s->totalDepth)/s->count;
    } else
    {
      avg=0;
      avgDepth=0;
    }
    qdbg("%3d %*s",i+1,(int)avgDepth," ");
    qdbg("%-*s  %8d   %10d   %10d  %3.1f\n",
      20-((int)avgDepth),s->name,s->count,
      s->totalTime/1000,avg/1000,avgDepth);
  }
}

/************************
* QProfileSection class *
************************/
#undef  DBG_CLASS
#define DBG_CLASS "QProfileSection"

void QProfileSection::Enter()
{
  // Track calling depths amongst profiling sections
  qprofiler.curDepth++;
  totalDepth+=qprofiler.curDepth;
  
  // Record time
  enterTime=qprofiler.tmr->GetMicroSeconds();
  // We've been here!
  count++;
}
void QProfileSection::Leave()
{
  // Record time that we spent here
  totalTime+=qprofiler.tmr->GetMicroSeconds()-enterTime;
  
  // Returning a level
  qprofiler.curDepth--;
}
