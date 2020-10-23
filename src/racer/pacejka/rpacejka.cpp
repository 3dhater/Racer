/*
 * Pacejka calculations
 * 10-03-2001: Created! (21:34:56)
 * NOTES:
 * - Formulas taken from G. Genta's book, pages 63 and 77.
 * (C) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

RPacejka::RPacejka()
{
  // Initial vars
  a0=a1=a2=a3=a4=a5=a6=a7=a8=a9=a10=a111=a112=a12=a13=0;
  b0=b1=b2=b3=b4=b5=b6=b7=b8=b9=b10=0;
  c0=c1=c2=c3=c4=c5=c6=c7=c8=c9=c10=c11=c12=c13=c14=c15=c16=c17=0;
  
  camber=0;
  sideSlip=0;
  slipPercentage=0;
  Fz=0;
  
  // Output
  Fx=Fy=Mz=0;
  longStiffness=latStiffness=0;
  maxForce.SetToZero();
}
RPacejka::~RPacejka()
{
}

/**********
* Loading *
**********/
bool RPacejka::Load(QInfo *info,cstring path)
// Get parameters from a file
{
  char buf[128],fname[256];
  
  // Fx
  sprintf(buf,"%s.a0",path); a0=info->GetFloat(buf);
  sprintf(buf,"%s.a1",path); a1=info->GetFloat(buf);
  sprintf(buf,"%s.a2",path); a2=info->GetFloat(buf);
  sprintf(buf,"%s.a3",path); a3=info->GetFloat(buf);
  sprintf(buf,"%s.a4",path); a4=info->GetFloat(buf);
  sprintf(buf,"%s.a5",path); a5=info->GetFloat(buf);
  sprintf(buf,"%s.a6",path); a6=info->GetFloat(buf);
  sprintf(buf,"%s.a7",path); a7=info->GetFloat(buf);
  sprintf(buf,"%s.a8",path); a8=info->GetFloat(buf);
  sprintf(buf,"%s.a9",path); a9=info->GetFloat(buf);
  sprintf(buf,"%s.a10",path); a10=info->GetFloat(buf);
  sprintf(buf,"%s.a111",path); a111=info->GetFloat(buf);
  sprintf(buf,"%s.a112",path); a112=info->GetFloat(buf);
  sprintf(buf,"%s.a13",path); a13=info->GetFloat(buf);
  
  // Fy
  sprintf(buf,"%s.b0",path); b0=info->GetFloat(buf);
  sprintf(buf,"%s.b1",path); b1=info->GetFloat(buf);
  sprintf(buf,"%s.b2",path); b2=info->GetFloat(buf);
  sprintf(buf,"%s.b3",path); b3=info->GetFloat(buf);
  sprintf(buf,"%s.b4",path); b4=info->GetFloat(buf);
  sprintf(buf,"%s.b5",path); b5=info->GetFloat(buf);
  sprintf(buf,"%s.b6",path); b6=info->GetFloat(buf);
  sprintf(buf,"%s.b7",path); b7=info->GetFloat(buf);
  sprintf(buf,"%s.b8",path); b8=info->GetFloat(buf);
  sprintf(buf,"%s.b9",path); b9=info->GetFloat(buf);
  sprintf(buf,"%s.b10",path); b10=info->GetFloat(buf);
  
  // Mz
  sprintf(buf,"%s.c0",path); c0=info->GetFloat(buf);
  sprintf(buf,"%s.c1",path); c1=info->GetFloat(buf);
  sprintf(buf,"%s.c2",path); c2=info->GetFloat(buf);
  sprintf(buf,"%s.c3",path); c3=info->GetFloat(buf);
  sprintf(buf,"%s.c4",path); c4=info->GetFloat(buf);
  sprintf(buf,"%s.c5",path); c5=info->GetFloat(buf);
  sprintf(buf,"%s.c6",path); c6=info->GetFloat(buf);
  sprintf(buf,"%s.c7",path); c7=info->GetFloat(buf);
  sprintf(buf,"%s.c8",path); c8=info->GetFloat(buf);
  sprintf(buf,"%s.c9",path); c9=info->GetFloat(buf);
  sprintf(buf,"%s.c10",path); c10=info->GetFloat(buf);
  sprintf(buf,"%s.c11",path); c11=info->GetFloat(buf);
  sprintf(buf,"%s.c12",path); c12=info->GetFloat(buf);
  sprintf(buf,"%s.c13",path); c13=info->GetFloat(buf);
  sprintf(buf,"%s.c14",path); c14=info->GetFloat(buf);
  sprintf(buf,"%s.c15",path); c15=info->GetFloat(buf);
  sprintf(buf,"%s.c16",path); c16=info->GetFloat(buf);
  sprintf(buf,"%s.c17",path); c17=info->GetFloat(buf);
  
  return TRUE;
}

/**********
* Physics *
**********/
void RPacejka::Calculate()
// Given the input parameters, calculate output.
// All output is calculated, although you might not need them all.
{
  // Calculate long. force (and long. stiffness plus max long. force)
  Fx=CalcFx();
  // Calculate lateral force, cornering stiffness and max lateral force
  Fy=CalcFy();
  // Aligning moment (force feedback)
  Mz=CalcMz();
}

