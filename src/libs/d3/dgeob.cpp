/*
 * DGeob - big loaded geomobjects (from 3D Studio Max / LightWave etc)
 * 23-12-00: Detached from dgeode.cpp. Support for indexed painting.
 * 25-12-00: All polygons must be triangles from now on; too much
 * code depends on it. GOB1 format replaces GOB0.
 * 03-03-01: Usage of display list is now optional; it doesn't do much
 * when you only have a vertex array, and sucks up memory! (MonzaF12K)
 * 21-10-01: Support for vertex colors.
 * NOTES:
 * - Indexing has benefits with regards to lighting; may be cached. Also
 * great for progressive meshes for example.
 * BUGS:
 * - Triangulate doesn't work
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/matrix.h>
#ifdef OBS
#ifdef linux
// Linux needs rand() prototype
#include <stdlib.h>
#endif
#endif
DEBUG_ENABLE

// Use compiled vertex arrays? (bit of a test)
//#define OPT_COMPILED_VERTEX_ARRAY
#ifndef OPT_COMPILED_VERTEX_ARRAY
#undef GL_EXT_compiled_vertex_array
#endif

// Indexed face sets instead of flat big arrays?
#define USE_INDEXED_FACES

// Use displays lists to speed up?
//#define USE_DISPLAY_LISTS

// Check for degenerate triangles at loading time?
// Note: none were found with the crashing F1 Sauber on Win32
//#define OPT_CHECK_DEGENERATE

#define N_PER_V     3        // Coordinates per vertex
#define N_PER_F     9        // Coordinates per face (triangles)

#define N_PER_TV    2        // Coordinates used per texture vertex (2D maps)
#define N_PER_TF    6        // Tex Coordinates per tex face

#undef  DBG_CLASS
#define DBG_CLASS "DGeob"

/**********
* HELPERS *
**********/
#ifdef OBS
static void DumpArray(cstring s,float *a,int count,int elts)
// 'elts' is number of elements per thing
{
  int i;
  qdbg("%s: ",s);
  for(i=0;i<count;i++)
  {
    qdbg("%.2f ",a[i]);
    if((i%elts)==elts-1)qdbg(",  ");
  }
  qdbg("\n");
}
static void DumpArray(cstring s,dindex *a,int count,int elts)
// 'elts' is number of elements per thing
{
  int i;
  qdbg("%s: ",s);
  for(i=0;i<count;i++)
  {
    qdbg("%d ",a[i]);
    if((i%elts)==elts-1)qdbg(",  ");
  }
  qdbg("\n");
}
#endif

/********
* Ctors *
********/
DGeob::DGeob(DGeode *_geode)
{
  geode=_geode;
  
  // Init vars
  vertices=faces=0;
  tvertices=tfaces=vcolors=0;
  vertex=0;
  face_v=0; face_nrm=0;
  tvertex=0;
  tface_v=0;
  vcolor=0;
  nrmVector=0;
  nrmVectors=0;
  // Indexed face sets
  index=0;
  indices=0;
  normal=0;
  normals=0;
  
  bursts=0;
  materialRef=0;
  list=0;
  face_v_elements=0;
  face_nrm_elements=0;
  tface_v_elements=0;
  flags=0;
  paintFlags=0;
}
DGeob::DGeob(DGeob *master,int _flags)
// Create a geode based on another geode
// Does a reference for the geometry data; the vertices are NOT copied
// _flags&1; don't reference materials; create a copy
{
  int i;

  vertices=master->vertices;
  faces=master->faces;
  tvertices=master->tvertices;
  tfaces=master->tfaces;
  vertex=master->vertex;
  face_v=master->face_v;
  face_nrm=master->face_nrm;
  tvertex=master->tvertex;
  tface_v=master->tface_v;
  vcolor=master->vcolor;
  vcolors=master->vcolors;
  nrmVectors=master->nrmVectors;
  nrmVector=master->nrmVector;
  
  index=master->index;
  indices=master->indices;
  normal=master->normal;
  normals=master->normals;
  
  face_v_elements=master->face_v_elements;
  face_nrm_elements=master->face_nrm_elements;
  tface_v_elements=master->tface_v_elements;
  flags=master->flags;
  paintFlags=master->paintFlags;
  list=master->list;
  geode=master->geode;
  
  // Clone bursts
  bursts=master->bursts;
  for(i=0;i<bursts;i++)
  { burstStart[i]=master->burstStart[i];
    burstCount[i]=master->burstCount[i];
    burstMtlID[i]=master->burstMtlID[i];
    burstVperP[i]=master->burstVperP[i];
  }
  materialRef=master->materialRef;
}

