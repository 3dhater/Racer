/*
 * Carlab - Modifying
 * 28-10-01: Created!
 * (c) Dolphinity/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#define REPORT_FNAME   "replab.txt"

static QRect repDefaultView(50,50,900,615);        // Default report view
static int   repLines=28;

void ModifyWeightRatio()
// Modifying the weight ratio is quite a heavy operation, but automated
// because it's such a nuisance to do correctly by hand.
{
  FILE *fp;
  int   i,r;
  rfloat pf,pr,zfNew,zrNew,shiftZ;
  char   buf[256],buf2[256];

  // Get current weight ratio
  Analyse(FALSE);

  // Request desired weight ratio
  sprintf(buf,"%.2f",(zr/L)*100.0);
  if(!QDlgString("Modify weight ratio","Enter desired FRONT weight ratio (%)",
    buf,sizeof(buf)))return;
  pf=atof(buf);

  // Report changes to be made to make this happen
  //
  fp=fopen(REPORT_FNAME,"w");
  if(!fp)
  {
    QMessageBox("Error","Can't open report file");
    return;
  }
  fprintf(fp,"Weight ratio shifting for car '%s'\n",car->GetName());
  fprintf(fp,"\n");

  // Weight ratio
  fprintf(fp,"Current suspension Z distances with respect to the"
    " center of gravity (CG):\n");
  fprintf(fp,"Front: %.2fm, rear: %.2fm\n",zf,zr);
  fprintf(fp,"\nCurrent weight distribution: front %.2f%%, rear %.2f%%.\n",
    (zr/L)*100.0,(zf/L)*100.0);
  fprintf(fp,"\n");
  fprintf(fp,"To get to the preferred %.2f to %.2f weight ratio, "
    "the following needs changing:\n",pf,100.0f-pf);
  zfNew=((100.0-pf)*L)/100.0;
  zrNew=(pf*L)/100.0;
  fprintf(fp,"- new suspension locations will move to Z=%.2f (front)"
    " and Z=%.2f rear.\n",zfNew,zrNew);
  shiftZ=zfNew-zf;
  fprintf(fp,"- body model (body.dof) will be translated by Z=%.2f\n",shiftZ);
  fprintf(fp,"  to accomodate for the new Z location of the"
    " center of gravity.\n");
 
  fprintf(fp,"\nIf the change is permanent, it may be a good idea to shift\n");
  fprintf(fp,"the original model itself to avoid having to rerun this\n");
  fprintf(fp,"operation for every re-export of the body model.\n");
  fprintf(fp,"\nPossibly you may also have to adjust the car's shadow.\n");
  fprintf(fp,"\nIt is advised to create a backup of the car.ini and\n");
  fprintf(fp,"body.dof (body model) files before clicking OK after\n");
  fprintf(fp,"closing this report.\n");
  fprintf(fp,"\n");

  // Close file and display
 fail:
  fclose(fp);

  QDlgViewFile(REPORT_FNAME,repLines,"Weight ratio",
    "Weight ratio change analysis",&repDefaultView);

  // Request to perform action
  if(QMessageBox("Modify weight ratio",
    "Do you want to apply these changes?")!=IDOK)return;

  // Modify car.ini's susp<x>.z locations
  for(i=0;i<car->GetWheels();i++)
  {
    car->GetSuspension(i)->GetPosition()->z+=shiftZ;
    sprintf(buf,"susp%d.z",i);
    sprintf(buf2,"%f",car->GetSuspension(i)->GetPosition()->z);
qdbg("%s=%s\n",buf,buf2);
    car->GetInfo()->SetString(buf,buf2);
  }
  car->GetInfo()->Write();

  // Translate body model
  car->GetBody()->GetModel()->GetGeode()->Translate(0,0,shiftZ);
  car->GetBody()->GetModel()->Save();

  QMessageBox("Modify weight ratio","Model and car.ini modified. Reload car.");
}

