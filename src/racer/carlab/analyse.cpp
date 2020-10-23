/*
 * Carlab - Analysis of the car
 * 24-10-01: Created!
 * NOTES:
 * - This should create a report with technical info, using
 * known setup 'rules' that may help in getting you further in
 * setting up a drivable car. It should give advises and guidelines
 * (c) Dolphinity/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#define REPORT_FNAME   "replab.txt"

static QRect repDefaultView(50,50,900,615);        // Default report view
static int   repLines=28;

// Export analyse results (for modify.cpp)
rfloat weightRatio[RCar::MAX_WHEEL]; // Ratio of full weight (sum=1)
rfloat weight[RCar::MAX_WHEEL];      // Tire weight (sum=mass)
rfloat zf,zr,L;           // Static load

void Analyse(bool display)
// To assist in physics tweaking, give some advice based on
// findings in actual books.
// If 'display'=FALSE, then no report is shown (use it to analyse
// the vehicle but to use the results only in a different module).
{
  FILE *fp;
  int   i;
  // Calculated results
  int    wheels;            // Cached car->GetWheels()
  rfloat m,G,W;             // Car mass, gravity, car weight
  rfloat mEngine;           // Engine mass
  rfloat defl;              // Desired suspension deflection
  rfloat wheelRate[RCar::MAX_WHEEL];      // Advised wheel rates
  rfloat natFreq[RCar::MAX_WHEEL];        // Natural frequencies w/out damping
  rfloat natFreqDampBump[RCar::MAX_WHEEL];    // Natural freq. with damping
  rfloat natFreqDampRebound[RCar::MAX_WHEEL]; // Natural freq. with damping
  rfloat dampAlpha[RCar::MAX_WHEEL];      // Amount of damping (R/(2m))
  rfloat dampingCoeffBump[RCar::MAX_WHEEL];      // Critical damping N/m/s
  rfloat dampingCoeffRebound[RCar::MAX_WHEEL];
  rfloat dampingRatioBump[RCar::MAX_WHEEL];      // 1=critically damped
  rfloat dampingRatioRebound[RCar::MAX_WHEEL];   // <1=underdamped
  rfloat Ixx,Iyy,Izz;       // Estimated inertia
  DVector3 size;            // Temp. size (used for inertia calculations)
  rfloat x,y,z;             // Generic temp variables
  rfloat t;
  DMatrix3 *tMat=car->GetBody()->GetInertia();

  fp=fopen(REPORT_FNAME,"w");
  if(!fp)
  {
    QMessageBox("Error","Can't open report file");
    return;
  }

  fprintf(fp,"Physics analysis for car '%s'\n",car->GetName());
  for(i=0;i<strlen(car->GetName())+28;i++)
    fprintf(fp,"-");
  fprintf(fp,"\n\n");

  // Setup
  m=car->GetBody()->GetMass();
  mEngine=car->GetEngine()->GetMass();
  m+=mEngine;        // Total sprung mass mass (not a typo)
  G=9.80665;         // Planet Earth (may get this from env.ini?)
  W=m*G;             // Weight is the amount of Newtons

  // Generic info
  fprintf(fp,"Generic info:\n");
  fprintf(fp,"-------------\n\n");
  fprintf(fp,"Body mass: %.2f kg, engine mass %.2f kg\n",
    car->GetBody()->GetMass(),mEngine);
  fprintf(fp,"Total car mass (sprung): %.2f kg, weight %.2f N\n",m,W);
  fprintf(fp,"\n");

  // Weight ratio
  // Assumes all fronts and both rear have equal Z values (!)
  // Formula from Gillespie, 'Fundamentals of Vehicle Dynamics', page 13.
  //
  wheels=car->GetWheels();
  // Get front and rear location
  for(i=0;i<wheels;i++)
  {
    z=car->GetSuspension(i)->GetZ();
    if(z<0)zr=z;
    else   zf=z;
  }
  // Calculate wheelbase (front to rear)
  L=zf-zr;
  if(fabs(L)<D3_EPSILON)
  { fprintf(fp,"Error: Wheelbase is 0.\n");
    goto fail;
  }
  // Continue with absolute locations
  zf=fabs(zf);
  zr=fabs(zr);
  // Calculate weight ratios; note that front ratios use the rear length
  // Assumes 2 wheels in front, 2 in rear (otherwise, count wheels in front
  // divide not by 2.0 but by #wheels_in_front (or rear)
  for(i=0;i<wheels;i++)
  {
    z=car->GetSuspension(i)->GetZ();
    if(z<0)weightRatio[i]=(zf/L)/2.0;
    else   weightRatio[i]=(zr/L)/2.0;
    //if(z<0)weightRatio[i]=(W*zf/L)/2.0;
    //else   weightRatio[i]=(W*zr/L)/2.0;
    // Calculate actual weight (N)
    weight[i]=weightRatio[i]*W;
  }
  // Report
  fprintf(fp,"Weight distribution\n");
  fprintf(fp,"-------------------\n\n");
  fprintf(fp,"Suspension Z distance with respect to center of gravity (CG):\n");
  fprintf(fp,"Front: %.2fm, rear: %.2fm\n",zf,zr);
  fprintf(fp,"\nResulting in weight distribution:\n");
  fprintf(fp,"Front %.2f%%, rear %.2f%%.\n",(zr/L)*100.0,(zf/L)*100.0);
  fprintf(fp,"\n");
  for(i=0;i<wheels;i++)
  {
    fprintf(fp,"Wheel %d: %.2fN = %.2f%%\n",i+1,weight[i],
      weightRatio[i]*100.0);
  }
  fprintf(fp,"\n");
 
  // Inertia
  //
  //size=*car->GetBody()->GetOBB()->GetExtents();
  size=*car->GetBody()->GetSize();
  t=1.0/12.0;
  Ixx=t*m*(size.y*size.y+size.z*size.z);
  Iyy=t*m*(size.x*size.x+size.z*size.z);
  Izz=t*m*(size.y*size.y+size.x*size.x);
  fprintf(fp,"Estimated inertia\n");
  fprintf(fp,"-----------------\n\n");
  fprintf(fp,"Based on a bounding box of width %.2f, height %.2f,"
    " depth %.2f,\n",
    size.x,size.y,size.z);
  fprintf(fp,"and mass %.2f, estimated start inertia values can be:\n",m);
  fprintf(fp,"  Ixx=%.2f, Iyy=%.2f, Izz=%.2f\n",Ixx,Iyy,Izz);
  fprintf(fp,"Current values are:\n");
  fprintf(fp,"  Ixx=%.2f, Iyy=%.2f, Izz=%.2f\n",
    tMat->GetRC(0,0),tMat->GetRC(1,1),tMat->GetRC(2,2));
  fprintf(fp,"These can be filled in for the body (body.inertia.x/y/z).\n");
  fprintf(fp,"\n");

  // Wheel rates
  //
  fprintf(fp,"Wheel rates\n");
  fprintf(fp,"-----------\n\n");
  fprintf(fp,"Suggested wheel rates are calculated based on a deflection\n");
  fprintf(fp,"of 1/3rd of the radius of the wheel at each suspension.\n\n");
  for(i=0;i<wheels;i++)
  {
    // Advise spring so that suspension sinks in to 1/3 of the wheel radius
    // (estimation taken from FastCar)
    // Calculate desired suspension deflection
    defl=car->GetWheel(i)->GetRadius()/3.0;
    wheelRate[i]=weight[i]/defl;
    fprintf(fp,"Wheel %d: desired deflection %.2f -> wheel rate %.2f N/m;"
      " current rate: %.2f N/m\n",
      i+1,defl,wheelRate[i],car->GetSuspension(i)->GetK());
  }

  // Suspension frequencies
  //
  fprintf(fp,"\nSuspension frequencies\n");
  fprintf(fp,"----------------------\n\n");
  fprintf(fp,"Advised is to have different front and rear suspension\n");
  fprintf(fp,"frequencies to avoid swinging motions.\n");
  for(i=0;i<wheels;i++)
  {
#ifdef ND_BAD
    natFreq[i]=sqrt(car->GetSuspension(i)->GetK()/car->GetWheel(i)->GetMass())/
      (2*PI);
#endif
    natFreq[i]=sqrt(car->GetSuspension(i)->GetK()/weight[i])/(2*PI);
    fprintf(fp,"Wheel %d: undamped frequency: %.2f Hz\n",
      i+1,natFreq[i]);
  }

  // Dampers
  //
  fprintf(fp,"\nDampers\n");
  fprintf(fp,"-------\n\n");
  fprintf(fp,"Road cars typically have critical damping (=1).\n");
  fprintf(fp,"Advised is not to use more than 1.5 critical damping,\n");
  fprintf(fp,"or even stay between 0.5 and 1.\n\n");
  for(i=0;i<wheels;i++)
  {
    // Calculate natural frequency WITH damper applied
    // Rebound
#ifdef ND_BAD
    dampAlpha[i]=car->GetSuspension(i)->GetReboundRate()/
      (2*car->GetWheel(i)->GetMass());
#endif
    dampAlpha[i]=car->GetSuspension(i)->GetReboundRate()/(2*weight[i]);
    natFreqDampRebound[i]=
      sqrt(natFreq[i]*natFreq[i]-dampAlpha[i]*dampAlpha[i]);
    // Bump
    dampAlpha[i]=car->GetSuspension(i)->GetBumpRate()/(2*weight[i]);
#ifdef ND_BAD
    dampAlpha[i]=car->GetSuspension(i)->GetBumpRate()/
      (2*car->GetWheel(i)->GetMass());
#endif
    natFreqDampBump[i]=
      sqrt(natFreq[i]*natFreq[i]-dampAlpha[i]*dampAlpha[i]);

    // Damping coefficient; at this damping rate, you get 1.0 critical damping
    dampingCoeffBump[i]=2*sqrt(car->GetSuspension(i)->GetBumpRate()*
      weight[i]);
      //car->GetWheel(i)->GetMass());
    dampingCoeffRebound[i]=2*sqrt(car->GetSuspension(i)->GetReboundRate()*
      weight[i]);
      //car->GetWheel(i)->GetMass());

    // Damping ratio (how far near critical damping)
    dampingRatioBump[i]=car->GetSuspension(i)->GetBumpRate()/
      dampingCoeffBump[i];
    dampingRatioRebound[i]=car->GetSuspension(i)->GetReboundRate()/
      dampingCoeffRebound[i];
  }
  fprintf(fp,"\nBump (compressing):\n\n");
  for(i=0;i<wheels;i++)
  {
#ifdef OBS
    fprintf(fp,"Wheel %d: damped frequency: %.2f Hz, "
      "damping ratio: %.2f, suggested damper rate: %.2f N/m/s\n",
      i+1,natFreqDampBump[i],dampingRatioBump[i],dampingCoeffBump[i]);
#endif
    fprintf(fp,"Wheel %d: damping ratio: %.2f,"
      " suggested damper rate: %.2f N/m/s\n",
      i+1,dampingRatioBump[i],dampingCoeffBump[i]);
  }
  fprintf(fp,"\nRebound (decompressing):\n\n");
  for(i=0;i<wheels;i++)
  {
#ifdef OBS
    fprintf(fp,"Wheel %d: damped frequency: %.2f Hz, "
      "damping ratio: %.2f, suggested damper rate: %.2f N/m/s\n",
      i+1,natFreqDampRebound[i],dampingRatioRebound[i],dampingCoeffRebound[i]);
#endif
    fprintf(fp,"Wheel %d: "
      "damping ratio: %.2f, suggested damper rate: %.2f N/m/s\n",
      i+1,dampingRatioRebound[i],dampingCoeffRebound[i]);
  }
  fprintf(fp,"\n");

  // Close file and display
 fail:
  fclose(fp);

  if(display)
  {
qdbg("QDlgViewFile the report\n");
    QDlgViewFile(REPORT_FNAME,repLines,"Report",
      "Carlab Analysis",&repDefaultView);
  }
}

