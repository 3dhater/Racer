/*
 * Pacejka Player - visually checking Pacejka curves
 * 02-04-01: Created!
 * 16-10-01: Picked up the project again for some better editing.
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <d3/global.h>
#include <racer/pacejka.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DRW        Q_BC

#define CAR_FNAME  "data/cars/devtest/car.ini"
#define CONTROLS_FILE   "controls.ini"

#define DEBUG_INI  "debug.ini"

#define MAX_GRAPH   3
// (Graph::MAX_PARAM)

// Pacejka vars
RPacejka pacejka;

// Modeler vars

// Menu
enum { MAX_IO=4 };
cstring  ioName[MAX_IO]={ "Open car","Revert","Save*","---" };
QButton *butIO[MAX_IO];
enum { MAX_MODIFY=1 };
cstring  modifyName[MAX_MODIFY]=
{ "Enter numerically"
};
QButton *butModify[MAX_MODIFY];
QProp *pInput[4];                    // Input parameters
QRadio *rFocus[4];                   // Focus on Fx/Fy/Mz or all
QRadio *rCoeff[18];                  // Coeff to modify
QCheck *cZoom;                       // Zoom functions?
QProp *pCoeff;
extern cstring appTitle;
extern QTitleBar *title;

// Graphics
DGlobal dglobal;
DGeode *model;
DBoundingBox *bbox;

// Pacejka editing
enum Focus
{
  FOCUS_NONE,
  FOCUS_FX,
  FOCUS_FY,
  FOCUS_MZ
};
Graph *graph[MAX_GRAPH];
int    curFocus=FOCUS_NONE;
int    curCoeff;
bool   fZoom;
rfloat *coeffPtr;                    // Address of current coefficient
rfloat  coeffScale;                  // Maximum editing range
rfloat  coeffBase;                   // Base value
qstring carName;                     // Last know car name

// Misc
char buf[256];

// Errors
QMessageHandler defQErr;

// Proto
static void DisplayCoeffSliderValue(float v);
void SetDefaults();

void exitfunc()
{
  int i;

  // Write settings
  info->SetString("last.car",carName);
  info->Write();
  QDELETE(info);

  // Free resources
  for(i=0;i<MAX_IO;i++)delete butIO[i];
  for(i=0;i<MAX_MODIFY;i++)delete butModify[i];
  if(model)delete model;
}

/********
* MENUS *
********/
void SetZoom()
// Based on 'fZoom', set the graph ranges
{
  if(fZoom)
  {
    graph[Graph::PARAM_FX]->minX=-2;
    graph[Graph::PARAM_FX]->maxX=2;
    graph[Graph::PARAM_FY]->minX=-20;
    graph[Graph::PARAM_FY]->maxX=20;
    graph[Graph::PARAM_MZ]->minX=-20;
    graph[Graph::PARAM_MZ]->maxX=20;
  } else
  {
    graph[Graph::PARAM_FX]->minX=-10;
    graph[Graph::PARAM_FX]->maxX=10;
    graph[Graph::PARAM_FY]->minX=-180;
    graph[Graph::PARAM_FY]->maxX=180;
    graph[Graph::PARAM_MZ]->minX=-180;
    graph[Graph::PARAM_MZ]->maxX=180;
  }
}
void SetFocus()
// Get graph to the front
{
  int i;

  for(i=0;i<MAX_GRAPH;i++)
    graph[i]->flags&=~Graph::SHOW_COEFF;
  for(i=0;i<MAX_GRAPH;i++)
  {
    if(curFocus==FOCUS_NONE||curFocus==i+1)
      graph[i]->flags&=~Graph::NO_FOCUS;
    else
      graph[i]->flags|=Graph::NO_FOCUS;
  }
  if(curFocus!=FOCUS_NONE)
    graph[curFocus-1]->flags|=Graph::SHOW_COEFF;
else graph[0]->flags|=Graph::SHOW_COEFF;
}
void SetCoeffNames()
// Based on 'curFocus', set the coefficient names
{
  int i;
  cstring s;
  cstring coeffNameFx[18]=
  { "b0","b1","b2","b3","b4","b5","b6","b7","b8","b9","b10",0,0,0,0,0,0,0 };
  cstring coeffNameFy[18]=
  { "a0","a1","a2","a3","a4","a5","a6","a7","a8","a9","a10","a111","a112",
    "a12","a13",0,0,0
  };
  cstring coeffNameMz[18]=
  { "c0","c1","c2","c3","c4","c5","c6","c7","c8","c9","c10",
    "c11","c12","c13","c14","c15","c16","c17"
  };

  for(i=0;i<18;i++)
  {
    switch(curFocus)
    {
      case FOCUS_NONE: case FOCUS_FX: s=coeffNameFx[i]; break;
      case FOCUS_FY: s=coeffNameFy[i]; break;
      case FOCUS_MZ: s=coeffNameMz[i]; break;
    }
    if(!s)s="---";
    rCoeff[i]->SetText((char*)s);
    rCoeff[i]->Invalidate();
  }
}
void SetEditCoeff()
// Determine which coefficient to edit
{
  Graph *g;
  float  scale,*p,base;

  switch(curFocus)
  {
    case FOCUS_NONE: case FOCUS_FX:
      g=graph[0];
      scale=10;
      base=0;
      switch(curCoeff)
      { case 0: p=&g->pacejka->b0; scale=2; base=0; break;   // OK
        case 1: p=&g->pacejka->b1; scale=50; base=-scale/2; break;
        case 2: p=&g->pacejka->b2; scale=3000; break;         // OK
        case 3: p=&g->pacejka->b3; scale=2; base=-scale/2; break;
        case 4: p=&g->pacejka->b4; scale=500; base=0; break;  // OK
        case 5: p=&g->pacejka->b5; scale=2; base=-scale/2; break;
        case 6: p=&g->pacejka->b6; scale=2; base=-scale/2; break;
        case 7: p=&g->pacejka->b7; scale=2; base=-scale/2; break;
        case 8: p=&g->pacejka->b8; scale=2; base=-scale/2; break;
        case 9: p=&g->pacejka->b9; scale=2; base=-scale/2; break;
        case 10: p=&g->pacejka->b10; scale=2; base=-scale/2; break;
      }
      break;
    case FOCUS_FY:
      g=graph[1];
      scale=10;
      base=0;
      switch(curCoeff)
      {
        case 0: p=&g->pacejka->a0; scale=2; base=0; break;
        case 1: p=&g->pacejka->a1; scale=100; base=-scale/2; break;
        case 2: p=&g->pacejka->a2; scale=3000; base=0; break;
        case 3: p=&g->pacejka->a3; scale=3000; base=0; break;
        case 4: p=&g->pacejka->a4; scale=50; base=0; break;
        case 5: p=&g->pacejka->a5; scale=2; base=-scale/2; break;
        case 6: p=&g->pacejka->a6; scale=5; base=-scale/2; break;
        case 7: p=&g->pacejka->a7; scale=5; base=-scale/2; break;
        case 8: p=&g->pacejka->a8; scale=2; base=-scale/2; break;
        case 9: p=&g->pacejka->a9; scale=2; base=-scale/2; break;
        case 10: p=&g->pacejka->a10; scale=2; base=-scale/2; break;
        case 111: p=&g->pacejka->a111; scale=30; base=-scale/2; break;
        case 112: p=&g->pacejka->a112; scale=2; base=-scale/2; break;
        case 12: p=&g->pacejka->a12; scale=20; base=-scale/2; break;
        case 13: p=&g->pacejka->a13; scale=100; base=0; break;
      }
      break;
    case FOCUS_MZ:
      g=graph[2];
      scale=10;
      base=0;
      switch(curCoeff)
      {
        case 0: p=&g->pacejka->c0; scale=5; base=-scale/2; break;
        case 1: p=&g->pacejka->c1; scale=10; base=-scale/2; break;
        case 2: p=&g->pacejka->c2; scale=30; base=-scale/2; break;
        case 3: p=&g->pacejka->c3; scale=3; base=-scale/2; break;
        case 4: p=&g->pacejka->c4; scale=20; base=-scale/2; break;
        case 5: p=&g->pacejka->c5; scale=2; base=-scale/2; break;
        case 6: p=&g->pacejka->c6; scale=2; base=-scale/2; break;
        case 7: p=&g->pacejka->c7; scale=2; base=-scale/2; break;
        case 8: p=&g->pacejka->c8; scale=15; base=-scale/2; break;
        case 9: p=&g->pacejka->c9; scale=50; base=-scale/2; break;
        case 10: p=&g->pacejka->c10; scale=2; base=-scale/2; break;
        case 11: p=&g->pacejka->c11; scale=1; base=-scale/2; break;
        case 12: p=&g->pacejka->c12; scale=1; base=-scale/2; break;
        case 13: p=&g->pacejka->c13; scale=2; base=-scale/2; break;
        case 14: p=&g->pacejka->c14; scale=2; base=-scale/2; break;
        case 15: p=&g->pacejka->c15; scale=5; base=-scale/2; break;
        case 16: p=&g->pacejka->c16; scale=3; base=-scale/2; break;
        case 17: p=&g->pacejka->c17; scale=5; base=-scale/2; break;
      }
      break;
    default:
      p=0; break;
  }
  // Copy over coefficient value pointer and scaling
  coeffPtr=p;
  coeffScale=scale;
  coeffBase=base;
  if(coeffPtr)
  {
    // Reflect value in pCoeff (the slider)
    pCoeff->SetPosition(1000.0*((*coeffPtr-coeffBase)/coeffScale));
    DisplayCoeffSliderValue(*coeffPtr);
  }
}
void SetupMenus()
{
  QRect r;
  int   i;
  
  // IO
  r.x=Q_BC->GetX()-2; r.y=Q_BC->GetHeight()+Q_BC->GetY()+10;
  r.wid=150; r.hgt=35;
  for(i=0;i<MAX_IO;i++)
  {
    butIO[i]=new QButton(QSHELL,&r,ioName[i]);
    r.x+=r.wid+10;
  }
  
  // Focus radio buttons
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10; r.y=Q_BC->GetY()-2;
  r.wid=150; r.hgt=16;
  cstring focusName[4]={ "No focus","Focus Fx","Focus Fy","Focus Mz" };
  for(i=0;i<4;i++)
  {
    rFocus[i]=new QRadio(QSHELL,&r,(string)focusName[i],1000);
    r.y+=r.hgt+2;
  }
  rFocus[0]->SetState(TRUE);

  // Zoom?
  cZoom=new QCheck(QSHELL,&r,"Zoom");
  r.y+=r.hgt+2;

  // Coefficient radio buttons
  for(i=0;i<18;i++)
  {
    rCoeff[i]=new QRadio(QSHELL,&r,"coeff",1001);
    r.y+=r.hgt+2;
  }
  rCoeff[0]->SetState(TRUE);
  SetCoeffNames();

  // Coefficient slider
  pCoeff=new QProp(QSHELL,&r);
  pCoeff->SetRange(0,1000);
  pCoeff->SetDisplayed(5,10);
  pCoeff->SetJump(1);
  r.y+=r.hgt+2;

  // Modify
  //r.x=Q_BC->GetX()+Q_BC->GetWidth()+10; r.y=Q_BC->GetY()-2;
  r.wid=150; r.hgt=30;
  for(i=0;i<MAX_MODIFY;i++)
  {
    butModify[i]=new QButton(QSHELL,&r,modifyName[i]);
    r.y+=r.hgt+5;
  }

  // Input parameters
  cstring sInput[]=
  { "Load","Slip ratio","Slip angle","Camber" };
  int rangeMin[]={ 0,-10*100,-180,-20*100 };
  int rangeMax[]={ 5000,10*100,180,20*100 };
  r.x=32-2;
  r.y=Q_BC->GetHeight()+Q_BC->GetY()+20+40;
  r.wid=Q_BC->GetWidth()+4;
  r.hgt=20;
  for(i=0;i<4;i++)
  {
    pInput[i]=new QProp(QSHELL,&r);
    // All controls get a range from 0..1000 which is later transformed
    //pInput[i]->SetRange(rangeMin[i],rangeMax[i]);
    pInput[i]->SetRange(0,1000);
    pInput[i]->SetDisplayed(3,10);
    //pInput[i]->SetPosition((rangeMin[i]+rangeMax[i])/2);
    pInput[i]->SetJump(1);
    // Value is shown implicitly
    //pInput[i]->EnableShowValue();
    r.y+=r.hgt+10;
  }
}

