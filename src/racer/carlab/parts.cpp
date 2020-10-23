/*
 * Parts
 * 06-12-2000: Created! (13:59:07)
 * NOTES:
 * - Parts are things like 'Body', 'Steer' etc.
 * - Elements are children of parts; 'Mass' etc.
 * (C) Dolphinity/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// Max #elements per part
#define MAX_ELT 10
// Max #operations per part
#define MAX_OP  10

// Offset for callback
#define OPBASE  1000

#define X 32
#define Y 512
#define BWID 100
#define BHGT 30
#define DX   (BWID+10)
#define DY   (BHGT+10)

static QButton *butPart[MAX_PART];
cstring partName[MAX_PART]=
{ "Car","Body","Steering","Engine","Gearbox","Wheels",
  "Suspensions","Cameras"
};
static QGroup *grpParts;
static QScrollBar *sbPart[MAX_PART];

static QButton *butElt[MAX_ELT];
static QGroup *grpElts;

static QButton *butOp[MAX_OP];
static QGroup *grpOps;

// Info references (from the car)
QInfo *iCar;

// Editing
int curPart;                 // Which part is active

// Protos for selecting parts
typedef void (*ELTSELECTFUNC)(int n);
static void GearSelect();
static void CamSelect();
static void BodySelect();
static void WheelSelect();
static void SuspSelect();
static void CarSelect();
static void SteerSelect();
static void EngineSelect();
ELTSELECTFUNC eltSelectFunc;

static void WheelHilite();
static void SuspUnhiliteAll();

/********
* Setup *
********/
void PartsSetup()
{
  int   i;
  int   x,y,dx,dy,bwid,bhgt,cols;
  QRect r,r2;
  QScrollBar *sb;
  
  // Get Info files
  iCar=car->GetInfo();
  
  bwid=100; bhgt=35;
  dx=bwid+10; dy=bhgt+5;
  x=48; y=560;
  cols=5;
  
  r.x=x-16-2; r.y=y-20;
  r.wid=640+4; r.hgt=200;
  /*grpParts=*/ new QGroup(QSHELL,&r,"Parts");
  
  // First parts are separate
  for(i=0;i<5;i++)
  {
    r.x=x+(i%cols)*dx;
    r.y=y+(i/cols)*dy;
    r.wid=bwid;
    r.hgt=bhgt;
    butPart[i]=new QButton(QSHELL,&r,partName[i]);
  }
  r.x=x; r.y=y+dy*1;
  butPart[PART_WHEEL]=new QButton(QSHELL,&r,partName[PART_WHEEL]);
  r2=r; r2.x+=dx; r2.y+=r.hgt/4; r2.wid*=2; r2.hgt/=2;
  sb=new QScrollBar(QSHELL,&r2,QScrollBar::HORIZONTAL);
  sb->GetProp()->SetRange(0,RCar::MAX_WHEEL-1);
  sb->GetProp()->SetDisplayed(2,RCar::MAX_WHEEL);
  sb->GetProp()->SetJump(1);
  sb->GetProp()->EnableShowValue();
  sbPart[PART_WHEEL]=sb;
  
  r.y+=dy;
  butPart[PART_SUSP]=new QButton(QSHELL,&r,partName[PART_SUSP]);
  r2=r; r2.x+=dx; r2.y+=r.hgt/4; r2.wid*=2; r2.hgt/=2;
  sb=new QScrollBar(QSHELL,&r2,QScrollBar::HORIZONTAL);
  sb->GetProp()->SetRange(0,RCar::MAX_WHEEL-1);
  sb->GetProp()->SetDisplayed(2,RCar::MAX_WHEEL);
  sb->GetProp()->SetJump(1);
  sb->GetProp()->EnableShowValue();
  sbPart[PART_SUSP]=sb;
  
  r.y+=dy;
  butPart[PART_CAM]=new QButton(QSHELL,&r,partName[PART_CAM]);
  r2=r; r2.x+=dx; r2.y+=r.hgt/4; r2.wid*=2; r2.hgt/=2;
  sb=new QScrollBar(QSHELL,&r2,QScrollBar::HORIZONTAL);
  sb->GetProp()->SetRange(0,RCar::MAX_CAMERA-1);
  sb->GetProp()->SetDisplayed(2,RCar::MAX_CAMERA);
  sb->GetProp()->SetJump(1);
  sb->GetProp()->EnableShowValue();
  sbPart[PART_CAM]=sb;

  // Elements
  bwid=120; bhgt=35;
  dx=bwid+10; dy=bhgt+5;
  x=640+32+30; y=32+20+7;
  cols=1;
  
  r.x=x-16-2; r.y=y-20;
  r.wid=150; r.hgt=480+11;
  /*grpElts=*/ new QGroup(QSHELL,&r,"Elements");
  
  // First parts are separate
  for(i=0;i<MAX_ELT;i++)
  {
    r.x=x+(i%cols)*dx;
    r.y=y+(i/cols)*dy;
    r.wid=bwid;
    r.hgt=bhgt;
    butElt[i]=new QButton(QSHELL,&r,"...");
  }
  
  // Operations
  bwid=120; bhgt=35;
  dx=bwid+10; dy=bhgt+5;
  x=640+32+30+160; y=32+20+7;
  cols=1;
  
  r.x=x-16-2; r.y=y-20;
  r.wid=150; r.hgt=480+11;
  /*grpOps=*/ new QGroup(QSHELL,&r,"Operations");
  
  // First parts are separate
  for(i=0;i<MAX_OP;i++)
  {
    r.x=x+(i%cols)*dx;
    r.y=y+(i/cols)*dy;
    r.wid=bwid;
    r.hgt=bhgt;
    butOp[i]=new QButton(QSHELL,&r,"...");
  }
}

