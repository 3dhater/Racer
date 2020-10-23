/*
 * DQuat - a quaternion class
 * 01-02-01: Created!
 * NOTES:
 * - Heavily based on Richard Chaney's Quaternion class
 * - Slerp() function based on Scian source code.
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

void DQuaternion::Rotation(DVector3 *axis,dfloat angle)
// Define as a rotation around an axis
// Note that a quaternion *is* a rotation around an axis (a definition):
// w=cos(angle/2), x/y/z=axis.x/y/z*sin(angle/2)
{
  DVector3 v;
  axis->NormalizeTo(&v);
  angle/=2.0f;
  w=cos(angle);
  dfloat s=sin(angle);
  x=v.x*s;
  y=v.y*s;
  z=v.z*s;
}

void DQuaternion::Multiply(DQuaternion *b)
// Multiply with other quat
{
  w=w*b->w-x*b->x-y*b->y-z*b->z;
  x=w*b->x+x*b->w+y*b->z-z*b->y;
  y=w*b->y-x*b->z+y*b->w+z*b->x;
  z=w*b->z+x*b->y-y*b->x+z*b->w;
}
void DQuaternion::Multiply(DQuaternion *a,DQuaternion *b)
// Multiply 2 quats into 'this'
{
  w=a->w*b->w-a->x*b->x-a->y*b->y-a->z*b->z;
  x=a->w*b->x+a->x*b->w+a->y*b->z-a->z*b->y;
  y=a->w*b->y-a->x*b->z+a->y*b->w+a->z*b->x;
  z=a->w*b->z+a->x*b->y-a->y*b->x+a->z*b->w;
}

void DQuaternion::FromMatrix(DMatrix3 *matrix)
// Define from a 3x3 matrix (column major)
{
  // Quaternion components squared
  dfloat qw2=0.25*(matrix->GetM()[0]+matrix->GetM()[4]+matrix->GetM()[8]+1.0);
  dfloat qx2=qw2-0.5*(matrix->GetM()[4]+matrix->GetM()[8]);
  dfloat qy2=qw2-0.5*(matrix->GetM()[8]+matrix->GetM()[0]);
  dfloat qz2=qw2-0.5*(matrix->GetM()[0]+matrix->GetM()[4]);
  // Decide maximum magnitude component
  int i=(qw2>qx2)?
    ((qw2>qy2)?((qw2>qz2)?0:3) : ((qy2>qz2)?2:3)) :
    ((qx2>qy2)?((qx2>qz2)?1:3) : ((qy2>qz2)?2:3));
  dfloat tmp;

  // Compute signed quat components using numerically stable method
  switch(i)
  {
    case 0:
      w=sqrt(qw2); tmp=0.25f/w;
      x=(matrix->GetM()[7]-matrix->GetM()[5])*tmp;
      y=(matrix->GetM()[2]-matrix->GetM()[6])*tmp;
      z=(matrix->GetM()[3]-matrix->GetM()[1])*tmp;
      break;
    case 1:
      x=sqrt(qx2); tmp=0.25f/x;
      w=(matrix->GetM()[7]-matrix->GetM()[5])*tmp;
      y=(matrix->GetM()[1]+matrix->GetM()[3])*tmp;
      z=(matrix->GetM()[6]+matrix->GetM()[2])*tmp;
      break;
    case 2:
      y=sqrt(qy2); tmp=0.25f/y;
      w=(matrix->GetM()[2]-matrix->GetM()[6])*tmp;
      x=(matrix->GetM()[1]+matrix->GetM()[3])*tmp;
      z=(matrix->GetM()[5]+matrix->GetM()[7])*tmp;
      break;
    case 3:
      z=sqrt(qz2); tmp=0.25f/z;
      w=(matrix->GetM()[3]-matrix->GetM()[1])*tmp;
      x=(matrix->GetM()[2]+matrix->GetM()[6])*tmp;
      y=(matrix->GetM()[5]+matrix->GetM()[7])*tmp;
      break;
  }

  // Always keep all components positive
  // (note that scalar*quat is equivalent to quat, so q==-q)
  if(i&&w<0.0){ w=-w; x=-x; y=-y; z=-z; }

  // Normalize it to be safe
  tmp=1.0f/sqrt(w*w+x*x+y*y+z*z);
  w*=tmp;
  x*=tmp;
  y*=tmp;
  z*=tmp;
}

void DQuaternion::Slerp(const DQuaternion *q,dfloat t,DQuaternion *res)
// Does the slerp operation from [Shoemake 1985]
{
  dfloat theta,costheta,w1,w2,sintheta;

  costheta=x*q->x+y*q->y+z*q->z+w*q->w;
  theta=acos(costheta);
  sintheta=sin(theta);
  if(sintheta>0.0f)
  {
    w1=sin((1.0f-t)*theta)/sintheta;
    w2=sin(t*theta)/sintheta;
  } else
  {
    // They're the same quaternion, so who cares?
    w1=1.0f;
    w2=0.0f;
  }

  res->x=w1*x+w2*q->x;
  res->y=w1*y+w2*q->y;
  res->z=w1*z+w2*q->z;
  res->w=w1*w+w2*q->w;
}
