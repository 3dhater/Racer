/*
 * RTrack - base class for Racer tracks
 * 01-11-00: Created!
 * (c) Dolphinity
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Local trace?
#define LTRACE

// Default track information file
#define TRACK_INI  "track.ini"
// File with special things, like grid positions, pit positions and timelines
#define FILE_SPECIAL   "special.ini"
// Spline surface info file
#define FILE_SPLINE    "spline.ini"
// Best laps file
#define BESTLAPS_INI   "bestlaps.ini"

RTrack::RTrack()
{
  int i;

  type="flat";
  cbLoad=0;
  timeLines=0;
  for(i=0;i<MAX_TIMELINE;i++)
    timeLine[i]=0;
  gridPosCount=pitPosCount=0;
  clearColor[0]=clearColor[1]=clearColor[2]=clearColor[3]=0.0f;
  for(i=0;i<MAX_TRACKCAM;i++)
    trackCam[i]=0;
  trackCams=0;
  for(i=0;i<MAX_WAYPOINT;i++)
    wayPoint[i]=0;
  wayPoints=0;
  surfaceTypes=0;
  infoStats=0;
  infoSpecial=0;
}
RTrack::~RTrack()
{
  int i;
 
qdbg("RTrack dtor\n");
qdbg("  infoSpecial=%p\n",infoSpecial);

  QDELETE(infoSpecial);
qdbg("  surfacesTypes=%d\n",surfaceTypes);
  for(i=0;i<trackCams;i++)
    QDELETE(trackCam[i]);
  for(i=0;i<wayPoints;i++)
    QDELETE(wayPoint[i]);
  for(i=0;i<surfaceTypes;i++)
    QDELETE(surfaceType[i]);
  for(i=0;i<timeLines;i++)
    QDELETE(timeLine[i]);
  QDELETE(infoStats);
qdbg("  dtor RET\n");
}

/**********
* Attribs *
**********/
RTimeLine *RTrack::GetTimeLine(int n)
{
  if(n<0||n>=timeLines)
  { qwarn("RTrack:GetTimeLine(%d) out of bounds",n);
    return 0;
  }
  return timeLine[n];
}

/****************
* Car positions *
****************/
RCarPos *RTrack::GetGridPos(int n)
{
  QASSERT_0(n>=0&&n<gridPosCount);
  return &gridPos[n];
}
RCarPos *RTrack::GetPitPos(int n)
{
  QASSERT_0(n>=0&&n<pitPosCount);
  return &pitPos[n];
}

/**********************
* SURFACE DESCRIPTION *
**********************/
void RTrack::GetSurfaceInfo(DVector3 *pos,DVector3 *dir,RSurfaceInfo *si)
// Retrieves info on track surface
// Note that normally only pos->x and pos->z are used. In case of
// bridges like in Suzuka however, pos->y may be used to return either
// the track above or below the bridge.
{
//qdbg("RTrack::GetSurfaceInfo()\n");

  // Default is flat boring track
  si->x=pos->x;
  si->y=0;
  si->z=pos->z;
  si->grade=0;
  si->bank=0;
  si->friction=1.0;
}

/*********
* Naming *
*********/
void RTrack::SetName(cstring _trackName)
// Load a track (of any type)
// This is the base function; a derived class should override this
// function, call us first (and check the return code), then load the
// actual track from 'file'.
{
  char    buf[256];
  //cstring s;
  
  trackName=_trackName;
  
  // Deduce track directory
  sprintf(buf,"data/tracks/%s",trackName.cstr());
  strcpy(buf,RFindDir(buf));
#ifdef OBS
  s=RFindFile(".",buf);
  // Strip "."
  strcpy(buf,s);
  buf[strlen(buf)-2]=0;
#endif
  trackDir=buf;
//qdbg("RTrack:SetName(%s) => trackDir='%s'\n",
//trackName.cstr(),(cstring)trackDir);
}

