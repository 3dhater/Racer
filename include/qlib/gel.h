// qlib/qgel.h - anything that is painted in a window

#ifndef __QLIB_QGEL_H
#define __QLIB_QGEL_H

#include <qlib/object.h>
#include <qlib/point.h>
#include <qlib/canvas.h>
#include <qlib/control.h>

// Gel flags; flags for Bob and Texts are also included
#define QGELF_DBLBUF	1	// Double buffered gel
#define QGELF_INVISIBLE	2	// Hide
#define QGELF_BLEND	4	// Use alpha
#define QGELF_DIRTY	8	// Gel needs repainting
#define QGELF_DIRTYBACK	16	// Other buffer is still dirty
#define QGELF_AA	32	// Anti-alias (texts only currently)
#define QGELF_DIRTYSCALE 64	// Scaling needs reworking
#define QGELF_DIRTYFILTER 128	// Filters need to (re)process bitmap
#define QGELF_INDPAINT	256	// Gel is indirectly painted by front gel
// ^^ INDPAINT is used with movies; since movies must be played fast,
//    and to be able to get a non-linear frame around the movie, this
//    flags enables the feature that a movie is framed by a bob, so the
//    restoration of the frame gel will paint the movie, but the (dirty)
//    movie gel is NOT painted in itself.
//    Call QMovieBob::SetFramed(TRUE) to enable this optimization for
//    moviebobs.

class QGelList;
class QGelControl;

class QBasicGel : public QObject
// Gel without knowledge of other gels surrounding him
// Not used much.
{
 public:
  string ClassName(){ return "basicgel"; }
 protected:
  QPoint3 loc;
  QSize   size;
  int     flags;
  int     group;		// Gels are divided into groups
  //QGC    *gc;			// Current graphics context
  QCanvas *cv;			// Where it lives

 public:
  QBasicGel(int flags=QGELF_DBLBUF);
  ~QBasicGel();

  // Attributes
  //bool IsDirty(){ if(flags&QGELF_DIRTY)return TRUE; return FALSE; }
  virtual void Move(int x,int y,int z=1);
  void Move(QPoint *l){ Move(l->x,l->y); }

  void Enable(int flag){ flags|=flag; }
  void Disable(int flag){ flags&=~flag; }

  void SetX(int x);
  void SetY(int y);
  void SetZ(int z);
  int GetX(){ return loc.x; }
  int GetY(){ return loc.y; }
  int GetZ(){ return loc.z; }
  QPoint3 *GetLocPtr(){ return &loc; }
  virtual int GetWidth(){ return size.wid; }
  virtual int GetHeight(){ return size.hgt; }
  virtual int GetDepth(){ return 0; }
  void GetRect(QRect *r);
  int  GetFlags(){ return flags; }
  int  GetGroup(){ return group; }
  void SetGroup(int n){ group=n; }

  // Painting
  //void SetGC(QGC *_gc){ gc=_gc; }
  //QGC *GetGC(){ return gc; }
  void SetCanvas(QCanvas *ncv){ cv=ncv; }
  QCanvas *GetCanvas(){ return cv; }

  virtual void Paint();
  virtual void PaintFast(){ Paint(); }
};

class QGel : public QBasicGel
// Smart gel, with layering features (priorities)
// Can paint just a part of itself (optimized repaints)
// Takes into account other gels (in a GelManager)
// Supports double buffering.
{
 public:
  string ClassName(){ return "gel"; }
 protected:
  int     pri;			// Gel layering
  QPoint3 oldLoc;		// Previous buffer
  QSize   oldSize;		// Its size at that time
  QPoint3 oldLoc2;		// Location 2 frames back
  QSize   oldSize2;
 public:
  QGelList *list;		// To which list do we belong
  QGel     *next;		// Linked list
  // Controls
  QControl *x,*y,*z;		// Dynamically controllable gel features
  QControl *wid,*hgt;

 public:
  QGel(int flags=0);
  ~QGel();

  void Hide();
  void Show();
  void MarkDirty();		// Mark for repaint
  bool IsHidden();
  bool IsDirty();

  // Attributes
  void SetPriority(int n);
  int  GetPriority(){ return pri; }
  virtual void SetControl(int ctrl,int v);
  virtual int  GetControl(int ctrl);

  // Double buffering
  void UpdatePipeline();
  virtual void Update();		// Update any gel-specifics
  QPoint3 *GetLocation(){ return &oldLoc; }
  QPoint3 *GetOldLocation(){ return &oldLoc2; }
  // Sizes last frame and 2 frames back
  QSize   *GetOldSize(){ return &oldSize; }
  QSize   *GetOldSize2(){ return &oldSize2; }

  // Painting
  void Paint();
  virtual void PaintPart(QRect *r);
};

//
// GelManager - does priority handling, painting etc.
//
const int QGELLISTF_ISSORTED=1;	// List is pri-sorted

//class QBlitQueue;

class QGelList : public QObject
{
 protected:
  int   flags;
  QGel *head;
  //QGC  *gc;
  QCanvas *cv;
  bool  bSorted;
  //QBlitQueue *bq;			// Blit optimizer

 protected:
  void RestoreGel(QGel *gel);

 public:
  QGelList(int flags=0);
  ~QGelList();

  void SetCanvas(QCanvas *ncv);
  QCanvas *GetCanvas(){ return cv; }
  //void SetGC(QGC *gc);
  //QGC *GetGC(){ return gc; }

  void Add(QGel *gel);
  void Remove(QGel *gel);

  bool IsSorted()
  { if(flags&QGELLISTF_ISSORTED)return TRUE;
    else return FALSE;
  }
  bool IsDirty();
  void MarkToSort(){ flags&=~QGELLISTF_ISSORTED; }
  void Sort();

  void UpdatePipeline();
  // Update all dirty gels; optimize it
  void Update();
  // Paint all gels, regardless dirty state
  void Paint();

  // Show and hide entire groups
  void HideAll();
  void ShowAll();
  void Show(int group=0);
  void Hide(int group=0);

  // Querying
  QGel *WhichGelAt(int x,int y);
  int   GetMaxPri();			// Top pri
  int   GetMinPri();

  void MarkDirty();
};

// Global functions
void QGelSetDefaultGroup(int n);	// Default create group

#endif

#ifdef OBSOLETE
//
// Gel list - also does priority handling, painting etc.
//
const QGELLISTF_ISSORTED=1;	// List is pri-sorted

class QGelList : public QObject
{
 protected:
  int   flags;
  QGel *head;
  QGC  *gc;
  bool  bSorted;

 protected:
  void RestoreGel(QGel *gel);

 public:
  QGelList(int flags=0);
  ~QGelList();

  void SetGC(QGC *gc);
  QGC *GetGC(){ return gc; }

  void Add(QGel *gel);
  void Remove(QGel *gel);

  bool IsSorted()
  { if(flags&QGELLISTF_ISSORTED)return TRUE;
    else return FALSE;
  }
  void MarkToSort(){ flags&=~QGELLISTF_ISSORTED; }
  void Sort();

  void UpdatePipeline();
  // Update all dirty gels; optimize it
  void Update();
  // Paint all gels, regardless dirty state
  void Paint();

};


#endif
