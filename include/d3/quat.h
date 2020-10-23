// d3/quat.h - quaternions
// 17-11-01: Swapped 'w' member to front of quat to be compliant with ODE.

#ifndef __D3_QUAT_H
#define __D3_QUAT_H

#include <d3/matrix.h>

class DQuaternion
{
 public:
  dfloat w,x,y,z;

 public:
  DQuaternion(){}
  DQuaternion(dfloat _w,dfloat _x,dfloat _y,dfloat _z)
  {
    x=_x; y=_y; z=_z; w=_w;
  }

  void Rotation(DVector3 *axis,dfloat angle);	// Define as a rotation
  void FromMatrix(DMatrix3 *matrix);		// Define from a matrix
  void FromEuler(dfloat x,dfloat y,dfloat z);   // Euler angles
  void Multiply(DQuaternion *q);		// Multiply with other quat
  void Multiply(DQuaternion *a,DQuaternion *b);	// Multiply 2 quats into 'this'
  dfloat Length() const
  {
    return sqrt(x*x+y*y+z*z+w*w);
  }
  dfloat LengthSquared() const
  {
    return x*x+y*y+z*z+w*w;
  }
  void Normalize()
  {
    dfloat l=LengthSquared();
    if(l<D3_EPSILON)
    {
      // Zero quat
      w=1.0f;
      x=y=z=0.0f;
    } else
    {
      dfloat f=1.0f/sqrt(l);
      w*=f;
      x*=f;
      y*=f;
      z*=f;
    }
  }
  void Slerp(const DQuaternion *q,dfloat t,DQuaternion *res);

  void ToAxisAngle(DVector3 *axis,dfloat *angle);
};

#endif
