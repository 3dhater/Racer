/*
 * RView - cockpit-like views
 * 29-01-2001: Created! (21:23:25)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/app.h>
DEBUG_ENABLE

// Standard view size on which view positions are based
#define BASE_VIEW_WID   800
#define BASE_VIEW_HGT   600

/***********
* RViewElt *
***********/
#undef  DBG_CLASS
#define DBG_CLASS "RViewElt"

RViewElt::RViewElt(RView *_view)
  : view(_view)
{
  flags=0;
  wid=hgt=0.0f;
  tex=0;
  type=NONE;
  pos.SetToZero();
  pivot.SetToZero();
  minValue=maxValue=0.0f;
  minAngle=maxAngle=0.0f;
  var=0;
}
RViewElt::~RViewElt()
{
  QDELETE(tex);
}

/*******
* Load *
*******/
bool RViewElt::Load(QInfo *info,cstring path)
// Read element from a QInfo
{
  char buf[128];
  qstring s;
  
//qdbg("RViewElt::Load(path=%s)\n",path);
  sprintf(buf,"%s.type",path);
  info->GetString(buf,s);
  if(s=="static")
  {
    type=STATIC;
  } else if(s=="angular")
  { type=ANGULAR;
  } else if(s=="time")
  { type=TIME;
  }
  // Read common attribs
  sprintf(buf,"%s.x",path);
  pos.x=info->GetFloat(buf);
  sprintf(buf,"%s.y",path);
  pos.y=info->GetFloat(buf);
  sprintf(buf,"%s.z",path);
  pos.z=info->GetFloat(buf);
  // Flags
  sprintf(buf,"%s.is3d",path);
  if(info->GetInt(buf))
    flags|=IS3D;

  // Size
  sprintf(buf,"%s.width",path);
  wid=info->GetFloat(buf);
  sprintf(buf,"%s.height",path);
  hgt=info->GetFloat(buf);
  if(type==STATIC||type==ANGULAR)
  {
    // Read possible image
    sprintf(buf,"%s.image",path);
    info->GetString(buf,s);
    if(s!="")
    {
//qdbg("RViewElt; image '%s'\n",s.cstr());
      DBitMapTexture *texBM;
      QImage *img;
      
      sprintf(buf,"%s/%s",
        GetView()->GetViews()->GetCar()->GetDir(),s.cstr());
      img=new QImage(buf);
      texBM=new DBitMapTexture(img);
      tex=texBM;
      QDELETE(img);
    }
  }
  if(type==ANGULAR||type==TIME)
  {
    // Get variable to measure
    sprintf(buf,"%s.var",path);
    info->GetString(buf,s);
    if(s=="rpm")var=VAR_RPM;
    else if(s=="laptime")var=VAR_LAPTIME;
    else if(s=="besttime")var=VAR_BESTTIME;
    else if(s=="gear")var=VAR_GEAR;
    else if(s=="velocity")var=VAR_VELOCITY;
    else if(s=="velocity_wc")var=VAR_VELOCITY_WC;
    else if(s=="lap")var=VAR_LAP;
    else if(s=="gearbox_vel")var=VAR_GEARBOX_VEL;
    else
    { 
      qwarn("RView: unknown variable '%s'",s.cstr());
      var=-1;                 // Unknown variable
    }
    // Read min/max values
    sprintf(buf,"%s.min_value",path);
    minValue=info->GetFloat(buf);
    sprintf(buf,"%s.max_value",path);
    maxValue=info->GetFloat(buf);
    // Read min/max angle
    sprintf(buf,"%s.min_angle",path);
    minAngle=info->GetFloat(buf);
    sprintf(buf,"%s.max_angle",path);
    maxAngle=info->GetFloat(buf);
    // Pivot
    sprintf(buf,"%s.pivot_x",path);
    pivot.x=info->GetFloat(buf);
    sprintf(buf,"%s.pivot_y",path);
    pivot.y=info->GetFloat(buf);
  }
  
  if(!(flags&IS3D))
  {
    // Auto-adjust view size based on a default rendering view size
    int vwid,vhgt;
    vwid=RMGR->GetGfxInfo()->GetInt("resolution.width");
    vhgt=RMGR->GetGfxInfo()->GetInt("resolution.height");
    pos.x=(pos.x*vwid)/BASE_VIEW_WID;
    pos.y=(pos.y*vhgt)/BASE_VIEW_HGT;
    wid=(wid*vwid)/BASE_VIEW_WID;
    hgt=(hgt*vhgt)/BASE_VIEW_HGT;
    pivot.x=(pivot.x*vwid)/BASE_VIEW_WID;
    pivot.y=(pivot.y*vhgt)/BASE_VIEW_HGT;
  }

  return TRUE;
}

