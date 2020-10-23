/*
 * TrackEd - editor for VRML-type tracks
 * 12-11-00: Created!
 * NOTES:
 * FUTURE:
 * - Maintain doc 'dirty' state for better saving
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#ifdef WIN32
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#endif
#include <nlib/global.h>
#include <d3/fps.h>
#include <d3/global.h>
#include <d3/aabbtree.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define APP_TITLE "Racer TrackEd v1.8"

// Default car for tests (hopefully present)
#define DEFAULT_CAR    "ferp4"

#define DRW        Q_BC

// Cursor movement step size ('1' means 1 meter per keyhit)
#define MOVE_STEP      0.1f

// Default view angle
#define DEFAULT_FOV    50.0f

// Edit mode
int  mode;

// GUI
QTitleBar *title;
QRadio *radMode[MODE_MAX];
cstring modeName[MODE_MAX]=
{ "Positions (F1)","Track cameras (F2)","AI Points (F3)",
  "Model properties (F4)","Road splines (F5)"
};

// Operations
enum { MAX_IO=8 };
cstring  ioName[MAX_IO]=
{ "Select track","Load track (Shift-L)","Save track (Shift-S)",
  "Convert SCGT wrl's","(I)mport DOF models",
  "Add lighting","Convert ASE batch","Build collision tree"
};
QButton *butIO[MAX_IO];

enum { MAX_MODIFY=12 };
QButton *butModify[MAX_MODIFY];

// Graphics
DGeode     *model;
DFPS       *fps;
DTransformation aTF,*tf=&aTF;
bool fFog=TRUE;

// The data
RTrackVRML *track;
RCar       *previewCar;

// Picking
DVector3 vPick,vPickEnd;

// Hack
char keyState[256];

// Document
bool isDirty;

// Errors
QMessageHandler defQErr;
cstring sError="Error";

// Proto
static void RecreateTrack();
static void LoadTrack();

void exitfunc()
{
  int  i;
  char buf[256];

qdbg("exitFunc()\n");

  // Save settings
  info->SetInt("settings.mode",mode);
  sprintf(buf,"%f %f %f %f %f %f",tf->x,tf->y,tf->z,tf->xa,tf->ya,tf->za);
  info->SetString("settings.transform",buf);

  // Shutdown networking
  qnShutdown();

  // Delete objects
  for(i=0;i<MAX_IO;i++)QDELETE(butIO[i]);
  for(i=0;i<MAX_MODIFY;i++)QDELETE(butModify[i]);
qdbg("QDEL RMGR\n");
  QDELETE(RMGR);
qdbg("QDEL model\n");
  QDELETE(model);
  // Track was deleted by RMGR
//qdbg("QDEL track\n");
  //QDELETE(track);
qdbg("QDEL info\n");
  QDELETE(info);
#ifdef Q_USE_FMOD
qdbg("FMOD close\n");
  FSOUND_Close();
#endif
qdbg("exitFunc() RET\n");
}

/**********
* Helpers *
**********/
void SetTrack(cstring name)
{
  char buf[256];
qdbg("SelectTrack(%s)\n",name);

  track->SetName(name);
  sprintf(buf,"%s - %s",APP_TITLE,name);
  title->SetTitle(buf);
}
static void SelectTrack()
{
  char buf[80];
  strcpy(buf,track->GetName());
  if(!QDlgString("Select track","Enter the track name",buf,sizeof(buf)))
    return;
  // Set track in ini file for next app start
  info->SetString("track.lasttrack",buf);
  info->Write();
  SetTrack(buf);
  
  // Automatically load track
  LoadTrack();
}
int GetIndex(cstring text,int maxIndex,int curIndex)
// Request an index between 0 and maxIndex-1.
// Pre-enters 'curIndex' into the edit control. If the edit control
// is cleared, 'maxIndex' is returned (indicating a 'new' item).
// Returns -1 if the action was cancelled.
{
  char buf[256],buf2[128];
  int  n;
  
  if(maxIndex==0)
  {
    // Just 1 choice
    sprintf(buf,"%s (none existing yet)",text);
  } else
  {
    // May select an existing index
    sprintf(buf,"%s (0..%d) (empty=new)",text,maxIndex-1);
  }
  // Default index
  if(curIndex==-1)buf2[0]=0;
  else            sprintf(buf2,"%d",curIndex);
  
 retry:
  if(!QDlgString("Enter an index",buf,buf2,sizeof(buf2)))
    return -1;
  // User wants new item?
  if(!buf2[0])
    return maxIndex;
  // Check index
  n=atoi(buf2);
  if(n<0||n>=maxIndex)
  {
    if(!QMessageBox("Error","Value out of range."))
      return -1;
    else goto retry;
  }
  return n;
}

