/*
 * RTrackF - flat track creation
 * 08-10-00: Created!
 * NOTES:
 * - Uses the OpenGL XYZ conventions (right-handed)
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
//#include <racer/trackf.h>
//#include <qlib/app.h>
#include <qlib/lex.h>
// stdlib is needed by some Linux installations
#include <stdlib.h>
#include <math.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "RTrackFlat"

RTrackFlat::RTrackFlat()
  : RTrack(), sectors(0)
{
  DBG_C("ctor")

  int i;
  createPos.SetToZero();
  for(i=0;i<MAX_SECTOR;i++)
  { sector[i]=0;
    sectorGfx[i]=0;
  }
  for(i=0;i<MAX_TEXTURE;i++)
    tex[i]=0;
  createHeading=0;
}

RTrackFlat::~RTrackFlat()
{
  int i;
  for(i=0;i<MAX_SECTOR;i++)
  { QFREE(sector[i]);
    QFREE(sectorGfx[i]);
  }
  for(i=0;i<MAX_TEXTURE;i++)
    QDELETE(tex[i]);
}

/***********
* CREATION *
***********/
static DBitMapTexture *CreateTexture(cstring fname)
{
  QImage img(fname);
  DBitMapTexture *t=0;
  
  if(img.IsRead())
  {
    t=new DBitMapTexture(&img);
  } else
  { qerr("RTrackFlat:CreateTextures(); can't load '%s'",fname);
  }
  return t;
}
bool RTrackFlat::CreateTextures()
// Returns FALSE if any texture was not completed
{
  DBG_C("CreateTextures")

  tex[0]=CreateTexture(RFindFile("images/road.tga",trackDir));
  if(!tex[0])return FALSE;
  // Let texture wrap (stuff in dtexture.cpp in the future)
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  return TRUE;
}

/*****************
* BASIC BUILDING *
*****************/
void RTrackFlat::ToWorld(float longi,float lat,DVector3 *vWorld)
// Convert long/lat position to actual world coordinates
// The position is offset to the current creation position, and rotated
// about the current heading to reflect the world position
{
  float ch=cos(createHeading);
  float sh=sin(createHeading);
//qdbg("ToWorld: %f,%f\n",longi,lat);
  // Rotate long/lat around current heading
  // 2D orientation for now
  vWorld->x=lat*ch-longi*sh;
  vWorld->y=0;
  vWorld->z=longi*ch+lat*sh;
  // Translate to creation position
  *vWorld+=createPos;
//qdbg("ToWorld: (%f,%f,%f)\n",vWorld->x,vWorld->y,vWorld->z);
}

