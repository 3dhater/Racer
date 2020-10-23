// qlib/qvideofx.h - effects on a widget's graphics contents

#ifndef __QLIB_QVIDEOFX_H
#define __QLIB_QVIDEOFX_H

#include <qlib/fx.h>
#include <qlib/canvas.h>
#include <qlib/rect.h>
#include <qlib/point.h>

// Onscreen effects
#define QVFXT_CUT	0		// QVideoFX types: hard cut
#define QVFXT_WIPE_LR	1		// Wipe left to right
#define QVFXT_WIPE_RL	2		// Wipe right to left
#define QVFXT_WIPE_TB	3
#define QVFXT_WIPE_UD	3		// Synonym from the Amiga
#define QVFXT_WIPE_BT	4
#define QVFXT_WIPE_DU	4		// Synonym from the Amiga
#define QVFXT_GROW	5		// Inside out
#define QVFXT_CROSS	6		// 24-bit cross
#define QVFXT_SCROLL_LR	7		// Scroll new contents into rect
#define QVFXT_SCROLL_RL	8		// NYI
#define QVFXT_SCROLLPUSH_LR	9	// Pushing scroll
#define QVFXT_SCROLLPUSH_RL	10
#define QVFXT_SCROLLPUSH_TB	11
#define QVFXT_SCROLLPUSH_BT	12	// Like NOS tekst-TV

// Free flight effects
const int QVFXT_FLY_BT=1000;		// Fly from bottom to top
const int QVFXT_EXPLODE=1001;		// Burst out

// Delay types for Free FX (particle starts)
const int QVFXDT_LINEAR=0;		// Delay types for free fx
const int QVFXDT_SWEEP_LR_TB=1;		// Left to right, top to bottom
const int QVFXDT_SWEEP_RL_TB=2;		// Right to left buildup

const int QVPF_EQUALIZE=1;		// Equalize particle
const int QVPF_DONE=2;			// Particle done

// Dynamic array
#define PARTICLE_X(n)	(n%xParticles)
#define PARTICLE_Y(n)	(n/xParticles)
#define PARTICLES	(xParticles*yParticles)

struct QVParticle
{ int x,y,              // Dest location (onscreen)
      vx,vy;            // Speed
  int ax,ay;            // Acceleration (ay=gravity)
  int x0,y0;            // Start location
  int xn,yn;            // Eventual location (onscreen)
  int sx,sy;            // Source location (bitmap)
  int wid,hgt;          // Size of particle
  int frame,delay;      // Where we are
  int frames;
  int flags;
};

class QVideoFX : public QFX
{
 public:
  string ClassName(){ return "fx"; }
  int FXType(){ return QFXT_VIDEO; }

 private:
  void DoCut();
  void DoWipeLR();
  void DoWipeRL();
  void DoWipeTB();
  void DoWipeBT();
  void DoGrow();
  void DoCross();
  void DoScrollLR();
  void DoScrollRL();
  void DoScrollPushLR();
  void DoScrollPushRL();
  void DoScrollPushTB();
  void DoScrollPushBT();
  // Free flight
  void DoFlyBT();
  // Free fx out
  void DoExplode();

 protected:
  QCanvas *cv;
  QRect r;			// Affected rectangle
  int   fxType;			// Effect type (QVFXT_xxx)
  int   frame,			// Current frame
        fdelay,			// Delay before start
        frames;			// Total duration
  // General parameters
  //QVideoFXParams *params;

  // Support for free flight effects
  QBitMap *srcBM;
  QRect    srcRect;		// Source image
  // Particles (free fx)
  QVParticle *pi;
  int xParticles,yParticles;	// Division of particles

  static void Setup(QCanvas *cv);
  bool InitEffect();

  bool IsFree();		// Free flight effect?
  bool IsOut();			// Outward effect or inward?

 protected:
  void CreateParticles(int xParts,int yParts);
  void RemoveParticles();
  void CreateParticleDelay(int delayType,int delayFactor);

 public:
  //QVideoFX(QWindow *win=0,int flags=0);
  QVideoFX(QCanvas *cv=0,int flags=0);
  ~QVideoFX();

  // Creating
  bool Define(int typ,QRect *r,int frames=1,int delay=0,
    QBitMap *sbm=0,QRect *rSource=0);
  //void SetParams(QVideoFXParams *p);
  // Creating (specialized)
  bool DefineFlight(QRect *rDst,QBitMap *sbm,QRect *rSrc,int frames,
    int xParts,int yParts,int delayType,int delayFactor,
    int delay=0);
  bool DefineExplode(QRect *rDst,QBitMap *sbm,QRect *rSrc,
    int xParts,int yParts,int delayType,int delayFactor,
    int vxMax,int vxBias,int vyMax,int vyBias,
    int gravity,int delay=0);

  // Attribs
  //QWindow *GetWindow(){ return win; }
  QCanvas *GetCanvas(){ return cv; }
  void GetBoundingRect(QRect *r);

  // Using
  void Iterate();
  void Equalize();
};

// Flat
QBitMap *QVideoFXGetBitMap();
void QVideoFXRun();

#endif