/********
* Modes *
********/
void SetMode(int newMode)
{
  int i;
  
  mode=newMode;
  switch(mode)
  {
    case MODE_POS   : PosSetMode(); break;
    case MODE_TCAM  : TCamSetMode(); break;
    case MODE_AI    : AISetMode(); break;
    case MODE_PROPS : PropsSetMode(); break;
    case MODE_SPLINE: SplineSetMode(); break;
  }
  // Set correct radio button
  for(i=0;i<MODE_MAX;i++)
  {
    radMode[i]->SetState(i==mode);
    radMode[i]->Invalidate();
  }
}

void SetOpButtons(cstring names[])
// Install new names in the buttons
{
  int i;
  for(i=0;;i++)
  {
//qdbg("names[%d]=%p (%s)\n",i,names[i],names[i]);
    if(names[i]==0)break;
    butModify[i]->SetText(names[i]);
  }
  for(;i<MAX_MODIFY;i++)
  {
    butModify[i]->SetText("---");
    butModify[i]->Invalidate();
  }
}

/********
* MENUS *
********/
void SetupMenus()
{
  QRect r;
  int   i;
  
  // IO
  r.x=Q_BC->GetX()-2;
  r.y=Q_BC->GetHeight()+Q_BC->GetY()+10;
  r.wid=150; r.hgt=35;
  for(i=0;i<MAX_IO;i++)
  {
    butIO[i]=new QButton(QSHELL,&r,ioName[i]);
    r.x+=r.wid+10;
    if((i%5)==4)
    {
      // Next line
      r.x=Q_BC->GetX()-2;
      r.y+=r.hgt+5;
    }
  }
  
  // Modes
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10;
  r.y=Q_BC->GetY()+2;
  r.wid=170; r.hgt=20;
  for(i=0;i<MODE_MAX;i++)
  {
    radMode[i]=new QRadio(QSHELL,&r,(string)modeName[i],1000);
    r.y+=r.hgt;
  }
  radMode[0]->SetState(TRUE);
  
  r.y+=5;
  new QLabel(QSHELL,&r,"Operations");
  r.y+=30;
  
  // Modify
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10;
  r.hgt=30;
  //r.y=Q_BC->GetY()-2;
  for(i=0;i<MAX_MODIFY;i++)
  {
    butModify[i]=new QButton(QSHELL,&r,"--op--");
    r.y+=r.hgt+5;
#ifdef OBS
    // Seperators
    if(i==1)
    {
      // Line ops coming up
      new QLabel(QSHELL,&r,"Line operations");
      r.y+=30;
    }
#endif
  }
}

/*********
* ERRORS *
*********/
void apperr(string s)
{
  qdbg("Error '%s'\n",s);
  QMessageBox("Error",s);
  if(defQErr)defQErr(s);
}

/********
* Reset *
********/
static void ResetTransform()
// Default looking position and direction
{
qdbg("ResetTransform()\n");
  tf->x=0; tf->y=10; tf->z=0;
  tf->xa=tf->ya=tf->za=0;
}

