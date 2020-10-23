/*
 * Track camera editing
 * 01-04-2001: Created! (17:31:32)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// GUI
cstring  opNameTCam[]=
{ // Track cam ops
  "Goto line start",
  "Declare as camera",
  "Edit radius",
  "Edit group",
  "Define dolly endpt",
  "Place edge cube",
  "Place close cube",
  "Place far cube",
  "(P)review motion",
  "Goto camera start",
  "Edit camera angles",
  0
};

int        curCamIndex=-1;          // Currently select camera
RTrackCam *curCam;                  // Current camera

void TCamSetMode()
// Install us as the current mode
{
  SetOpButtons(opNameTCam);
}

/**********
* Helpers *
**********/
static void HiliteCurrentCam()
{
  int i;
  for(i=0;i<track->GetTrackCamCount();i++)
  {
    if(curCamIndex==i)
      track->trackCam[i]->flags|=RTrackCam::HILITE;
    else
      track->trackCam[i]->flags&=~RTrackCam::HILITE;
  }
}

/********
* Stats *
********/
void TCamStats()
{
  char buf[40],buf2[40];
  strcpy(buf,"Current camera: ");
  if(!curCam)strcpy(buf2,"NONE");
  else       sprintf(buf2,"#%d",curCamIndex);
  strcat(buf,buf2);
  StatsAdd(buf);
}

/******
* Ops *
******/
static void DeclareTCam()
{
  RTrackCam *cam;
  int n;
  
  // Declare time line
  n=GetIndex("Select camera index",track->GetTrackCamCount(),curCamIndex);
  if(n!=-1)
  {
    if(n>=track->GetTrackCamCount())
    {
      // New cam
      cam=new RTrackCam();
      track->AddTrackCam(cam);
    } else
    {
      // Existing cam
      cam=track->GetTrackCam(n);
    }
    // Install settings that are known
    curCam=cam;
    curCamIndex=n;
    // Use current view position
    cam->pos.x=tf->x;
    cam->pos.y=tf->y;
    cam->pos.z=tf->z;
    // Use pick point as the direction
    cam->posDir=vPick;
#ifdef OBS
    // Give it some height
    cam->pos.y+=3.0f;
    cam->posDir.y+=3.0f;
#endif
    // Defaults
    cam->radius=20;
    cam->zoomEdge=20;
    cam->zoomClose=30;
    cam->zoomFar=20;
  }
  // Make the possibly new cam the current camera
  HiliteCurrentCam();
}
static bool GetValue(cstring title,cstring text,int v,int *res)
{
  int n;
  char buf[80];
  QRect r(500,150,350,150);
  
  sprintf(buf,"%d",v);
  n=QDlgString(title,text,buf,sizeof(buf),1,&r);
  if(!n)return FALSE;
  *res=Eval(buf);
  return TRUE;
}
static void EditRadius()
// Notice only integer radii are accepted.
{
  int n;
  
  if(!curCam)return;
  if(GetValue("Edit radius","Enter camera pick-up radius",(int)curCam->radius,&n))
  {
    curCam->radius=(rfloat)n;
  }
}
static void EditGroup()
{
  int n;
  
  if(!curCam)return;
  if(GetValue("Edit group","Enter camera group number",curCam->group,&n))
  {
    curCam->group=n;
  }
}
static void DefineDolly()
// Set the end point for the camera to move to
// Also enables the dolly feature automatically
{
  if(!curCam)
  {
    QMessageBox("Error","No camera selected");
    return;
  }
  // Use current view position
  curCam->posDolly.x=tf->x;
  curCam->posDolly.y=tf->y;
  curCam->posDolly.z=tf->z;
  //curCam->posDolly=vPick;
  // Turn on dollying
  curCam->flags|=RTrackCam::DOLLY;
}
static void DefineCube(int n)
// Define test cube (edge,close,far)
{
  if(!curCam)return;
qdbg("DefineCube(%d)\n",n);
  curCam->cubePreview[n]=vPick;
  // Place the cube just on top of the track
  curCam->cubePreview[n].y+=1.0f;
}