/*********
* ERRORS *
*********/
void apperr(string s)
{
  QRect r(100,100,500,150);
  QMessageBox("Error",s,0,&r);
  if(defQErr)defQErr(s);
}

/*********
* MODIFY *
*********/

/*********
* EVENTS *
*********/
static void MapXYtoCtl(int x,int y)
// Convert X/Y coords to controller input
{
  x-=DRW->GetWidth()/2;
  y-=DRW->GetHeight()/2;
}

static void SetLoad(float v)
{
  int j;
//qdbg("SetLoad(%f)\n",v);
  for(j=0;j<MAX_GRAPH;j++)
  {
    graph[j]->pacejka->SetNormalForce(v);
  }
  sprintf(buf,"Load %.fN",v);
  pInput[0]->SetText(buf);
}
static void SetSlipRatio(float v)
{
  int j;

  //qdbg("Slip ratio %.2f\n",v);
  for(j=0;j<MAX_GRAPH;j++)
  {
    graph[j]->pacejka->SetSlipRatio(v);
  }
  sprintf(buf,"Slipratio %.2f",v);
  pInput[1]->SetText(buf);
}
static void SetSlipAngle(float v)
{
  int j;

//qdbg("Slip angle %.2f\n",v);
  for(j=0;j<MAX_GRAPH;j++)
  {
    graph[j]->pacejka->SetSlipAngle(v);
  }
  sprintf(buf,"Slipangle %.2f",v);
  pInput[2]->SetText(buf);
}
static void SetCamber(float v)
{
  int j;
//qdbg("Camber %.2f\n",v);
  for(j=0;j<MAX_GRAPH;j++)
  {
    graph[j]->pacejka->SetCamber(v/RR_RAD2DEG);
  }
  sprintf(buf,"Camber %.2f",v);
  pInput[3]->SetText(buf);
}
static void DisplayCoeffSliderValue(float v)
// Show the coefficient value in the coeff slider
{
  sprintf(buf,"%.4f",v);
  pCoeff->SetText(buf);
  pCoeff->Invalidate();
}
static void SetTitle()
// Display car name and application title in the titlebar
{
  sprintf(buf,"%s - %s",appTitle,carName.cstr());
  title->SetTitle(buf);
  title->Invalidate();
}
static void SelectCar()
{
  char buf[80];
  int  i;

  strcpy(buf,carName);
  if(!QDlgString("Select car","Enter the car name",buf,sizeof(buf)))
    return;
  // Remember car name
  carName=buf;
  SetTitle();
  for(i=0;i<MAX_GRAPH;i++)
    graph[i]->LoadFrom(carName);

  SetDefaults();
}
static void Revert()
{
  int i;

  if(QMessageBox("Revert",
    "Are you sure you want to revert to the last saved Pacejka set?")!=IDOK)
    return;
qdbg("Reverting\n");
  for(i=0;i<MAX_GRAPH;i++)
    graph[i]->LoadFrom(carName);
}
static void NumericEntry()
{
  char buf[80];
  int  i;

  if(!coeffPtr)return;

  sprintf(buf,"%f",*coeffPtr);
  if(!QDlgString("Enter numerical","Enter the value",buf,sizeof(buf)))
    return;
  *coeffPtr=atof(buf);
  // Get slider corrected
  DisplayCoeffSliderValue(*coeffPtr);
  SetEditCoeff();
}
bool event(QEvent *e)
{
  int i,j;

  if(e->type==QEvent::CLICK||e->type==QEvent::CHANGE)
  {
    for(i=0;i<4;i++)
    {
      if(e->win==pInput[i])
      {
        int n;
        float v;

        n=e->n;
        pInput[i]->SetPosition(n);
        pInput[i]->Paint();
        // Pass parameter to graph input
        if(i==0)
        {
          SetLoad(n*10);
        } else if(i==1)
        {
          SetSlipRatio((float)n/100.0-5.0f);
        } else if(i==2)
        {
          SetSlipAngle((float)n/5.0-100.0f);
        } else if(i==3)
        {
          SetCamber((float)n/50.0-10.0f);
        }
      }
    }
    for(i=0;i<4;i++)
    {
      if(e->win==rFocus[i])
      {
        curFocus=i;
        curCoeff=0;
        SetCoeffNames();
        SetFocus();
        // Select the first coefficient
        SetEditCoeff();
        for(i=0;i<18;i++)rCoeff[i]->SetState(FALSE);
        rCoeff[curCoeff]->SetState(TRUE);
        return TRUE;
      }
    }
    // Coefficient selection?
    for(i=0;i<18;i++)
    {
      if(e->win==rCoeff[i])
      {
        curCoeff=i;
        // Determine effects of this selection
        SetEditCoeff();
      }
    }
    // Zoom in?
    if(e->win==cZoom)
    {
      fZoom=cZoom->GetState();
      SetZoom();
      return TRUE;
    }
    // Coeff edit?
    if(e->win==pCoeff)
    {
      if(coeffPtr)
      {
        // Calculate slider position back to value
        rfloat v;
        v=pCoeff->GetPosition();
        v=v/(1000.0/coeffScale)+coeffBase;
        *coeffPtr=v;
        DisplayCoeffSliderValue(v);
      }
    }
    for(i=0;i<MAX_IO;i++)
    {
      if(e->win==butIO[i])
      {
        if(i==0)
        { SelectCar();
          return TRUE;
        } else if(i==1)
        { Revert();
          return TRUE;
        }
      }
    }
    for(i=0;i<MAX_MODIFY;i++)
    {
      if(e->win==butModify[i])
      {
        if(i==0)
        { NumericEntry();
        }
      }
    }
  }
  if(e->type==QEvent::MOTIONNOTIFY)
  {
  } else if(e->type==QEvent::KEYPRESS)
  {
    if(e->n==QK_ESC)
      app->Exit(0);
  }

  return FALSE;
}

