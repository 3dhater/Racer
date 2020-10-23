/*
 * RTrackVRML - VRML/SCGT-type tracks
 * 12-11-00: Created!
 * 10-03-01: Optimized a bit in the ray intersection parts.
 * NOTES:
 * - Currently uses .wrl files, although this will probably change
 * into .dof files for speed later on.
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/trackv.h>
#include <d3/global.h>
#include <d3/culler.h>
#include <d3/intersect.h>
#include <qlib/app.h>
#include <math.h>
DEBUG_ENABLE

// Local trace?
//#define LTRACE

// Use only the Y spline? (v0.4.7 behavior)
// Newer versions use a 3D surface patch (XYZ splines)
//#define USE_Y_SPLINE

// File to contain geode names
#define FILE_GEOMETRY  "geometry.ini"

// Macro to avoid calling BuildCuller() at every track trace
#define QUICK_BUILD_CULLER   if(!(flags&CULLER_READY))BuildCuller()

#undef  DBG_CLASS
#define DBG_CLASS "RTrackVRML"

RTrackVRML::RTrackVRML()
  : RTrack(), flags(0), nodes(0)
{
  DBG_C("ctor")

  int i;
  for(i=0;i<MAX_NODE;i++)
  { node[i]=0;
  }
  culler=new DCullerSphereList();
}

RTrackVRML::~RTrackVRML()
{
  int i;
  for(i=0;i<MAX_NODE;i++)
  { if(node[i])
    { QDELETE(node[i]->geode);
      QDELETE(node[i]);
    }
  }
  QDELETE(culler);
}

/*****************
* BASIC BUILDING *
*****************/
static void OptimizeModel(DGeode *geode)
// Make sure the geode paints fast for VRML tracks
{
  int i;
  
  if(RMGR&&RMGR->IsEnabled(RManager::NO_TRACK_LIGHTING))
  {
    // Make sure geode has no normals (no lighting used)
    geode->DestroyNormals();
  }
  
  // All materials are largely equal, just a decal-ed texture.
  // Avoid state changes in the geode's materials
  for(i=0;i<geode->materials;i++)
  {
    geode->material[i]->Enable(
      /*DMaterial::NO_TEXTURE_ENABLING|*/
      DMaterial::NO_MATERIAL_PROPERTIES|
      DMaterial::NO_BLEND_PROPERTIES);
  }
  // SCGT VRML nodes are never drawn at a different position
  // This may change in the future, since I'd really like
  // reusing of grandstands, little houses etc.
  // So this optimization may disappear in the future (or be
  // done only if the position is (0,0,0))
  geode->Set2D();
  // All geodes are part of one big scene
  geode->EnableCSG();
}
#ifdef OBS
// Seemed to take 1.4% of time in a small profile session! Inlined.
RTV_Node *RTrackVRML::GetNode(int n)
{
  if(n<0||n>=nodes)return 0;
  return node[n];
}
#endif
bool RTrackVRML::AddNode(const RTV_Node *newNode)
{
  RTV_Node *nn;
  
  // Out of space?
  if(nodes==MAX_NODE)
  {
    qwarn("RTrackVRML:AddNodes(); out of nodes");
    return FALSE;
  }
  //node[nodes]=(RTV_Node*)qcalloc(sizeof(RTV_Node));
  node[nodes]=new RTV_Node();
  
  // Copy parameters
  nn=node[nodes];
  nn->type=newNode->type;
  nn->flags=newNode->flags;
  nn->fileName=newNode->fileName;
  nn->geode=newNode->geode;

  // Prepare model for fast batch drawing
  OptimizeModel(newNode->geode);
  
  // Node added
  nodes++;
  
  // Track was modified
  DestroyCuller();
  return TRUE;
}

/**********
* Culling *
**********/
void RTrackVRML::BuildCuller()
// Create culling structure
{
  int i;
  
  if(flags&CULLER_READY)return;
qdbg("RTrackVRML:Paint(); create culler\n");
  
  for(i=0;i<nodes;i++)
  {
    // Don't include sky objects, they are drawn separately
    if(node[i]->flags&RTV_Node::F_SKY)
      continue;

    culler->AddGeode(node[i]->geode);
    
    // Copy some culler properties for use in intersection functions
    node[i]->center=culler->GetNode(culler->GetNodes()-1)->center;
    node[i]->radius=culler->GetNode(culler->GetNodes()-1)->radius;
qdbg("add geode %p\n",node[i]->geode);
  }
  flags|=CULLER_READY;
}
void RTrackVRML::DestroyCuller()
{
  culler->Destroy();
  flags&=~CULLER_READY;
}

/***********
* PAINTING *
***********/
void RTrackVRML::PaintNode(int n)
// Paint sector #n
{
  DBG_C("PaintNode")
  DBG_ARG_I(n)

  RTV_Node *pNode;
  
  pNode=node[n];
//qdbg("RTrackVRML:PaintNode(%d)=%p\n",n,pNode);
  pNode->geode->EnableCSG();
  pNode->geode->Paint();
}

