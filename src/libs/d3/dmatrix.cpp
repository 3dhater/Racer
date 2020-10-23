/*
 * DMatrix - generic matrix code (no pun intended with the name)
 * 20-11-2000: Created! (14:09:05)
 * NOTES:
 * - The memory storage for DMatrix4 is that of OpenGL,
 * this makes it incompatible with C's m[4][4]-type format.
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

/*************
* 4x4 MATRIX *
*************/

// Shortcut to get at row/column members
#define RC(r,c) m[(r)+(c)*4]
#define RCM(m,r,c) (m)[(r)+(c)*4]

void DMatrix4::TransformVectorOr(const DVector3 *v,DVector3 *vOut) const
// Transform a vector 'v' through the matrix, and put result
// in 'vOut'.
// NOTE: Only does orientation! Use TransformVector() to use the full
// matrix. Before 18-10-01, this function was called TransformVector(),
// so be aware of any problems with this in code that I missed. Although
// it was rarely used (only found 1 occurence in dgeode_ase.cpp's normals)
{
//qdbg("QM4:TransformVec: vIn=%f,%f,%f\n",v->x,v->y,v->z);
  vOut->x=v->x*RC(0,0)+v->y*RC(0,1)+v->z*RC(0,2);
  vOut->y=v->x*RC(1,0)+v->y*RC(1,1)+v->z*RC(1,2);
  vOut->z=v->x*RC(2,0)+v->y*RC(2,1)+v->z*RC(2,2);
//qdbg("  vOut=%f,%f,%f\n",vOut->x,vOut->y,vOut->z);
}
void DMatrix4::TransformVector(const DVector3 *v,DVector3 *vOut) const
// Transform a vector 'v' through the matrix, and put result
// in 'vOut'
// BUG: Only does orientation!
{
  float w;
//qdbg("QM4:TransformVec: vIn=%f,%f,%f\n",v->x,v->y,v->z);
  vOut->x=v->x*RC(0,0)+v->y*RC(0,1)+v->z*RC(0,2)+RC(0,3);
  vOut->y=v->x*RC(1,0)+v->y*RC(1,1)+v->z*RC(1,2)+RC(1,3);
  vOut->z=v->x*RC(2,0)+v->y*RC(2,1)+v->z*RC(2,2)+RC(2,3);
  // Homogenous coordinates or perspective divide
  w=v->x*RC(3,0)+v->y*RC(3,1)+v->z*RC(3,2)+RC(3,3);
  w=1.0f/w;
  vOut->x*=w;
  vOut->y*=w;
  vOut->z*=w;
//qdbg("  vOut=%f,%f,%f\n",vOut->x,vOut->y,vOut->z);
}

/*************
* Operations *
*************/
void DMatrix4::Multiply(DMatrix4 *matN)
// Multiply 4x4 with other matrix
{
  dfloat *n=matN->GetM();
  dfloat o[16];
  int i;
  
  for(i=0;i<16;i++)o[i]=m[i];
  // Row 0
  RC(0,0)=RCM(o,0,0)*RCM(n,0,0)+RCM(o,0,1)*RCM(n,1,0)+
          RCM(o,0,2)*RCM(n,2,0)+RCM(o,0,3)*RCM(n,3,0);
  RC(0,1)=RCM(o,0,0)*RCM(n,0,1)+RCM(o,0,1)*RCM(n,1,1)+
          RCM(o,0,2)*RCM(n,2,1)+RCM(o,0,3)*RCM(n,3,1);
  RC(0,2)=RCM(o,0,0)*RCM(n,0,2)+RCM(o,0,1)*RCM(n,1,2)+
          RCM(o,0,2)*RCM(n,2,2)+RCM(o,0,3)*RCM(n,3,2);
  RC(0,3)=RCM(o,0,0)*RCM(n,0,3)+RCM(o,0,1)*RCM(n,1,3)+
          RCM(o,0,2)*RCM(n,2,3)+RCM(o,0,3)*RCM(n,3,3);
  // Row 1
  RC(1,0)=RCM(o,1,0)*RCM(n,0,0)+RCM(o,1,1)*RCM(n,1,0)+
          RCM(o,1,2)*RCM(n,2,0)+RCM(o,1,3)*RCM(n,3,0);
  RC(1,1)=RCM(o,1,0)*RCM(n,0,1)+RCM(o,1,1)*RCM(n,1,1)+
          RCM(o,1,2)*RCM(n,2,1)+RCM(o,1,3)*RCM(n,3,1);
  RC(1,2)=RCM(o,1,0)*RCM(n,0,2)+RCM(o,1,1)*RCM(n,1,2)+
          RCM(o,1,2)*RCM(n,2,2)+RCM(o,1,3)*RCM(n,3,2);
  RC(1,3)=RCM(o,1,0)*RCM(n,0,3)+RCM(o,1,1)*RCM(n,1,3)+
          RCM(o,1,2)*RCM(n,2,3)+RCM(o,1,3)*RCM(n,3,3);
  // Row 2
  RC(2,0)=RCM(o,2,0)*RCM(n,0,0)+RCM(o,2,1)*RCM(n,1,0)+
          RCM(o,2,2)*RCM(n,2,0)+RCM(o,2,3)*RCM(n,3,0);
  RC(2,1)=RCM(o,2,0)*RCM(n,0,1)+RCM(o,2,1)*RCM(n,1,1)+
          RCM(o,2,2)*RCM(n,2,1)+RCM(o,2,3)*RCM(n,3,1);
  RC(2,2)=RCM(o,2,0)*RCM(n,0,2)+RCM(o,2,1)*RCM(n,1,2)+
          RCM(o,2,2)*RCM(n,2,2)+RCM(o,2,3)*RCM(n,3,2);
  RC(2,3)=RCM(o,2,0)*RCM(n,0,3)+RCM(o,2,1)*RCM(n,1,3)+
          RCM(o,2,2)*RCM(n,2,3)+RCM(o,2,3)*RCM(n,3,3);
  // Row 3
  RC(3,0)=RCM(o,3,0)*RCM(n,0,0)+RCM(o,3,1)*RCM(n,1,0)+
          RCM(o,3,2)*RCM(n,2,0)+RCM(o,3,3)*RCM(n,3,0);
  RC(3,1)=RCM(o,3,0)*RCM(n,0,1)+RCM(o,3,1)*RCM(n,1,1)+
          RCM(o,3,2)*RCM(n,2,1)+RCM(o,3,3)*RCM(n,3,1);
  RC(3,2)=RCM(o,3,0)*RCM(n,0,2)+RCM(o,3,1)*RCM(n,1,2)+
          RCM(o,3,2)*RCM(n,2,2)+RCM(o,3,3)*RCM(n,3,2);
  RC(3,3)=RCM(o,3,0)*RCM(n,0,3)+RCM(o,3,1)*RCM(n,1,3)+
          RCM(o,3,2)*RCM(n,2,3)+RCM(o,3,3)*RCM(n,3,3);
}