DGeob::~DGeob()
{
  Destroy();
}

/*************
* Destroying *
*************/
void DGeob::Destroy()
// Free all associated arrays
{
  QFREE(vertex);
  QFREE(face_v);
  QFREE(face_nrm);
  QFREE(tface_v);
  QFREE(tvertex);
  QFREE(vcolor);
  QFREE(nrmVector);
  QFREE(index);
  QFREE(normal);

  // Reset counts
  vertices=0;
  faces=0;
  tvertices=0;
  tfaces=0;
  vcolors=0;
  face_v_elements=0;
  face_nrm_elements=0;
  tface_v_elements=0;
  indices=0;
  normals=0;
  
  bursts=0;
  DestroyList();
}
void DGeob::DestroyList()
// Free the display list
{
#ifdef USE_DISPLAY_LISTS
  if(list)
  {
    glDeleteLists(list,1);
    list=0;
  }
#endif
}
void DGeob::DestroyNormals()
// If any normals were generated/loaded, they are thrown away
// Notice this can be faster for geobs that don't use lighting, as they
// don't need normals (CULL_FACE is done based on the vertices).
// For example, Racer's SCGT tracks don't do lighting and are drawn faster
// without supplying normals.
{
  if(normal)
  { QFREE(normal);
    normals=0;
    DestroyList();
  }
}

/**************
* Information *
**************/
DMaterial *DGeob::GetMaterialForID(int id)
// Returns material to use given id 'id'
// This is harder than it sounds, since multi-subobject materials
// use the submaterials for material id, and with normal materials
// (like SCGT cars use after VRL->VRML->MAX->ASE) without submaterials
// just use the material from the materialRef
// Returns 0 if no material exists (!)
// BUGS: nested multi/sub materials are not supported, just 1 deep
{
  //DMaterial *mat;
  int n;
  
//qdbg("DGeob:GetMat4ID; materials=%d, id=%d, matRef=%d\n",
 //geode->materials,id,materialRef);
  if(!geode->materials)
  { // Default material
    QTRACE_PRINT("No materials defined; returning 0.\n");
    return 0;
  } else if(geode->material[materialRef])
  {
    if(!geode->material[materialRef]->submaterials)
    { // Use topmost material
//qdbg("  no submats\n");
      n=materialRef;
      if(n<0)n=0; else if(n>=geode->materials)n=geode->materials-1;
//qdbg("  ret geode=%p, n=%d, mat=%p\n",geode,n,geode->material[n]);
      return geode->material[n];
    } else
    {
//qdbg(" DGeob:GetMat4ID; multisub: materials=%d, id=%d\n",geode->materials,id);
//qdbg("  p=%p\n",geode->material[materialRef]->submaterial[id]);

      // Use material in a multi/sub object material (see 3D Studio Max)
      // You CAN define less submaterials in Max then there are material ID's.
      // Max seems to spread the materials over the id's. Here I truncate
      // the material id's to keep it within range.
      n=id;
      if(n<0)n=0;
      else if(n>=geode->material[materialRef]->submaterials)
        n=geode->material[materialRef]->submaterials-1;
//qdbg("  using %d\n",n);
      return geode->material[materialRef]->submaterial[n];
    }
  }
  return 0;
}

int DGeob::GetBurstForFace(int n)
// Given face #n, return the burst index in which this face
// belongs.
// Returns -1 if no burst was found
{
  int i;
  for(i=0;i<bursts;i++)
  {
    if(n<burstStart[i]+burstCount[i])
      return i;
  }
  return -1;
}

