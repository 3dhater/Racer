/*
 * DGeode - big loaded meshes (from 3D Studio Max / LightWave etc)
 * 10-01-00: Created!
 * 15-03-00: Bursts of polygons with the same material. Materials used.
 * 10-11-00: VRML importing (ASE was already present).
 * 23-12-00: DGeob class detached
 * 25-12-00: DOF1 replaces DOF0 format; more flexible and full PC/Big-endian.
 * NOTES:
 * BUGS:
 * - Triangulate doesn't work
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/matrix.h>
DEBUG_ENABLE

#ifdef OBS
#define N_PER_V     3        // Coordinates per vertex
#define N_PER_F     9        // Coordinates per face (triangles)

#define N_PER_TV    2        // Coordinates used per texture vertex (2D maps)
#define N_PER_TF    6        // Tex Coordinates per tex face
#endif

#undef  DBG_CLASS
#define DBG_CLASS "DGeode"

DGeode::DGeode(cstring fname)
  : DObject()
{
  int i;

  materials=0;
  geobs=0;
  flags=0;
  for(i=0;i<MAX_GEOB;i++)
    geob[i]=0;
  for(i=0;i<MAX_MATERIAL;i++)
    material[i]=0;
}
DGeode::DGeode(DGeode *master,int cloneFlags) : DObject()
// Create a geode based on another geode
// flags&1; don't reference materials; create a copy
{
  int i;

  // Clone the geometric objects
  geobs=master->GetNoofGeobs();
  for(i=0;i<master->GetNoofGeobs();i++)
  {
    // Clone geob
    geob[i]=new DGeob(master->geob[i],cloneFlags);
    // Own this geob
    geob[i]->geode=this;
  }
  
  // Take over materials by COPY
  materials=master->materials;
  for(i=0;i<materials;i++)
    material[i]=new DMaterial(master->material[i]);
}

DGeode::~DGeode()
{
  DBG_C("dtor")
  Destroy();
}
void DGeode::Destroy()
{
  DBG_C("Destroy")
  
  int i;
  for(i=0;i<geobs;i++)
    QDELETE(geob[i]);
  for(i=0;i<materials;i++)
    QDELETE(material[i]);
}
void DGeode::DestroyLists()
// Destroy (rebuild on next occasion) the display lists from all geobs
{
  DBG_C("DestroyLists")
  
  int i;
  for(i=0;i<geobs;i++)
  {
    if(geob[i])
      geob[i]->DestroyList();
  }
}
void DGeode::DestroyNormals()
// Destroy all normal information
{
  DBG_C("DestroyNormals")
  int i;
  for(i=0;i<geobs;i++)
    if(geob[i])
      geob[i]->DestroyNormals();
}

/*************
* Attributes *
*************/
DMaterial *DGeode::GetMaterial(int n)
// Retrieve material
{
//qdbg("DBM:GetMat this=%p\n",this);
  return material[n];
}
DMaterial *DGeode::FindMaterial(cstring name)
// Find material by name
// Returns 0 in case it's not found
{
  DBG_C("FindMaterial")
  DBG_ARG_S(name)
  
  int i;
  for(i=0;i<materials;i++)
  {
    if(!strcmp(material[i]->GetName(),name))
    {
      return material[i];
    }
  }
  QTRACE_PRINT("Material not found in %d materials",materials);
  return 0;
}
void DGeode::SetMapPath(cstring path)
// Set image loading path
// Pass 0 for 'path' to reset the default loading path
{
  if(path)pathMap=path;
  else    pathMap="";
}
void DGeode::FindMapFile(cstring mapName,string d)
// Finds image and stores result in 'd' (at least 256 chars big)
{
  if(pathMap!="")
  {
    // Try map path first
    sprintf(d,"%s/%s",(cstring)pathMap,mapName);
    if(QFileExists(d))return;
  }
  // Try images/ directory (relative to current directory)
  sprintf(d,"images/%s",mapName);
}
void DGeode::SetViewBursts(bool yn)
// Enables/disable burst viewing; this is done by colorizing
// the geobs/bursts instead of texturing for example.
{
  if(yn)flags|=VIEW_BURSTS;
  else  flags&=~VIEW_BURSTS;
  DestroyLists();
}
void DGeode::GetBoundingBox(DBox *b)
// Returns bounding box of all geobs in 'b'
{
  int i;
  DBox boxGeob;
  
  b->min.x=b->min.y=b->min.z=99999.0f;
  b->max.x=b->max.y=b->max.z=-99999.0f;
  for(i=0;i<geobs;i++)
  {
    geob[i]->GetBoundingBox(&boxGeob);
    b->Union(&boxGeob);
  }
}

