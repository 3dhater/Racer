// qlib/pres/pres.h - Presentation (scrolling etc)

#ifndef __QLIB_PRES_PRES_H
#define __QLIB_PRES_PRES_H

#include <qlib/file.h>
#include <qlib/bob.h>
#include <qlib/text.h>
#include <qlib/shape.h>

// Presentation class tree:
// QPresentation
// -> QPPage list
//    -> QPGroup
//       ->QPGel (QPImage/QPText)

// Effect types
//#define QPET_CUT	0	// Hard cut
//#define QPET_WIPE_UD	1	// Wipe from top to bottom
//#define QPET_WIPE_DU	2
//#define QPET_WIPE_LR	3
//#define QPET_WIPE_RL	4
// Effect types may differ for pages and groups

struct QPEffect
// Generic effect description (Q Presentation)
{ int type;			// Effect type
  int speed;			// Yes, speed
  int param[6];			// Extensions
  int timeStart;		// Relative time to start effect
};

// Font description
#define QPF_MAXFAMILYCHAR	40
#define QPFF_BOLD		1	// Font flags (slants etc)
#define QPFF_ITALIC		2
#define QPFF_UNDERLINE		4

struct QPFont
// Complete font description; mimics ShowCase parameters
{ char family[QPF_MAXFAMILYCHAR];
  int  size;
  int  flags;
};

// Style description (applies to groups)

#define QPSF_SHADOW		1	// Style flags; use shadow
#define QPSF_OUTLINE		2	// Outline the gels
#define QPSF_xxx

struct QPColor
{ long   rgba;				// 32-bit color
  long   rgbaGradient;			// In case of a gradient
};

struct QPStyle
// A gel style (NOT the font, NOT the effects)
{
  int     flags;			// QPSF_xxx
  QPoint  shadowOff;			// Location of shadow
  int     outlineDepth;			// Number of outlines
  QPColor shadowColor;
  QPColor outlineColor;
  QPColor faceColor;
};

//
// Gels
//

class QPGroup;

class QPGel : public QObject
{
 protected:
  int x,y;
 public:
  QPGel *next;
  //QPGroup *group;		// To which group
  // Filters on gel
  QFilterBrightness *fb;
  QFilterFade       *ff;

 public:
  QPGel();
  ~QPGel();

  //QPGroup *GetGroup(){ return group; }
  //void SetGroup(QPGroup *g);

  void GetRect(QRect *r);
  virtual int GetWidth();
  virtual int GetHeight();
  int GetX(){ return x; }
  int GetY(){ return y; }
 
  virtual void Hide();
  virtual void Show();

  virtual void Move(int x,int y);
  // Info for presentation to move bobs/texts
  virtual QBob *GetBob();

  virtual bool Read(QFile *f);
  virtual bool Write(QFile *f);
};

class QPImage : public QPGel
{
 protected:
  QBob *bob;
  QImage *img;
 public:
  QPImage(QImage *img);
  ~QPImage();
  QBob *GetBob();
  void Hide();
  void Show();
  bool Read(QFile *f);
  bool Write(QFile *f);
};

class QPText : public QPGel
// Mostly just a single char
{
 public:
  QText *text;
  QFont *font;

 public:
  QPText(string text,QFont *font=0);
  ~QPText();

  QBob *GetBob();
  QFont *GetFont(){ return font; }

  void GetRect(QRect *r);
  int GetWidth();
  int GetHeight();
  void Move(int x,int y);

  void Hide();
  void Show();

  bool Read(QFile *f);
  bool Write(QFile *f);
};

/********
* GROUP *
********/

// QPGroup effects
#define QPGE_CUT	0
#define QPGE_FLASHIN	1		// Flash per gel
#define QPGE_FLYLR	2		// Flight of the bumblebees
#define QPGE_FLYRL	3
#define QPGE_FLYTB	4
#define QPGE_FLYBT	5

class QPGroup : public QObject
{
 protected:
  int id;
 public:
  QPGel *gelList;
  QPGroup *next;
  int    flags;
  int    centerX;
  const int CENTERH=1;
  QPEffect effIn,effOut;	// Effect after page comes in, before page out
  QPStyle  style;

 public:
  QPGroup(int id=0);
  ~QPGroup();

  int GetID(){ return id; }
  int GetFlags(){ return flags; }
  void SetFlags(int flags);

  bool IsCentered();
  void GetRect(QRect *r);	// Bounding box of all PGels

  void Hide();
  void Show();

  bool AddGel(QPGel *gel);
  bool RemoveGel(QPGel *gel);
  QPGel *FindGelByXlt(int x);	// To the left
  QPGel *FindGelByXgt(int x);	// To the right (greater than X)

  void CenterHor(int mx);

  bool Read(QFile *f);
  bool Write(QFile *f);
};

/********
* PAGES *
********/
#define QPPT_STATIC	0		// Static page
#define QPPT_SCROLL	1		// Vert. scroll
#define QPPT_CRAWL	2		// Hor.

#define QPPD_LEFT	0		// Direction
#define QPPD_UP		0
#define QPPD_RIGHT	1
#define QPPD_DOWN	1

// Page flip effects (see also the README file)
#define QPE_CUT		0
#define QPE_WIPE_LR	1		// Wipes Left to Right
#define QPE_WIPE_RL	2
#define QPE_WIPE_TB	3
#define QPE_WIPE_BT	4		// Bottom to top
#define QPE_SCROLL_LR	5		// Scrolling
#define QPE_SCROLL_RL	6
#define QPE_SCROLL_TB	7
#define QPE_SCROLL_BT	8
#define QPE_xxx

class QPPage : public QObject
{
 protected:
  int pageNr;
  int offX,offY;
  string bgr;				// Bgr filename
  int type,dir;				// Static/scroll, up/down/lt/rt?
  int speed;				// Scroll/crawl speed
 public:
  QPEffect effIn,effOut;
  bool cueIn,cueOut;			// Cue in/out effects?

 public:
  QPPage *next;
  QPGroup *groupList;

 public:
  QPPage(int pageNr);
  ~QPPage();

  void Hide();
  void Show();

  void SetType(int type);
  int  GetType(){ return type; }

  void SetDir(int dir);
  int  GetDir(){ return dir; }

  void SetSpeed(int spd);
  int  GetSpeed(){ return speed; }

  int GetPageNr(){ return pageNr; }
  bool AddGroup(QPGroup *grp);
  QPGroup *FindGroup(int x,int y);	// Mouse loc
  QPGroup *FindGroupByYlt(int x);
  QPGroup *FindGroupByYgt(int x);

  void SetBackgroundName(string fname);
  string GetBackgroundName(){ return bgr; }

  bool Read(QFile *f);
  bool Write(QFile *f);
};

//
// Presentation
//
class QPresentation : public QObject
{
 public:
  QPPage *pageList;
  // Editing
  //QShape *pgCursor;
  //QPPage *curPage;

 public:
  QPresentation(string name);
  ~QPresentation();

  int GetMaxPageNr();
  //QPPage *GetCurPage(){ return curPage; }
  //void SetCurPage(QPPage *pg){ curPage=pg; }
  QPPage *GetPage(int pageNr);

  bool PageExists(int nr);

  bool AddPage(QPPage *page);
  void FreeAll();

  bool ProcessEvent(QEvent *e);
  void ShowPage(QPPage *page);
  void SlideShow(int fromPage,int toPage);

  bool Read(QFile *f);
  bool Write(QFile *f);
};

#endif
