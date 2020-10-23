// d3/matrix.h

#ifndef __D3_MATRIX_H
#define __D3_MATRIX_H

#include <d3/types.h>
#include <d3/vector.h>
//#include <d3/quat.h>
//#include <iostream.h>

/*************
* 4x4 MATRIX *
*************/

/*
   Storage for 4x4 matrices (C++):

   OpenGL storage is used, but column major is written:
   0  4  8  12                 X  Y  Z  T (unconfirmed XYZT locations)
   1  5  9  13                 X  Y  Z  T
   2  6  10 14                 X  Y  Z  T
   3  7  11 15                 0  0  0  1

   Because this doesn't allow m[4][4] notation, the macro [QM_]RC(r,c)
   is used to access members by (row,column).
*/

// Use the following macro to access direct (row,column)
// members in the matrix.
#define QM_RC(r,c)	m[(r)+(c)*4]

class DMatrix3;

class DMatrix4
// 4x4 matrix
{
 protected:
  dfloat m[16];			// Not public because of complex memory format

 public:
  DMatrix4(){}
  ~DMatrix4(){}

  // Attributes
  dfloat *GetM(){ return m; }	// Access to actual float (experts only!)
  dfloat  GetRC(int r,int c) const { return m[r+c*4]; }
  void    SetRC(int r,int c,dfloat v){ m[r+c*4]=v; }

  inline void SetX(const DVector3* v);
  inline void SetY(const DVector3* v);
  inline void SetZ(const DVector3* v);
  inline void SetT(const DVector3* v);

  inline void SetIdentity();

  // Operations
  void Multiply(DMatrix4 *n);

  void TransformVector(const DVector3 *v,DVector3 *vOut) const;
  void TransformVectorOr(const DVector3 *v,DVector3 *vOut) const;

  // Deriving matrices
  void FromMatrix3(DMatrix3 *m3);

  // Debugging
  void DbgPrint(cstring s);
};

// Inlines

inline void DMatrix4::SetIdentity()
// Load the identity matrix
{
  m[0]=1.f; m[1]=0.f; m[2]=0.f; m[3]=0.f;
  m[4]=0.f; m[5]=1.f; m[6]=0.f; m[7]=0.f;
  m[8]=0.f;  m[9]=0.f;  m[10]=1.f; m[11]=0.f;
  m[12]=0.f; m[13]=0.f; m[14]=0.f; m[15]=1.f;
}

inline void DMatrix4::SetX(const DVector3* v)
{
  QM_RC(0,0)=v->x;
  QM_RC(1,0)=v->y;
  QM_RC(2,0)=v->z;
}
inline void DMatrix4::SetY(const DVector3* v)
{
  QM_RC(0,1)=v->x;
  QM_RC(1,1)=v->y;
  QM_RC(2,1)=v->z;
}
inline void DMatrix4::SetZ(const DVector3* v)
{
  QM_RC(0,2)=v->x;
  QM_RC(1,2)=v->y;
  QM_RC(2,2)=v->z;
}
inline void DMatrix4::SetT(const DVector3* v)
{
  //qerr("DMatrix4:SetT nyi");
  QM_RC(0,3)=v->x;
  QM_RC(1,3)=v->y;
  QM_RC(2,3)=v->z;
}

/*************
* 3x3 MATRIX *
*************/

/*
   Storage for 3x3 matrices (C++):

   Mathematical common storage is used:

   0  1  2
   3  4  5
   6  7  8

   or

   m[0][0]  m[0][1]  m[0][2]       (not possible to use in code though)
   m[1][0]  m[1][1]  m[1][2]
   m[2][0]  m[2][1]  m[2][2]

   Note this is transposed with respect to DMatrix4!!
   However, it is more common with regular column-major source code.
*/

class DQuaternion;

class DMatrix3
{
 protected:
  dfloat m[9];                 // Not public because of complex memory format

 public:
  DMatrix3(){}
  ~DMatrix3(){}

  // Attributes
  dfloat *GetM(){ return m; }   // Access to actual float (experts only!)
  dfloat  GetRC(int r,int c) const { return m[r*3+c]; }
  void    SetRC(int r,int c,dfloat v){ m[r*3+c]=v; }

  inline void SetIdentity();

  // Operations
  void Multiply(DMatrix3 *m);

  // Creating the matrix from quaternions
  void FromQuaternion(DQuaternion *q);
  void FromQuaternionL2(DQuaternion *q,dfloat l2);

  // Transforming vectors
  void Transform(const DVector3 *vIn,DVector3 *vOut)
  // vOut=M*vIn
  {
    vOut->x=vIn->x*m[0]+vIn->y*m[1]+vIn->z*m[2];
    vOut->y=vIn->x*m[3]+vIn->y*m[4]+vIn->z*m[5];
    vOut->z=vIn->x*m[6]+vIn->y*m[7]+vIn->z*m[8];
  }
  void TransposeTransform(const DVector3 *vIn,DVector3 *vOut)
  // vOut=transpose(M)*vIn
  // Note that the transpose of an orthogonal matrix M
  // is the inverse. (M^-1*M=I) Useful for world->body and body->world
  // transformations.
  {
    vOut->x=vIn->x*m[0]+vIn->y*m[3]+vIn->z*m[6];
    vOut->y=vIn->x*m[1]+vIn->y*m[4]+vIn->z*m[7];
    vOut->z=vIn->x*m[2]+vIn->y*m[5]+vIn->z*m[8];
  }

  // Debugging
  void DbgPrint(cstring s);
};

inline void DMatrix3::SetIdentity()
// Load the identity matrix
{
  m[0]=1.f; m[1]=0.f; m[2]=0.f;
  m[3]=0.f; m[4]=1.f; m[5]=0.f;
  m[6]=0.f; m[7]=0.f; m[8]=1.f;
}

#ifdef OBS_EXAMPLE_FROM_PROPHECY
/*
	Twilight Prophecy 3D/Multimedia SDK
	A multi-platform development system for virtual reality and multimedia.

	Copyright (C) 1997-2000 by Twilight 3D Finland Oy Ltd.
*/
#ifndef PRCORE_MATRIX_H
#define PRCORE_MATRIX_H

#define PR_API

	/*

	[00 01 02 03 ]
	[            ]  Offsets
	[04 05 06 07 ]
	[            ]
	[08 09 10 11 ]
	[            ]
	[12 13 14 15 ]
	[            ]

	[00 01 02 03 ]
	[            ]  Index: float[4][4]
	[10 11 12 13 ]
	[            ]
	[20 21 22 23 ]
	[            ]
	[30 31 32 33 ]
	[            ]

	[?  ?  ?  Tx ]
	[            ]  Translation: Tx, Ty, Tz
	[?  ?  ?  Ty ]
	[            ]
	[?  ?  ?  Tz ]
	[            ]
	[?  ?  ?  ?  ]
	[            ]

	[Xx Yx Zx ?  ]
	[            ]  Rotation: Xn, Yn, Zn
	[Xy Yy Zy ?  ]
	[            ]
	[Xz Yz Zz ?  ]
	[            ]
	[?  ?  ?  ?  ]
	[            ]

	*/

#endif

#endif

#endif