/************
* Measuring *
************/
rfloat RViewElt::GetVarValue()
// Returns value of variable
{
  RCar *car;
//qdbg("RVE:GetVarValue, var=%d, VAR_RPM=%d\n",var,VAR_RPM);
  // Lots of variables require the currently focused car
  car=view->GetViews()->GetCar();
  switch(var)
  {
    case VAR_RPM:
      // RPM of engine
      return car->GetEngine()->GetRPM();
    case VAR_LAPTIME:
      return RMGR->scene->GetCurrentLapTime(car);
    case VAR_LAP:
      return RMGR->scene->curLap[car->GetIndex()];
    case VAR_BESTTIME:
      return RMGR->scene->GetBestLapTime(car);
    case VAR_GEAR:
      return car->GetGearBox()->GetGear();
    case VAR_VELOCITY:
      return car->GetVelocityTach();
    case VAR_VELOCITY_WC:
      return car->GetBody()->GetLinVel()->Length();
    case VAR_GEARBOX_VEL:
      return car->GetGearBox()->GetRotationVel();
    default: return 0.0f;
  }
}
int RViewElt::GetVarValueInt()
// Returns integer value of variable
{
  RCar *car;
  // Lots of variables require the currently focused car
  car=view->GetViews()->GetCar();
  switch(var)
  {
    case VAR_RPM:
      // RPM of engine
      return (int)car->GetEngine()->GetRPM();
    case VAR_LAPTIME:
      return RMGR->scene->GetCurrentLapTime(car);
    case VAR_LAP:
      return RMGR->scene->curLap[car->GetIndex()];
    case VAR_BESTTIME:
      return RMGR->scene->GetBestLapTime(car);
    case VAR_GEAR:
      return car->GetGearBox()->GetGear();
    case VAR_VELOCITY:
      return (int)car->GetVelocityTach();
    case VAR_VELOCITY_WC:
      return (int)car->GetBody()->GetLinVel()->Length();
    case VAR_GEARBOX_VEL:
      return (int)car->GetGearBox()->GetRotationVel();
    default: return 0;
  }
}
rfloat RViewElt::GetVarAngle()
// Returns angle for the given variable value
// Clamps to minValue/maxValue if the value exceeds the possible range.
{
  rfloat v,rangeValue,rangeAngle;
  
  // Get the current value of the variable
  v=GetVarValue();
//qdbg("RViewElt; GetVarValue=%f\n",v);
  // Clamp to possible range
  if(v<minValue)v=minValue;
  else if(v>maxValue)v=maxValue;
  
  // Calculate ranges
  rangeAngle=fabs(maxAngle-minAngle);
  if(rangeAngle<0.001f)return 0.0;
  rangeValue=fabs(maxValue-minValue);
  if(rangeValue<0.001f)return 0.0;
  
  // Calculate linearly interpolate angle
  if(minAngle<maxAngle)
    return (v-minValue)*rangeAngle/rangeValue+minAngle;
  else
    return (maxValue-v)*rangeAngle/rangeValue+maxAngle;
}