void RTrackVRML::Paint()
// Given the current graphics matrix, paint the track
{
  DBG_C("Paint")

  int i;
  DVector3 origin;
  
  QUICK_BUILD_CULLER;
  
  // Update culler (if viewpoint has changed)
  culler->CalcFrustumEquations();
  
  // Set OpenGL state
  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  
  if(RMGR->IsEnabled(RManager::NO_SKY))
    goto skip_sky;

  // Paint sky objects in a first, separate pass
  glDisable(GL_LIGHTING);
  //glDisable(GL_DEPTH_TEST);
  glEnable(GL_DEPTH_TEST);
  // No fog for the sky; it's too far away and should be included
  // already in the sky model
  if(RMGR->IsEnabled(RManager::ENABLE_FOG))
    glDisable(GL_FOG);

  // Make sure node is visible; the sky is far away
  rfloat w,h;
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  w=RMGR->resWidth;
  h=RMGR->resHeight;
  glLoadIdentity();
  gluPerspective(RMGR->scene->GetCamAngle(),(GLfloat)w/(GLfloat)h,
    1.0,10000.0f);
  glMatrixMode(GL_MODELVIEW);
  //glPushMatrix();
  for(i=0;i<nodes;i++)
  {
//qdbg("RTrackVRML:Paint(); sky node %d\n",i);
    // Is it a sky model?
    if(node[i]->flags&RTV_Node::F_SKY)
    {
//qdbg("node %p SKY\n",node[i]);
//qdbg("RMGR=%p, sc=%p\n",RMGR,RMGR->scene);
      // Relative to camera or car?
      if(node[i]->flags&RTV_Node::F_RELATIVE_POS)
      {
        // Translate the sky model so it approximately is always the
        // same distance from the viewer
        RMGR->scene->GetCameraOrigin(&origin);
        glPushMatrix();
        //glTranslatef(origin.x,origin.y,origin.z);
        glTranslatef(origin.x,0,origin.z);
//origin.DbgPrint("RTrackV:Paint() origin");
      }
      if(node[i]->flags&RTV_Node::F_NO_ZBUFFER)
      {
        glDisable(GL_DEPTH_TEST);
      } else
      {
        glEnable(GL_DEPTH_TEST);
      }
      node[i]->geode->Paint();
      if(node[i]->flags&RTV_Node::F_RELATIVE_POS)
      {
        // Get back untranslated position
        glPopMatrix();
      }
    }
  }
  // Restore projection after skies have been drawn
  //glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  // Re-enable fog if applicable
  if(RMGR->IsEnabled(RManager::ENABLE_FOG))
    glEnable(GL_FOG);

 skip_sky:
 
  // Track objects are normally drawn with Z-buffer
  glEnable(GL_DEPTH_TEST);
  
  if(RMGR&&RMGR->IsEnabled(RManager::NO_TRACK_LIGHTING))
  {
    // No lighting
    glDisable(GL_LIGHTING);
    glShadeModel(GL_FLAT);
    // Octane1 and Indigo2 gets red tracks; trying to resolve it here
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  } else
  {
    // Lit track
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glEnable(GL_LIGHT0);
    
    float emission[4]={ .4,.4,.4,0 };
    //float emission[4]={ 1,1,1,0 };
    glMaterialfv(GL_FRONT,GL_EMISSION,emission);
    glMaterialf(GL_FRONT,GL_SHININESS,50.0);
  }

  dglobal.EnableBatchRendering();
  // Leave painting to culler
//qdbg("RTrackVRML:Paint(); culler Paint\n",i);
  culler->Paint();
  dglobal.DisableBatchRendering();
//qdbg("RTrackVRML:Paint(); culler Painted\n",i);
}

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
static void PaintRope(DVector3 *v1,DVector3 *v2,QColor *col)
// Paint rope with poles
{
  SetGLColor(col);
  glBegin(GL_LINES);
    // Both 
    glVertex3f(v1->x,v1->y,v1->z);
    glVertex3f(v1->x,v1->y+1,v1->z);
    glVertex3f(v2->x,v2->y,v2->z);
    glVertex3f(v2->x,v2->y+1,v2->z);
    // Connection
    glVertex3f(v1->x,v1->y+1,v1->z);
    glVertex3f(v2->x,v2->y+1,v2->z);
  glEnd();
}
static void PaintCircle(DVector3 *v,rfloat radius,QColor *col)
// Paint circle
{
  int i,steps=30;
  rfloat x,z;
  SetGLColor(col);
  glBegin(GL_LINE_LOOP);
    // Calc segment
    for(i=0;i<=steps;i++)
    {
      x=radius*sin((i*360/steps)/RR_RAD2DEG);
      z=radius*cos((i*360/steps)/RR_RAD2DEG);
      glVertex3f(v->x+x,v->y,v->z+z);
    }
  glEnd();
}
void RTrackVRML::PaintHidden()
// Paint hidden stuff
{
  DBG_C("PaintHidden")

  int i;
  
#ifdef RR_GFX_OGL
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
#endif

  // Paint timelines
  for(i=0;i<GetTimeLines();i++)
  {
    RTimeLine *t=GetTimeLine(i);
    QColor col(255,255,0);
    PaintRope(&t->from,&t->to,&col);
  }
  for(i=0;i<GetGridPosCount();i++)
  {
    RCarPos *p=GetGridPos(i);
    QColor col(0,255,0);
    PaintRope(&p->from,&p->to,&col);
  }
  for(i=0;i<GetPitPosCount();i++)
  {
    RCarPos *p=GetPitPos(i);
    QColor col(255,0,255);
    PaintRope(&p->from,&p->to,&col);
  }
  
  // Restore
#ifdef RR_GFX_OGL
  glEnable(GL_DEPTH_TEST);
#endif
}
void RTrackVRML::PaintHiddenCams()
// Paint track cameras (for editing)
{
  DBG_C("PaintHiddenCams")
  
  int i;
  
#ifdef RR_GFX_OGL
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
  //glDisable(GL_DEPTH_TEST);
#endif
  for(i=0;i<GetTrackCamCount();i++)
  {
    RTrackCam *t=GetTrackCam(i);
    QColor colNrm(255,0,0),colHi(255,180,180),
      colPerpNrm(0,255,0),colPerpHi(180,255,180),
      *col;
//qdbg("tcam r=%f\n",t->radius);
    if(t->flags&RTrackCam::HILITE)col=&colHi;
    else                          col=&colNrm;
    PaintRope(&t->pos,&t->posDir,col);
    PaintCircle(&t->pos,t->radius,col);
    if(t->flags&RTrackCam::DOLLY)
    {
      // Paint dolly line
      if(t->flags&RTrackCam::HILITE)col=&colPerpHi;
      else                          col=&colPerpNrm;
      PaintRope(&t->pos,&t->posDolly,col);
    }
  }
#ifdef RR_GFX_OGL
  // Restore
  glEnable(GL_DEPTH_TEST);
#endif
}
void RTrackVRML::PaintHiddenWayPoints()
// Paint waypoints (for editing)
{
  DBG_C("PaintHiddenWayPoints")
  int i;
  
#ifdef RR_GFX_OGL
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
#endif
  for(i=0;i<GetWayPointCount();i++)
  {
    RWayPoint *p=wayPoint[i];
    QColor colNrm(255,0,0),colHi(255,180,180),
      colPerpNrm(0,255,0),colPerpHi(180,255,180),
      *col;
    if(p->flags&RWayPoint::SELECTED)col=&colHi;
    else                            col=&colNrm;
    PaintRope(&p->pos,&p->posDir,col);
    if(p->flags&RWayPoint::SELECTED)col=&colPerpHi;
    else                            col=&colPerpNrm;
    PaintRope(&p->left,&p->right,col);
  }
#ifdef RR_GFX_OGL
  // Restore
  glEnable(GL_DEPTH_TEST);
#endif
}
void RTrackVRML::PaintHiddenRoadVertices()
// Paint vertices (for road splines)
{
  DBG_C("PaintHiddenRoadVertices")
  int     i,j,v;
  DGeode *geode;
  DGeob  *geob;
  
#ifdef RR_GFX_OGL
//qdbg("RTrackVRML:PaintHiddenRoadVertices()\n");
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glPointSize(2);
  for(i=0;i<nodes;i++)
  {
    geode=node[i]->geode;
    for(j=0;j<geode->geobs;j++)
    {
      geob=geode->geob[j];
      glColor3f(0,1,0);
      glBegin(GL_POINTS);
      for(v=0;v<geob->vertices;v++)
      {
        glVertex3f(geob->vertex[v*3+0],
          geob->vertex[v*3+1],geob->vertex[v*3+2]);
      }
      glEnd();
    }
  }
#endif
}
void RTrackVRML::PaintHiddenSplineTris()
// Paint tris of the spline surface
{
  DBG_C("PaintHiddenSplineTris")
  
#ifdef RR_GFX_OGL
  int i,j;
  float x,ix;
  RSplineRep *sr=GetSplineRep();
  RSplineTri *st;
  DHermiteSpline *sp;
  
//qdbg("PaintHiddenSplineTris; tris=%d\n",sr->GetTris());
  // Paint triangles from the spline
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  for(i=0;i<sr->GetTris();i++)
  {
    st=sr->GetTri(i);
    glColor3f(.5,1,.5);
    glBegin(GL_LINE_LOOP);
      for(j=0;j<3;j++)
      {
        glVertex3f(st->v[j].x,st->v[j].y,st->v[j].z);
      }
    glEnd();
  }
  // Paint spline line traces
    DVector3 *v1,*v2,v;
    int       maxLine;
  
  for(i=0;i<2;i++)
  {
    sp=sr->GetSplineHM(i,1);          // Y values only
//qdbg("sp=%p\n",sp);
    glColor3f(1,0,0);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    // Connect all lines
    maxLine=sr->GetLines();
    for(j=0;j<maxLine;j++)
    {
      x=j;
      v1=&sr->GetLine(j)->cp[i];
      v2=&sr->GetLine((j+1)%maxLine)->cp[i];
      for(ix=0;ix<1.0;ix+=0.1)
      {
        // Linear interpolate location
        v.x=v1->x+(v2->x-v1->x)*ix;
        v.z=v1->z+(v2->z-v1->z)*ix;
        // Get height from spline
        v.y=sp->GetValue(x+ix);
//v.DbgPrint("v");
        glVertex3f(v.x,v.y,v.z);
      }
    }
    glEnd();
    glLineWidth(1);
  }
  // Paint spline flow (spacing is important)
  for(i=0;i<2;i++)
  {
    sp=sr->GetSplineHM(i,1);
//qdbg("sp=%p\n",sp);
    glColor3f(1,1,0);
    glLineWidth(3);
    glBegin(GL_POINTS);
    // Draw the dots
    maxLine=sr->GetLines();
    for(j=0;j<maxLine;j++)
    {
      x=j;
      v1=&sr->GetLine(j)->cp[i];
      v2=&sr->GetLine((j+1)%maxLine)->cp[i];
      for(ix=0;ix<1.0;ix+=0.1)
      {
        // Linear interpolate location
        v.x=v1->x+(v2->x-v1->x)*ix;
        v.z=v1->z+(v2->z-v1->z)*ix;
        // Get height from spline
        v.y=sp->GetValue(x+ix);
//v.DbgPrint("v");
        glVertex3f(v.x,v.y,v.z);
      }
    }
    glEnd();
    glLineWidth(1);
  }
#endif
}