void DGeob::GetBoundingBox(DBox *b)
// Stores the bounding box info in 'b'
{
  int i;
  dfloat *p;
  
  if(!vertices)
  { 
    b->min.SetToZero();
    b->max.SetToZero();
    return;
  }
  b->min.x=b->min.y=b->min.z=99999.0f;
  b->max.x=b->max.y=b->max.z=-99999.0f;
  p=vertex;
  for(i=0;i<vertices;i++)
  {
    if(*p<b->min.x)b->min.x=*p;
    if(*p>b->max.x)b->max.x=*p;
    p++;
    if(*p<b->min.y)b->min.y=*p;
    if(*p>b->max.y)b->max.y=*p;
    p++;
    if(*p<b->min.z)b->min.z=*p;
    if(*p>b->max.z)b->max.z=*p;
    p++;
  }
}

/*****************
* Checking model *
*****************/
#ifdef OPT_CHECK_DEGENERATE
static bool CheckDegenerate(dfloat *vertex,dindex *index,int indices)
// Test for degenerate triangles; vertices that are not nicely on a plane
// The F1_Sauber crashes Win2K hard when drawn small tris.
// Probably a driver thing though.
{
  int     i,tris=0;
  float   min=0.0001;
  dfloat *v1,*v2,*v3;
  bool r=TRUE;
  
  for(i=0;i<indices/3;i++)
  {
    v1=&vertex[index[i*3+0]*3];
    v2=&vertex[index[i*3+1]*3];
    v3=&vertex[index[i*3+2]*3];
    // v2==v1?
    if(fabs(v2[0]-v1[0])<min&&
       fabs(v2[1]-v1[1])<min&&
       fabs(v2[2]-v1[2])<min)
    {
     found:
      qdbg("Degenerate vertex? index=%d\n",i);
      qdbg("Tri=%d, indices=(%d,%d,%d)\n",
        i,index[i*3+0],index[i*3+1],index[i*3+2]);
      qdbg("v1=(%.4f,%.4f,%.4f)\n",v1[0],v1[1],v1[2]);
      qdbg("v2=(%.4f,%.4f,%.4f)\n",v2[0],v2[1],v2[2]);
      qdbg("v3=(%.4f,%.4f,%.4f)\n",v3[0],v3[1],v3[2]);
      r=FALSE;
      tris++;
      continue;
    }
    // v3==v1?
    if(fabs(v3[0]-v1[0])<min&&
       fabs(v3[1]-v1[1])<min&&
       fabs(v3[2]-v1[2])<min)goto found;
    // v3==v2?
    if(fabs(v3[0]-v2[0])<min&&
       fabs(v3[1]-v2[1])<min&&
       fabs(v3[2]-v2[2])<min)goto found;
  }
  if(!r)qdbg("CheckDegenerate() found %d degenerate tris\n",tris);
  //qdbg("CheckDegenerate() was completed succesfully\n");
  return r;
}
#endif