/******
* I/O *
******/
static QProgressDialog *dlgProgress;
static bool cbProgress(int cur,int total,cstring text)
{
  // If no dialog, go on
  if(!dlgProgress)return TRUE;

//qdbg("cbProgress dlg=%p\n",dlgProgress);
  dlgProgress->SetProgressText(text);
  dlgProgress->SetProgress(cur,total);
//qdbg("  ret Poll\n");
  return dlgProgress->Poll();
}
static void RecreateTrack()
// Delete an existing track and create a new one
{
  qstring tname;

  QDELETE(track);
  // Clear texture pool to avoid name conflicts a bit
  dglobal.texturePool->Clear();
  // Build a new track infrastructure
#ifdef ND_CRASH_LINUX
qdbg("RMGR=%p\n",RMGR);
qdbg("  cbProgress=%p\n",cbProgress);
  info->GetString("track.lasttrack",tname);
  RMGR->LoadTrack(tname,cbProgress);
qdbg("  loaded\n");
  //RMGR->LoadTrack(info->GetStringDirect("track.lasttrack"));
  track=RMGR->GetTrackVRML();
#endif
  track=new RTrackVRML();
//qlog(QLOG_INFO,"track=%p\n",track);

  // Reset selections
  curCamIndex=-1;
  curCam=0;
  curWPIndex=-1;
  curWP=0;
  nodePropIndex=-1;
  nodeProp=0;
  ResetTransform();

  // Hang into the Racer system
  RMGR->scene->SetTrack(track);
  RMGR->trackVRML=track;

  // Set the right name
  SetTrack(info->GetStringDirect("track.lasttrack"));
}
static void LoadTrack()
{
  QRect r(100,100,400,150);
  
  dlgProgress=new QProgressDialog(QSHELL,&r,"Loading","Loading models.");
    //QProgressDialog::NO_CANCEL);
  dlgProgress->Create();
  RecreateTrack();
  track->SetLoadCallback(cbProgress);
qdbg("LoadTrack0\n");
  if(!track->Load())
    QMessageBox(sError,"Can't load track!");
  track->SetLoadCallback(0);
qdbg("LoadTrack1\n");
  QDELETE(dlgProgress);
qdbg("LoadTrack2\n");
  isDirty=FALSE;
qdbg("LoadTrack3\n");
}
void SaveTrack()
{
  QRect r(100,100,400,100);
  dlgProgress=new QProgressDialog(QSHELL,&r,"Saving",
    "Saving track definition.",QProgressDialog::NO_CANCEL);
  dlgProgress->Create();
  if(!track->Save())
    QMessageBox(sError,"Can't save track!");
  QDELETE(dlgProgress);
  isDirty=FALSE;
}
void DocDirty()
// Mark document as dirty
{
  isDirty=TRUE;
}
bool DocIsDirty()
// Returns TRUE if document is dirty
{
  return isDirty;
}
void BuildCDTree()
// Create a collision AABB tree for (much) improved collision detection
{
  DAABBTree *tree;
  int i;
  tree=new DAABBTree();
  for(i=0;i<track->GetNodes();i++)
  {
qdbg("Add tree geode %d\n",i);
    tree->AddGeode(track->GetNode(i)->geode);
  }
  tree->BuildTree();
  tree->Save("test.cdt");
  QDELETE(tree);
}

/**************
* Default Ops *
**************/
void GotoPoint(DVector3 *pt)
// Move the camera to the point
{
  // Goto start pos
  tf->x=pt->x;
  tf->y=pt->y;
  tf->z=pt->z;
#ifdef OBS
  // Float above the pick point
  tf->y+=4.0f;
#endif
}
void GotoPickPoint()
// Move the camera to the pick point
{
  // Goto start pos
  tf->x=vPick.x;
  tf->y=vPick.y;
  tf->z=vPick.z;
  // Float above the pick point
  tf->y+=4.0f;
}

