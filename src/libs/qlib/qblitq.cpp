/*
 * QBlitQ - a queue for multiple blits; optimizing blits
 * 19-04-97: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/canvas.h>
#include <qlib/blitq.h>
#include <stdio.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define QUEUESIZE   500

QBlitQueue::QBlitQueue(QCanvas *icv)
{ cv=icv;
  queueSize=QUEUESIZE;
  q=(QBBlit*)qcalloc(queueSize*sizeof(QBBlit));
  count=0;
}
QBlitQueue::~QBlitQueue()
{
  if(q)qfree((void*)q);
}

bool QBlitQueue::AddBlit(QBitMap *sbm,int sx,int sy,int wid,int hgt,
  int dx,int dy,int flags)
// Adds a blit to the queue
// Never fails (queue is flushed in case of overflow)
{ QBBlit *b;
  // In case of overflow, execute the current list, so it is empty again
  // (not that this is good optimizing this way!)
  if(count>=queueSize)
  { qwarn("QBlitQueue: overflow; executing current list");
    Execute();
  }
  b=&q[count];
  b->sbm=sbm;
  b->sx=sx; b->sy=sy;
  b->wid=wid; b->hgt=hgt;
  b->dx=dx; b->dy=dy;
  b->flags=flags;
  count++;
  return TRUE;
}

void QBlitQueue::SetGC(QCanvas *ncv)
{
  cv=ncv;
}

QBBlit *QBlitQueue::GetQBlit(int n)
// Used by QCanvas to optimized blitting a queue internally
{ if(n<0||n>=count)return 0;
  return &q[n];
}

static void OutQBB(QBBlit *b)
{
  qdbg("  sbm=%8p, src %3d,%3d size %3dx%3d, dst %3d,%3d, flags=%d\n",
    b->sbm,b->sx,b->sy,b->wid,b->hgt,b->dx,b->dy,b->flags);
}
int QBlitQueue::DebugOut()
{ int i;
  int area=0;
  //qdbg("QBlitQueue (%p)\n",this);
  for(i=0;i<count;i++)
  { if(q[i].flags&QBBF_OBSOLETE)continue;
    //qdbg("%4d: ",i); OutQBB(&q[i]);
    area+=q[i].wid*q[i].hgt;
  }
  //qdbg("---------- total area size: %7ld\n",area);
  return area;
}

void QBlitQueue::Optimize()
// Try to optimize the blits by reasoning out obsolete ones, merging
// attached blits, and merging large and small overlapping blits, etc.
{ QBBlit *b,*bEarlier;
  int i,j,diff;
  
#ifdef ND_TEMP
  if(count<=1)return;
  b=&q[count-1];
  // Work through the blits in the order later to earlier.
  // This way, a single blit may in 1 pass rule out several earlier blits.
  // This looks faster than checking blits from earlier to later.
  for(i=0;i<count;i++,b--)
  { // Don't check when the blit was already obsolete
    // or the blit uses alpha (in which case it always needs something
    // behind it).
    if(b->flags&(QBBF_OBSOLETE|QBBF_BLEND))continue;
    //qdbg("Compare "); OutQBB(b);
    bEarlier=q;
    for(j=i;j<count-1;j++,bEarlier++)
    { if(bEarlier->flags&QBBF_OBSOLETE)continue;
      //qdbg("to      "); OutQBB(bEarlier);
      // Check overlapping destination
      if(b->dx==bEarlier->dx&&b->dy==bEarlier->dy)
      { if(b->wid>=bEarlier->wid)
        { if(b->hgt>=bEarlier->hgt)
          { // We can rule out the earlier blit entirely
            bEarlier->flags|=QBBF_OBSOLETE;
            //qdbg("(earlier blit is obsolete; same dst & size overlap)\n");
          } else
          { // We can strip some height of the earlier blit
            diff=bEarlier->hgt-b->hgt;
            //qdbg("(strip height of earlier blit; %d pixels)\n",diff);
            bEarlier->sy+=b->hgt;
            bEarlier->dy+=b->hgt;
            bEarlier->hgt=diff;
          }
        }
#ifdef NOTDEF_BUGGY
          else if(b->sbm==bEarlier->sbm&&
                  b->sx==bEarlier->sx&&b->sy==bEarlier->sy)
        { // The same image will be plotted, albeit not as big as the earlier
          // blit; rule OURSELVES out.
          printf("(done as part of earlier blit)\n");
          b->flags|=QBBF_OBSOLETE;
          // Do not continue?
          break;
        }
#endif
      }
    }
  }
#endif
}

void QBlitQueue::Execute()
{ //QBBlit *b;
  //int i;
  //int aBefore,aAfter;          // Area optimization
  if(!count)return;

  if(cv)cv->BlitQueue(this);
#ifdef OBSOLETE
  //qdbg("QBlitQ: Execute\n");
  aBefore=DebugOut();
  Optimize();
  aAfter=DebugOut();
  qdbg("  Optimized from %ld to %ld pixels =%ld%%\n",aBefore,aAfter,
    (aBefore-aAfter)*100/aBefore);
  // Execute each blit
  cv->Disable(QCANVASF_OPTBLIT);        // Really blit now
  for(i=0;i<count;i++)
  { b=&q[i];
    if(b->flags&QBBF_OBSOLETE)continue;
    if(b->flags&QBBF_BLEND)cv->Enable(QCANVASF_BLEND);
    else                   cv->Disable(QCANVASF_BLEND);
    cv->Blit(b->sbm,b->dx,b->dy,b->wid,b->hgt,b->sx,b->sy);
  }
  count=0;
#endif
}

//#define TEST
#ifdef TEST

void main()
{ QBlitQueue *bq;
  QBitMap *bm[4];
  int i;
  for(i=0;i<4;i++)bm[i]=(QBitMap*)(i+1);
  bq=new QBlitQueue();
#ifdef NOTDEF
  // Lingo situation
  bq->AddBlit(bm[0],250,500,51,52,250,500,0);   // Rest bgr
  bq->AddBlit(bm[1],50,60,51,52,250,500,0);     // Board
  bq->AddBlit(bm[2],0,0,51,52,250,460,1);       // New letter position
#endif
//#ifdef NOTDEF
  // Situation (Lingo) where lots of letters/hilites are suddenly used
  // and get restored at their previous location, which is at first (0,0)
  bq->AddBlit(bm[0],0,0,51,52,0,0);
  bq->AddBlit(bm[0],0,0,51,52,0,0);
  bq->AddBlit(bm[0],0,0,51,52,0,0);
  bq->AddBlit(bm[0],0,0,51,52,0,0);
  bq->AddBlit(bm[0],0,0,38,42,0,0);             // Restore other sized thing
  bq->AddBlit(bm[0],0,0,38,42,0,0);
  //bq->AddBlit(bm[0],0,0,51,52,0,0);         // Overrule last 2
  bq->AddBlit(bm[2],0,0,51,52,200,400);         // Hilite under letter
  bq->AddBlit(bm[1],0,0,51,52,200,400,1);         // New letters
  bq->AddBlit(bm[1],0,0,51,52,251,400,1);
//#endif
#ifdef NOTDEF
  // A letter half on the background, half on the board
  bq->AddBlit(bm[0],250,400,38,42,250,400);     // Bgr restore
  bq->AddBlit(bm[1],50,100,38,20,250,400);      // Board restore part
  bq->AddBlit(bm[2],0,0,250,400,38,42,1);       // Letter
#endif
  bq->Execute();
}
#endif