/******
* DOF *
******/
bool DGeob::ExportDOF(QFile *f)
// Export the geob into a file
// It doesn't include the original vertices, just the faces.
// All writing is done in PC (Big-endian) format.
// 25-12-00; GOB1, GOB0 used non-indexed primitives/faces (obsolete!)
{
  int len,off,off2;
  int n;

  f->Write((void*)"GOB1",4);
  off=f->Tell();
  len=0; len=QHost2PC_L(len); f->Write(&len,sizeof(len));
  
  // Header
  f->Write((void*)"GHDR",4);
  len=QHost2PC_L(3*sizeof(int)); f->Write(&len,sizeof(len));
  n=QHost2PC_L(flags); f->Write(&n,sizeof(n));
  n=QHost2PC_L(paintFlags); f->Write(&n,sizeof(n));
  n=QHost2PC_L(materialRef); f->Write(&n,sizeof(n));
  
  // Indices
  f->Write((void*)"INDI",4);
  len=QHost2PC_L(indices*sizeof(dindex)+sizeof(int));
  f->Write(&len,sizeof(len));
  n=QHost2PC_L(indices); f->Write(&n,sizeof(n));
  QHost2PC_SA(index,indices);
  f->Write(index,indices*sizeof(dindex));
  QPC2Host_SA(index,indices);
    
  // Vertices
  f->Write((void*)"VERT",4);
  len=QHost2PC_L(vertices*sizeof(dfloat)*3+sizeof(int));
  f->Write(&len,sizeof(len));
  n=QHost2PC_L(vertices); f->Write(&n,sizeof(n));
  QHost2PC_FA(vertex,vertices*3);
  f->Write(vertex,vertices*sizeof(dfloat)*3);
  QPC2Host_FA(vertex,vertices*3);

  // Texture vertices (optional)
  if(tvertex)
  {
    f->Write((void*)"TVER",4);
    len=QHost2PC_L(tvertices*sizeof(dfloat)*2+sizeof(int));
    f->Write(&len,sizeof(len));
    n=QHost2PC_L(tvertices); f->Write(&n,sizeof(n));
    QHost2PC_FA(tvertex,tvertices*2);
    f->Write(tvertex,tvertices*sizeof(dfloat)*2);
    QPC2Host_FA(tvertex,tvertices*2);
  }
  
  // Normals
  if(normal)
  {
    f->Write((void*)"NORM",4);
    len=QHost2PC_L(normals*sizeof(dfloat)*3+sizeof(int));
    f->Write(&len,sizeof(len));
    n=QHost2PC_L(normals); f->Write(&n,sizeof(n));
    QHost2PC_FA(normal,normals*3);
    f->Write(normal,normals*sizeof(dfloat)*3);
    QPC2Host_FA(normal,normals*3);
  }
  
  // Vertex colors
  if(vcolor)
  {
    f->Write("VCOL",4);
    len=QHost2PC_L(vcolors*sizeof(dfloat)*3+sizeof(int));
    f->Write(&len,sizeof(len));
    n=QHost2PC_L(vcolors); f->Write(&n,sizeof(n));
    QHost2PC_FA(vcolor,vcolors*3);
    f->Write(vcolor,vcolors*sizeof(dfloat)*3);
    QPC2Host_FA(vcolor,vcolors*3);
  }

  // Bursts
  f->Write((void*)"BRST",4);
  len=QHost2PC_L(bursts*4*sizeof(int)+sizeof(int));
  f->Write(&len,sizeof(len));
  n=QHost2PC_L(bursts); f->Write(&n,sizeof(n));
  QHost2PC_IA(burstStart,bursts);
  QHost2PC_IA(burstCount,bursts);
  QHost2PC_IA(burstMtlID,bursts);
  QHost2PC_IA(burstVperP,bursts);
  f->Write(burstStart,bursts*sizeof(int));
  f->Write(burstCount,bursts*sizeof(int));
  f->Write(burstMtlID,bursts*sizeof(int));
  f->Write(burstVperP,bursts*sizeof(int));
  QPC2Host_IA(burstStart,bursts);
  QPC2Host_IA(burstCount,bursts);
  QPC2Host_IA(burstMtlID,bursts);
  QPC2Host_IA(burstVperP,bursts);

  // End chunk
  f->Write((void*)"GEND",4);
  
  // Record size of entire geob
  // 19-09-01: Fixed bug; GOB1 length was off by 4 (length was 4
  // bytes too big). Thanks to OgerM (from ZModeler) for finding it.
  off2=f->Tell();
  f->Seek(off);
  len=QHost2PC_L(off2-off)-sizeof(int); f->Write(&len,sizeof(len));
  
  // Return to previous write position for future writes
  f->Seek(off2);
  
  return TRUE;
}
bool DGeob::ImportDOF(QFile *f)
// Read a geob from a file
// 25-12-00; GOB1, much more flexible for future enhancements
{
  char buf[8];
  int  len,n;
  
  // Destroy any old geob
  Destroy();
  
  // Read GEOB identifier ("GOB1")
  f->Read(buf,4);
  f->Read(&n,sizeof(n)); len=QPC2Host_L(n);
  // Ignore old GOB0 geobs (obsolete)
  if(!strncmp(buf,"GOB0",4))
  {
    f->Seek(len,QFile::S_CUR);
    return FALSE;
  }
  
  // Read chunks
  while(1)
  {
    f->Read(buf,4); buf[4]=0;
    if(f->IsEOF())break;
    if(!strcmp(buf,"GEND"))break;
    // All chunks have a length
    f->Read(&n,sizeof(n)); len=QPC2Host_L(n);
//qdbg("Chunk '%s' len %d, filepos %d\n",buf,len,f->Tell());
    if(!strcmp(buf,"GHDR"))
    {
      f->Read(&n,sizeof(n)); flags=QPC2Host_L(n);
      f->Read(&n,sizeof(n)); paintFlags=QPC2Host_L(n);
      f->Read(&n,sizeof(n)); materialRef=QPC2Host_L(n);
    } else if(!strcmp(buf,"INDI"))
    {
      f->Read(&n,sizeof(n)); indices=QPC2Host_L(n);
      if(!indices)
      {
        // Allocate something to continue
        // Some skyboxes for example have 0-vertex geobs
        index=(dindex*)qcalloc(sizeof(dindex));
      } else
      {
        index=(dindex*)qcalloc(indices*sizeof(dindex));
      }
      if(!index)
      {nomem:
        qwarn("DGeob:ImportDOF(); out of memory");
        return FALSE;
      }
      f->Read(index,indices*sizeof(dindex));
      QPC2Host_SA(index,indices);
    } else if(!strcmp(buf,"VERT"))
    {
      f->Read(&n,sizeof(n)); vertices=QPC2Host_L(n);
      if(!vertices)
      {
        // Allocate something to continue (we NEED vertices)
        // Some skyboxes for example have 0-vertex geobs
        vertex=(float*)qcalloc(sizeof(dfloat));
      } else
      {
        vertex=(float*)qcalloc(vertices*sizeof(dfloat)*3);
      }
      if(!vertex)goto nomem;
      f->Read(vertex,vertices*sizeof(dfloat)*3);
      QPC2Host_FA(vertex,vertices*3);
    } else if(!strcmp(buf,"NORM"))
    {
      f->Read(&n,sizeof(n)); normals=QPC2Host_L(n);
      if(!normals)
      {
        // No normals
        normal=0;
      } else
      {
        normal=(float*)qcalloc(normals*sizeof(dfloat)*3);
//qdbg("DGeob:ImportDOF(); normals=%d, normal=%p\n",normals,normal);
        if(!normal)goto nomem;
        f->Read(normal,normals*sizeof(dfloat)*3);
        QPC2Host_FA(normal,normals*3);
      }
    } else if(!strcmp(buf,"VCOL"))
    {
      f->Read(&n,sizeof(n)); vcolors=QPC2Host_L(n);
      if(!vcolors)
      {
        // No vertex colors
        vcolor=0;
      } else
      {
        vcolor=(dfloat*)qcalloc(vcolors*sizeof(dfloat)*3);
//qdbg("DGeob:ImportDOF(); vcolors=%d, vcolor=%p\n",vcolors,vcolor);
        if(!vcolor)goto nomem;
        f->Read(vcolor,vcolors*sizeof(dfloat)*3);
        QPC2Host_FA(vcolor,vcolors*3);
      }
    } else if(!strcmp(buf,"TVER"))
    {
      f->Read(&n,sizeof(n)); tvertices=QPC2Host_L(n);
      if(!tvertices)
      {
        // No texture vertices
        tvertex=0;
      } else
      {
        tvertex=(float*)qcalloc(tvertices*sizeof(dfloat)*2);
        if(!tvertex)goto nomem;
        f->Read(tvertex,tvertices*sizeof(dfloat)*2);
        QPC2Host_FA(tvertex,tvertices*2);
      }
    } else if(!strcmp(buf,"BRST"))
    {
      f->Read(&n,sizeof(n)); bursts=QPC2Host_L(n);
      f->Read(burstStart,bursts*sizeof(int));
      f->Read(burstCount,bursts*sizeof(int));
      f->Read(burstMtlID,bursts*sizeof(int));
      f->Read(burstVperP,bursts*sizeof(int));
      QPC2Host_IA(burstStart,bursts);
      QPC2Host_IA(burstCount,bursts);
      QPC2Host_IA(burstMtlID,bursts);
      QPC2Host_IA(burstVperP,bursts);
    } else
    {
      // Skip unknown chunk
qdbg("Skipping unknown geob chunk '%s'\n",buf);
      f->Seek(len,QFile::S_CUR);
    }
  }

#ifdef OBS
DumpArray("geob",vertex,vertices*3,3);
DumpArray("geob",index,indices,3);
#endif
#ifdef OPT_CHECK_DEGENERATE
  CheckDegenerate(vertex,index,indices);
#endif
  return TRUE;
}