/*********
* EVENTS *
*********/
bool event(QEvent *e)
{
  int i;

  static int drag,dragX,dragY;

#ifdef OBS
  if(model)tf=model->GetTransformation();
  else     tf=0;
#endif
  
/*  {RTrackVRML *tp=track;
   i=5;
   i++;
  }*/

  if(e->type==QEvent::BUTTONPRESS)
  {
    //if(!drag)
    if(e->win==Q_BC)
    { drag|=(1<<(e->n-1));
      dragX=e->x; dragY=e->y;
      if(e->n==1)
      {
        PickTest(e->x,DRW->GetHeight()-e->y,tf,&vPick);
        vPickEnd=vPick;
      }
    }
  } else if(e->type==QEvent::BUTTONRELEASE)
  {
    drag&=~(1<<(e->n-1));
  } else if(e->type==QEvent::MOTIONNOTIFY)
  {
    if(!tf)return FALSE;
    
    // 2 mouse button version
    if((drag&(1|4))==(1|4))
    { tf->x+=(e->x-dragX)*400./720.;
      tf->y-=(e->y-dragY)*400./576.;
    } else if(drag&1)
    {
      // if(mode==PICK)
      PickTest(e->x,DRW->GetHeight()-e->y,tf,&vPickEnd);
      // else if(mode==CAMERA)
      // { tf->xa+=((float)(e->y-dragY))*300./576.;
      //   tf->ya+=((float)(e->x-dragX))*300./720.;
      // }
    } else if(drag&4)
    {
      // Rotate camera
      tf->xa+=((float)(e->y-dragY))*300./576.;
      tf->ya+=((float)(e->x-dragX))*300./720.;
      //tf->za+=(e->y-dragY)*300./576.;
      //tf->z+=(e->x-dragX);
    }
    dragX=e->x; dragY=e->y;
#ifdef OBS
qdbg("Camera location=(%.2f,%.2f,%.2f)\n",tf->x,tf->y,tf->z);
qdbg("       direction=(%.2f,%.2f,%.2f)\n",tf->xa,tf->ya,tf->za);
#endif
    return TRUE;
  }

  if(e->type==QEvent::CLICK)
  {
    if(e->win==butIO[0])
    {
      SelectTrack();
    } else if(e->win==butIO[1])
    {
      LoadTrack();
    } else if(e->win==butIO[2])
    {
      SaveTrack();
    } else if(e->win==butIO[3])
    {
      ConvertSCGTwrl();
    } else if(e->win==butIO[4])
    {
      ImportDOFs();
    } else if(e->win==butIO[5])
    {
      BatchAddLighting();
    } else if(e->win==butIO[6])
    {
      ConvertASE();
    } else if(e->win==butIO[7])
    {
      BuildCDTree();
    }

    // Check mode selection
    for(i=0;i<MODE_MAX;i++)
    {
      if(e->win==radMode[i])
        SetMode(i);
    }

    // Check op/modify buttons
    for(i=0;i<MAX_MODIFY;i++)
    {
      if(e->win==butModify[i])
      {
        switch(mode)
        {
          case MODE_POS   : PosOp(i); break;
          case MODE_TCAM  : TCamOp(i); break;
          case MODE_AI    : AIOp(i); break;
          case MODE_PROPS : PropsOp(i); break;
          case MODE_SPLINE: SplineOp(i); break;
        }
      }
    }
  }
  if(e->type==QEvent::MOTIONNOTIFY)
  {
    //MapXYtoCtl(e->x,e->y);
  } else if(e->type==QEvent::KEYPRESS)
  {
    switch(mode)
    {
      case MODE_POS   : PosKey(e->n); break;
      case MODE_TCAM  : TCamKey(e->n); break;
      case MODE_AI    : AIKey(e->n); break;
      case MODE_PROPS : PropsKey(e->n); break;
      case MODE_SPLINE: SplineKey(e->n); break;
    }
    if(e->n==QK_ESC)
      app->Exit(0);
    else
    if(e->n==QK_I)
    { ImportDOFs();
    } else if(e->n==QK_G)
    {
      // Always 'Goto'
      GotoPickPoint();
    } else if(e->n==QK_M)
    {
      // Mode switch
      switch(mode)
      {
        case MODE_POS   : SetMode(MODE_TCAM); break;
        case MODE_TCAM  : SetMode(MODE_AI); break;
        case MODE_AI    : SetMode(MODE_PROPS); break;
        case MODE_PROPS : SetMode(MODE_SPLINE); break;
        case MODE_SPLINE: SetMode(MODE_POS); break;
      }
    } else if(e->n==(QK_SHIFT|QK_L))
    {
      LoadTrack();
    } else if(e->n==(QK_SHIFT|QK_S))
    {
      SaveTrack();
    } else if(e->n==QK_F1)
    { SetMode(MODE_POS);
    } else if(e->n==QK_F2)
    { SetMode(MODE_TCAM);
    } else if(e->n==QK_F3)
    { SetMode(MODE_AI);
    } else if(e->n==QK_F4)
    { SetMode(MODE_PROPS);
    } else if(e->n==QK_F5)
    { SetMode(MODE_SPLINE);
    } else if(e->n==QK_LEFT)
    { tf->x-=MOVE_STEP;
    } else if(e->n==QK_RIGHT)
    { tf->x+=MOVE_STEP;
    } else if(e->n==QK_UP)
    { tf->z-=MOVE_STEP;
    } else if(e->n==QK_DOWN)
    { tf->z+=MOVE_STEP;
    } else if(e->n==(QK_SHIFT|QK_UP))
    { tf->y+=MOVE_STEP;
    } else if(e->n==(QK_SHIFT|QK_DOWN))
    { tf->y-=MOVE_STEP;
    } else if(e->n==(QK_CTRL|QK_M))
    { Mqreport(1);
    } else if(e->n==(QK_CTRL|QK_W))
    {
      ConvertDOF2WRL();
    }
  }

  return FALSE;
}