/**********
* LOADING *
**********/
static void RestoreVector(QInfo *info,cstring vName,DVector3 *v)
// Shortcut to store a vector
{
  info->GetFloats(vName,3,&v->x);
}
bool RTrack::LoadSpecial()
// Load positions and timelines etc
{
  QInfo *info;
  char   buf[256];
  int    i,j,n;
  RCarPos *p;
  cstring cubeName[3]={ "edge","close","far" };
 

  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_SPECIAL);
  infoSpecial=new QInfo(buf);
  info=infoSpecial;
  

  // Restore grid positions
  gridPosCount=info->GetInt("grid.count");
  if(gridPosCount>MAX_GRID_POS)gridPosCount=MAX_GRID_POS;
  for(i=0;i<gridPosCount;i++)
  {
    p=&gridPos[i];
    sprintf(buf,"grid.pos%d.from.x",i); p->from.x=info->GetFloat(buf);
    sprintf(buf,"grid.pos%d.from.y",i); p->from.y=info->GetFloat(buf);
    sprintf(buf,"grid.pos%d.from.z",i); p->from.z=info->GetFloat(buf);
    sprintf(buf,"grid.pos%d.to.x",i); p->to.x=info->GetFloat(buf);
    sprintf(buf,"grid.pos%d.to.y",i); p->to.y=info->GetFloat(buf);
    sprintf(buf,"grid.pos%d.to.z",i); p->to.z=info->GetFloat(buf);
  }
  
  // Restore pit positions
  pitPosCount=info->GetInt("pit.count");
  if(pitPosCount>MAX_PIT_POS)pitPosCount=MAX_PIT_POS;
  for(i=0;i<pitPosCount;i++)
  {
    p=&pitPos[i];
    sprintf(buf,"pit.pos%d.from.x",i); p->from.x=info->GetFloat(buf);
    sprintf(buf,"pit.pos%d.from.y",i); p->from.y=info->GetFloat(buf);
    sprintf(buf,"pit.pos%d.from.z",i); p->from.z=info->GetFloat(buf);
    sprintf(buf,"pit.pos%d.to.x",i); p->to.x=info->GetFloat(buf);
    sprintf(buf,"pit.pos%d.to.y",i); p->to.y=info->GetFloat(buf);
    sprintf(buf,"pit.pos%d.to.z",i); p->to.z=info->GetFloat(buf);
  }
  
  // Restore timelines
  timeLines=info->GetInt("timeline.count");
  if(timeLines>MAX_TIMELINE)timeLines=MAX_TIMELINE;
  for(i=0;i<timeLines;i++)
  {
    RTimeLine *t;
    DVector3 v1,v2;
    v1.SetToZero(); v2.SetToZero();
    timeLine[i]=new RTimeLine(&v1,&v2);
    t=timeLine[i];
    sprintf(buf,"timeline.line%d.from.x",i); v1.x=info->GetFloat(buf);
    sprintf(buf,"timeline.line%d.from.y",i); v1.y=info->GetFloat(buf);
    sprintf(buf,"timeline.line%d.from.z",i); v1.z=info->GetFloat(buf);
    sprintf(buf,"timeline.line%d.to.x",i); v2.x=info->GetFloat(buf);
    sprintf(buf,"timeline.line%d.to.y",i); v2.y=info->GetFloat(buf);
    sprintf(buf,"timeline.line%d.to.z",i); v2.z=info->GetFloat(buf);
    // Redefine to get variables right
    t->Define(&v1,&v2);
  }

  // Restore trackcams
  n=info->GetInt("tcam.count");
  if(n>=MAX_TRACKCAM)
  { qwarn("Track contains too many trackcams (max=%d, count=%d)",
      MAX_TRACKCAM,n);
    n=MAX_TRACKCAM;
  }
  for(i=0;i<n;i++)
  {
    RTrackCam *t;
    t=new RTrackCam();
    //trackCam.push_back(t);
    trackCam[trackCams]=t;
    trackCams++;
    sprintf(buf,"tcam.cam%d.pos.x",i); t->pos.x=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.pos.y",i); t->pos.y=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.pos.z",i); t->pos.z=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.pos2.x",i); t->posDir.x=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.pos2.y",i); t->posDir.y=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.pos2.z",i); t->posDir.z=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.posdolly.x",i); t->posDolly.x=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.posdolly.y",i); t->posDolly.y=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.posdolly.z",i); t->posDolly.z=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot.x",i); t->rot.x=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot.y",i); t->rot.y=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot.z",i); t->rot.z=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot2.x",i); t->rot2.x=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot2.y",i); t->rot2.y=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.rot2.z",i); t->rot2.z=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.radius",i); t->radius=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.group",i); t->group=info->GetInt(buf);
    sprintf(buf,"tcam.cam%d.zoom_edge",i); t->zoomEdge=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.zoom_close",i); t->zoomClose=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.zoom_far",i); t->zoomFar=info->GetFloat(buf);
    sprintf(buf,"tcam.cam%d.flags",i); t->flags=info->GetInt(buf);
    for(j=0;j<3;j++)
    {
      sprintf(buf,"tcam.cam%d.cube.%s",i,cubeName[j]);
      RestoreVector(info,buf,&t->cubePreview[j]);
//t->cubePreview[j].DbgPrint(cubeName[j]);
    }
    
    // Post-import; reset selection flag
    t->flags&=~RTrackCam::HILITE;
  }
  
  // Restore waypoints
  n=info->GetInt("waypoint.count");
  if(n>=MAX_WAYPOINT)
  { qwarn("Track contains too many waypoints (max=%d, count=%d)",
      MAX_WAYPOINT,n);
    n=MAX_WAYPOINT;
  }
  for(i=0;i<n;i++)
  {
    RWayPoint *p;
    p=new RWayPoint();
    wayPoint[i]=p;
    wayPoints++;
    //wayPoint.push_back(p);
    sprintf(buf,"waypoint.wp%d.pos",i); RestoreVector(info,buf,&p->pos);
    sprintf(buf,"waypoint.wp%d.posdir",i); RestoreVector(info,buf,&p->posDir);
    sprintf(buf,"waypoint.wp%d.left",i); RestoreVector(info,buf,&p->left);
    sprintf(buf,"waypoint.wp%d.right",i); RestoreVector(info,buf,&p->right);
    sprintf(buf,"waypoint.wp%d.lon",i); p->lon=info->GetFloat(buf);
  }

  // Restore graphics
  info->GetFloats("gfx.sky",4,clearColor);
