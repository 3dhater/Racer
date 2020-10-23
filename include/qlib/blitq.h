// qlib/qblitq.h

#ifndef __QLIB_QBLITQ_H
#define __QLIB_QBLITQ_H

#include <qlib/object.h>

class QCanvas;
class QBitMap;

#define QBBF_BLEND  1   // QBBlit flags
#define QBBF_OBSOLETE 2 // Blit not needed anymore

struct QBBlit
{
  QBitMap *sbm;         // Source bitmap
  int      sx,sy,       // Source location
           wid,hgt;     // Size
  int      dx,dy;       // Destination location
  int      flags;       // QBBF_xxx (BLEND etc.)
};

class QBlitQueue //: public QObject
// A blit queue for blits to a specific cv;
// when executed, an attempt is made to optimize the blits
// (such as omitting blits which are not necessary)
{
 protected:
  QCanvas *cv;          // Canvas for actual blits
  QBBlit *q;            // The queue
  int    queueSize;
  int    count;         // Items in queue

 public:
  QBlitQueue(QCanvas *cv=0);
  ~QBlitQueue();

  void SetGC(QCanvas *cv);

  bool AddBlit(QBitMap *sbm,int sx,int sy,int wid,int hgt,int dx,int dy,
    int flags=0);

  int     GetCount(){ return count; }
  void    SetCount(int n){ count=n; }		// For QCanvas use only!
  QBBlit *GetQBlit(int n);

  void Optimize();
  int  DebugOut();

  void Execute();
};

#endif