void DMatrix4::FromMatrix3(DMatrix3 *matrix3)
// Build a 4x4 matrix from a 3x3 matrix
// The 3x3 matrix is put into the upper left-hand side of the 4x4 matrix.
// The remaining elements are filled as the 4x4 identity matrix.
{
  dfloat *m3=matrix3->GetM();
  m[0]=m3[0];
  m[1]=m3[3];
  m[2]=m3[6];
  m[3]=0.0f;

  m[4]=m3[1];
  m[5]=m3[4];
  m[6]=m3[7];
  m[7]=0.0f;

  m[8]=m3[2];
  m[9]=m3[5];
  m[10]=m3[8];
  m[11]=0.0f;

  m[12]=0;
  m[13]=0;
  m[14]=0;
  m[15]=1.0f;
}

/************
* Debugging *
************/
void DMatrix4::DbgPrint(cstring s)
{
  int r,c;
  qdbg(" _                                    _\n");
  for(r=0;r<4;r++)
  {
    if(r==3)qdbg("|_");
    else    qdbg("| ");
    for(c=0;c<4;c++)
    {
      qdbg(" %8.2f",RC(r,c));
    }
    if(r==0)qdbg(" | %s\n",s);
    else if(r==3)qdbg("_|\n");
    else         qdbg(" |\n");
  }
}

/*************
* 3x3 MATRIX *
*************/

// Shortcut to get at row/column members
#undef RC
#undef RCM
#define RC(r,c) m[(r)*3+(c)]
#define RCM(m,r,c) (m)[(r)*3+(c)]

void DMatrix3::FromQuaternion(DQuaternion *q)
// Create an orientation matrix from a normalized quaternion.
{
  dfloat x2,y2,z2;
  x2=q->x*q->x;
  y2=q->y*q->y;
  z2=q->z*q->z;
  dfloat xy,wz,xz,wy,yz,wx;
  xy=q->x*q->y;
  wz=q->w*q->z;
  xz=q->x*q->z;
  wy=q->w*q->y;
  yz=q->y*q->z;
  wx=q->w*q->x;
  m[0]=1.0f-2.0f*(y2+z2);
  m[1]=2.0f*(xy-wz);
  m[2]=2.0f*(xz+wy);
  m[3]=2.0f*(xy+wz);
  m[4]=1.0f-2.0f*(z2+x2);
  m[5]=2.0f*(yz-wx);
  m[6]=2.0f*(xz-wy);
  m[7]=2.0f*(yz+wx);
  m[8]=1.0f-2.0f*(x2+y2);
}

void DMatrix3::FromQuaternionL2(DQuaternion *q,dfloat l2)
// Create an orientation matrix from a quaternion.
// 'l2' is the squared length of the quaternion, used to get 'this' orthogonal
{
  dfloat x2,y2,z2;
  x2=q->x*q->x;
  y2=q->y*q->y;
  z2=q->z*q->z;
  dfloat xy,wz,xz,wy,yz,wx;
  xy=q->x*q->y;
  wz=q->w*q->z;
  xz=q->x*q->z;
  wy=q->w*q->y;
  yz=q->y*q->z;
  wx=q->w*q->x;
  dfloat scale=2.0f/l2;       // Scale to make it orthogonal
  m[0]=1.0f-scale*(y2+z2);
  m[1]=scale*(xy-wz);
  m[2]=scale*(xz+wy);
  m[3]=scale*(xy+wz);
  m[4]=1.0f-scale*(z2+x2);
  m[5]=scale*(yz-wx);
  m[6]=scale*(xz-wy);
  m[7]=scale*(yz+wx);
  m[8]=1.0f-scale*(x2+y2);
}

/************
* Debugging *
************/
void DMatrix3::DbgPrint(cstring s)
{
  int r,c;
  qdbg(" _                           _\n");
  for(r=0;r<3;r++)
  {
    if(r==2)qdbg("|_");
    else    qdbg("| ");
    for(c=0;c<3;c++)
    {
      qdbg(" %8.2f",RC(r,c));
    }
    if(r==0)qdbg(" | %s\n",s);
    else if(r==2)qdbg("_|\n");
    else         qdbg(" |\n");
  }
}

