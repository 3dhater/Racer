/*
 * QGel.cpp - base for graphics elements in a graphics widget
 * 04-10-96: Created!
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/gel.h>
#include <qlib/blitq.h>
#include <qlib/app.h>
#include <qlib/controls.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// To ease the creation of lots of gels, a default group initializer exists
static int defGelGroup=0;

/*******************
* GLOBAL FUNCTIONS *
*******************/
void QGelSetDefaultGroup(int n)
{
  defGelGroup=n;
}

/************
* BASIC GEL *
************/
QBasicGel::QBasicGel(int _flags)
{
  //printf("QBasicGel ctor\n");
  flags=_flags;
  loc.x=loc.y=loc.z=0;
  cv=0;
  group=defGelGroup;
  //printf("  QBasicGel ctor RET\n");
}
QBasicGel::~QBasicGel()
{}

void QBasicGel::Move(int x,int y,int z)
{
  loc.x=x;
  loc.y=y;
  loc.z=z;
}

void QBasicGel::SetX(int x)
{ loc.x=x;
}
void QBasicGel::SetY(int y)
{ loc.y=y;
}
void QBasicGel::SetZ(int z)
{ loc.z=z;
}

void QBasicGel::Paint()
{ int wid,hgt;
  QASSERT_V(cv);
  wid=GetWidth();
  hgt=GetHeight();
  if(wid>0||hgt>0)
  { // Paint gelly thing
  }
}

void QBasicGel::GetRect(QRect *r)
{ r->x=GetX();
  r->y=GetY();
  r->wid=GetWidth();
  r->hgt=GetHeight();
}

/******
* GEL *
******/
QGel::QGel(int _flags)
  : QBasicGel(_flags|QGELF_DBLBUF|QGELF_DIRTY|QGELF_DIRTYBACK)
{ 
  //printf("QGel ctor\n");
  pri=0;
  next=0;
  list=0;
  oldLoc.x=oldLoc.y=oldLoc.z=0;
  oldSize.wid=0; oldSize.hgt=0;
  oldLoc2.x=oldLoc2.y=oldLoc2.z=0;
  oldSize2.wid=0; oldSize2.hgt=0;

  // Default is to connect to the application's bc shell.
  app->GetGelList()->Add(this);
  SetCanvas(app->GetBC()->GetCanvas());
  //printf("  QGel ctor RET\n");
}
QGel::~QGel()
{ // Remove obsolete gel from any list it is in
  if(list)
    list->Remove(this);
}

void QGel::Show()
{
  Disable(QGELF_INVISIBLE);
  Enable(QGELF_DIRTY|QGELF_DIRTYBACK);
}
void QGel::Hide()
{
  if(flags&QGELF_INVISIBLE)return;
  Enable(QGELF_INVISIBLE|QGELF_DIRTY|QGELF_DIRTYBACK);
}
void QGel::MarkDirty()
{
  Enable(QGELF_DIRTY|QGELF_DIRTYBACK);
}
bool QGel::IsHidden()
// Returns TRUE if bob is hidden from both buffers
{ if(flags&QGELF_INVISIBLE)
  { // Only hidden when done processing
    if(flags&(QGELF_DIRTY|QGELF_DIRTYBACK))
      return FALSE;
    return TRUE;
  }
  return FALSE;
}
bool QGel::IsDirty()
{ if(flags&QGELF_DIRTY)
    return TRUE;
  return FALSE;
}

void QGel::UpdatePipeline()
// Pipeline coordinates for multiple buffering
{
  //printf("Update pipeline");
  if(flags&QGELF_DBLBUF)
  { // Remember previous location/size
    //printf(" DBLBUF");
    oldLoc2.x=oldLoc.x;
    oldLoc2.y=oldLoc.y;
    oldLoc2.z=oldLoc.z;
    oldSize2.wid=oldSize.wid;
    oldSize2.hgt=oldSize.hgt;
    // Remember last location
    oldLoc.x=loc.x;
    oldLoc.y=loc.y;
    oldLoc.z=loc.z;
    oldSize.wid=size.wid;
    oldSize.hgt=size.hgt;
  }
  //printf("\n");
}

void QGel::SetPriority(int n)
{ pri=n;
  // Mark sure to resort the gellist when necessary (Paint() etc.)
  if(list)
    list->MarkToSort();
}
void QGel::Paint()
{ if(!(flags&QGELF_INVISIBLE))
    QBasicGel::Paint();
}
void QGel::PaintPart(QRect *r)
{ printf("** QGel::PaintPart NYI!\n");
  QBasicGel::Paint();
}
void QGel::Update(){}

