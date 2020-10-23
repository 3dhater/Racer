// qlib/qcontrol.h - Control around an int

#ifndef __QLIB_QCONTROL_H
#define __QLIB_QCONTROL_H

#include <qlib/fx.h>
#include <qlib/envelope.h>
#include <qlib/controls.h>

const int QCONTROLF_DBLBUF=1;	// Dblbuf'd envelope
const int QCONTROLF_EQUALIZE=2;	// Double last value for dblbuf

const int QCONTROL_LINEAR=0;	// Linear curve
const int QCONTROL_EASEIN=1;	// Ease in curve
const int QCONTROL_EASEOUT=2;

const int QCONTROLLT_NORMAL=0;	// Loop types; just do it
const int QCONTROLLT_FLIP=1;	// Reverse when hitting end

#define QC_MAXKEY	6	// Key values

class QControl;
typedef void (*QControlCallback)(QControl *c,void *p,int v);

class QControl : public QFX
// Dynamic control; base class for specific control types
{
  string ClassName(){ return "control"; }

 protected:
  int flags;			// QCONTROLF_xxx
  //int type;                     // QCONTROL_xxx
  // Value keying
  int  keyTime[QC_MAXKEY];	// Time at which the value should be current
  int  keyValue[QC_MAXKEY];	// Value at that time
  char keyFlags[QC_MAXKEY];	// For easein/out
  char keys;			// #keyframes
  int curTime;			// Timer
  int  vCurrent;		// Current value
/*
  int vFrom,vTo;
  int vCurrent;                 // Last/current value
  int vBase;                    // Base value (for ADD, SUB etc.)
  int iteration;                // Current iteration
  int iterations;               // Total number of iterations
  int delay;                    // Before starting actual curve
  int curSkip,skip;             // Skip iterations inbetween
  int vMin,vMax;                // Restrict values that may exist
*/
  // Looping
  int loopType;
  int loopsToGo;

  int   controlID;		// CONTROL_xxx
  void *cbPtr;			// Callback data
  //QEnvelope *env;		// Any envelope that generates dynamic val's
  QControlCallback cbFunc;	// Call back when modifying vCurrent

 public:
  QControl(int controlID=0);
  ~QControl();

  void SetCallback(QControlCallback func,void *clientPtr);
  int GetCurrent();		// Obsolete; use Get() preferrably
  int Get();
  int GetControlID(){ return controlID; }

  // Functions on top of envelope
  //void Vary(int type,int from,int to,int iterations,int delay=0);
  //void VaryTo(int type,int to,int iterations,int delay=0);
  //void Go(int to,int delay=0);
  void Set(int v);
  // Key functions (new)
  void Key(int frame,int value,char flags=0);

  void Loop(int loopType,int loops=-1);
  void Skip(int n);

  void Iterate();
};

#ifdef NOTDEF
class QGel;

class QGelControl : public QControl
// Control inside a gel
{
 protected:
  QGel *gel;			// Affected object
  int   ctrl;		// Which gel control

 public:
  QGelControl(QGel *gel,int ctrl);
  ~QGelControl();

  void Iterate();
};
#endif

#endif