/**********
* SCALING *
**********/
void DGeob::ScaleVertices(float x,float y,float z)
// Scale all vertices by an amount
// BUGS:
// - Assumes model is made of triangles!
{
  int i;
  
  // On more than 1 occasion, accidental scaling by 0
  // had the model disappear, resulting in debugging frenzy,
  // so warn for that.
  if(x==0.0f&&y==0.0f&&z==0.0f)
    qwarn("DGeob:ScaleVertices() scaling by 0; model will be invisible");

  // Vertices themselves
  for(i=0;i<vertices;i++)
  { vertex[i*3+0]*=x;
    vertex[i*3+1]*=y;
    vertex[i*3+2]*=z;
  }
  // Faces that where created out of vertices
  if(face_v)
    for(i=0;i<faces;i++)
    { face_v[i*N_PER_F+0]*=x;
      face_v[i*N_PER_F+1]*=y;
      face_v[i*N_PER_F+2]*=z;
      face_v[i*N_PER_F+3]*=x;
      face_v[i*N_PER_F+4]*=y;
      face_v[i*N_PER_F+5]*=z;
      face_v[i*N_PER_F+6]*=x;
      face_v[i*N_PER_F+7]*=y;
      face_v[i*N_PER_F+8]*=z;
    }

  // Rebuild display list upon the next Paint()
  DestroyList();
}