/** General control setting **/
void QGel::SetControl(int ctrl,int v)
{ //printf("QGel::SetCtrl(%d,%d)\n",ctrl,v);
  // Check for any change; if none, then don't set it
  if(GetControl(ctrl)==v)
  { //printf("  no change\n");
    return;
  }
  switch(ctrl)
  { case CONTROL_X: SetX(v); break;
    case CONTROL_Y: SetY(v); break;
    default: return;		// Not supported
  }
  // Mark gel as dirty
  flags|=QGELF_DIRTY|QGELF_DIRTYBACK;
}
int QGel::GetControl(int ctrl)
{ switch(ctrl)
  { case CONTROL_X: return GetX();
    case CONTROL_Y: return GetY();
    default: return 0;		// Not supported
  }
}

/**********
* GELLIST *
**********/
QGelList::QGelList(int _flags)
{
  flags=_flags;
  head=0;
  cv=0;
  bSorted=FALSE;
  //bq=new QBlitQueue();
  // Default is to use the app's bc canvas
  cv=app->GetBC()->GetCanvas();
}
QGelList::~QGelList()
{
}

bool QGelList::IsDirty()
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
  { if(gel->IsDirty())return TRUE;
  }
  return FALSE;
}

void QGelList::SetCanvas(QCanvas *_cv)
{ QGel *gel;
  cv=_cv;
  for(gel=head;gel;gel=gel->next)
  { gel->SetCanvas(cv);
  }
  //bq->SetCanvas(cv);
}

void QGelList::Add(QGel *gel)
{ // Check if gel wasn't inserted already
#ifdef DEBUG
  QGel *g;
  for(g=head;g;g=g->next)
    if(g==gel)
    { qerr("Attempt to insert gel (%s/%p) twice",gel->ClassName(),gel);
      return;
    }
#endif
  if(gel->next)			// Not failsafe
  { qerr("QGelList::Add; %p was already inserted into a list\n",gel);
    return;
  }
  gel->next=head;
  head=gel;
  gel->list=this;		// Connect the gel to the list
}
void QGelList::Remove(QGel *gel)
{ //printf("\007** QGelList::Remove nyi\n");
  //QGel *prev,*g;
  QGel *g;
  //prev=0;
  //qdbg("QGelList:Remove gel %p\n",gel);
  if(gel==head)
  { head=gel->next;
    return;
  }
  //printf("  find gel\n");
  // Gel must be somewhere in the list
  for(g=head;g;g=g->next)
  { //qdbg("  gel %p\n",g);
    //qdbg("  gel->next=%p\n",g->next);
    if(g->next==gel)
    { // Bridge
      //qdbg("  found it; bridge\n");
      g->next=gel->next;
      //qdbg("  return\n");
      return;
    }
  }
  //qdbg("  out of loop\n");
  qerr("QGelList::Remove: gel was not found");
}

void QGelList::UpdatePipeline()
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    gel->UpdatePipeline();
}

void QGelList::Sort()
// Sort the gel list priority-wise
{ QGel *current,*next,*prev;
  bool  goOn;

  if(IsSorted())return;
  if(!head)goto skip_sort;
  /*
  printf("QGelList::Sort: sorting\n");
  for(current=head;current;current=current->next)
    printf(" - %s pri %d\n",current->ClassName(),current->GetPriority());
  */
 do_sort:
  goOn=FALSE;
  prev=0;
  //printf("  sorting batch\n");
  for(current=head;current;)
  { next=current->next;
    if(!next)break;
    if(current->GetPriority()>next->GetPriority())
    { // Swap current & next nodes
      if(!prev)head=next;
      else     prev->next=next;
      current->next=next->next;
      next->next=current;
      prev=next;		// The swapped one
      goOn=TRUE;
#ifdef OBS
    } else if(current->GetPriority()==next->GetPriority())
    { qwarn("Double gel priority %d found!",current->GetPriority());
#endif
    } else
    { prev=current;
      current=current->next;
    }
  }
  if(goOn)goto do_sort;		// If any swaps; sort again

 skip_sort:
  flags|=QGELLISTF_ISSORTED;

//#ifdef DEBUG
  // Check for multiple equal priorities
  if(!head)return;
  int lastPri=head->GetPriority();
  int equal=1;
  for(current=head->next;current;current=current->next)
  { if(current->GetPriority()==lastPri)
    { equal++;
    } else
    { 
#ifdef OBS_EQUAL_PRI_ACCEPTED
      if(equal!=1)
      { qwarn("%d gels found with priority %d",equal,lastPri);
      }
#endif
      equal=1;
      lastPri=current->GetPriority();
    }
  }
#ifdef OBS_EQUAL_PRI_ACCEPTED
  if(equal!=1)
  { qwarn("%d gels found with priority %d",equal,lastPri);
  }
#endif
//printf("  QGelList::Sort RET\n");
}