/*************
* Previewing *
*************/
static DVector3 previewTarget;

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
static void PaintCube(DVector3 *center,float size)
// Paint rope with poles
{
  QColor col(185,85,185);
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  SetGLColor(&col);
  glBegin(GL_QUADS);
    // Front
    glNormal3f(0,0,1);
    glColor3f(1,0.5,1);
    glVertex3f(center->x-size,center->y+size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y+size,center->z-size);
    // Left
    glNormal3f(-1,0,0);
    glColor3f(1,0.2,1);
    glVertex3f(center->x-size,center->y+size,center->z+size);
    glVertex3f(center->x-size,center->y+size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z+size);
    // Right
    glNormal3f(1,0,0);
    glColor3f(1,0.1,1);
    glVertex3f(center->x+size,center->y+size,center->z+size);
    glVertex3f(center->x+size,center->y+size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z+size);
    // Back
    glColor3f(1,0.4,1);
    glNormal3f(0,0,-1);
    glVertex3f(center->x-size,center->y+size,center->z+size);
    glVertex3f(center->x-size,center->y-size,center->z+size);
    glVertex3f(center->x+size,center->y-size,center->z+size);
    glVertex3f(center->x+size,center->y+size,center->z+size);
  glEnd();
  glPopAttrib();
}
void TCamTarget()
// Target camera at current preview location
{
  if(!curCam)return;
  
  curCam->SetCar(previewCar);
  // Copy preview position into fake car
  *previewCar->GetPosition()=previewTarget;
  // Make camera point at the car
  curCam->CalcState();
  curCam->Go( /*&previewTarget*/ );
}
void TCamPaintTarget()
// Paint the current cube that is the target
{
  if(!curCam)return;
  PaintCube(&previewTarget,1);
}
static void PreviewMotion()
{
  dfloat i,t,max=3.0f;
  DVector3 from,to,diff,cur;
  QTimer tmr;
  
  if(!curCam)return;
  
  tmr.Start();
  while(tmr.GetMilliSeconds()<max*1000.f)
  {
    t=((dfloat)tmr.GetMilliSeconds())/(max*1000.0f);
//qdbg("t=%f\n",t);
    if(t>0.5)
    {
      t=(t-0.5f)*2.0f;
      from=curCam->cubePreview[1];
      to=curCam->cubePreview[2];
      diff=to;
      diff.Subtract(&from);
    } else
    {
      t*=2.0f;
      from=curCam->cubePreview[0];
      to=curCam->cubePreview[1];
      diff=to;
      diff.Subtract(&from);
    }
    cur.x=from.x+t*diff.x;
    cur.y=from.y+t*diff.y;
    cur.z=from.z+t*diff.z;
    // Pass target to TCamTarget() above (called from PaintTrack())
    previewTarget=cur;
    PaintTrack(2);
    //GSwap();
  }
}

static void GotoCam()
{
  if(!curCam)return;
  GotoPoint(&curCam->pos);
}
static void EditCamAngles()
{
  int i;
  cstring camName[3]={ "edge","close","far" };   // ECF
  char buf[200],buf2[200];
  dfloat v,*pAngle;
  
  if(!curCam)return;
  
  for(i=0;i<3;i++)
  {
    sprintf(buf,"Enter camera view angle for '%s'",camName[i]);
    switch(i)
    {
      case 0: pAngle=&curCam->zoomEdge; break;
      case 1: pAngle=&curCam->zoomClose; break;
      case 2: pAngle=&curCam->zoomFar; break;
    }
    sprintf(buf2,"%f",*pAngle);
    if(!QDlgString("Camera FOV angles",buf,buf2,sizeof(buf2)))
      return;
    v=atof(buf2);
    if(v<1)v=1;
    else if(v>180)v=180;
    *pAngle=v;
    DocDirty();
  }
}

/******************
* Op distribution *
******************/
void TCamOp(int op)
{
  switch(op)
  {
    case 0: GotoPickPoint(); break;
    case 1: DeclareTCam(); break;
    case 2: EditRadius(); break;
    case 3: EditGroup(); break;
    case 4: DefineDolly(); break;
    case 5: DefineCube(0); break;
    case 6: DefineCube(1); break;
    case 7: DefineCube(2); break;
    case 8: PreviewMotion(); break;
    case 9: GotoCam(); break;
    case 10: EditCamAngles(); break;
  }
}

/*******
* Keys *
*******/
static void NextCam()
{
  if(!curCam)
  {
    // First cam?
    if(track->GetTrackCamCount())
    {
      curCam=track->trackCam[0];
      curCamIndex=0;
    }
  } else
  {
    // Next cam
    if(curCamIndex<track->GetTrackCamCount()-1)
    {
      curCamIndex++;
      curCam=track->trackCam[curCamIndex];
    }
  }
  HiliteCurrentCam();
}
static void PrevCam()
{
  if(!curCam)
  {
    NextCam();
  } else
  {
    if(curCamIndex>0)
    {
      curCamIndex--;
      curCam=track->trackCam[curCamIndex];
    }
  }
  HiliteCurrentCam();
}

bool TCamKey(int key)
{
  if(key==QK_PAGEUP)
  { PrevCam();
  } else if(key==QK_PAGEDOWN)
  {
    NextCam();
  } else if(key==QK_P)
  {
    PreviewMotion();
  }
  return FALSE;
}