/**********
* LOADING *
**********/
bool RTrackVRML::Load()
// Loads a track in the TrackFlat format (very simple)
// Calls the application back if a function is supplied
// using SetLoadCallback() (this function may take some time).
{
  QInfo *infoTrk;
  QInfoIterator *ii;
  qstring s,fileName;
  RTV_Node node;
  DGeode *model;
  char buf[256];
  int  current,total;

qdbg("RTrackVRML:Load()\n");
  // First load generic data
  if(!RTrack::Load())
  {
    qdbg("RTrack:Load() failed\n");
    return FALSE;
  }
//return FALSE;
qdbg("  load VRML stuff\n");

  // Select OpenGL context to create textures in
  if(!QCV)
  { qerr("RTrackVRML: can't load track without a canvas");
    return FALSE;
  }
  
  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_GEOMETRY);
  infoTrk=new QInfo(buf);
  
  // Count #objects to come (for the callback function)
  ii=new QInfoIterator(infoTrk,"objects");
  for(total=0;ii->GetNext(s);total++);
  QDELETE(ii);
  
  // Actually load the objects
  current=0;
  // Optimize texture loading
  dglobal.EnableTexturePool();
  
  ii=new QInfoIterator(infoTrk,"objects");
  while(ii->GetNext(s))
  {
//qdbg("object '%s'\n",(cstring)s);
    node.type=0;
    node.flags=0;
    
    sprintf(buf,"%s.file",s.cstr());
    infoTrk->GetString(buf,fileName);
    if(fileName.IsEmpty())
    {
      // No file given, this may be done to avoid loading some
      // objects temporarily
      continue;
    }
    // Get flags; for old tracks (<v0.4.6), the default is to have everything
    // collide and everything be a surface (which it was back then)
    sprintf(buf,"%s.flags",s.cstr());
    node.flags=infoTrk->GetInt(buf,RTV_Node::F_COLLIDE|RTV_Node::F_SURFACE);

    // Use last part of the info path as the name
    node.fileName=QFileExtension(s)+1;
    // Create textures in QCV (!)
    QCV->Select();
    model=new DGeode(fileName);
    { 
      sprintf(buf,"%s/%s",(cstring)trackDir,(cstring)fileName);
      QFile f(RFindFile(fileName,trackDir));
//qdbg("  importing '%s'\n",RFindFile(fileName,trackDir));
      model->SetMapPath(RFindFile(".",trackDir));
//qdbg("  map path='%s'\n",RFindFile(".",trackDir));
//qdbg("  model=%p\n",model);
      if(!model->ImportDOF(&f))
      {
        qwarn("Can't import DOF '%s'",(cstring)fileName);
        continue;
      }
    }
//qdbg("  model loaded\n");
    // Transfer model to node (don't free geode)
    node.geode=model;
//qdbg("  AddNode()\n");
    AddNode(&node);
    
    // Notify application of progress
    current++;
//qdbg("  cbLoad %p\n",cbLoad);
    if(cbLoad)
      if(!cbLoad(current,total,node.fileName))break;
  }