/***************
* SMART UPDATE *
***************/
//static bool GelsOverlap(QGel *g1,QGel *g2,QRect *r)
static bool GelsOverlap(QGel *g1,QGel *g2,QRect *r,QPoint3 *restLoc=0)
// Returns TRUE if gels g1 and g2 overlap, in which case 'r'
// will contain the overlapping area.
// Gel 'g1' is the restore gel (using the previous location)
// Using divide & conquer; 1 dimension at a time
// Should use negative logic; try to return FALSE (no overlap) quickly
{ int x1,y1,wid1,hgt1;
  int x2,y2,wid2,hgt2;
  //QSize *size;
  //QPoint3 *restLoc;
  // Default location is previous location
  if(!restLoc)
    restLoc=g1->GetOldLocation();
  //printf("g1 old location=%d,%d\n",restLoc->x,restLoc->y);
  x1=restLoc->x; y1=restLoc->y;
  // Get the size of the restore object; largest size of the last 2 frames
  wid1=g1->GetWidth(); hgt1=g1->GetHeight();
  //size=g1->GetOldSize(); wid1=size->wid; hgt1=size->hgt;
  //size=g1->GetOldSize2();
  //if(size->wid>wid1)wid1=size->wid;
  //if(size->hgt>hgt1)hgt1=size->hgt;
  x2=g2->GetX(); y2=g2->GetY(); wid2=g2->GetWidth(); hgt2=g2->GetHeight();
  // Horizontally
  if(x1<x2)r->x=x2; else r->x=x1;
  if(x1+wid1<x2+wid2)r->wid=(x1+wid1)-r->x;
  else               r->wid=(x2+wid2)-r->x;
  // Vertically
  if(y1<y2)r->y=y2; else r->y=y1;
  if(y1+hgt1<y2+hgt2)r->hgt=(y1+hgt1)-r->y;
  else               r->hgt=(y2+hgt2)-r->y;
  if(r->wid>0&&r->hgt>0)
  { // There is an overlapping area
    return TRUE;
  }
  return FALSE;
}
void QGelList::RestoreGel(QGel *rGel)
// The name is a bit off; it should say 'HitGel'
// Repaint gels that are under 'rGel' and marks gels that are above 'rGel'
// as dirty (so they will be repainted)
// gel's location should be restored to the original image
// Method: repaint part of the gels that are behind 'gel' and mark
//         the gels that are in front of 'gel' as dirty.
{ QPoint3 *restLoc;
  QPoint3 curPt;
  QGel    *g;
  QRect   r;
  int     pri;

  restLoc=rGel->GetOldLocation();
  pri=rGel->GetPriority();
  // Current location
  curPt.x=rGel->GetX();
  curPt.y=rGel->GetY();
  curPt.z=0;
  // Find all gels that are behind 'gel'; order is back to front already,
  // so the paint order is ok
  for(g=head;g;g=g->next)
  { if(g==rGel)continue;
    if(g->GetFlags()&QGELF_INVISIBLE)continue;
    if(g->GetPriority()<=pri)
    { // 'g' is behind the restoregel; find any overlapping area
      if(GelsOverlap(rGel,g,&r))
      { /*printf("  gels rGel=%s and g=%s overlap in %d,%d %dx%d; PaintPartly\n",
          rGel->Name(),g->Name(),r.x,r.y,r.wid,r.hgt);*/
        r.x-=g->GetX();
        r.y-=g->GetY();
        //printf("  paintpart %s rGel %s\n",g->Name(),rGel->Name());
        g->PaintPart(&r);
      }
    } else
    { // 'g' is in front of the restore_gel; if it overlaps then repaint
      // Check it is in front of the NEW location
      if(GelsOverlap(rGel,g,&r,&curPt))
      { /*printf("  gels rGel=%s creeps under g=%s; mark dirty\n",
          rGel->Name(),g->Name(),r.x,r.y,r.wid,r.hgt);*/
        g->Enable(QGELF_DIRTY /*|QGELF_DIRTYBACK*/);
        // Gel will be painted later on by this::Update()
      }
      // Check if gel needs repainting because rGel left the area
      if(GelsOverlap(rGel,g,&r))
      { /*printf("gels rGel=%s damaged g=%s by leaving; mark dirty\n",
          rGel->Name(),g->Name(),r.x,r.y,r.wid,r.hgt);*/
        g->Enable(QGELF_DIRTY /*|QGELF_DIRTYBACK*/);
        //g->Enable(QGELF_DIRTY |QGELF_DIRTYBACK);
        // Gel will be painted later on by this::Update()
      }
    }
  }
}