/*********
* Events *
*********/
void NYI()
{
  QMessageBox("Error","Not yet implemented.");
}
bool PartsEvent(QEvent *e)
{
  int i;
  bool fSelected;
  
  if(e->type==QEvent::CLICK)
  {
    // Any part selected?
    fSelected=FALSE;
    for(i=0;i<MAX_PART;i++)
    { if(e->win==butPart[i])
      {
        curPart=i;
        // Set default modes etc
        curMode=MODE_DRIVE;
        car->GetBody()->DisableBoundingBox();
        for(i=0;i<RCar::MAX_WHEEL;i++)
          if(car->GetWheel(i))
            car->GetWheel(i)->DisableBoundingBox();
        SuspUnhiliteAll();
        fSelected=TRUE;
        break;
      }
    }
    
    // Which part?
    if(e->win==butPart[PART_CAM])CamSelect();
    else if(e->win==butPart[PART_BODY])BodySelect();
    else if(e->win==butPart[PART_GEARS])GearSelect();
    else if(e->win==butPart[PART_WHEEL])WheelSelect();
    else if(e->win==butPart[PART_ENGINE])EngineSelect();
    else if(e->win==butPart[PART_STEER])SteerSelect();
    else if(e->win==butPart[PART_SUSP])SuspSelect();
    else if(e->win==butPart[PART_CAR])CarSelect();
    else if(fSelected)
    { NYI();
    }
    
    // Any element selected?
    for(i=0;i<MAX_ELT;i++)
    {
      if(e->win==butElt[i])
      {
        if(eltSelectFunc)
          eltSelectFunc(i);
      }
    }
    
    // Any operation selected?
    for(i=0;i<MAX_OP;i++)
    {
      if(e->win==butOp[i])
      {
        if(eltSelectFunc)
          eltSelectFunc(OPBASE+i);
      }
    }
  }
  // Scrollbar changes? (dynamic update)
  if(e->type==QEvent::CHANGE||e->type==QEvent::CLICK)
  {
    if(e->win==sbPart[PART_WHEEL])
    {
      curWheel=sbPart[PART_WHEEL]->GetProp()->GetPosition();
      for(i=0;i<RCar::MAX_WHEEL;i++)
        if(car->GetWheel(i))
          car->GetWheel(i)->DisableBoundingBox();
      // Hilite wheel if in the right mode
      if(curPart==PART_WHEEL)
        WheelHilite();
    } else if(e->win==sbPart[PART_SUSP])
    {
      curSusp=sbPart[PART_SUSP]->GetProp()->GetPosition();
    } else if(e->win==sbPart[PART_CAM])
    {
      curCam=sbPart[PART_CAM]->GetProp()->GetPosition();
    }
  }
  return FALSE;
}