//qdbg("  QDEL ii/infoTrk\n");
  QDELETE(ii);
  QDELETE(infoTrk);

  // Load the rest (AFTER loading all models)
  if(!LoadSurfaceTypes())return FALSE;

qdbg("RTrackVRML:Load() RET\n");
  return TRUE;
}

/*********
* Saving *
*********/
bool RTrackVRML::Save()
{
  int i;
  QInfo *infoTrk;
  char buf[256],buf2[256];
  
  // First save generic data
  if(!RTrack::Save())return FALSE;
  
  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_GEOMETRY);
  infoTrk=new QInfo(buf);
  
  // Write geode names
  for(i=0;i<nodes;i++)
  {
//qdbg("Save node '%s'\n",(cstring)node[i]->fileName);
    sprintf(buf,"objects.%s.file",node[i]->fileName.cstr());
    sprintf(buf2,"%s.dof",(cstring)node[i]->fileName);
    infoTrk->SetString(buf,buf2);
    sprintf(buf,"objects.%s.flags",node[i]->fileName.cstr());
    sprintf(buf2,"%d",node[i]->flags);
    infoTrk->SetString(buf,buf2);
  }
  QDELETE(infoTrk);
  return TRUE;
}

/********************
* Surface detection *
********************/

/********************
* Manual projection *
********************/
#ifdef OBS
static void Unproject(DVector3 *vWin,DVector3 *vObj)
// Unprojects a window coordinate (vWin) into a 3D coordinate (vObj)
// vWin->z defines the depth into the display
// Is not that fast, since it retrieves matrix state information,
// and it inverts the model*proj matrix.
// For some easy picking, it will do though.
{
  double mModel[16],mPrj[16];
  int    viewport[4];
  GLdouble wx,wy,wz;
  GLdouble ox,oy,oz;
  
//qdbg("Unprj: glCtx: %p\n",glXGetCurrentContext());
  // Retrieve matrices and viewport settings
  glGetDoublev(GL_MODELVIEW_MATRIX,mModel);
  glGetDoublev(GL_PROJECTION_MATRIX,mPrj);
  glGetIntegerv(GL_VIEWPORT,viewport);

#ifdef OBS
  for(i=0;i<16;i++)
  {
    qdbg("mModel[%d]=%g\n",i,mModel[i]);
  }  
  for(i=0;i<16;i++)
  {
    qdbg("mPrj[%d]=%g\n",i,mPrj[i]);
  }
#endif
  
  // Reverse projection
  wx=vWin->x;
  wy=vWin->y;
  wz=vWin->z;
  if(!gluUnProject(wx,wy,wz,mModel,mPrj,viewport,&ox,&oy,&oz))
  {
    qwarn("Unproject failed");
    vObj->SetToZero();
    return;
  }
  
  vObj->x=(dfloat)ox;
  vObj->y=(dfloat)oy;
  vObj->z=(dfloat)oz;
  
  if(!gluProject(ox,oy,oz,mModel,mPrj,viewport,&wx,&wy,&wz))
  {
    qwarn("Project failed");
  }
//qdbg("gluProject: %f,%f,%f\n",wx,wy,wz);
}
static void Project(DVector3 *vObj,DVector3 *vWin)
// !! Text must be updated
// Projects a window coordinate (vWin) into a 3D coordinate (vObj)
// vWin->z defines the depth into the display
// Is not that fast, since it retrieves matrix state information,
// and it inverts the model*proj matrix.
// For some easy picking, it will do though.
{
  double mModel[16],mPrj[16];
  int    viewport[4];
  GLdouble wx,wy,wz;
  GLdouble ox,oy,oz;
  
  // Retrieve matrices and viewport settings
  glGetDoublev(GL_MODELVIEW_MATRIX,mModel);
  glGetDoublev(GL_PROJECTION_MATRIX,mPrj);
  glGetIntegerv(GL_VIEWPORT,viewport);

#ifdef OBS
  for(i=0;i<16;i++)
  {
    qdbg("mModel[%d]=%g\n",i,mModel[i]);
  }  
  for(i=0;i<16;i++)
  {
    qdbg("mPrj[%d]=%g\n",i,mPrj[i]);
  }
#endif
  
  // Reverse projection
  ox=vObj->x;
  oy=vObj->y;
  oz=vObj->z;
  if(!gluProject(ox,oy,oz,mModel,mPrj,viewport,&wx,&wy,&wz))
  {
    qwarn("gluProject failed");
    vWin->SetToZero();
    return;
  }
  
  vWin->x=(dfloat)wx;
  vWin->y=(dfloat)wy;
  vWin->z=(dfloat)wz;
}
#endif