#ifdef OBS
qdbg("RTrack:LoadSpecial(); clearCol=%f,%f,%f\n",
 clearColor[0],clearColor[1],clearColor[2]);
//qdbg("RTrack:LoadSpecial(); timeLines=%d\n",timeLines);
#endif

  // Restore splines
  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_SPLINE);
  info=new QInfo(buf);
qdbg("RTrack:LoadSpecial(); load splines\n");
  // Only load splines if user wants to use them (dev)
  if(!RMGR->IsDevEnabled(RManager::NO_SPLINES))
    splineRep.Load(info);
  QDELETE(info);
  
  return TRUE;
}
bool RTrack::LoadSurfaceTypes()
// Load surface definitions and apply those to the materials
{
  QInfo *info;
  RSurfaceType *st;
  RTrackVRML *track;
  char   buf[256],buf2[256];
  int    i,j,count;
  cstring surfName[RSurfaceType::MAX]=
  { "road","grass","gravel","stone","water","kerb","ice","snow","mud","sand" };
  bool   ret=TRUE;
  
  info=infoSpecial;
  // Need track pointer to VRML-type track (merge track classes in the
  // future)
  track=RMGR->GetTrackVRML();

  // Read surfaces and apply
  QInfoIterator it(info,"surfaces");
  qstring       name,pattern;
  while(it.GetNext(name))
  {
    if(surfaceTypes==MAX_SURFACE_TYPE)
    {
      qwarn("RTrack:LoadSurfaceTypes(); max (%d) reached",MAX_SURFACE_TYPE);
      break;
    }
    st=new RSurfaceType();
    surfaceType[surfaceTypes]=st;
    surfaceTypes++;
    sprintf(buf,"%s.type",name.cstr());
    info->GetString(buf,buf2);
qdbg("buf='%s', buf2='%s'\n",buf,buf2);
    strlwr(buf2);
    for(i=0;i<RSurfaceType::MAX;i++)
    { if(!strcmp(buf2,surfName[i]))
      {
        st->baseType=i;
        goto found_base;
      }
    }
    qwarn("RTrack:LoadSurfaceTypes(); type '%s' not recognized",buf2);
    ret=FALSE;
   found_base:
    sprintf(buf,"%s.subtype",name.cstr());
    st->subType=info->GetInt(buf);
    sprintf(buf,"%s.grip_factor",name.cstr());
    st->frictionFactor=info->GetFloat(buf,1);
    sprintf(buf,"%s.rolling_resistance_factor",name.cstr());
    st->rollingResistanceFactor=info->GetFloat(buf,1);
    // Pattern
    sprintf(buf,"%s.pattern",name.cstr());
    info->GetString(buf,pattern);
qdbg("Surface type %d: '%s', pattern '%s'\n",surfaceTypes-1,
 name.cstr(),pattern.cstr());

    // Apply surface type to all materials
    // Note that a lot of materials are found multiple times, but this
    // shouldn't matter
    count=0;
qdbg("  track=%p\n",track);
qdbg("  GetNodes=%d\n",track->GetNodes());
    for(i=0;i<track->GetNodes();i++)
    {
      DGeode *g=track->GetNode(i)->geode;
      for(j=0;j<g->materials;j++)
      {
        if(QMatch(pattern,g->material[j]->GetName()))
        {
//qdbg("  found material '%s' which matches pattern\n",
 //g->material[j]->GetName());
          g->material[j]->SetUserData((void*)st);
          count++;
//qdbg("  set data %p, get data %p\n",st,g->material[j]->GetUserData());
        }
      }
    }
    // Warn if surface wasn't applied to any material
    if(count==0)
    { qwarn("No surfaces found that match pattern '%s'\n",pattern.cstr());
    }
    //...
  }

qdbg("RTrack:LoadSurfaceTypes()\n");
for(i=0;i<surfaceTypes;i++)
qdbg("surfType[%d]=%p\n",i,surfaceType[i]);

  return ret;
}
bool RTrack::Load()
// Load a track (of any type)
// This is the base function; a derived class should override this
// function, call us first (and check the return code), then load the
// actual track from 'file'.
// After that, call LoadSurfaceTypes() to load an apply the surfaces.
{
  QInfo  *infoTrk;
  char    buf[256];

  // Find generic info on track
  infoTrk=new QInfo(RFindFile(TRACK_INI,trackDir));
  // Found the track.ini?
  if(!infoTrk->FileExists())
  { qwarn("Can't find track.ini for track %s",trackName.cstr());
    return FALSE;
  }
  infoTrk->GetString("track.type",type);
  infoTrk->GetString("track.name",name);
  infoTrk->GetString("track.creator",creator);
  infoTrk->GetString("track.length",length);
  infoTrk->GetString("track.file",file);
  QDELETE(infoTrk);

  // Load statistics
  sprintf(buf,"%s/%s",trackDir.cstr(),BESTLAPS_INI);
  //infoStats=new QInfo(RFindFile(BESTLAPS_INI,trackDir));
  infoStats=new QInfo(buf);
  stats.Load(infoStats,"results");

  if(!LoadSpecial())return FALSE;
 
#ifdef OBS
  // Find actual track 3D file
  strcpy(buf,RFindFile(file,trackDir));
  file=buf;
  if(!QFileExists(file))
  { qwarn("Can't find track file (%s) for track %s",(cstring)file,trackName);
    return FALSE;
  }
#endif
  return TRUE;
}