/***********
* Elements *
***********/
static void EltsSelect(cstring *name,int elts,cstring *op=0,int ops=0)
// Select a set of elements to display
{
  int i;
  for(i=0;i<elts;i++)
  {
    butElt[i]->SetText(name[i]);
  }
  for(;i<MAX_ELT;i++)
  {
    butElt[i]->SetText("---");
  }
  for(i=0;i<ops;i++)
  {
    butOp[i]->SetText(op[i]);
  }
  for(;i<MAX_OP;i++)
  {
    butOp[i]->SetText("---");
  }
}

/********
* Gears *
********/
static void GearNoof()
{
  cstring help[]=
  {
    "Number of gears, for example '4'"
  };
  cstring prop[]={ "# Gears" };
  QInfo  *info[]={ iCar };
  cstring infoPath[]={ "gearbox.gears" };
  
  if(RDlgProps("Gear count",prop,help,info,infoPath,1))
  {
    // Reread settings
    car->GetEngine()->Load(iCar);
  }
}
static void GearRatios()
{
  cstring help[]=
  {
    "Reverse gear ratio, probably a negative number",
    "First gear ratio",
    "2nd gear ratio",
    "3rd gear ratio",
    "4th gear ratio",
    "5th gear ratio",
    "6th gear ratio",
    "7th gear ratio"
  };
  cstring prop[]=
  { "Reverse gear","1st gear","2nd gear","3rd gear","4th gear",
    "5th gear","6th gear","7th gear"
  };
  QInfo  *info[]={ iCar,iCar,iCar,iCar,iCar,iCar,iCar,iCar };
  cstring infoPath[]=
  { "gearbox.gear0","gearbox.gear1","gearbox.gear2","gearbox.gear3",
    "gearbox.gear4","gearbox.gear5","gearbox.gear6","gearbox.gear7"
  };
  
  if(RDlgProps("Gear ratios",prop,help,info,infoPath,8))
  {
    // Reread settings
    car->GetEngine()->Load(iCar);
  }
}
static void GearFinal()
{
  cstring help[]={ "The ratio that multiplies all gear ratios" };
  cstring prop[]={ "End ratio" };
  QInfo  *info[]={ iCar };
  cstring infoPath[]={ "gearbox.end_ratio" };
  
  if(RDlgProps("Final gear",prop,help,info,infoPath,1))
    car->GetEngine()->Load(iCar);
}
static void GearElt(int n)
// An element was selected
{
  switch(n)
  {
    case 0: GearNoof(); break;
    case 1: GearRatios(); break;
    case 2: GearFinal(); break;
    default: NYI(); break;
  }
}

static void GearSelect()
// Select gearbox elements
{
  cstring elts[]=
  { "# Gears","Gear ratios","Final ratio"
  };
  EltsSelect(elts,3);
  eltSelectFunc=GearElt;
}