/********
* PAINT *
********/
void DGeode::_Paint()
{
  int i;
  DGeob *gb;
  
#ifdef OBS
qdbg("==> DGeode:_Paint(); this=%p\n",this);
for(i=0;i<materials;i++)
qdbg("      mat[%d]=%p\n",i,material[i]);
#endif

  // Transparency support
  for(i=0;i<geobs;i++)
  {
    geob[i]->Paint();
  }
  for(i=0;i<geobs;i++)
  {
    gb=geob[i];
    gb->flags|=DGeob::PASS_TRANSPARENT;
    gb->Paint();
    gb->flags&=~DGeob::PASS_TRANSPARENT;
  }
}

/*****************
* MODEL JUGGLING *
*****************/
void DGeode::ScaleVertices(float x,float y,float z)
// Scale all vertices by an amount
{
  int i;
  // Scale all geobs
  for(i=0;i<geobs;i++)
    geob[i]->ScaleVertices(x,y,z);
}
void DGeode::Translate(float x,float y,float z)
// Translate entire geode
{
  int i;
  for(i=0;i<geobs;i++)
    geob[i]->Translate(x,y,z);
}

/******
* DOF *
******/
bool DGeode::ExportDOF(QFile *f)
// Export the geode into a file
{
  int i,n;
//qdbg("DGeode:ExportDOF(f=%p)\n",f);
  int pos,posNext;
  
  // Header
  f->Write((void*)"DOF1",4);
  pos=f->Tell();
  n=0; f->Write(&n,sizeof(n));
  // Write materials
  {
    int pos,posNext;
    f->Write((void*)"MATS",4);
    pos=f->Tell(); n=0; f->Write(&n,sizeof(n));
    n=QHost2PC_L(materials); f->Write(&n,sizeof(n));
    for(i=0;i<materials;i++)
    {
//qdbg("DGeode:ExportDOF; mat %d (%p)\n",i,material[i]);
      if(!material[i]->ExportDOF(f))
        return FALSE;
    }
    // Write size of MATS chunk
    posNext=f->Tell();
    f->Seek(pos);
    n=QHost2PC_L(posNext-pos-4); f->Write(&n,sizeof(n));
    f->Seek(posNext);
  }
  
  // Write geobs
  {
    int pos,posNext;
    f->Write((void*)"GEOB",4);
    pos=f->Tell(); n=0; f->Write(&n,sizeof(n));
    n=QHost2PC_L(geobs); f->Write(&n,sizeof(n));
    for(i=0;i<geobs;i++)
    {
      if(!geob[i]->ExportDOF(f))
        return FALSE;
    }
    // Write size of MATS chunk
    posNext=f->Tell();
    f->Seek(pos);
    n=QHost2PC_L(posNext-pos-4); f->Write(&n,sizeof(n));
    f->Seek(posNext);
  }
  
  // End this DOF
  f->Write((void*)"EDOF",4);
  
  // Write size of DOF1 chunk
  posNext=f->Tell();
  f->Seek(pos);
  n=QHost2PC_L(posNext-pos-4); f->Write(&n,sizeof(n));
  f->Seek(posNext);
  
  return TRUE;
}
bool DGeode::ImportDOF(QFile *f)
// Read the geode from a file
{
  char buf[8];
  int  i,n,len;
  
  // Destroy previous contents
  Destroy();
  
  // Header
  f->Read(buf,4); buf[4]=0;        // DOF0
  // A DOF1 file?
  if(strcmp(buf,"DOF1"))return FALSE;
  // Read length
  f->Read(&n,sizeof(n)); n=QPC2Host_L(n);
  
  // Chunks
 next_chunk:
  f->Read(buf,4); buf[4]=0;
  if(!strcmp(buf,"EDOF"))
  { // That's all
    return TRUE;
  }
  f->Read(&len,sizeof(len)); len=QPC2Host_L(len);
  if(f->IsEOF())
  { // Unexpected EOF
    return TRUE;
  }
  
  if(!strcmp(buf,"MATS"))  //$DEV
  {
    // Read materials
    f->Read(&materials,sizeof(materials));
    materials=QPC2Host_L(materials);
//qdbg("  materials=%d\n",materials);
    for(i=0;i<materials;i++)
    {
      material[i]=new DMaterial();
      if(!material[i]->ImportDOF(f,this))
        return FALSE;
    }
  } else if(!strncmp(buf,"GEOB",4))
  {
    // Read Geobs
    f->Read(&geobs,sizeof(geobs));
    geobs=QPC2Host_L(geobs);

    for(i=0;i<geobs;i++)
    {
//qdbg("DGeode:ImportDOF; geob %d\n",i);
      geob[i]=new DGeob(this);
      if(!geob[i]->ImportDOF(f))
      { QDELETE(geob[i]);
        return FALSE;
      }
    }
  } else
  {
    // Skip unknown chunk
qdbg("Skipping unknown chunk '%s'\n",buf);
    f->Seek(len,QFile::S_CUR);
  }
  goto next_chunk;
}