/********
* Stats *
********/
struct Stats
{
  int x,y;
} stats;
void StatsStart()
{
  stats.x=20; stats.y=20;
  QCV->Set2D();
  QCV->SetColor(255,255,255);
  QCV->SetFont(app->GetSystemFont());
}
void StatsAdd(cstring s)
{
  QCV->Text(s,stats.x,stats.y);
  stats.y+=20;
}

/***********
* Painting *
***********/
void DbgPrj(cstring s)
{
  DMatrix4 matPrj;
  glGetFloatv(GL_PROJECTION_MATRIX,matPrj.GetM());
  matPrj.DbgPrint(s);
//qdbg("glCtx: %p\n",glXGetCurrentContext());
}
void SetupViewport(int w,int h,dfloat fov)
{
  static bool fSetup=FALSE;
  static DMatrix4 matPrj;
  static dfloat visibility;
  
#ifdef ND_FOV_IS_DYNAMIC
  if(fSetup)
  {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matPrj.GetM());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    return;
  }
#endif
  
  // Calculate projection
//qdbg("w=%d, h=%d\n",w,h);
  //glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if(visibility==0.0f)
    visibility=info->GetFloat("gfx.visibility",300.f);
  gluPerspective(fov,(GLfloat)w/(GLfloat)h,1.0,visibility);
  
  // Remember for later faster projection
  glGetFloatv(GL_PROJECTION_MATRIX,matPrj.GetM());
//DbgPrj("SetupViewport");
#ifdef OBS
matPrj.DbgPrint("Projection");
qdbg("glCtx: %p\n",glXGetCurrentContext());
#endif
  
#ifdef OBS
  float d=1000.0f;
  glFrustum(-w/d/2,w/d/2,-h/d/2,h/d/2,1,100000);
#endif
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //glTranslatef(0,0,-d);
  
  fSetup=TRUE;
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