/****************************
* RAY - SPHERE Intersection *
****************************/
static bool RaySphereIntersect(const DVector3 *origin,const DVector3 *dir,
  const DVector3 *center,dfloat radius,DVector3 *intersect)
// Determines whether a ray intersects the sphere
// From 'Real-Time Rendering', page 299.
// Calculates intersection point in 'intersect', if wished. Otherwise,
// pass 0 for 'intersect' (is quite a bit faster).
{
  DVector3 l;
  dfloat   d,lSquared,mSquared;
  dfloat   rSquared=radius*radius;
  
#ifdef OBS
qdbg("origin %f,%f,%f\n",origin->x,origin->y,origin->z);
qdbg("dir %f,%f,%f\n",dir->x,dir->y,dir->z);
#endif
  // Calculate line from origin to center
  l.x=center->x-origin->x;
  l.y=center->y-origin->y;
  l.z=center->z-origin->z;
//qdbg("line=%f,%f,%f\n",l.x,l.y,l.z);
  // Calculate length of projection of direction onto that line
  d=l.Dot(*dir);
//qdbg("  proj. d=%f\n",d);
  lSquared=l.DotSelf();
//qdbg("  l^2=%f, radius=%f (^2=%f)\n",lSquared,radius,rSquared);
  if(d<0&&lSquared>rSquared)
  {
    // No intersection
    return FALSE;
  }
  
  // Check for how far we are off the center
  mSquared=lSquared-d*d;
  if(mSquared>rSquared)return FALSE;
  
  // Calculate intersection point, if requested
  if(!intersect)return TRUE;
  
  dfloat q,t;
  q=sqrt(rSquared-mSquared);
  if(lSquared>rSquared)t=d-q;
  else                 t=d+q;
//qdbg("t=%f\n",t);
  // Intersection point is t*(*dir) away from the origin
  intersect->x=origin->x+t*dir->x;
  intersect->y=origin->y+t*dir->y;
  intersect->z=origin->z+t*dir->z;
  return TRUE;
}

/******************************
* RAY - TRIANGLE Intersection *
******************************/
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2)\
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 

// Cull backfacing polygons?
// 17-06-01: Not culling because the spline triangles are half
// upward, half downward (done to get good ordering for easier
// U/V coordinates)
// 18-06-01: Reinstated culling (is faster); spline triangles
// now always point upward.
#define OPT_CULL_RAY_TRIANGLE

static int RayTriangleIntersect(const dfloat orig[3],const dfloat dir[3],
  const dfloat vert0[3],const dfloat vert1[3],const dfloat vert2[3],
  dfloat *t,dfloat *u,dfloat *v)
// From Real-Time Rendering, page 305
// Returns 0 if not hit is found
// If a hit is found, t contains the distance along the ray (dir)
// and u/v contain u/v coordinates into the triangle (like texture
// coordinates).
{
  dfloat edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
  dfloat det,inv_det;

   /* find vectors for two edges sharing vert0 */
   SUB(edge1, vert1, vert0);
   SUB(edge2, vert2, vert0);

   /* begin calculating determinant - also used to calculate U parameter */
   CROSS(pvec, dir, edge2);

   /* if determinant is near zero, ray lies in plane of triangle */
   det = DOT(edge1, pvec);

#ifdef OPT_CULL_RAY_TRIANGLE
  // Culling section; triangle will be culled if not facing the right
  // way.
  if(det<D3_EPSILON)
    return 0;

   /* calculate distance from vert0 to ray origin */
   SUB(tvec, orig, vert0);

   /* calculate U parameter and test bounds */
   *u = DOT(tvec, pvec);
   if (*u < 0.0 || *u > det)
      return 0;

   /* prepare to test V parameter */
   CROSS(qvec, tvec, edge1);

    /* calculate V parameter and test bounds */
   *v = DOT(dir, qvec);
   if (*v < 0.0 || *u + *v > det)
      return 0;

   /* calculate t, scale parameters, ray intersects triangle */
   *t = DOT(edge2, qvec);
   inv_det = 1.0 / det;
   *t *= inv_det;
   *u *= inv_det;
   *v *= inv_det;
#else
  // The non-culling branch
  if(det>-D3_EPSILON&&det<D3_EPSILON)
    return 0;
  inv_det = 1.0 / det;

  /* calculate distance from vert0 to ray origin */
  SUB(tvec, orig, vert0);

  /* calculate U parameter and test bounds */
  *u = DOT(tvec, pvec) * inv_det;
  if (*u < 0.0 || *u > 1.0)
    return 0;

  /* prepare to test V parameter */
  CROSS(qvec, tvec, edge1);

  /* calculate V parameter and test bounds */
  *v = DOT(dir, qvec) * inv_det;
  if (*v < 0.0 || *u + *v > 1.0)
    return 0;

  /* calculate t, ray intersects triangle */
  *t = DOT(edge2, qvec) * inv_det;
#endif
  // We've got an intersection!
  return 1;
}

