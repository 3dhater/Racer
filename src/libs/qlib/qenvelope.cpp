/*
 * QEnvelope - envelope generator for control values
 * OBSOLETE since 16-5-97
 * 18-12-96: Created!
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <stdlib.h>
#include <qlib/envelope.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QEnvelope::QEnvelope()
  : QFX(0)
{
  flags=QENVF_DBLBUF;
  type=QENV_LINEAR;
  vFrom=0; vTo=0;
  vBase=vCurrent=0;
  iteration=iterations=0;
  delay=0;
  loopsToGo=0;
  //vMin=-2000000000; vMax=2000000000;	// Pretty much everything
  vMin=0x80000000; vMax=0x7FFFFFFF;	// Pretty much everything
}

QEnvelope::~QEnvelope()
{
}

void QEnvelope::Vary(int _type,int _from,int _to,int _iterations,int _delay)
{
  type=_type;
  vFrom=_from; vTo=_to;
  //printf("QEnvelope::Vary: (%p) %d\n",this,iterations);
  iterations=_iterations;
  delay=_delay;

  // Start at vFrom immediately (for any delays)
  vCurrent=vFrom;

  // Setup envelope
  iteration=0;
  loopsToGo=0;
  curSkip=skip=0;
  // Indirect envelopes need base value
  switch(type)
  { case QENV_ADD:
    case QENV_SUB:
      //vBase=vCurrent; vCurrent=0; break;
      vBase=0; break;
    default:
      vBase=0; break;
  }
  // Forget any equalization (for fast Vary()'s in a line)
  flags&=~QENVF_EQUALIZE;
  Done(FALSE);
}
void QEnvelope::VaryTo(int _type,int _to,int _iterations,int _delay)
{ Vary(_type,GetCurrent(),_to,_iterations,_delay);
}
void QEnvelope::Go(int _to,int delay)
// Shortcut to set a value directly (asap)
{ Vary(QENV_LINEAR,_to,_to,0,delay);
}

void QEnvelope::Loop(int _loopType,int loops)
{
  //printf("QEnv::Loop: loops=%d\n",loops);
  loopType=_loopType;
  loopsToGo=loops;
}
void QEnvelope::Skip(int n)
{ skip=n;
  curSkip=0;
}

void QEnvelope::Iterate()
{ int vAdd;
  if(IsDone())return;
  if(delay)
  { delay--;
    return;
  }
  //printf("QEnvelope::Iterate %p: %d of %d\n",this,iteration,iterations);
  if(flags&QENVF_EQUALIZE)
  { // Use last generated value one more time (dblbuf)
    //printf("QEnv EQUALIZE\n");
    //if(obj)
      //obj->SetControl(control,vCurrent);
    flags&=~QENVF_EQUALIZE;
    if(loopsToGo==0)
    { // Really done
      Done(TRUE);
      return;
    } // else loop effect is in place
  }

  // Check for any skipping
  if(curSkip<skip)
  { curSkip++;
    return;
  } else curSkip=0;		// Restart skip count

  // Calculate current value
  switch(type)
  { case QENV_LINEAR:
      if(iterations==0)vCurrent=vTo;
      else vCurrent=vFrom+(vTo-vFrom)*iteration/iterations; break;
    case QENV_ADD:
      // Added value is a linear line
      if(iterations==0)vAdd=vTo;
      else vAdd=vFrom+(vTo-vFrom)*iteration/iterations;
      // Add it to the control (indirect)
      vCurrent+=vAdd; break;
    default:
      vCurrent=0; break;
  }

  // Next iteration; finished if at the end (or loop)
  if(++iteration>iterations)
  { if(loopsToGo>0)
    { loopsToGo--;
     do_loop:
      //printf("    do_loop\n");
      iteration=0;
    } else if(loopsToGo==-1)
    { // Loop forever
      goto do_loop;
    } else if(flags&QENVF_DBLBUF)
    { // Just one more go with the current value
      flags|=QENVF_EQUALIZE;
    } else
    { Done(TRUE);
    }
  }
}