void PaintTrack(int flags)
// Paint the entire track
// flags&1; output on debug the timing results
// flags&2; use tcam.cpp's TCamTarget() function (a bit hardcoded)
{
  QVector3 *v;
  int  i,x,y;
  char buf[256];
  
  GGetCV()->Select();
  GGetCV()->Enable(QCanvas::IS3D);
  
//qdbg("-------- PaintTrack\n");

  dfloat fov=DEFAULT_FOV;
  if(flags&2)
  {
    // Get FOV from camera
    if(curCam)
      fov=curCam->GetZoom();
  } else
  {
    // Get FOV from scene
    fov=RMGR->scene->GetCamAngle();
  }
  SetupViewport(DRW->GetWidth(),DRW->GetHeight(),fov);

  // Fog
  if(fFog)
  {
    GLfloat density;
    GLfloat fogColor[4]={ 0.4,0.4,0.5,1.0 };
    
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE,GL_LINEAR);
    glFogfv(GL_FOG_COLOR,fogColor);
    glFogf(GL_FOG_DENSITY,0.01);
    glFogf(GL_FOG_END,info->GetFloat("gfx.visibility",300.f));
    glHint(GL_FOG_HINT,GL_DONT_CARE);
    
    glClearColor(track->clearColor[0],track->clearColor[1],
      track->clearColor[2],track->clearColor[3]);
  } else
  {
    glDisable(GL_FOG);
    glClearColor(track->clearColor[0],track->clearColor[1],
      track->clearColor[2],track->clearColor[3]);
    //glClearColor(51.f/255.f,93.f/255.f,165.f/255.f,0);
  }
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  //glClear(GL_DEPTH_BUFFER_BIT);

  // Paint track
  if(track)
  {
    if(flags&2)
    {
      TCamTarget();
    } else
    {
      // Use normal camera
      // Camera is negated, angles should be negative
      glRotatef(tf->za,0,0,1);
      glRotatef(tf->xa,1,0,0);
      glRotatef(tf->ya,0,1,0);
      glTranslatef(-tf->x,-tf->y,-tf->z);
      // Fake car camera position for sky displacements
      previewCar->GetPosition()->x=tf->x;
      previewCar->GetPosition()->y=tf->y;
      previewCar->GetPosition()->z=tf->z;
      // Make the current camera select the preview car to
      // pick up the position/origin
      RMGR->scene->SetCamCar(previewCar);
    }
    
    // Light
    float lightPos[4]={ -1,1,1,0 };
    //float ambient[4]={ .8,.8,1,0 };
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
    //glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);

#ifdef OBS_BLACK_BUG
glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
glDisable(GL_LIGHTING);
float em[4]={ 1,1,1,1 };
glMaterialfv(GL_FRONT,GL_EMISSION,em);
track->sun->Disable();
glDisable(GL_TEXTURE_2D);
glDisable(GL_DEPTH_TEST);
glDisable(GL_FOG);
glShadeModel(GL_FLAT);
QCV->GetGLContext()->SetSeparateSpecularColor(FALSE);
#endif

    track->GetCuller()->CalcFrustumEquations();
    track->Paint();
    switch(mode)
    {
      case MODE_POS   : track->PaintHidden(); break;
      case MODE_TCAM  :
        track->PaintHiddenCams();
        if(curCamIndex>=0)
          track->PaintHiddenTCamCubes(curCamIndex);
        if(flags&2)
        {
          // Preview is underway; paint the target cube
          TCamPaintTarget();
        }
        break;
      case MODE_AI    : track->PaintHiddenWayPoints(); break;
      case MODE_PROPS : PropsPaint(); break;
      case MODE_SPLINE: track->PaintHiddenRoadVertices(); SplinePaint(); break;
    }
    
    // Paint pick vectors
//#ifdef OBS
glDisable(GL_LIGHTING);
glDisable(GL_CULL_FACE);
//glDisable(GL_DEPTH_TEST);
glDisable(GL_TEXTURE_2D);
//#endif
    glColor3f(1,1,1);
    glBegin(GL_LINES);
      glVertex3f(vPick.x,vPick.y,vPick.z);
      glVertex3f(vPick.x,vPick.y+1.0f,vPick.z);
      glColor3f(1,1,0);
      glVertex3f(vPickEnd.x,vPickEnd.y,vPickEnd.z);
      glVertex3f(vPickEnd.x,vPickEnd.y+1.0f,vPickEnd.z);
      // Connect the two
      glColor3f(1,1,.5);
      glVertex3f(vPick.x,vPick.y+1.0f,vPick.z);
      glVertex3f(vPickEnd.x,vPickEnd.y+1.0f,vPickEnd.z);
    glEnd();
  }
  
  // 2D stats; remember matrices for picking
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  
  // Stats
  fps->FrameRendered();
  StatsStart();
  sprintf(buf,"FPS: %.2f",fps->GetFPS());
  StatsAdd(buf);
  // Display selection line
  sprintf(buf,"Pick line: (X=%.2f, Y=%.2f, Z=%.2f) - (X=%.2f, Y=%.2f, Z=%.2f)",
    vPick.x,vPick.y,vPick.z,vPickEnd.x,vPickEnd.y,vPickEnd.z);
  StatsAdd(buf);
  switch(mode)
  {
    case MODE_POS   : PosStats(); break;
    case MODE_TCAM  : TCamStats(); break;
    case MODE_AI    : AIStats(); break;
    case MODE_PROPS : PropsStats(); break;
    case MODE_SPLINE: SplineStats(); break;
  }
  
  GSwap();

  // Restore matrices
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
//DbgPrj("PaintTrack1");

}