/*********
* Saving *
*********/
static void StoreVector(QInfo *info,cstring vName,DVector3 *v)
// Shortcut to store a vector
{
  char buf[100];
  sprintf(buf,"%f %f %f",v->x,v->y,v->z);
  info->SetString(vName,buf);
}
bool RTrack::SaveSpecial()
// Load positions and timelines etc
{
  QInfo *info;
  int i,j;
  RCarPos *p;
  char buf[100];
  cstring cubeName[3]={ "edge","close","far" };
  
  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_SPECIAL);
  info=new QInfo(buf);
  
  // Store grid positions
  info->SetInt("grid.count",gridPosCount);
  for(i=0;i<gridPosCount;i++)
  {
    p=&gridPos[i];
    sprintf(buf,"grid.pos%d.from.x",i); info->SetFloat(buf,p->from.x);
    sprintf(buf,"grid.pos%d.from.y",i); info->SetFloat(buf,p->from.y);
    sprintf(buf,"grid.pos%d.from.z",i); info->SetFloat(buf,p->from.z);
    sprintf(buf,"grid.pos%d.to.x",i); info->SetFloat(buf,p->to.x);
    sprintf(buf,"grid.pos%d.to.y",i); info->SetFloat(buf,p->to.y);
    sprintf(buf,"grid.pos%d.to.z",i); info->SetFloat(buf,p->to.z);
  }
  
  // Store pit positions
  info->SetInt("pit.count",pitPosCount);
  for(i=0;i<pitPosCount;i++)
  {
    p=&pitPos[i];
    sprintf(buf,"pit.pos%d.from.x",i); info->SetFloat(buf,p->from.x);
    sprintf(buf,"pit.pos%d.from.y",i); info->SetFloat(buf,p->from.y);
    sprintf(buf,"pit.pos%d.from.z",i); info->SetFloat(buf,p->from.z);
    sprintf(buf,"pit.pos%d.to.x",i); info->SetFloat(buf,p->to.x);
    sprintf(buf,"pit.pos%d.to.y",i); info->SetFloat(buf,p->to.y);
    sprintf(buf,"pit.pos%d.to.z",i); info->SetFloat(buf,p->to.z);
  }
  
  // Store timelines
  info->SetInt("timeline.count",timeLines);
  for(i=0;i<timeLines;i++)
  {
    RTimeLine *t;
    t=timeLine[i];
    sprintf(buf,"timeline.line%d.from.x",i); info->SetFloat(buf,t->from.x);
    sprintf(buf,"timeline.line%d.from.y",i); info->SetFloat(buf,t->from.y);
    sprintf(buf,"timeline.line%d.from.z",i); info->SetFloat(buf,t->from.z);
    sprintf(buf,"timeline.line%d.to.x",i); info->SetFloat(buf,t->to.x);
    sprintf(buf,"timeline.line%d.to.y",i); info->SetFloat(buf,t->to.y);
    sprintf(buf,"timeline.line%d.to.z",i); info->SetFloat(buf,t->to.z);
  }
  
  // Store trackcams
  info->SetInt("tcam.count",trackCams);
  for(i=0;i<trackCams;i++)
  {
    RTrackCam *t;
    t=trackCam[i];
    sprintf(buf,"tcam.cam%d.pos.x",i); info->SetFloat(buf,t->pos.x);
    sprintf(buf,"tcam.cam%d.pos.y",i); info->SetFloat(buf,t->pos.y);
    sprintf(buf,"tcam.cam%d.pos.z",i); info->SetFloat(buf,t->pos.z);
    sprintf(buf,"tcam.cam%d.pos2.x",i); info->SetFloat(buf,t->posDir.x);
    sprintf(buf,"tcam.cam%d.pos2.y",i); info->SetFloat(buf,t->posDir.y);
    sprintf(buf,"tcam.cam%d.pos2.z",i); info->SetFloat(buf,t->posDir.z);
    sprintf(buf,"tcam.cam%d.posdolly.x",i); info->SetFloat(buf,t->posDolly.x);
    sprintf(buf,"tcam.cam%d.posdolly.y",i); info->SetFloat(buf,t->posDolly.y);
    sprintf(buf,"tcam.cam%d.posdolly.z",i); info->SetFloat(buf,t->posDolly.z);
    sprintf(buf,"tcam.cam%d.rot.x",i); info->SetFloat(buf,t->rot.x);
    sprintf(buf,"tcam.cam%d.rot.y",i); info->SetFloat(buf,t->rot.y);
    sprintf(buf,"tcam.cam%d.rot.z",i); info->SetFloat(buf,t->rot.z);
    sprintf(buf,"tcam.cam%d.rot2.x",i); info->SetFloat(buf,t->rot2.x);
    sprintf(buf,"tcam.cam%d.rot2.y",i); info->SetFloat(buf,t->rot2.y);
    sprintf(buf,"tcam.cam%d.rot2.z",i); info->SetFloat(buf,t->rot2.z);
    sprintf(buf,"tcam.cam%d.radius",i); info->SetFloat(buf,t->radius);
    sprintf(buf,"tcam.cam%d.group",i); info->SetFloat(buf,t->group);
    sprintf(buf,"tcam.cam%d.zoom_edge",i); info->SetFloat(buf,t->zoomEdge);
    sprintf(buf,"tcam.cam%d.zoom_close",i); info->SetFloat(buf,t->zoomClose);
    sprintf(buf,"tcam.cam%d.zoom_far",i); info->SetFloat(buf,t->zoomFar);
    sprintf(buf,"tcam.cam%d.flags",i); info->SetInt(buf,t->flags);
    for(j=0;j<3;j++)
    {
      sprintf(buf,"tcam.cam%d.cube.%s",i,cubeName[j]);
      StoreVector(info,buf,&t->cubePreview[j]);
    }
  }
  
  // Store waypoints
  info->SetInt("waypoint.count",wayPoints);
  for(i=0;i<wayPoints;i++)
  {
    RWayPoint *p=wayPoint[i];
    sprintf(buf,"waypoint.wp%d.pos",i); StoreVector(info,buf,&p->pos);
    sprintf(buf,"waypoint.wp%d.posdir",i); StoreVector(info,buf,&p->posDir);
    sprintf(buf,"waypoint.wp%d.left",i); StoreVector(info,buf,&p->left);
    sprintf(buf,"waypoint.wp%d.right",i); StoreVector(info,buf,&p->right);
    sprintf(buf,"waypoint.wp%d.lon",i); info->SetFloat(buf,p->lon);
  }
  
  // Store graphics
  sprintf(buf,"%f %f %f %f",clearColor[0],clearColor[1],clearColor[2],
    clearColor[3]);
  info->SetString("gfx.sky",buf);
  
  QDELETE(info);
  
  // Store splines
  sprintf(buf,"%s/%s",(cstring)trackDir,FILE_SPLINE);
  info=new QInfo(buf);
  splineRep.Save(info);
  QDELETE(info);
  
  return TRUE;
}
bool RTrack::Save()
{
  if(!SaveSpecial())return FALSE;
  
  return TRUE;
}