/********************************
* Finding a triangle in a geode *
********************************/
static bool FindGeodeTriangle(DVector3 *origin,DVector3 *dir,DGeode *geode,
  RSurfaceInfo *si,DVector3 *inter)
// The ray is traced and tested if it hits any triangle in 'geode'.
// Returns index of triangle.
// If a triangle is hit, the info is saved as cache info in 'si'.
{
  int     g,tri,n,nTris;
  dfloat *pVertex;
  dindex *pIndex;
  DGeob  *geob;
  dfloat  t,u,v;
  dfloat  minT;
  
  // Find the surface CLOSEST to the origin
  minT=9999.0f;
  for(g=0;g<geode->geobs;g++)
  {
    geob=geode->geob[g];
    pVertex=geob->vertex;
    if(!pVertex)continue;
    pIndex=geob->index;
    //for(tri=geob->indices/3;tri>0;tri--,pIndex+=3)
    nTris=geob->indices/3;
    for(tri=0;tri<nTris;tri++,pIndex+=3)
    {
      if(RayTriangleIntersect(&origin->x,&dir->x,
        &pVertex[(pIndex[0])*3],
        &pVertex[(pIndex[1])*3],
        &pVertex[(pIndex[2])*3],
        &t,&u,&v))
      {
//qdbg("Intersect TRI! t=%.2f, u=%.2f, v=%.2f\n",t,u,v);
        // Don't count this triangle if it's further away than
        // a previously found triangle, or the triangle is found
        // in the other direction (above the wheel for example; a bridge
        // possibly).
        if(t<0||t>minT)continue;
        
        // Calculate intersection point
        // 2 ways are possible:
        // - follow the ray: origin+t*direction
        // - interpolate vertices; u and v are barycentric coordinates
        //   which we can average and thus get the weighted average
        //   of the 3 vertices that make up the triangle
        inter->x=origin->x+t*dir->x;
        inter->y=origin->y+t*dir->y;
        inter->z=origin->z+t*dir->z;
        // Store cache information (for next time)
        si->lastTri=tri;
        si->lastGeob=g;

        // Detect material
        n=geob->GetBurstForFace(tri);
        si->material=geob->GetMaterialForID(geob->burstMtlID[n]);
        si->surfaceType=(RSurfaceType*)si->material->GetUserData();
//qdbg("mat '%s'\n",si->material->name.cstr());

        // Caller must store si->lastNode (!) for cache info to be complete
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**********
* Caching *
**********/
static bool CacheCheckTriangle(const DVector3 *origin,const DVector3 *dir,
  const DGeode *geode,const RSurfaceInfo *si,DVector3 *inter)
// Check if the cached triangle still hits
// If so, it returs TRUE and the intersection in 'inter'
{
  //int     g,tri,n;
  dfloat *pVertex;
  dindex *pIndex;
  DGeob  *geob;
  dfloat  t,u,v;

//qdbg("CacheCheckTriangle(geode=%p, si=%p)\n",geode,si);
  // Cache info available?
  if(si->lastNode==-1)return FALSE;
  if(geode==0)return FALSE;
  
  // Get back to the right triangle
  geob=geode->geob[si->lastGeob];
  pVertex=geob->vertex;
  if(!pVertex)return FALSE;
  pIndex=&geob->index[si->lastTri*3];
  if(RayTriangleIntersect(&origin->x,&dir->x,
    &pVertex[pIndex[0]*3],
    &pVertex[pIndex[1]*3],
    &pVertex[pIndex[2]*3],
    &t,&u,&v))
  {
//qdbg("Cached tri still hits\n");
    // Calculate intersection point
    inter->x=origin->x+t*dir->x;
    inter->y=origin->y+t*dir->y;
    inter->z=origin->z+t*dir->z;
    return TRUE;
  }
//qdbg("Cache miss!\n");
  return FALSE;
}

/*************************
* Spline surface testing *
*************************/
static bool FindSplineTri(RTrack *track,DVector3 *origin,DVector3 *dir,
  RSurfaceInfo *si,DVector3 *inter)
// Much like FindGeodeTriangle(), this searches the spline representation
// for a hit.
// If a hit is found, TRUE is returned and 'inter' contains the
// intersection point.
{
  RSplineRep *sr=track->GetSplineRep();
  RSplineTri *st;
  int    i,tris;
  dfloat t,u,v,minT=9999.0f;

  // Check cache first; if a node was specified, we're probably
  // off the splined track
  if(si->lastNode<0&&si->lastTri>=0)
  {
    st=sr->GetTri(si->lastTri);
    if(RayTriangleIntersect(&origin->x,&dir->x,
      &st->v[0].x,&st->v[1].x,&st->v[2].x,
      &t,&u,&v))
    {
      // Cached triangle is still being hit
      i=si->lastTri;
      goto cache_hit;
    } //else qdbg("cahce miss\n");
  }
  
  tris=sr->GetTris();
#ifdef OBS
qdbg("FST(), tris=%d\n",tris);
origin->DbgPrint("org");
dir->DbgPrint("dir");
#endif

  for(i=0;i<tris;i++)
  {
    st=sr->GetTri(i);
#ifdef OBS
st->v[0].DbgPrint("v[0]");
st->v[1].DbgPrint("v[1]");
st->v[2].DbgPrint("v[2]");
#endif
    if(RayTriangleIntersect(&origin->x,&dir->x,
      &st->v[0].x,&st->v[1].x,&st->v[2].x,
      &t,&u,&v))
    {
      // Don't count the triangle if it's further
      if(t<0||t>minT)continue;
     
     cache_hit:
      // Calculate intersection point
//qdbg("FST: intersect tri %d at t=%f, u=%f, v=%f\n",i,t,u,v);
      //dfloat tSpline,hLeft,hRight,h,h0,h1;
      dfloat tSpline,h0,h1;
      // Get distance along spline
      if(i&1)
      {
        // Every 2nd triangle is ordered so that the V coordinate
        // is reversed. The U coordinate goes correctly from 0 to 1
        // for both triangles (in every quad pair) so that doesn't
        // need any adjustments (U measures how far the intersection
        // is in the direction of the 2nd vertex in the triangle, V
        // measures how far we are reaching for the 3rd vertex).
        v=1.0f-v;
        u=1.0f-u;
      } else
      {
        //v=1.0f-v;
      }
//if(i&1)v=1.0f-v;
      
#ifdef USE_Y_SPLINE
      // Get left and right height values
      tSpline=((dfloat)(i/2))+v;
      hLeft=sr->GetSplineHM(0,1)->GetValue(tSpline);
      hRight=sr->GetSplineHM(1,1)->GetValue(tSpline);
//qdbg("  hgt Left=%f, Right=%f\n",hLeft,hRight);
      // Linearly interpolate from left to right
      // For more explanations, check Racer's website.
      h=hLeft+u*(hRight-hLeft);
      inter->x=origin->x+t*dir->x;
      //inter->y=origin->y+t*dir->y;
      inter->y=h;
      inter->z=origin->z+t*dir->z;
#else
      // Use 3D patch
      tSpline=((dfloat)(i/2))+v;
      h0=sr->GetSplineHM(0,0)->GetValue(tSpline);
      h1=sr->GetSplineHM(1,0)->GetValue(tSpline);
      inter->x=h0+u*(h1-h0);

      h0=sr->GetSplineHM(0,1)->GetValue(tSpline);
      h1=sr->GetSplineHM(1,1)->GetValue(tSpline);
      inter->y=h0+u*(h1-h0);

      h0=sr->GetSplineHM(0,2)->GetValue(tSpline);
      h1=sr->GetSplineHM(1,2)->GetValue(tSpline);
      inter->z=h0+u*(h1-h0);
#endif
      // Cache this info because temporal coherency is high
      si->lastNode=-1;
      si->lastTri=i;
      // Store results
      si->x=inter->x;
      si->y=inter->y;
      si->z=inter->z;
      // Extra info (debugging)
      si->t=t;
      si->u=u;
      si->v=v;
      // Clear material, this indicates a normal road
      si->material=0;
      si->surfaceType=0;
      // Return now (should we look for other closer triangles?)
      return TRUE;
    }
  }
  // No hits in the spline surface
  return FALSE;
}
  
/*****************************
* Wheel-surface intersection *
*****************************/
void RTrackVRML::GetSurfaceInfo(DVector3 *pos,DVector3 *dir,
  RSurfaceInfo *si)
// Determines what the surface looks like under 'pos'
// 'dir' is the direction of the ray to trace (often like (0,-1,0))
// Currently returns valid (si->): x,y,z
{
  //DVector3 vWin,vPrj;
  DVector3 vOrigin,vDir,vIntersect;
  int i;
  RTV_Node *node;

#ifdef LTRACE
qdbg("RTrackVRML:GetSurfaceInfo()\n");
#endif

  // Default miss
  si->x=si->y=si->z=0;

  // Build ray definition
  vOrigin=*pos;
  vDir=*dir;
  //vDir.Normalize();
  
  // See if it hits a sphere in the track
  QUICK_BUILD_CULLER;
  
  // Check splines first (less triangles, faster hits, smooth surface)
  if(FindSplineTri(this,pos,dir,si,&vIntersect))
  {
    // Found a spline hit (on the surface)
    return;
  }
  
  // Check cached triangle first
  if(si->lastNode>=0)
  {
    if(CacheCheckTriangle(&vOrigin,&vDir,
      /*GetCuller()->*/ GetNode(si->lastNode)->geode,si,&vIntersect))
    {
      // We're done; no change!
      si->x=vIntersect.x;
      si->y=vIntersect.y;
      si->z=vIntersect.z;
      return;
    }
  }
  
  // Search all geodes, all geobs in every geode, and every
  // triangle in each geob
#ifdef OBS
  n=GetCuller()->GetNodes();
  for(i=0;i<n;i++)
#endif
  for(i=0;i<nodes;i++)
  {
    //node=GetCuller()->GetNode(i);
    node=GetNode(i);
    
    // Is this a surface node? (speeds up to declare less
    // surfaces nodes ofcourse, trees for example)
    if(!(node->flags&RTV_Node::F_SURFACE))continue;
    
#ifdef OBS
    DVector3 v;
    Project(&node->center,&v);
qdbg("Projected node center: %f,%f,%f\n",v.x,v.y,v.z);
#endif

#ifdef OBS
v=node->center;
qdbg("Sphere center: %.2f,%.2f,%.2f, r=%.f\n",v.x,v.y,v.z,node->radius);
#endif
    if(RaySphereIntersect(&vOrigin,&vDir,&node->center,node->radius,0))
    {
      if(FindGeodeTriangle(&vOrigin,&vDir,node->geode,si,&vIntersect))
      {
        // vIntersect contains 3D coordinate of triangle intersection
        //*pt=vIntersect;
        // Pass on point of intersection for surface
        si->x=vIntersect.x;
        si->y=vIntersect.y;
        si->z=vIntersect.z;
#ifdef OBS
qdbg("intersection point=(%.2f,%.2f,%.2f) node %d, geob %d, tri %d\n",
vIntersect.x,vIntersect.y,vIntersect.z,i,si->lastGeob,si->lastTri);
vOrigin.DbgPrint("origin");
vDir.DbgPrint("dir");
#endif
        // Store cache info that wasn't already stored
        // by FindGeodeTriangle()
        si->lastNode=i;
        
#ifdef OBS
        // Don't seek further
        break;
#endif
      }
    }
  }
//qdbg("--\n");

  // No hit detected; assume some default values
  //si->x=si->y=si->z=0;
}

/**********************
* Collision detection *
**********************/
static bool CDGeodeTriOBB(DGeode *geode,DOBB *obb,DVector3 *inter,
  DVector3 *normal)
// The geode is tested against the obb. If any interpenetration
// is present, TRUE will be returned and 'inter' contains the
// collision point, and 'normal' will contain the normal vector
// from the collision triangle (pointing to the OBB).
{
  int     g,tri,i,nTris;
  dfloat *pVertex;
  //dfloat *pCurrentVertex;
  dindex *pIndex;
  DGeob  *geob;
  //dfloat  t,u,v;
  //dfloat  minT;
  int     r;
  DVector3 triVertex[3],*pTriVertex[3];
  DVector3 vel0;
  dfloat   T;

  // For all geobs, test all triangles
  vel0.SetToZero();
#ifdef OBS
obb->center.DbgPrint("obb center");
obb->extents.DbgPrint("obb extents");
obb->axis[0].DbgPrint("obb axis0");
obb->axis[1].DbgPrint("obb axis1");
obb->axis[2].DbgPrint("obb axis2");
#endif
  pTriVertex[0]=&triVertex[0];
  pTriVertex[1]=&triVertex[1];
  pTriVertex[2]=&triVertex[2];
  for(g=0;g<geode->geobs;g++)
  {
    geob=geode->geob[g];
    pVertex=geob->vertex;
    if(!pVertex)continue;
    pIndex=geob->index;
    nTris=geob->indices/3;
    for(tri=0;tri<nTris;tri++,pIndex+=3)
    {
      for(i=0;i<3;i++)
      {
        // A bit of a hack to simulate vectors that use
        // the array contents directly (to get more speed)
        pTriVertex[i]=(DVector3*)&pVertex[pIndex[i]*3];
#ifdef OBS
        pCurrentVertex=&pVertex[pIndex[i]*3];
        triVertex[i].x=pCurrentVertex[0];
        triVertex[i].y=pCurrentVertex[1];
        triVertex[i].z=pCurrentVertex[2];
#endif
      }
#ifdef OBS
triVertex[0].DbgPrint("triV0");
triVertex[1].DbgPrint("triV1");
triVertex[2].DbgPrint("triV2");
#endif
      // Check if there's a hit during this timestep; notice
      // the triangle's velocity is 0 (static)
      r=d3FindTriObb(RR_TIMESTEP,pTriVertex,vel0,
        *obb,obb->linVel,T,*inter);
//qdbg("geob %p, tri %d; r=%d\n",geob,tri,r);
      // No separating axis?
      if(r==0)
      {
        // Collision!
        DVector3 e[2];
#ifdef OBS
triVertex[0].DbgPrint("v0");
triVertex[1].DbgPrint("v1");
triVertex[2].DbgPrint("v2");
#endif
        e[0]=*pTriVertex[1]-*pTriVertex[0];
        e[1]=*pTriVertex[2]-*pTriVertex[0];
#ifdef OBS
e[0].DbgPrint("e0");
e[1].DbgPrint("e1");
#endif
        normal->Cross(&e[0],&e[1]);
//normal->DbgPrint("normal");

        // Check if the OBB isn't LEAVING the triangle
        if(normal->Dot(obb->linVel)>0)
        {
          // We're moving away (linearly at least), so don't
          // record this as a collision.
//qdbg("avoiding collision\n");
          continue;
        }

        return TRUE;
      }
    }
  }
  return FALSE;
}
bool SphereIsNear(DVector3 *p,DVector3 *center,rfloat radius)
// Returns TRUE if point 'p' is inside the sphere at 'center'
// with radius 'radius'
{
  DVector3 l(center->x-p->x,center->y-p->y,center->z-p->z);
  if(l.LengthSquared()<radius*radius)
    return TRUE;
  return FALSE;
}
bool RTrackVRML::CollisionOBB(DOBB *obb,DVector3 *intersection,
  DVector3 *normal)
// Determines whether 'obb' hits a triangle in the track.
// If so, this function returns TRUE and 'intersection' contains
// the collision point.
// If not, 'intersection' is left untouched
{
  int i;
  RTV_Node *node;

  // Test for car-track collisions? (debugging)
  if(RMGR->IsDevEnabled(RManager::NO_TRACK_COLLISION))
    return FALSE;

//qdbg("RTrkV:CD_OBB\n");
  // See if it hits a sphere in the track
  QUICK_BUILD_CULLER;
  
  // Search all geodes, all geobs in every geode, and every
  // triangle in each geob
  //n=GetCuller()->GetNodes();
  for(i=0;i<nodes;i++)
  {
    node=GetNode(i);
    //node=GetCuller()->GetNode(i);
    
    // Can we collide with this node? (turning off lots of these
    // speeds up collision detection ofcourse, and sometimes
    // avoids hitting scenery, like some pit trees in Monaco)
    if(!(node->flags&RTV_Node::F_COLLIDE))continue;
    
#ifdef OBS
v=node->center;
qdbg("Sphere center: %.2f,%.2f,%.2f, r=%.f\n",v.x,v.y,v.z,node->radius);
#endif
    // Check if sphere is close to the box
    if(SphereIsNear(obb->GetCenter(),&node->center,node->radius))
    {
      // Check triangles
      if(CDGeodeTriOBB(node->geode,obb,intersection,normal))
      {
        // vIntersect contains 3D coordinate of triangle intersection
#ifdef OBS
qdbg("intersection point=(%.2f,%.2f,%.2f) node %d, geob %d, tri %d\n",
vIntersect.x,vIntersect.y,vIntersect.z,i,si->lastGeob,si->lastTri);
vOrigin.DbgPrint("origin");
vDir.DbgPrint("dir");
#endif
  
        // Don't seek further
        return TRUE;
      }
    }
  }
  return FALSE;
}