void idlefunc()
{
  // Only draw if no dialog is on top (lots of buggy drivers on Win98
  // paint over the dialog)
  if(Q_BC->IsEnabled())
    PaintTrack();
}

void Setup()
{
  app->RunPoll();
  qnInit();

  // D3
  // Use texture pool to save image memory
  dglobal.flags|=DGlobal::USE_TEXTURE_POOL;
  
  // GUI
  title=new QTitleBar(QSHELL,APP_TITLE);
  
  defQErr=QSetErrorHandler(apperr);

  // Create a Racer Manager, which is used in some places
  new RManager();
  RMGR->Create();
  RMGR->scene->env=new REnvironment();

  // Hardcode settings for TrackEd
  // Always turn on skies
  RMGR->flags&=~RManager::NO_SKY;
  
  // Fake like a car is present for the target/dolly etc
  if(!previewCar)
    previewCar=new RCar(DEFAULT_CAR);

  // Create a track and use the latest loaded track as its name
  RecreateTrack();
  
  // Track FPS
  fps=new DFPS();
  fFog=info->GetInt("gfx.fog");
  
  // Setup transformation
  tf=&aTF;
#ifdef OBS
  // Give it an initial tilt and distance
  tf->xa=90;
  tf->z=-700;
  // Start pos
  tf->x=0; tf->y=10; tf->z=0;
  tf->xa=tf->ya=tf->za=0;
#endif
  
  // GUI
  SetupMenus();
  
  // Get window up
  QSHELL->Invalidate();
  app->RunPoll();
  app->SetIdleProc(idlefunc);
  app->SetExitProc(exitfunc);
  app->SetEventProc(event);
  
  // Load track, just before restoring the transform (the load resets it)
  if(info->GetInt("settings.auto_load"))
    LoadTrack();

  // Restore settings
  dfloat v[6];
  SetMode(info->GetInt("settings.mode",MODE_POS));
  info->GetFloats("settings.transform",6,v);
  tf->x=v[0];
  tf->y=v[1];
  tf->z=v[2];
  tf->xa=v[3];
  tf->ya=v[4];
  tf->za=v[5];
}

void Run()
{
  Setup();
  app->Run();
}
