// qlib/qenvelope.h - envelope generator; mainly used for QControl

#ifndef __QLIB_QENVELOPE_H
#define __QLIB_QENVELOPE_H

#include <qlib/fx.h>
#include <qlib/controls.h>

const int QENVF_DBLBUF=1;		// Dblbuf'd envelope
const int QENVF_EQUALIZE=2;		// Double last value for dblbuf

const int QENV_LINEAR=0;		// Linear curve
const int QENV_EASEIN=1;		// Ease in curve
const int QENV_EASEOUT=2;
const int QENV_ADD=3;		// Add to value (linear); velocity
const int QENV_SUB=4;		// Subtract (-add)

const int QENVLT_NORMAL=0;		// Also see qcontrol.h
const int QENVLT_FLIP=1;

class QEnvelope : public QFX
{
  string ClassName(){ return "envelope"; }

 protected:
  int flags;			// QENVF_xxx
  int type;			// QENV_xxx
  int vFrom,vTo;
  int vCurrent;			// Last/current value
  int vBase;			// Base value (for ADD, SUB etc.)
  int iteration;		// Current iteration
  int iterations;		// Total number of iterations
  int delay;			// Before starting actual curve
  int curSkip,skip;		// Skip iterations inbetween
  int vMin,vMax;		// Restrict values that may exist
  // Looping
  int loopType;
  int loopsToGo;

 public:
  QEnvelope();
  ~QEnvelope();

  int  GetCurrent(){ return vBase+vCurrent; }

  void Vary(int type,int from,int to,int iterations,int delay=0);
  void VaryTo(int type,int to,int iterations,int delay=0);
  void Go(int to,int delay=0);

  void Loop(int loopType,int loops=-1);
  void Skip(int n);		// Skip #it's between updates

  void Iterate();
};

#endif