/******************
* Transformations *
******************/
void DGeob::Translate(float x,float y,float z)
// Translate all vertices
// Only works for indexed primitives
{
  int i;
  if(!vertex)return;
  
  for(i=0;i<vertices;i++)
  { vertex[i*3+0]+=x;
    vertex[i*3+1]+=y;
    vertex[i*3+2]+=z;
  }
  
  // Rebuild display list upon the next Paint()
  DestroyList();
}
void DGeob::FlipFaces(int burst)
// Flip all faces. Useful when the ordering of a burst is wrong (such
// as in found in some 3DO models)
// If 'burst'==-1, then all bursts are flipped (NYI!)
// Only works for indexed primitives.
{
  int i,n,i0;
  int t;

  if(burst==-1)
  { qwarn("DGeob:FlipFaces(%d) NYI",burst);
    return;
  }

  i0=burstStart[burst]/3;
  n=burstCount[burst]/9;
qdbg("DGeob:FlipFaces(); i0=%d, n=%d\n",i0,n);
  for(i=i0;i<i0+n;i++)
  {
    // To reverse, for example 012, make it 021. That's just a swap.
qdbg("%d, %d, %d ->",index[i*3+0],index[i*3+1],index[i*3+2]);
    t=index[i*3+1];
    index[i*3+1]=index[i*3+2];
    index[i*3+2]=t;
qdbg(" %d, %d, %d\n",index[i*3+0],index[i*3+1],index[i*3+2]);
  }

  // Rebuild display list upon the next Paint()
  DestroyList();
}