#ifdef OBSOLETE
static void DrawRect(QCanvas *cv,QPoint3 *p,int wid,int hgt)
{ QRect r;
  r.x=p->x; r.y=p->y;
  r.wid=wid; r.hgt=hgt;
  cv->Rectangle(&r);
}
#endif
void QGelList::Update()
// Intelligently update all dirty gels, optimizing blitting regions,
// restoration etc.
{ QGel *gel;
  QRect r;
  int   flags;
  //proftime_t t;

  //printf("QGelList::Update\n");
  //profStart(&t);
  if(!IsSorted())Sort();
  //cv->Enable(QCANVASF_STATAREA);
  //cv->StatReset(QCanvasSTAT_AREA);
  // Make the cv queue all the blits to be optimized
  cv->Enable(QCanvas::OPTBLIT);
  for(gel=head;gel;gel=gel->next)
  { flags=gel->GetFlags();
    //if(flags&(QGELF_DIRTY|QGELF_DIRTYFILTER))
    // If filter is dirty, QGELF_DIRTY will also be set
    if(flags&QGELF_DIRTY)
    { // Restore old location if new gel doesn't overwrite it completely
      // Restoring might not be necessary for gels that don't use blending
      // and that did not move or change size.
#ifdef NOTDEF
      printf(">> dirty gel '%s'",gel->ClassName());
      if(flags&QGELF_DIRTY)printf(" DIRTY");
      if(flags&QGELF_DIRTYFILTER)printf(" DFILTER");
      printf("\n");
#endif
      //gel->PaintPart(new QRect());
      RestoreGel(gel);
      // Paint fresh gel
      //printf("  paint gel %s\n",gel->Name());
      gel->Paint();
      //DrawRect(cv,gel->GetOldLocation(),gel->GetWidth(),gel->GetHeight());
      // Gel is not dirty anymore
      if(flags&QGELF_DIRTYBACK)
      { // Also do the other buffer
        gel->Disable(QGELF_DIRTYBACK);
      } else
      { gel->Disable(QGELF_DIRTY);
      }
    }
  }
  cv->Flush();			// Do all the paints
  cv->Disable(QCanvas::OPTBLIT);	// AFTER the flush please
  //profReport(&t,"QGelList::Update");
  //printf("---- end update\n");
  /*int stat=cv->GetStat(QCanvasSTAT_AREA);
  if(stat)
    printf("QGelList::Update: %d pixels blitted\n",stat);*/
  for(gel=head;gel;gel=gel->next)
    gel->Update();
}

void QGelList::Paint()
{ QGel *gel;
  //printf("QGelList::Paint\n");
  if(cv==0)
  { printf("** cv==0\n"); return; }
  if(!IsSorted())Sort();
  for(gel=head;gel;gel=gel->next)
  { //printf("  gel %p\n",gel);
    gel->Paint();
  }
  //printf("  QGelList::Paint RET\n");
}

/************
* SHOW/HIDE *
************/
void QGelList::HideAll()
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    gel->Hide();
}
void QGelList::Hide(int group)
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    if(gel->GetGroup()==group)
      gel->Hide();
}
void QGelList::ShowAll()
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    gel->Show();
}
void QGelList::Show(int group)
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    if(gel->GetGroup()==group)
      gel->Show();
}

void QGelList::MarkDirty()
// Makes all gels repaint themselves
{ QGel *gel;
  for(gel=head;gel;gel=gel->next)
    gel->MarkDirty();
}

/********
* QUERY *
********/
QGel *QGelList::WhichGelAt(int gx,int gy)
// Find out which gel is on top at mouse location (x,y)
// Notices priority
{ int x,y,wid,hgt;
  QGel *g,*gBest=0;
  for(g=head;g;g=g->next)
  { if(g->GetFlags()&QGELF_INVISIBLE)continue;
    x=g->GetX(); y=g->GetY();
    wid=g->GetWidth(); hgt=g->GetHeight();
    if(gx>=x&&gx<x+wid&&
       gy>=y&&gy<y+hgt)
    { if(gBest==0)
      { gBest=g;
      } else if(gBest->GetPriority()<g->GetPriority())
      { gBest=g;
      }
    }
  }
  return gBest;
}

int QGelList::GetMaxPri()
{ int max;
  QGel *g;
  if(!head)return 0;
  max=head->GetPriority();
  for(g=head;g;g=g->next)
  { if(g->GetPriority()>max)
      max=g->GetPriority();
  }
  return max;
}

int QGelList::GetMinPri()
{ int min;
  QGel *g;
  if(!head)return 0;
  min=head->GetPriority();
  for(g=head;g;g=g->next)
  { if(g->GetPriority()<min)
      min=g->GetPriority();
  }
  return min;
}