rfloat RPacejka::CalcFx()
// Calculates longitudinal force
// From G. Genta's book, page 63
// Note that the units are inconsistent:
//   Fz is in kN
//   slipRatio is in percentage (=slipRatio*100=slipPercentage)
//   camber and slipAngle are in degrees
// Resulting forces are better defined:
//   Fx, Fy are in N
//   Mz     is in Nm
{
  rfloat B,C,D,E;
  rfloat Fx;
  rfloat Sh,Sv;
  rfloat uP;
  rfloat FzSquared;

  // Calculate derived coefficients
  FzSquared=Fz*Fz;
  C=b0;
  uP=b1*Fz+b2;
  D=uP*Fz;
  
  // Avoid div by 0
  if((C>-D3_EPSILON&&C<D3_EPSILON)||
     (D>-D3_EPSILON&&D<D3_EPSILON))
  {
    B=99999.0f;
  } else
  {
#ifdef OBS
qdbg("b34=%f, b5Fz=%f, exp=%f, C*D=%f, C=%f, D=%f\n",
b3*FzSquared+b4*Fz,-b5*Fz,expf(-b5*Fz),C*D,C,D);
#endif
    B=((b3*FzSquared+b4*Fz)*expf(-b5*Fz))/(C*D);
  }
  
  E=b6*FzSquared+b7*Fz+b8;
  Sh=b9*Fz+b10;
  Sv=0;
  
  // Notice that product BCD is the longitudinal tire stiffness
  longStiffness=B*C*D; // *RR_RAD2DEG;    // RR_RAD2DEG == *360/2PI
  
  // Remember the max longitudinal force (where sin(...)==1)
  maxForce.x=D+Sv;

  // Calculate result force
  Fx=D*sinf(C*atanf(B*(1.0f-E)*(slipPercentage+Sh)+
            E*atanf(B*(slipPercentage+Sh))))+Sv;

#ifdef OBS
qdbg("CalcFx; atan(0)=%f\n",atanf(0.0f));
qdbg("  B=%f, C=%f, D=%f, E=%f, Sh=%f, Sv=%f, slipPerc=%f%%\n",
B,C,D,E,Sh,Sv,slipPercentage);
#endif

  return Fx;
}
rfloat RPacejka::CalcFy()
// Calculates lateral force
// Note that BCD is the cornering stiffness, and
// Sh and Sv account for ply steer and conicity forces
{
  rfloat B,C,D,E;
  rfloat Fy;
  rfloat Sh,Sv;
  rfloat uP;
  
  // Calculate derived coefficients
  C=a0;
  uP=a1*Fz+a2;
  D=uP*Fz;
  E=a6*Fz+a7;
  
  // Avoid div by 0
  if((C>-D3_EPSILON&&C<D3_EPSILON)||
     (D>-D3_EPSILON&&D<D3_EPSILON))
  {
    B=99999.0f;
  } else
  {
    if(a4>-D3_EPSILON&&a4<D3_EPSILON)
    {
      B=99999.0f;
    } else
    {
      // Notice that product BCD is the lateral stiffness (=cornering)
      latStiffness=a3*sinf(2*atanf(Fz/a4))*(1-a5*fabs(camber));
      B=latStiffness/(C*D);
    }
  }

//qdbg("RPac: camber=%f, a8=%f\n",camber,a8);
  Sh=a8*camber+a9*Fz+a10;
  Sv=(a111*Fz+a112)*camber*Fz+a12*Fz+a13;
  
  // Remember maximum lateral force
  maxForce.y=D+Sv;
  
  // Calculate result force
  Fy=D*sinf(C*atanf(B*(1.0f-E)*(sideSlip+Sh)+
            E*atanf(B*(sideSlip+Sh))))+Sv;

#ifdef OBS
qdbg("CalcFy=%f\n",Fy);
qdbg("  B=%f, C=%f, D=%f, E=%f, Sh=%f, Sv=%f, slipAngle=%f deg\n",
B,C,D,E,Sh,Sv,sideSlip);
#endif

  return Fy;
}
rfloat RPacejka::CalcMz()
// Calculates aligning moment
{
  rfloat Mz;
  rfloat B,C,D,E,Sh,Sv;
  rfloat uP;
  rfloat FzSquared;
  
  // Calculate derived coefficients
  FzSquared=Fz*Fz;
  C=c0;
  D=c1*FzSquared+c2*Fz;
  E=(c7*FzSquared+c8*Fz+c9)*(1-c10*fabs(camber));
  if((C>-D3_EPSILON&&C<D3_EPSILON)||
     (D>-D3_EPSILON&&D<D3_EPSILON))
  {
    B=99999.0f;
  } else
  {
    B=((c3*FzSquared+c4*Fz)*(1-c6*fabs(camber))*expf(-c5*Fz))/(C*D);
  }
  Sh=c11*camber+c12*Fz+c13;
  Sv=(c14*FzSquared+c15*Fz)*camber+c16*Fz+c17;
  
  Mz=D*sinf(C*atanf(B*(1.0f-E)*(sideSlip+Sh)+
            E*atanf(B*(sideSlip+Sh))))+Sv;
  return Mz;
}

#ifdef OBS
/*********
* Extras *
*********/
rfloat RPacejka::CalcMaxForce()
// Calculates maximum force that the tire can produce
// If the longitudinal and lateral force combined exceed this,
// a violation of the friction circle (max total tire force) is broken.
// In that case, reduce the lateral force, since the longitudinal force
// is more prominent (this simulates a locked braking wheel, which
// generates no lateral force anymore but does maximum longitudinal force).
{
  rfloat uP;

  // Calculate derived coefficients
  uP=b1*Fz+b2;

  return uP*Fz;
}
#endif