/***********
* HANDLING *
***********/
void SetupViewport(int w,int h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(65.0,(GLfloat)w/(GLfloat)h,1.0,1000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

QDraw *GGetDrawable()
{
  return DRW;
}

QCanvas *GGetCV()
{
  return DRW->GetCanvas();
}
void GSwap()
{ DRW->Swap();
}

void SetDefaults()
{
  // Set default slider positions
  pInput[0]->SetPosition(250);
  SetLoad(2500);
  pInput[1]->SetPosition(500);
  SetSlipRatio(0);
  pInput[2]->SetPosition(500);
  SetSlipAngle(0);
  pInput[3]->SetPosition(500);
  SetCamber(0);

  cZoom->SetState(TRUE);
  fZoom=TRUE;
  SetZoom();

  curFocus=FOCUS_NONE;
  SetFocus();

  curCoeff=0;
  SetEditCoeff();
}

void idlefunc()
{
  QVector3 *v;
  int j;
  
//qdbg("---\n");

#ifdef WIN32
  // Task swapping grinds to a halt on Win32, duh!
  QNap(1);
#endif

  // Don't refresh while window is disabled. This happens
  // when a dialog gets on top, and redrawing doesn't work
  // correctly on a lot of buggy drivers on Win98.
  if(!Q_BC->IsEnabled())return;

  GGetCV()->Select();
  SetupViewport(DRW->GetWidth(),DRW->GetHeight());
  //GGetCV()->SetFont(app->GetSystemFont());
  GGetCV()->Set2D();
  GGetCV()->Set3D();
  GGetCV()->Set2D();
  
  glClearColor(.8,.8,.8,0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 
  for(j=0;j<MAX_GRAPH;j++)
  {
#ifdef ND_DRAW_ALL
    // Don't draw if we're focusing one 1 graph
    if(curFocus!=FOCUS_NONE)
    {
      if(curFocus!=j+1)
        continue;
    }
#endif
    graph[j]->CalculatePoints();
    graph[j]->Paint();
  }
  // Redraw focus graph for depth ordering
  if(curFocus!=FOCUS_NONE)
    graph[curFocus-1]->Paint();
  
  GSwap();
}

void Setup()
{
  float v;
  int   j;
  
  defQErr=QSetErrorHandler(apperr);

  // GUI
  SetupMenus();

  // Get window up
  app->RunPoll();
  app->SetIdleProc(idlefunc);
  app->SetExitProc(exitfunc);
  app->SetEventProc(event);
  
  // Graphics
  bbox=new DBoundingBox();
  
  // Get last known car
  info->GetString("last.car",carName);
qdbg("last car='%s'\n",carName.cstr());
  SetTitle();

  // Data
  for(j=0;j<MAX_GRAPH;j++)
  {
    graph[j]=new Graph(j);
    graph[j]->LoadFrom(carName);
  }
  SetZoom();
  SetDefaults();
}

void Run()
{
  Setup();
  app->Run();
}
