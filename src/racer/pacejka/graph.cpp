/*
 * Graph
 * 02-04-2001: Created! (20:36:45)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "Graph"

/*******
* Ctor *
*******/
Graph::Graph(int _param)
  : param(_param)
{
  col=0;
  wid=580;
  hgt=380;
  steps=wid/2;
  offX=wid/2;
  offY=hgt/2;
  flags=0;
  // Default min/max axes values for parameters
  switch(param)
  {
    case PARAM_FX: minX=-10; maxX=10; minY=-5000; maxY=5000;
      col=new QColor(0,0,0);
      break;
    case PARAM_FY: minX=-180; maxX=180; minY=-5000; maxY=5000;
      col=new QColor(255,0,0);
      break;
    case PARAM_MZ: minX=-180; maxX=180; minY=-300; maxY=300;
      col=new QColor(0,128,0);
      break;
    default: minX=-180; maxX=180; minY=-2000; maxY=2000;
      col=new QColor(0,0,0);
      break;
  }
  xName="X-axis";
  yName="Y-axis";
  scaleX=((float)wid)/(maxX-minX);
  scaleY=((float)hgt)/(maxY-minY);
  
  pacejka=new RPacejka();
  // Default input
  pacejka->SetCamber(0);
  pacejka->SetSlipRatio(0);
  pacejka->SetSlipAngle(0);
  pacejka->SetNormalForce(1000);
}
Graph::~Graph()
{
  QDELETE(col);
  QDELETE(pacejka);
}

void Graph::LoadFrom(cstring carName)
// Reads the Pacejka constants from wheel0 from 'carName'
// Separated from the ctor so you can load any car's data
{
  char buf[256];

  sprintf(buf,"data/cars/%s",carName);

  QInfo info(RFindFile("car.ini",buf));
  pacejka->Load(&info,"wheel0.pacejka");
}

/********
* Units *
********/
cstring Graph::GetHorizontalUnit()
{
  switch(param)
  {
    case PARAM_FX: return "SR";
    case PARAM_FY: return "SA";
    case PARAM_MZ: return "SA";
    default: return "?";
  }
}
cstring Graph::GetVerticalUnit()
{
  switch(param)
  {
    case PARAM_FX: return "N";
    case PARAM_FY: return "N";
    case PARAM_MZ: return "Nm";
    default: return "?";
  }
}

/********
* Paint *
********/
static void SetGLColor(QColor *color)
// Local function to convert rgba to float
{
  GLfloat cr,cg,cb,ca;
  cr=(GLfloat)color->GetR()/255;
  cg=(GLfloat)color->GetG()/255;
  cb=(GLfloat)color->GetB()/255;
  ca=(GLfloat)color->GetA()/255;
  glColor4f(cr,cg,cb,ca);
}
#define ShowVal(v,s)\
 { char buf[100]; sprintf(buf,"%s=%.5f",(s),(v)); QCV->Text(buf,cx,cy); }