/********
* Paint *
********/
void DGeob::Paint()
// If this function changes, change PaintOptimized() as well!
{
  int i,first,
      pflags;             // 1=first
  // Layering
  //DMatLayer *layer;
  int        curLayer,lastLayer;
  GLboolean writeMask;
  // Client state caching
  bool csNormal=FALSE;

#ifdef USE_DISPLAY_LISTS
  // If the object has been put in a display list, use that
  if(list)
  {
//qdbg("DGeob::Paint(); glCallList(%d)\n",list);
    glCallList(list);
//qdbg("  done\n",list);
    return;
  }
  
  // Generate a display list for the object
  list=glGenLists(1);
  glNewList(list,GL_COMPILE_AND_EXECUTE);
//qdbg("DGeob::Paint(); glNewList(%d)\n",list);
#endif

#ifdef OBS
qdbg("DGeob::Paint (this=%p), master geode=%p\n",this,geode);
qdbg("bursts=%d\n",bursts);
#endif
 
  pflags=1;

  // Layers init
#ifdef ND_AVOID_AND_LEAVE_MASK_AT_TRUE_AT_RETURN_TIME
  glGetBooleanv(GL_DEPTH_WRITEMASK,&writeMask);
#endif
  writeMask=GL_TRUE;
  
  // Draw all bursts
  for(i=0;i<bursts;i++)
  {
//qdbg("paint burst %d, mref=%d, count=%d\n",i,materialRef,burstCount[i]);
#ifdef OBS
qdbg("geode=%p, matRef=%d\n",geode,materialRef);
qdbg("geode->mat[ref]=%p\n",geode->material[materialRef]);
#endif
    // Install material in OpenGL engine
    DMaterial *mat=GetMaterialForID(burstMtlID[i]);
    if(!mat)
    {
      qwarn("DGeob::Paint(); no material in burst %d",i);
      continue;
    }
    
    // Check for transparency support
    if((flags&PASS_TRANSPARENT)!=0&&mat->IsTransparent()==FALSE)
    {
      continue;
    }
    
    if(pflags&1)
    {
      // First burst to draw
      // Enable arrays
      glEnableClientState(GL_VERTEX_ARRAY);
      // Turn off 'first' flag
      pflags^=1;
    }
    
    curLayer=0;
    lastLayer=mat->GetLayers()-1;
//if(mat->GetLayers()>1){curLayer=1;lastLayer=0;}

   paint_layer:
//qdbg("DGeob:Paint(); curLayer=%d, lastLayer=%d\n",curLayer,lastLayer);
    if(!mat->PrepareToPaint(curLayer))
    {
      // No need to paint this layer
      goto next_layer;
    }
    if(mat->IsTransparent())
    {
      // Use Z-buffer, but don't write
      //glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
    } else
    {
      // Determine layer usage; logic should really be
      // in DMaterial class.
      if(lastLayer==0)
      {
        // 1 layer
        glDepthMask(GL_TRUE);
      } else
      {
        // Multiple layers
        if(curLayer>0)
        {
          // Make sure it gets on top
          glPolygonOffset(-curLayer,0);
          glEnable(GL_POLYGON_OFFSET_FILL);
        } else
        {
          // Lowest layer is opaque
          glDisable(GL_BLEND);
        }
        if(curLayer==lastLayer)
        {
          // Last layer, no Z-buffering writes (done by previous layers)
          glDepthMask(GL_FALSE);
        } else
        {
          // Generate Z data in 1st layer(s)
          glDepthMask(GL_TRUE);
        }
      }
    }

    // Check for debug viewing options
    if(geode->IsViewBurstsEnabled())
    {
      // Test mode to recognize bursts
      float diffuse[4];
      int   r,val;
      glDisable(GL_TEXTURE_2D);
      // Generate nice color
      val=((((int)geode)/123)&7)+i;
      r=(((int)mat)/721)&127;
//qdbg("r=%x, val=%x, geode=%p, mat=%p\n",r,val,geode,mat);
      if(r&1)diffuse[0]=1.0f; else diffuse[0]=val/256.0f;
      if(r&2)diffuse[1]=1.0f; else diffuse[1]=val/256.0f;
      if(r&4)diffuse[2]=1.0f; else diffuse[2]=val/256.0f;
      diffuse[3]=0.0f;
//qdbg("diffuse mat %f,%f,%f\n",diffuse[0],diffuse[1],diffuse[2]);
      glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
    }
    
#ifdef OBS_DO_BEFOREHAND
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
#endif

    first=burstStart[i];

    // Install vertex array
    glVertexPointer(3,GL_FLOAT,0,vertex);
  
    // Install normal array, if present
    if(normal)
    { 
      if(!csNormal)
      { glEnableClientState(GL_NORMAL_ARRAY);
        csNormal=TRUE;
      }
      glNormalPointer(GL_FLOAT,0,normal);
    } else
    { // Turn of normal array
      if(csNormal)
      { glDisableClientState(GL_NORMAL_ARRAY);
        csNormal=FALSE;
      }
    }

    // Install texture coordinate array if applicable
    if(tvertex)
    { 
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2,GL_FLOAT,0,tvertex);
    }
    
    // Install vertex color array if applicable
    if(vcolor!=0&&mat->IsSelected()==FALSE)
    { 
//qdbg("vcolor paint\n");
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(3,GL_FLOAT,0,vcolor);
//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
      // Lighting will overrule vertex colors (but why)
      glDisable(GL_LIGHTING);
//glEnable(GL_TEXTURE_2D);
    }
    
#ifdef GL_EXT_compiled_vertex_array
    glLockArraysEXT(0,burstCount[i]/3);
#endif 

    // Decide how to draw
    if(burstVperP[i]==3)
    { // Most common; triangles
//qdbg("geob %p; count %d, start %d\n",this,burstCount[i]/3,burstStart[i]/3);

      glDrawElements(GL_TRIANGLES,burstCount[i]/3,GL_UNSIGNED_SHORT,
        &index[burstStart[i]/3]);
    } else if(burstVperP[i]==4)
    {
      glDrawElements(GL_QUADS,burstCount[i]/4,GL_UNSIGNED_SHORT,
        &index[burstStart[i]/4]);
    } else
    {
#ifdef USE_INDEXED_FACES
      qwarn("DGeob; draw poly NYI\n");
#else
      // Assume N-polygon; hopefully only 1 polygon is defined
      glDrawArrays(GL_POLYGON,0,burstCount[i]);
#endif
    }

#ifdef GL_EXT_compiled_vertex_array
    glUnlockArraysEXT();
#endif 
    // Next material layer?
    mat->UnPrepare(curLayer);
   next_layer:
    if(++curLayer<mat->GetLayers())
      goto paint_layer;
    
    // Restore depthbuffer write mask
    glDepthMask(writeMask);
    if(lastLayer>0)
    {
      // Restore state
      glPolygonOffset(0,0);
      glDisable(GL_POLYGON_OFFSET_FILL);
    }
  }

  if(!(pflags&1))
  {
    // Something was drawn
    // Turn off states
    glDisableClientState(GL_VERTEX_ARRAY);
    if(csNormal)glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }

