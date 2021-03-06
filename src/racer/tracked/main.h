#ifndef __MAIN_H
#define __MAIN_H

#include <qlib/common/ui.h>
#include <qlib/app.h>
#include <qlib/canvas.h>
#include <qlib/timer.h>
#include <qlib/dir.h>
#include <d3/d3.h>
#include <d3/culler.h>
#include <GL/glu.h>
#include <unistd.h>

// Local mods
#include <racer/racer.h>
#include <racer/trackv.h>
#include "impscgt.h"
#include "intersect.h"
#include "tcam.h"
#include "pos.h"
#include "ai.h"
#include "props.h"
#include "spline.h"

#define MENU_BASE       1000            // Menu events
#define FONT(n)	(ZFONTPOOL->GetFont(n))

//#define PI  3.14159265358979f
// Factor to get from rad to deg (degVar=radVar*RAD_DEG_FACTOR)
#define RAD_DEG_FACTOR  57.29578

// Floating point rounding errors
#define NEAR_0(n)   ((n)>-.000001&&(n)<.000001)

extern QInfo *info;

// Time taken (in seconds) from last to current frame
// This is needed for all animation velocities/accelerations in the RCar
// classes
// Made global for speed
extern float timeSpan;

// Debugging
enum RDebugFlags
{
  RF_DRAW_EVERY_STEP=1,         // No realtime; show gfx at each int. step
  RF_SLOMO=2,                   // Slowmotion delay
  RF_NO_STEERING=4,             // Turn off steering
  RF_WHEEL_INFO=8		// Text info at wheels
};

struct RDebugInfo
{
  int   flags;
  float timeSpan;
  float slomo;
};

// Edit mode
enum Mode
{
  MODE_POS,                // Positions (grid/pit)
  MODE_TCAM,               // Track cams
  MODE_AI,                 // AI waypoints
  MODE_PROPS,              // Geode properties
  MODE_SPLINE,             // Track spline editing
  MODE_MAX
};
extern int  mode;

// Global vars
extern RDebugInfo *rdbg;
extern REnvironment *rEnv;
extern RAudio *rAudio;
extern QInfo *rDebugInfo;

extern RTrackVRML *track;
extern RCar       *previewCar;

extern DTransformation *tf;
extern DVector3 vPick,vPickEnd;

// Protos from mrun.cpp
void    SetOpButtons(cstring names[]);
int     GetIndex(cstring text,int maxIndex,int curIndex=-1);
cstring GetTrackDir();
void PaintTrack(int flags=0);
void GSwap();
void StatsAdd(cstring s);
// Standard ops
void GotoPoint(DVector3 *v);
void GotoPickPoint();

// Document
void DocDirty();
bool DocIsDirty();

// Debugging
void DbgPrj(cstring s);		// Projection matrix check

#endif