/**********
* Cameras *
**********/
static void elCamOffset()
{
  int i;
  cstring help[]=
  { "X offset","Y offset","Z offset" };
  cstring prop[]=
  { "X offset","Y offset","Z offset" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"camera%d.offset.%c",curCam,i+'x');
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Camera offset",prop,help,info,infoPath,3))
  {
    car->GetCamera(curCam)->Load(iCar);
  }
}
static void elCamOrientation()
{
  int i;
  cstring help[]=
  { "X angle (pitch)","Y offset (heading/yaw)","Z offset (roll)" };
  cstring prop[]={ "Pitch","Heading/yaw","Roll" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"camera%d.angle.%c",curCam,i+'x');
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Camera angle",prop,help,info,infoPath,3))
    car->GetCamera(curCam)->Load(iCar);
}
static void elCamFollow()
{
  int i;
  cstring help[]=
  { "Follow the car's pitch?","Follow the car's heading",
    "Follow the car's roll"
  };
  cstring prop[]={ "Follow pitch","Follow heading/yaw","Follow roll" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  cstring yprName[3]={ "pitch","yaw","roll" };
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"camera%d.follow.%s",curCam,yprName[i]);
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Camera following",prop,help,info,infoPath,3))
    car->GetCamera(curCam)->Load(iCar);
}
static void elCamName()
{
  cstring help[]={ "Enter a display name for the camera" };
  cstring prop[]={ "Name" };
  QInfo  *info[]={ iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  sprintf(infoPathBuf[0],"camera%d.name",curCam);
  infoPath[0]=infoPathBuf[0];
  if(RDlgProps("Camera name",prop,help,info,infoPath,1))
    car->GetCamera(curCam)->Load(iCar);
}
static void elCamFOV()
{
  cstring help[]={ "Enter the FOV (field of view, in degrees)" };
  cstring prop[]={ "FOV" };
  QInfo  *info[]={ iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  sprintf(infoPathBuf[0],"camera%d.fov",curCam);
  infoPath[0]=infoPathBuf[0];
  if(RDlgProps("Camera FOV",prop,help,info,infoPath,1))
    car->GetCamera(curCam)->Load(iCar);
}
static void CamStore()
// Store camera positioning information in the car file
// (instead of directly editing the properties)
{
  float v[3];
  int   i;
  RCamera *cam=car->GetCamera(curCam);
  char buf[256],vbuf[256];
  
  // Store location
  v[0]=cam->offset.x;
  v[1]=cam->offset.y;
  v[2]=cam->offset.z;
  for(i=0;i<3;i++)
  {
    sprintf(buf,"camera%d.offset.%c",curCam,i+'x');
    sprintf(vbuf,"%f",v[i]);
    iCar->SetString(buf,vbuf);
  }
  // Store angle
  v[0]=cam->angle.x;
  v[1]=cam->angle.y;
  v[2]=cam->angle.z;
  for(i=0;i<3;i++)
  {
    sprintf(buf,"camera%d.angle.%c",curCam,i+'x');
    sprintf(vbuf,"%f",v[i]);
    iCar->SetString(buf,vbuf);
  }
  iCar->Write();
}
static void CamElt(int n)
// An element was selected
{
  switch(n)
  {
    case 0: elCamOffset(); break;
    case 1: elCamOrientation(); break;
    case 2: elCamFollow(); break;
    case 3: elCamName(); break;
    case 4: elCamFOV(); break;
    case OPBASE+0: curMode=MODE_DRIVE; curSubMode=SUBMODE_OFFSET; break;
    case OPBASE+1: curMode=MODE_CAM; curSubMode=SUBMODE_OFFSET; break;
    case OPBASE+2: curMode=MODE_CAM; curSubMode=SUBMODE_ANGLE; break;
    case OPBASE+3: CamStore(); break;
    default: NYI(); break;
  }
}
static void CamSelect()
// Select camera elements
{
  cstring elts[]=
  { "Location","Orientation","Following","Name","Edit FOV"
  };
  cstring ops[]=
  { "Drive mode","Edit offset","Edit angle","Store camera"
  };
  EltsSelect(elts,sizeof(elts)/sizeof(elts[0]),ops,sizeof(ops)/sizeof(ops[0]));
  eltSelectFunc=CamElt;
  
  // Set camera mode
  curMode=MODE_CAM;
  curSubMode=SUBMODE_OFFSET;
}

/*******************
* Models (generic) *
*******************/
static bool ModelProps(cstring path)
// Edit generic model properties.
// 'path' indicates the path in the car.ini file (i.e. 'body')
// Returns TRUE if properties were OK-ed; the caller
// should then reread its properties to get the model properties
// into action.
{
  int i;
  cstring help[]=
  { "Filename relative to the car directory.",
    "Scaling","Scaling","Scaling"
  };
  cstring prop[]={ "File","Scale X","Scale Y","Scale Z" };
  QInfo  *info[]={ iCar,iCar,iCar,iCar };
  char    infoPathBuf[4][40];
  cstring infoPath[4];
  
  // Generate paths into the ini file
  sprintf(infoPathBuf[0],"%s.model.file",path);
  for(i=0;i<3;i++)
    sprintf(infoPathBuf[i+1],"%s.model.scale.%c",path,i+'x');

  for(i=0;i<4;i++)
    infoPath[i]=infoPathBuf[i];
for(i=0;i<4;i++)
qdbg("path[%d]=%s\n",infoPathBuf[i]);
  
  return RDlgProps("Model properties",prop,help,info,infoPath,4);
}

/*******
* Body *
*******/
static void elBodyMass()
{
  cstring help[]={ "Mass of body without driver or fuel" };
  cstring prop[]={ "Tub mass" };
  QInfo  *info[]={ iCar };
  cstring infoPath[]={ "body.mass" };
  
  if(RDlgProps("Body masses",prop,help,info,infoPath,1))
    car->GetBody()->Load(iCar);
}
static void elBodyModel()
{
  if(ModelProps("body"))
    car->GetBody()->Load(iCar);
}
static void elBodyPosition()
{
  int i;
  cstring help[]={ "X position (m)","Y position (m)","Z position (m)" };
  cstring prop[]={ "X position","Y position","Z position" };
  QInfo  *info[]={ iCar,iCar,iCar };
  cstring infoPath[3]={ "body.x","body.y","body.z" };
  
  if(RDlgProps("Body position",prop,help,info,infoPath,3))
    car->GetBody()->Load(iCar);
}
static void elBodySize()
{
  int i;
  cstring help[]={ "Width (m)","Height (m)","Length (m)",
    "Start of cockpit relative to nose (m)","Length of cockpit (m)" };
  cstring prop[]={ "Width","Height","Length","Cockpit start",
    "Cockpit length" };
  QInfo  *info[]={ iCar,iCar,iCar,iCar,iCar };
  cstring infoPath[5]={ "body.width","body.height","body.length",
    "body.cockpit_start","body.cockpit_length" };
  
  if(RDlgProps("Body position",prop,help,info,infoPath,5))
    car->GetBody()->Load(iCar);
}
static void BodyElt(int n)
// An element was selected
{
  switch(n)
  {
    case 0: elBodyMass(); break;
    case 1: elBodyModel(); break;
    case 2: elBodyPosition(); break;
    case 3: elBodySize(); break;
    default: NYI(); break;
  }
}

static void BodySelect()
// Select gearbox elements
{
  cstring elts[]=
  { "Masses","Model","Position","Size"
  };
  EltsSelect(elts,4);
  eltSelectFunc=BodyElt;
  // Specials
  car->GetBody()->EnableBoundingBox();
}

/*********
* Wheels *
*********/
static cstring elWheelName()
// Returns path for wheel 'curWheel' (in the ini file)
{
  static char buf[40];
  sprintf(buf,"wheel%d",curWheel);
  return buf;
}
static void elWheelSize()
{
  int i;
  cstring help[]={ "Total mass in kg","Radius in meters","Width in meters" };
  cstring prop[]={ "Mass","Radius","Width" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  cstring propName[3]={ "mass","radius","width" };
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"%s.%s",elWheelName(),propName[i]);
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Wheel mass and size",prop,help,info,infoPath,3))
    car->GetWheel(curWheel)->Load(iCar,elWheelName());
}
static void elWheelModel()
{
  char buf[256];
  sprintf(buf,"wheel%d",curWheel);
  if(ModelProps(buf))
    car->GetWheel(curWheel)->Load(iCar,elWheelName());
}
static void elWheelPosition()
{
  int i;
  cstring help[]={ "X position relative to suspension (in meters)",
    "Y position (m)","Z position (m)" };
  cstring prop[]={ "X position","Y position","Z position" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"%s.%c",elWheelName(),i+'x');
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Wheel position",prop,help,info,infoPath,3))
    car->GetWheel(curWheel)->Load(iCar,elWheelName());
}
static void elWheelRates()
{
  int i;
  cstring help[]={ "Tire spring rate in N/m/s (for example: 140000)",
    "Braking capability in Nm (torque), i.e. 2000" };
  cstring prop[]={ "Tire rate","Max. braking torque" };
  QInfo  *info[]={ iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  cstring propName[3]={ "tire_rate","max_braking" };
  
  for(i=0;i<2;i++)
  {
    sprintf(infoPathBuf[i],"%s.%s",elWheelName(),propName[i]);
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Rates",prop,help,info,infoPath,2))
    car->GetWheel(curWheel)->Load(iCar,elWheelName());
}
static void WheelElt(int n)
// An element was selected
{
  switch(n)
  {
    case 0: elWheelSize(); break;
    case 1: elWheelModel(); break;
    case 2: elWheelPosition(); break;
    case 3: elWheelRates(); break;
    default: NYI(); break;
  }
}

static void WheelHilite()
{
  RWheel *w=car->GetWheel(curWheel);
  if(w)w->EnableBoundingBox();
}
static void WheelSelect()
{
  cstring elts[]=
  { "Size & mass","Model","Position","Rates"
  };
  EltsSelect(elts,4);
  eltSelectFunc=WheelElt;
  // Specials
  WheelHilite();
}

/*************
* Suspension *
*************/
static cstring elSuspName()
// Returns path for wheel 'curSusp' (in the ini file)
{
  static char buf[40];
  sprintf(buf,"susp%d",curSusp);
  return buf;
}
static void elSuspSize()
{
  int i;
  cstring help[]={ "Rest length (m)","Minimal length (m)",
    "Maximal length (m)","Radius (m)" };
  cstring prop[]={ "Rest length","Min. length","Max. length","Radius" };
  QInfo  *info[]={ iCar,iCar,iCar,iCar };
  char    infoPathBuf[4][40];
  cstring infoPath[4];
  cstring propName[4]={ "restlen","minlen","maxlen","radius" };
  
  for(i=0;i<4;i++)
  {
    sprintf(infoPathBuf[i],"%s.%s",elSuspName(),propName[i]);
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Suspension size",prop,help,info,infoPath,4))
    car->GetWheel(curSusp)->GetSuspension()->Load(iCar,elSuspName());
}
static void elSuspModel()
{
#ifdef OBS
  char buf[256];
  sprintf(buf,"susp%d",cursusp);
#endif
  if(ModelProps(elSuspName()))
    car->GetWheel(curSusp)->GetSuspension()->Load(iCar,elSuspName());
}
static void elSuspPosition()
{
  int i;
  cstring help[]={ "X position relative to body (in meters)",
    "Y position (m)","Z position (m)" };
  cstring prop[]={ "X position","Y position","Z position" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"%s.%c",elSuspName(),i+'x');
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Suspension position",prop,help,info,infoPath,3))
    car->GetWheel(curSusp)->GetSuspension()->Load(iCar,elSuspName());
}
static void elSuspRates()
{
  int i;
  cstring help[]={ "Spring rate (N/m)","Damper bump rate (N/m/s)",
    "Damper rebound rate (N/m/s)" };
  cstring prop[]={ "Spring rate","Bump rate","Rebound rate" };
  QInfo  *info[]={ iCar,iCar,iCar };
  char    infoPathBuf[3][40];
  cstring infoPath[3];
  cstring propName[3]={ "k","bump_rate","rebound_rate" };
  
  for(i=0;i<3;i++)
  {
    sprintf(infoPathBuf[i],"%s.%s",elSuspName(),propName[i]);
    infoPath[i]=infoPathBuf[i];
  }
  if(RDlgProps("Suspension size",prop,help,info,infoPath,3))
    car->GetWheel(curSusp)->GetSuspension()->Load(iCar,elSuspName());
}
static void SuspElt(int n)
// An element was selected
{
  switch(n)
  {
    case 0: elSuspSize(); break;
    case 1: elSuspModel(); break;
    case 2: elSuspPosition(); break;
    case 3: elSuspRates(); break;
    default: NYI(); break;
  }
}
static void SuspHilite()
{
  RWheel *w=car->GetWheel(curWheel);
  RSuspension *s=0;
  if(w)
  { s=car->GetWheel(curSusp)->GetSuspension();
    if(s)s->EnableBoundingBox();
  }
}
static void SuspUnhiliteAll()
{
  int i;
  for(i=0;i<RCar::MAX_WHEEL;i++)
  {
    RWheel *w=car->GetWheel(i);
    RSuspension *s=0;
    if(w)
    { s=car->GetWheel(curSusp)->GetSuspension();
      if(s)s->DisableBoundingBox();
    }
  }
}
static void SuspSelect()
{
  cstring elts[]=
  { "Size & mass","Model","Position","Rates"
  };
  EltsSelect(elts,4);
  eltSelectFunc=SuspElt;
  // Specials
  WheelHilite();
}

/******
* Car *
******/
static void CarElt(int n)
{
  switch(n)
  {
#ifdef OBS
    case 0: elSuspSize(); break;
    case 1: elSuspModel(); break;
    case 2: elSuspPosition(); break;
    case 3: elSuspRates(); break;
#endif
    default: NYI(); break;
  }
}
static void CarSelect()
{
  cstring elts[]=
  { "Sound","Parts","Aerodynamics","Center of gravity"
  };
  EltsSelect(elts,4);
  eltSelectFunc=CarElt;
}

/***********
* Steering *
***********/
static void elSteerPosition()
{
  int i;
  cstring help[]={ "X position (m)","Y position (m)","Z position (m)",
    "Rotation towards user (deg)" };
  cstring prop[]={ "X position","Y position","Z position","Rotation" };
  QInfo  *info[]={ iCar,iCar,iCar,iCar };
  cstring infoPath[4]={ "steer.x","steer.y","steer.z","steer.xa" };
  
  if(RDlgProps("Steering wheel position",prop,help,info,infoPath,4))
    car->GetSteer()->Load(iCar);
}
static void elSteerSize()
{
  int i;
  cstring help[]={ "Radius of the steering wheel" };
  cstring prop[]={ "Radius (m)" };
  QInfo  *info[]={ iCar };
  cstring infoPath[1]={ "steer.radius" };
  
  if(RDlgProps("Steering sizes",prop,help,info,infoPath,1))
    car->GetSteer()->Load(iCar);
}
static void elSteerLock()
{
  int i;
  cstring help[]={ "Total lock; angle varies from -lock/2..lock/2" };
  cstring prop[]={ "Lock (deg)" };
  QInfo  *info[]={ iCar };
  cstring infoPath[1]={ "steer.lock" };
  
  if(RDlgProps("Steering lock",prop,help,info,infoPath,1))
    car->GetSteer()->Load(iCar);
}
static void SteerElt(int n)
{
  switch(n)
  {
    case 0: elSteerPosition(); break;
    case 1: elSteerSize(); break;
    case 2: elSteerLock(); break;
    default: NYI(); break;
  }
}
static void SteerSelect()
{
  cstring elts[]=
  { "Position","Size","Lock"
  };
  EltsSelect(elts,3);
  eltSelectFunc=SteerElt;
}

/*********
* Engine *
*********/
static void EngineElt(int n)
{
  switch(n)
  {
#ifdef OBS
    case 0: elSuspSize(); break;
    case 1: elSuspModel(); break;
    case 2: elSuspPosition(); break;
    case 3: elSuspRates(); break;
#endif
    default: NYI(); break;
  }
}
static void EngineSelect()
{
  cstring elts[]=
  { "Size & mass","Model","Position","Rates"
  };
  EltsSelect(elts,4);
  eltSelectFunc=EngineElt;
}