bool RTrackFlat::AddSector(const RTF_Sector *newSector)
{
  RTF_SectorGfx *gs;
  int i;
  
  // Out of space?
  if(sectors==MAX_SECTOR)
  {
    qwarn("RTrackFlat:AddSector(); out of sectors");
    return FALSE;
  }
  sector[sectors]=(RTF_Sector*)qcalloc(sizeof(RTF_Sector));
  // Copy parameters
  *sector[sectors]=*newSector;

  // Create graphics for this sector
  sectorGfx[sectors]=(RTF_SectorGfx*)qcalloc(sizeof(RTF_SectorGfx));
  gs=sectorGfx[sectors];

  // #vertices
  int nVertices;
  DVector3 vWorld;
  rfloat *v,*tv;
  // Real life texture sizes (in meters)
  rfloat textureRLWid=10.0f,textureRLLen=5.0f;
  int    trackWidth;
  
  if(newSector->type==RTrackFlat::SECTOR_STRAIGHT)
  {
    nVertices=6;
//qdbg("> Straight with %d vertices\n",nVertices);
    gs->fVertex=(float*)qcalloc(sizeof(float)*nVertices*3);
    gs->vertices=nVertices;
    gs->fTexCoord=(float*)qcalloc(sizeof(float)*nVertices*2);
    v=gs->fVertex;
    tv=gs->fTexCoord;
    trackWidth=(int)(newSector->maxLatStart-newSector->minLatStart);
    
    // Create poly's
    // 012; 0 and 1 are at long=0, 2 and 3 are at the same X at long=length
    ToWorld(0,newSector->minLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=0; *tv++=0;
    ToWorld(0,newSector->maxLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=trackWidth/textureRLWid; *tv++=0;
    ToWorld(newSector->length,newSector->minLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=0; *tv++=newSector->length/textureRLLen;

    // 132
    ToWorld(0,newSector->maxLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=trackWidth/textureRLWid; *tv++=0;
    ToWorld(newSector->length,newSector->maxLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=trackWidth/textureRLWid; *tv++=newSector->length/textureRLLen;
    ToWorld(newSector->length,newSector->minLatStart,&vWorld);
    *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
    *tv++=0; *tv++=newSector->length/textureRLLen;
    
    // Next position
    DVector3 offset;
    ToWorld(newSector->length,0,&offset);
    offset-=createPos;
    createPos+=offset;
    
  } else if(newSector->type==RTrackFlat::SECTOR_CURVE)
  {
    int steps=newSector->steps;
    DVector3 p[4];
    rfloat innerRadius,outerRadius,meanRadius;
    rfloat angle,longDistance,longDistanceNext;
    rfloat txLeft,txRight;
    
    // Calculate average radius (for textures)
    // Does NOT work for modifiable radii during the curve
    meanRadius=(newSector->outerRadiusStart-newSector->innerRadiusStart)/2;
    
    nVertices=3*2*steps;
qdbg("> Curve with %d vertices\n",nVertices);
    gs->fVertex=(rfloat*)qcalloc(sizeof(rfloat)*nVertices*3);
    gs->vertices=nVertices;
    gs->fTexCoord=(float*)qcalloc(sizeof(float)*nVertices*2);
    v=gs->fVertex;
    tv=gs->fTexCoord;
    
    for(i=0;i<steps;i++)
    {
      // Calculate radii at this step
      innerRadius=newSector->innerRadiusStart;
      outerRadius=newSector->outerRadiusStart;
      
      // Calculate longitudinal distance (in the center of the track)
      angle=((rfloat)(i+1))*newSector->angle/((rfloat)steps);
      angle=angle/RR_RAD_DEG_FACTOR;
      longDistanceNext=angle*meanRadius;
      // Calculate angle at this step
      angle=((rfloat)i)*newSector->angle/((rfloat)steps);
      angle=angle/RR_RAD_DEG_FACTOR;
      longDistance=angle*meanRadius;
      // Calculate resulting texture left/right
      txLeft=0;
      txRight=(outerRadius-innerRadius)/textureRLWid;
      
      // Radians
      p[0].x=cos(angle)*innerRadius-(outerRadius-innerRadius)/2;
      p[0].y=0;
      p[0].z=sin(angle)*innerRadius;
      p[1].x=cos(angle)*outerRadius-(outerRadius-innerRadius)/2;
      p[1].y=0;
      p[1].z=sin(angle)*outerRadius;
      
      // Calculate angle at next step
      angle=((rfloat)(i+1))*newSector->angle/((rfloat)steps);
//qdbg("next step angle=%f\n",angle);
      // Radians
      angle=angle/RR_RAD_DEG_FACTOR;
      p[2].x=cos(angle)*innerRadius-(outerRadius-innerRadius)/2;
      p[2].y=0;
      p[2].z=sin(angle)*innerRadius;
      p[3].x=cos(angle)*outerRadius-(outerRadius-innerRadius)/2;
      p[3].y=0;
      p[3].z=sin(angle)*outerRadius;
      
      // Offset
      p[0].x-=innerRadius;
      p[1].x-=innerRadius;
      p[2].x-=innerRadius;
      p[3].x-=innerRadius;
      
      ToWorld(p[0].z,p[0].x,&vWorld);
//qdbg("curve p0=(%f,%f,%f)\n",vWorld.x,vWorld.y,vWorld.z);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txLeft; *tv++=longDistance/textureRLLen;
      ToWorld(p[1].z,p[1].x,&vWorld);
//qdbg("curve p1=(%f,%f,%f)\n",vWorld.x,vWorld.y,vWorld.z);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txRight; *tv++=longDistance/textureRLLen;
      ToWorld(p[2].z,p[2].x,&vWorld);
//qdbg("curve p2=(%f,%f,%f)\n",vWorld.x,vWorld.y,vWorld.z);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txLeft; *tv++=longDistanceNext/textureRLLen;
      
      ToWorld(p[1].z,p[1].x,&vWorld);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txRight; *tv++=longDistance/textureRLLen;
      ToWorld(p[3].z,p[3].x,&vWorld);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txRight; *tv++=longDistanceNext/textureRLLen;
      ToWorld(p[2].z,p[2].x,&vWorld);
      *v++=vWorld.x; *v++=vWorld.y; *v++=vWorld.z;
      *tv++=txLeft; *tv++=longDistanceNext/textureRLLen;
    }
    
    // Next position
    DVector3 offset;
    rfloat   radiusEnd;
    angle=newSector->angle/RR_RAD_DEG_FACTOR;
    radiusEnd=(newSector->innerRadiusStart+newSector->outerRadiusStart)/2;
//qdbg("radiusEnd=%f, long=%f, lat=%f\n",
//radiusEnd,sin(angle)*radiusEnd,cos(angle)*radiusEnd);
    ToWorld(sin(angle)*radiusEnd,cos(angle)*radiusEnd-radiusEnd,&offset);
    // Undo translation
//qdbg("offset=(%f,%f,%f)\n",offset.x,offset.y,offset.z);
    offset-=createPos;
    createPos+=offset;
//qdbg("new createPos=(%f,%f,%f)\n",createPos.x,createPos.y,createPos.z);
    createHeading+=newSector->angle/RR_RAD_DEG_FACTOR;
    
  } else
  { qwarn("Curve nyi");
    nVertices=0;
  }

  // Sector added
  sectors++;
  return TRUE;
}

/***********
* PAINTING *
***********/
void RTrackFlat::PaintSector(int n)
// Paint sector #n
{
  DBG_C("PaintSector")
  DBG_ARG_I(n)

//qdbg("RTrackFlat:PaintSector(%d)\n",n);
  RTF_SectorGfx *sec=sectorGfx[n];
  
  // Paint polygons
  glEnableClientState(GL_VERTEX_ARRAY);
  //glEnableClientState(GL_NORMAL_ARRAY);
  //glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  /*if(1)
  { glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }*/
  // Disable features
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  glVertexPointer(3,GL_FLOAT,0,sec->fVertex);
  //glNormalPointer(GL_FLOAT,0,&face_nrm[first*N_PER_F]);
  if(sec->fTexCoord)
  { // Select texture
//qdbg("tex[0]=%p\n",tex[0]);
    if(tex[0])tex[0]->Select();
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,sec->fTexCoord);
  } else
  { glDisable(GL_TEXTURE_2D);
  }
  /*if(tface_v)
  { glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,&tface_v[first*N_PER_TF]);
  }*/
  //glDisable(GL_BLEND);
  //glDrawArrays(GL_TRIANGLES,0,faces*3);
  glDrawArrays(GL_TRIANGLES,0,sec->vertices);
  
  // Restore state
  glDisableClientState(GL_VERTEX_ARRAY);
  //glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void RTrackFlat::Paint()
// Given the current position, paint the track
{
  DBG_C("Paint")

//qdbg("RTrackFlat::Paint()\n");
  int i;
  for(i=0;i<sectors;i++)
  {
    // Paint sector i
    PaintSector(i);
  }
}

/**********
* LOADING *
**********/
#define LA      lex->GetLookahead()
#define NEXTLA	lex->FetchToken()

enum // Tokens
{
  T_STRAIGHT,
  T_CURVE,
  T_WIDTH,
  T_STEPS,T_INNER,T_OUTER
};

static void MatchSC(QLex *lex)
// Next token should be ';' to signal end of command
{
  if(lex->GetLookahead()!=LEX_EOC)
  {
    lex->Error("Expected ';' to end command");
  }
}

bool RTrackFlat::Load()
// Loads a track in the TrackFlat format (very simple)
{
  QLex *lex;
  qstring s;
  int     la;
  RTF_Sector sector;
  rfloat  width=10;             // Default width of track

  // First load generic data
  if(!RTrack::Load())return FALSE;

  lex=new QLex();
  lex->AddToken("straight",T_STRAIGHT);
  lex->AddToken("curve",T_CURVE);
  lex->AddToken("width",T_WIDTH);
  lex->AddToken("steps",T_STEPS);
  lex->AddToken("inner",T_INNER);
  lex->AddToken("outer",T_OUTER);

  cstring fname=(cstring)file;
qdbg("RTrackFlat:Load(); file=%s\n",fname);
  //if(!lex->Load((string)fname))
  if(!lex->Load((string)RFindFile(fname,GetTrackDir())))
  {
    qerr("RTrackFlat:Load; Can't load source file '%s'",fname);
    QDELETE(lex);
    return FALSE;
  }

  // Select OpenGL context to create textures in
  if(!QCV)
  { qerr("RTrackFlat: can't load track without a canvas");
    QDELETE(lex);
    return FALSE;
  }
  QCV->Select();
  CreateTextures();
  
  memset(&sector,0,sizeof(sector));
  while(1)
  {
    la=lex->GetLookahead();
    if(la==LEX_EOF)break;
//qdbg("token '%s' %d\n",lex->GetLookaheadStr(),lex->GetLookahead());
    if(la==T_STRAIGHT)
    {
      sector.type=RTrackFlat::SECTOR_STRAIGHT;
      NEXTLA;
      sector.length=atof(lex->GetLookaheadStr());
      NEXTLA; MatchSC(lex);
      // Rest of data
      sector.minLatStart=-width/2;
      sector.maxLatStart=width/2;
      sector.minLatEnd=-width/2;
      sector.maxLatEnd=width/2;
      AddSector(&sector);
    } else if(la==T_CURVE)
    {
      // CURVE
      sector.type=RTrackFlat::SECTOR_CURVE;
      // Defaults
      sector.steps=4;
      NEXTLA;
      sector.angle=atof(lex->GetLookaheadStr());
      NEXTLA;
      // Options
      sector.innerRadiusStart=1;
      sector.innerRadiusEnd=1;
      sector.outerRadiusStart=width+sector.innerRadiusStart;
      sector.outerRadiusEnd=width+sector.innerRadiusEnd;
     do_curve_opt:
      if(LA==T_STEPS)
      { NEXTLA;
        sector.steps=atoi(lex->GetLookaheadStr());
        NEXTLA; goto do_curve_opt;
      } else if(LA==T_INNER)
      { NEXTLA;
        sector.innerRadiusStart=atof(lex->GetLookaheadStr());
        NEXTLA;
        sector.innerRadiusEnd=atof(lex->GetLookaheadStr());
        NEXTLA; goto do_curve_opt;
      } else if(LA==T_OUTER)
      { NEXTLA;
        sector.outerRadiusStart=atof(lex->GetLookaheadStr());
        NEXTLA;
        sector.outerRadiusEnd=atof(lex->GetLookaheadStr());
        NEXTLA; goto do_curve_opt;
      }
      AddSector(&sector);
    } else if(la==T_WIDTH)
    {
      // WIDTH <w>
      NEXTLA;
      width=atof(lex->GetLookaheadStr());
      NEXTLA;
    } else
    { // Unknown
      char buf[1024];
      sprintf(buf,"Unknown keyword '%s'",lex->GetLookaheadStr());
      lex->Warn(buf);
    }
    NEXTLA;
  }
  QDELETE(lex);

  return TRUE;
}