/********
* PAINT *
********/
void RTrack::Paint()
// Painter; override in the derived class
{
}
void RTrack::PaintHidden()
// Paint hidden parts; override in subclasses
{
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
static void PaintCube(DVector3 *center,float size)
// Paint rope with poles
{
  QColor col(155,55,155);
  SetGLColor(&col);
  glBegin(GL_QUADS);
    // Front
    glColor3f(1,0.5,1);
    glVertex3f(center->x-size,center->y+size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y+size,center->z-size);
    // Left
    glColor3f(1,0.2,1);
    glVertex3f(center->x-size,center->y+size,center->z+size);
    glVertex3f(center->x-size,center->y+size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z-size);
    glVertex3f(center->x-size,center->y-size,center->z+size);
    // Right
    glColor3f(1,0.1,1);
    glVertex3f(center->x+size,center->y+size,center->z+size);
    glVertex3f(center->x+size,center->y+size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z-size);
    glVertex3f(center->x+size,center->y-size,center->z+size);
    // Back
    glColor3f(1,0.4,1);
    glVertex3f(center->x-size,center->y+size,center->z+size);
    glVertex3f(center->x-size,center->y-size,center->z+size);
    glVertex3f(center->x+size,center->y-size,center->z+size);
    glVertex3f(center->x+size,center->y+size,center->z+size);
  glEnd();
}
void RTrack::PaintHiddenTCamCubes(int n)
// Paint hidden parts; all camera cubes (n==-1), or 1 camera cube (n!=-1)
{
  int i;
  RTrackCam *tcam;
  
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  if(n==-1)
  {
    for(i=0;i<GetTrackCamCount();i++)
    {
     paint_cube:
      tcam=GetTrackCam(i);
      PaintCube(&tcam->cubePreview[0],2.0f);
      PaintCube(&tcam->cubePreview[1],2.0f);
      PaintCube(&tcam->cubePreview[2],2.0f);
      // Only 1 tcam?
      if(n!=-1)break;
    }
  } else
  {
    // 1 tcam only
    i=n;
    goto paint_cube;
  }
  glPopAttrib();
}

/*************
* Track cams *
*************/
RTrackCam *RTrack::GetTrackCam(int n)
{
//qdbg("RTrack:GetTrackCam(%d), count=%d, ptr=%p\n",n,trackCams,trackCam[n]);
  return trackCam[n];
}
bool RTrack::AddTrackCam(RTrackCam *cam)
{
//qdbg("RTrack:AddTrackCam\n");
  if(trackCams>=MAX_TRACKCAM)
  {
    qwarn("RTrack:AddTrackCam(); max (%d) exceeded",MAX_TRACKCAM);
    return FALSE;
  }
  trackCam[trackCams]=cam;
  trackCams++;
  //trackCam.push_back(cam);
  return TRUE;
}

/************
* Waypoints *
************/
bool RTrack::AddWayPoint(RWayPoint *wp)
{
//qdbg("RTrack:AddTrackCam\n");
  if(wayPoints>=MAX_WAYPOINT)
  {
    qwarn("RTrack:AddWayPoint(); max (%d) exceeded",MAX_WAYPOINT);
    return FALSE;
  }
  wayPoint[wayPoints]=wp;
  wayPoints++;
  return TRUE;
}

/**********************
* Collision detection *
**********************/
bool RTrack::CollisionOBB(DOBB *obb,DVector3 *intersection,DVector3 *normal)
// Stub
{
  return FALSE;
}

/*************
* Statistics *
*************/
void RTrack::Update()
// At a moment of idle processing, update any statistics
{
  stats.Update(infoStats,"results");
}