void Graph::Paint()
{
  int i,x,y,subdivs,tackHgt=2,dy=16;
  int cx,cy;
  char buf[100];
  
  // Center of graph
  cx=Q_BC->GetWidth()/2;
  cy=Q_BC->GetHeight()/2;
  
  // Axes
  glColor3f(.5,.5,.5);
  subdivs=10;
  glBegin(GL_LINES);
    glVertex2f(cx-wid/2,cy);
    glVertex2f(cx+wid/2,cy);
    glVertex2f(cx,cy-hgt/2);
    glVertex2f(cx,cy+hgt/2);
    // Tacks
    for(i=0;i<=subdivs;i++)
    {
      glVertex2f(cx-(wid/2)*i/subdivs,cy-tackHgt);
      glVertex2f(cx-(wid/2)*i/subdivs,cy+tackHgt+1);
      glVertex2f(cx+(wid/2)*i/subdivs,cy-tackHgt);
      glVertex2f(cx+(wid/2)*i/subdivs,cy+tackHgt+1);
    }
  glEnd();
  
  // Values
  //glColor3f(0,0,0);
  if(flags&NO_FOCUS)
  { QColor grayish(190,190,190);
    QCV->SetColor(&grayish);
    SetGLColor(&grayish);
  } else
  {
    QCV->SetColor(col);
    SetGLColor(col);
  }
  glBegin(GL_LINE_STRIP);
  for(i=0;i<steps;i++)
  {
    x=i*wid/(steps-1)-wid/2;
    y=v[i]*scaleY;
//qdbg("step %d; (%d,%d)\n",i,x,y);
    glVertex2f(x+cx,y+cy);
  }
  glEnd();
  
  // Extents
  //QCV->SetColor(0,0,0);
  QCV->SetFont(app->GetSystemFont());
  sprintf(buf,"%.1f %s",minX,GetHorizontalUnit());
  QCV->Text(buf,cx-wid/2,cy+param*dy);
  sprintf(buf,"%.1f %s",maxX,GetHorizontalUnit());
  QCV->Text(buf,cx+wid/2,cy+param*dy);
  sprintf(buf,"%.1f %s",minY,GetVerticalUnit());
  QCV->Text(buf,cx,cy+hgt/2+param*dy);
  sprintf(buf,"%.1f %s",maxY,GetVerticalUnit());
  QCV->Text(buf,cx,cy-hgt/2-param*dy);

  // Legend
  cx=50; cy=50+param*dy;
  switch(param)
  {
    case PARAM_FX: QCV->Text("Fx (longitudinal force)",cx,cy); break;
    case PARAM_FY: QCV->Text("Fy (lateral force)",cx,cy); break;
    case PARAM_MZ: QCV->Text("Mz (aligning moment)",cx,cy); break;
  }

  if(flags&SHOW_COEFF)
  {
    // Display all coefficients
    int dx,dy,cy0;
    cx=wid-100;
    cy0=hgt-50; cy=cy0;
    dx=100; dy=16;
    switch(param)
    {
      case PARAM_FX:
        ShowVal(pacejka->b0,"b0"); cy+=dy;
        ShowVal(pacejka->b1,"b1"); cy+=dy;
        ShowVal(pacejka->b2,"b2"); cy+=dy;
        ShowVal(pacejka->b3,"b3"); cy+=dy;
        ShowVal(pacejka->b4,"b4"); cy+=dy;
        ShowVal(pacejka->b5,"b5"); cy+=dy;
        ShowVal(pacejka->b6,"b6"); cy+=dy;
        ShowVal(pacejka->b7,"b7"); cy+=dy; cx+=dx; cy=cy0;
        ShowVal(pacejka->b8,"b8"); cy+=dy;
        ShowVal(pacejka->b9,"b9"); cy+=dy;
        ShowVal(pacejka->b10,"b10"); cy+=dy;
        break;
      case PARAM_FY:
        ShowVal(pacejka->a0,"a0"); cy+=dy;
        ShowVal(pacejka->a1,"a1"); cy+=dy;
        ShowVal(pacejka->a2,"a2"); cy+=dy;
        ShowVal(pacejka->a3,"a3"); cy+=dy;
        ShowVal(pacejka->a4,"a4"); cy+=dy;
        ShowVal(pacejka->a5,"a5"); cy+=dy;
        ShowVal(pacejka->a6,"a6"); cy+=dy;
        ShowVal(pacejka->a7,"a7"); cy+=dy; cx+=dx; cy=cy0;
        ShowVal(pacejka->a8,"a8"); cy+=dy;
        ShowVal(pacejka->a9,"a9"); cy+=dy;
        ShowVal(pacejka->a10,"a10"); cy+=dy;
        ShowVal(pacejka->a111,"a111"); cy+=dy;
        ShowVal(pacejka->a112,"a112"); cy+=dy;
        ShowVal(pacejka->a12,"a12"); cy+=dy;
        ShowVal(pacejka->a13,"a13"); cy+=dy;
        break;
      case PARAM_MZ:
        ShowVal(pacejka->c0,"c0"); cy+=dy;
        ShowVal(pacejka->c1,"c1"); cy+=dy;
        ShowVal(pacejka->c2,"c2"); cy+=dy;
        ShowVal(pacejka->c3,"c3"); cy+=dy;
        ShowVal(pacejka->c4,"c4"); cy+=dy;
        ShowVal(pacejka->c5,"c5"); cy+=dy;
        ShowVal(pacejka->c6,"c6"); cy+=dy;
        ShowVal(pacejka->c7,"c7"); cy+=dy;
        ShowVal(pacejka->c8,"c8"); cy+=dy;
        ShowVal(pacejka->c9,"c9"); cy+=dy; cx+=dx; cy=cy0;
        ShowVal(pacejka->c10,"c10"); cy+=dy;
        ShowVal(pacejka->c11,"c11"); cy+=dy;
        ShowVal(pacejka->c12,"c12"); cy+=dy;
        ShowVal(pacejka->c13,"c13"); cy+=dy;
        ShowVal(pacejka->c14,"c14"); cy+=dy;
        ShowVal(pacejka->c15,"c15"); cy+=dy;
        ShowVal(pacejka->c16,"c16"); cy+=dy;
        ShowVal(pacejka->c17,"c17"); cy+=dy;
        break;
    }
  }
}

/*********************
* Calculating values *
*********************/
void Graph::CalculatePoints()
{
  float x,y;
  int   i;
  
  for(i=0;i<steps;i++)
  {
    x=((float)i)*(maxX-minX)/((float)steps)+minX;
    switch(param)
    {
      case PARAM_FX:
        // X is slipratio
        pacejka->SetSlipRatio(x);
        pacejka->Calculate();
        y=pacejka->GetFx();
//qdbg("FX(%f)=%f\n",x,y);
//y=0;
        break;
      case PARAM_FY:
        // X is in degrees
        pacejka->SetSlipAngle(x/RR_RAD2DEG);
        pacejka->Calculate();
        y=pacejka->GetFy();
        break;
      case PARAM_MZ:
        // X is slipratio
        pacejka->SetSlipAngle(x/RR_RAD2DEG);
        pacejka->Calculate();
        y=pacejka->GetMz();
//qdbg("Mz step %d = %f\n",i,y);
        break;
      default:
        y=0;
    }
    v[i]=y;
#ifdef OBS
    v[1][i]=pacejka->GetFx();
    v[2][i]=pacejka->GetMz();
    v[3][i]=pacejka->GetLongitudinalStiffness();
    v[4][i]=pacejka->GetCorneringStiffness();
#endif
//qdbg("CP; x=%f -> Fy=%f\n",x,y);
  }
}
