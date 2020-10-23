/*
 * RFilter - a filtered value changer
 * 14-01-02: Created!
 * NOTES:
 * - Based on a high-inertia mass to constrain a value's motions.
 * - At a certain threshold, the filter can be turned off (for example,
 * in aligning moment calculations in tires).
 * - Created because of severe shaking in the tire aligning moment, which
 * resulting in shaky force feedback (bad for the wheel and feedback).
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/filter.h>
DEBUG_ENABLE

// Local module trace
//#define LTRACE

RFilter::RFilter(rfloat _damping)
{
  value=0;
  flags=DAMPING;
  SetDamping(_damping);
}
RFilter::~RFilter()
{
}

void RFilter::SetDamping(rfloat _damping)
{
  damping=_damping;
  if(fabs(damping)<D3_EPSILON)
  {
    qwarn("RFilter; can't use 0 damping; forcing to 1");
    damping=1;
  }
  invDamping=1.0f/damping;
}

void RFilter::MoveTo(rfloat newValue)
{
  // Like with inertia, the value will go to the value slowly
#ifdef LTRACE
qdbg("Moving from %.3f to %.3f => delta=%.3f\n",value,newValue,
(newValue-value)*invDamping*RR_TIMESTEP);
#endif

  value+=(newValue-value)*invDamping*RR_TIMESTEP;
}