#ifdef USE_DISPLAY_LIST
  glEndList();
#endif
  
}
void DGeob::PaintOptimized(int burst,int flags)
// Fast painter for batch rendering (Render.cpp)
// Doesn't install the material, assumes it is already setup
// Also minimizes state changes.
// If Paint() changes, change this function as well!
{
  int i,first;
  // Client state caching
  bool csNormal=FALSE;

#ifdef USE_DISPLAY_LISTS
  // If the object has been put in a display list, use that
  if(list)
  {
    glCallList(list);
    return;
  }
  
  // Generate a display list for the object
  list=glGenLists(1);
  glNewList(list,GL_COMPILE_AND_EXECUTE);
#endif

#ifdef OBS
  // Enable arrays
  glEnableClientState(GL_VERTEX_ARRAY);
#endif

//qdbg("DGeob:PaintOptimized()\n");

  // Draw all bursts
  i=burst;
  {
    first=burstStart[i];

    // Install vertex array
    glVertexPointer(3,GL_FLOAT,0,vertex);
  
    // Install normal array, if present
    if(normal)
    { 
//qdbg("DGeob:Paint(); Normals\n");
      if(!csNormal)
      { glEnableClientState(GL_NORMAL_ARRAY);
        csNormal=TRUE;
      }
      glNormalPointer(GL_FLOAT,0,normal);
    } else
    { // Turn of normal array
      if(csNormal)
      { glDisableClientState(GL_NORMAL_ARRAY);
        csNormal=FALSE;
      }
    }

    // Install texture coordinate array if applicable
    if(tvertex)
    { 
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2,GL_FLOAT,0,tvertex);
    }

    if(vcolor)
    {
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(3,GL_FLOAT,0,vcolor);
      // Lighting will overrule vertex colors (but why)
      glDisable(GL_LIGHTING);
      //glDisable(GL_CULL_FACE);
    }

    // Decide how to draw
    if(burstVperP[i]==3)
    { // Most common; triangles
      glDrawElements(GL_TRIANGLES,burstCount[i]/3,GL_UNSIGNED_SHORT,
        &index[burstStart[i]/3]);
    } else if(burstVperP[i]==4)
    {
      glDrawElements(GL_QUADS,burstCount[i]/4,GL_UNSIGNED_SHORT,
        &index[burstStart[i]/4]);
    } else
    {
#ifdef USE_INDEXED_FACES
      qwarn("DGeob; draw poly NYI\n");
#else
      // Assume N-polygon; hopefully only 1 polygon is defined
      glDrawArrays(GL_POLYGON,0,burstCount[i]);
#endif
    }
  }

  // Turn off
#ifdef OBS_OPTIMIZED
  glDisableClientState(GL_VERTEX_ARRAY);
#endif
  if(csNormal)glDisableClientState(GL_NORMAL_ARRAY);
  if(tvertex)glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  if(vcolor)
  {
    glDisableClientState(GL_COLOR_ARRAY);
    //glColor3f(1,1,1);
  }

#ifdef USE_DISPLAY_LIST
  glEndList();
#endif
}