/********
* Paint *
********/
void RViewElt::Paint()
{
  rfloat angle;
  
  glPushMatrix();
  glTranslatef(pos.x,pos.y,pos.z);
//qdbg("Paint elt @%f,%f,%f\n",pos.x,pos.y,pos.z);
  if(type==STATIC||type==ANGULAR)
  {
    // A simple quad with image
    if(tex)tex->Select();
    
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
//glDisable(GL_BLEND);
 
    if(type==ANGULAR)
    {
      // Calculate angle of polygon
      angle=GetVarAngle();
//qdbg("Var %d angle=%f\n",var,angle);
      // Rotate over pivot point
      glTranslatef(pivot.x,pivot.y,pivot.z);
      if(Is3D())
        glRotatef(-angle,0,0,1);
      else
        glRotatef(angle,0,0,1);
      glTranslatef(-pivot.x,-pivot.y,-pivot.z);
    }
    
    glBegin(GL_QUADS);
    if(Is3D())
    {
      // Reverse order (otherwise face is culled)
      glNormal3f(0,0,-1);
      glTexCoord2f(1,0);
      glVertex3f(-wid/2,-hgt/2,0);
      glTexCoord2f(1,1);
      glVertex3f(-wid/2,hgt/2,0);
      glTexCoord2f(0,1);
      glVertex3f(wid/2,hgt/2,0);
      glTexCoord2f(0,0);
      glVertex3f(wid/2,-hgt/2,0);
    } else
    {
      // 2D
      glTexCoord2f(0,0);
      glVertex3f(-wid/2,-hgt/2,0);
      glTexCoord2f(1,0);
      glVertex3f(wid/2,-hgt/2,0);
      glTexCoord2f(1,1);
      glVertex3f(wid/2,hgt/2,0);
      glTexCoord2f(0,1);
      glVertex3f(-wid/2,hgt/2,0);
    }
    glEnd();
    
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
  } else if(type==TIME)
  {
    // Timer text HH:MM:SS or MM:SS:MMM (1/1000th sec accuracy)
    int  t,tm,ts,tms;
    char buf[20];    
    // Get time in msecs
    t=GetVarValueInt();
    if(t==0)
    { strcpy(buf,"--:--:--");
    } else
    {
      // Calculate hours, minutes etc
      //th=t/(1000*60*60);
      tm=(t/(1000*60))%60;
      ts=(t/1000)%60;
      tms=t%1000;
      sprintf(buf,"%02d:%02d:%03d",tm,ts,tms);
    }
//qdbg("Paint %s @%f,%f\n",buf,pos.x,pos.y);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    //RMGR->fontDefault->Paint(buf,pos.x,pos.y);
    RMGR->fontDefault->Paint(buf,0,0);
#ifdef OBS
    RMGR->fontDefault->Paint(buf,-100,-100);
    RMGR->fontDefault->Paint(buf,100,100);
#endif
  }
  glPopMatrix();
}

/********
* RView *
********/
#undef  DBG_CLASS
#define DBG_CLASS "RView"

RView::RView(RViews *_views)
  : views(_views)
{
  elts=0;
}
RView::~RView()
{
  int i;
  for(i=0;i<elts;i++)
    QDELETE(elt[i]);
}

bool RView::Load(QInfo *info,cstring path)
{
  char buf[128];
  
  sprintf(buf,"%s.name",path);
  name=info->GetStringDirect(buf);
//qdbg("RView:Load(); name='%s'\n",name.cstr());
//qdbg("  path='%s'\n",path);
  QInfoIterator ii(info,path);
  qstring s;
  while(ii.GetNext(s))
  {
    // All elements must start with 'elt'
    if(!strstr(s,"elt"))continue;
//qdbg("RView:Load(); elt '%s'\n",s.cstr());
    elt[elts]=new RViewElt(this);
    elt[elts]->Load(info,s);
    elts++;
  }
  return TRUE;
}

void RView::Paint3D()
// Paint all 3D (!) elements from the view.
// May be called for each car if you want fully working 3D instruments,
// or just for the close cars where it is worth it to paint the dials.
{
  int i;

  // First paint all 3D elements; assumes
  // the modelview matrix is displaying the car
  //glEnable(GL_CULL_FACE);
  // Make the dials bright (they use lighting)
  glColor4f(1,1,1,0);
  //glDisable(GL_LIGHTING);
  //glShadeModel(GL_FLAT);
  //glDisable(GL_DEPTH_TEST);
//glEnable(GL_DEPTH_TEST);
//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  for(i=0;i<elts;i++)
    if(elt[i]->Is3D())
      elt[i]->Paint();
  //glEnable(GL_CULL_FACE);
}
void RView::Paint2D()
// Paint all 2D (!) elements from the view
// Call this for 1 car only.
// NOTE: Switches to 2D! After this, no more 3D please.
{
  int i;

#ifdef OBS_DONE_BY_RSCENE
  // Switch to 2D
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0,QSHELL->GetWidth(),0,QSHELL->GetHeight(),-1,1);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glColor4f(1,1,1,1);
  glDisable(GL_LIGHTING);
#endif
  for(i=0;i<elts;i++)
    if(!elt[i]->Is3D())
      elt[i]->Paint();
}

/*********
* RViews *
*********/
#undef  DBG_CLASS
#define DBG_CLASS "RViews"

RViews::RViews(RCar *_car)
  : car(_car)
{
  int i;

  views=0;
  for(i=0;i<MAX_VIEW;i++)
    view[i]=0;
}
RViews::~RViews()
{
  int i;
  for(i=0;i<views;i++)
    QDELETE(view[i]);
}

bool RViews::Load(QInfo *info)
{
  QInfoIterator ii(info,"views");
  qstring s;
  while(ii.GetNext(s))
  {
//qdbg("RViews:Load(); view '%s'\n",s.cstr());
    view[views]=new RView(this);
    view[views]->Load(info,s);
    views++;
  }
  return TRUE;
}
