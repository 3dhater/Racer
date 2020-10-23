/*
 * DGeode - big loaded meshes (from 3D Studio Max / LightWave etc)
 * 10-01-00: Created!
 * 15-03-00: Bursts of polygons with the same material. Materials used.
 * 10-11-00: VRML importing (ASE was already present).
 * NOTES:
 * BUGS:
 * - Triangulate doesn't work
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#pragma hdrstop
#include <qlib/app.h>
#include <qlib/matrix.h>
#include <d3/geode.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define N_PER_V     3        // Coordinates per vertex
#define N_PER_F     9        // Coordinates per face (triangles)

#define N_PER_TV    2        // Coordinates used per texture vertex (2D maps)
#define N_PER_TF    6        // Tex Coordinates per tex face

// SCGT hack?
//#define SCGT_FIX_DIFFUSE_COLOR

/********
* DGeob *
********/
#undef  DBG_CLASS
#define DBG_CLASS "DGeob"

DGeob::DGeob(DGeode *_geode)
{
  geode=_geode;
  
  // Init vars
  vertices=faces=0;
  tvertices=tfaces=0;
  vertex=0;
  face_v=0; face_nrm=0;
  tvertex=0;
  tface_v=0;
  nrmVector=0;
  nrmVectors=0;
  bursts=0;
  materialRef=0;
  list=0;
  face_v_elements=0;
  face_nrm_elements=0;
  tface_v_elements=0;
  paintFlags=0;
}
DGeob::DGeob(DGeob *master,int flags)
// Create a geode based on another geode
// Does a reference for the geometry data; the vertices are NOT copied
// flags&1; don't reference materials; create a copy
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
  nrmVectors=master->nrmVectors;
  nrmVector=master->nrmVector;
  face_v_elements=master->face_v_elements;
  face_nrm_elements=master->face_nrm_elements;
  tface_v_elements=master->tface_v_elements;
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

void DGeob::Destroy()
// Free all associated arrays
{
  if(vertex)   { qfree(vertex); vertex=0; }
  if(face_v)   { qfree(face_v); face_v=0; }
  if(face_nrm) { qfree(face_nrm); face_nrm=0; }
  if(tface_v)  { qfree(tface_v); tface_v=0; }
  if(tvertex)  { qfree(tvertex); tvertex=0; }
  if(nrmVector){ qfree(nrmVector); nrmVector=0; }
  // Reset counts
  vertices=0;
  faces=0;
  tvertices=0;
  tfaces=0;
  face_v_elements=0;
  face_nrm_elements=0;
  tface_v_elements=0;
  
  bursts=0;
  DestroyList();
}
void DGeob::DestroyList()
// Free the display list
{
  if(list)
  {
    glDeleteLists(list,1);
    list=0;
  }
}

/**********
* HELPERS *
**********/
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

/*************
* ASE Import *
*************/
DMaterial *DGeob::GetMaterialForID(int id)
// Returns material to use given id 'id'
// This is harder than it sounds, since multi-subobject materials
// use the submaterials for material id, and with normal materials
// (like SCGT cars use after VRL->VRML->MAX->ASE) without submaterials
// just use the material from the materialRef
// Returns 0 if no material exists (!)
// BUGS: nested multi/sub materials are not supported, just 1 deep
{
  DMaterial *mat;
  int n;
  
//qdbg("DGeob:GetMat4ID; materials=%d, id=%d\n",geode->materials,id);
  if(!geode->materials)
  { // Default material
    QTRACE("No materials defined; returning 0.\n");
    return 0;
  } else if(geode->material[materialRef])
  {
    if(!geode->material[materialRef]->submaterials)
    { // Use topmost material
      n=materialRef;
      if(n<0)n=0; else if(n>=geode->materials)n=geode->materials-1;
      return geode->material[n];
    } else
    {
//qdbg("DGeob:GetMat4ID; multisub: materials=%d, id=%d\n",geode->materials,id);
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
bool DGeob::ImportASE(QFile *f)
// Read a GEOMOBJECT from the ASE file
// Assumes the pointer is at the opening { of the object
{
  int nest;
  qstring cmd,ns;
  int i,n;
  float x,y,z;
  float r,g,b;
  int v1,v2,v3;
  DMaterial *curMat;           // Current material being edited
  int curMatIndex,curSubMatIndex;  // Tree-ing materials 1 deep
  int vnrmIndex;               // Normal vertex index
  int curMtlID;
  int curFace;
  QMatrix4 matNode;
  QVector3 vec,vecT;
  
qdbg("DGeob: import\n");
  nest=0;
  curMtlID=-1;
  curFace=0;
  vnrmIndex=0;
  matNode.Identity();
  
  while(1)
  {
    f->GetString(cmd);
    // We don't expect EOF
    if(f->IsEOF())return FALSE;
    if(cmd=="{")
    { nest++;
    } else if(cmd=="}")
    { nest--;
      // Detect last closing brace for GEOMOBJECT
      if(nest==0)
        break;
      
    // Node details
    //
    } else if(cmd=="*NODE_NAME")
    {
      f->GetString(ns);
qdbg("Node name '%s'\n",(cstring)ns);
    } else if(cmd=="*TM_ROW0")
    {
      f->GetString(ns); vec.x=atof(ns);
      f->GetString(ns); vec.y=atof(ns);
      f->GetString(ns); vec.z=atof(ns);
      matNode.SetX(&vec);
    } else if(cmd=="*TM_ROW1")
    {
      f->GetString(ns); vec.x=atof(ns);
      f->GetString(ns); vec.y=atof(ns);
      f->GetString(ns); vec.z=atof(ns);
      matNode.SetY(&vec);
    } else if(cmd=="*TM_ROW2")
    {
      f->GetString(ns); vec.x=atof(ns);
      f->GetString(ns); vec.y=atof(ns);
      f->GetString(ns); vec.z=atof(ns);
      matNode.SetZ(&vec);

    // Vertices, faces, texture vertices and texture faces
    //
    } else if(cmd=="*MESH_NUMVERTEX")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d vertices\n",n);
      vertices=n;
      vertex=(float*)qcalloc(sizeof(float)*vertices*N_PER_V);
      if(!vertex)qerr("?vertex");
    } else if(cmd=="*MESH_VERTEX")
    { f->GetString(ns); n=Eval(ns);
      f->GetString(ns); x=atof(ns);
      f->GetString(ns); y=atof(ns);
      f->GetString(ns); z=atof(ns);
      // 3DS Max to OpenGL axis system
      vertex[n*N_PER_V+0]=x;
      vertex[n*N_PER_V+1]=z; //z;
      vertex[n*N_PER_V+2]=-y; //-y;
//qdbg("vertex %d: %f %f %f\n",n,x,y,z);
    } else if(cmd=="*MESH_NUMFACES")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d faces\n",n);
      // Only triangles are expected (for ASE files)
      faces=n;
      face_v_elements=faces*N_PER_F;
      face_v=(float*)qcalloc(sizeof(float)*faces*N_PER_F);
      if(!face_v)qerr("?face_v");
      face_nrm_elements=faces*N_PER_F;
      face_nrm=(float*)qcalloc(sizeof(float)*faces*N_PER_F);
      if(!face_nrm)qerr("?face_nrm");
    } else if(cmd=="*MESH_FACE")
    { f->GetString(ns); n=atoi(ns);
      curFace=n;
//qdbg("MESH_FACE %d\n",n);
      f->GetString(ns); f->GetString(ns); v1=atoi(ns);
      f->GetString(ns); f->GetString(ns); v2=atoi(ns);
      f->GetString(ns); f->GetString(ns); v3=atoi(ns);
      for(i=0;i<N_PER_V;i++)
      {
        face_v[n*N_PER_F+0+i]=vertex[v1*N_PER_V+i];
        face_v[n*N_PER_F+3+i]=vertex[v2*N_PER_V+i];
        face_v[n*N_PER_F+6+i]=vertex[v3*N_PER_V+i];
      }
      // Keep track of face ranges with the same material id
      f->GetString(ns); f->GetString(ns); // AB, BC, CA edge flags
      f->GetString(ns); f->GetString(ns);
      f->GetString(ns); f->GetString(ns);
      // Mesh smoothing
      f->GetString(ns);
      // Get all groups until *MESH_MTLID
      while(1)
      { f->GetString(ns);
        if(f->IsEOF())break;
        if(ns=="*MESH_MTLID")break;
      }
      f->GetString(ns); // MTL id
      n=atoi(ns);
      if(n!=curMtlID)
      {
        // New material ID range
        if(bursts>=MAX_FACE_BURST)
        { qwarn("DGeode:ImportASE(): max face burst (%d) maxed out",bursts);
        } else
        { burstStart[bursts]=curFace*3*3;     // Assumes tris
          burstCount[bursts]=0;
          burstMtlID[bursts]=n;
          burstVperP[bursts]=3;               // Assumes tris only
          bursts++;
          curMtlID=n;
//qdbg("new burst; bursts=%d\n",bursts);
        }
      }
      burstCount[bursts-1]+=3*3;      // Count #consecutive faces (tri's)
//qdbg("  updated burstCount=%d\n",burstCount[bursts-1]);
//if((n%1000)==0)qdbg("face %d: %d %d %d\n",n,v1,v2,v3);
    } else if(cmd=="*MESH_VERTEXNORMAL")
    { //qdbg("vertexNormal %d\n",vnrmIndex);
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); vec.x=atof(ns);
      f->GetString(ns); vec.y=atof(ns);
      f->GetString(ns); vec.z=atof(ns);
      // Transform normal through node matrix
      matNode.TransformVector(&vec,&vecT);
      n=vnrmIndex;
      face_nrm[vnrmIndex*3+0]=-vecT.x;
      face_nrm[vnrmIndex*3+1]=-vecT.z;
      face_nrm[vnrmIndex*3+2]=vecT.y;
      vnrmIndex++;
//if((vnrmIndex%1000)==0)qdbg("facenrm %d: %d %d %d\n",n,v1,v2,v3);
    } else if(cmd=="*MESH_NUMTVERTEX")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d texture vertices\n",n);
      tvertices=n;
      if(n)
      { tvertex=(float*)qcalloc(sizeof(float)*tvertices*N_PER_V);
        if(!tvertex)qerr("?tvertex");
      }
    } else if(cmd=="*MESH_TVERT")
    { f->GetString(ns); n=Eval(ns);
      f->GetString(ns); x=atof(ns);
      f->GetString(ns); y=atof(ns);
      f->GetString(ns); z=atof(ns);
      tvertex[n*N_PER_V+0]=x;
      tvertex[n*N_PER_V+1]=y;
      tvertex[n*N_PER_V+2]=z;
//qdbg("tvertex %d: %f %f %f\n",n,x,y,z);
    } else if(cmd=="*MESH_NUMTVFACES")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d texture faces\n",n);
      tfaces=n;
      // Assumes all triangles here
      tface_v_elements=tfaces*N_PER_TF;
      tface_v=(float*)qcalloc(sizeof(float)*tface_v_elements);
      if(!tface_v)qerr("?tface_v");
    } else if(cmd=="*MESH_TFACE")
    { int burst;
      dfloat xScale,yScale,scale[2],offset[2];
      DMaterial *mat=0;
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); v1=atoi(ns);
      f->GetString(ns); v2=atoi(ns);
      f->GetString(ns); v3=atoi(ns);
      // UVW tiling
      burst=GetBurstForFace(n*9);    // Assume triangles; 3x3 floats/v
      if(burst!=-1)
      {
        mat=GetMaterialForID(burstMtlID[burst]);
        // Calculate scaling to apply to texture coordinates
        xScale=mat->uvwUtiling;
        yScale=mat->uvwVtiling;
        // Give it a nice form for the 'for' loop below
        scale[0]=xScale;
        scale[1]=yScale;
        offset[0]=(1-scale[0])/2;
        offset[1]=(1-scale[1])/2;
      } else
      { 
        // Default tiling
        scale[0]=scale[1]=1.0f;
        offset[0]=offset[1]=1.0f;
      }
//qdbg("TFACE %d; burst %d (mtlid %d), scale=%f,%f\n",
//n,burst,burstMtlID[burst],scale[0],scale[1]);
      // We use just 2D texture maps, no Z of texture vertex is used
      for(i=0;i<N_PER_TV;i++)
      {
        tface_v[n*N_PER_TF+0+i]=tvertex[v1*N_PER_V+i]*scale[i]+offset[i];
        tface_v[n*N_PER_TF+2+i]=tvertex[v2*N_PER_V+i]*scale[i]+offset[i];
        tface_v[n*N_PER_TF+4+i]=tvertex[v3*N_PER_V+i]*scale[i]+offset[i];
      }
if((n%1000)==0)qdbg("tface %d: %d %d %d\n",n,v1,v2,v3);
    } else if(cmd=="*MATERIAL_REF")
    { f->GetString(ns); n=atoi(ns);
      materialRef=n;
    }
  }
  
  // End of importing this GEOMOBJECT
#ifdef OBS
DumpArray("vertex",vertex,vertices*N_PER_V,N_PER_V);
DumpArray("face",face_v,face_v_elements,N_PER_F);
DumpArray("face_nrm",face_nrm,face_nrm_elements,N_PER_F);
#endif
#ifdef OBS_DBG
  for(i=0;i<faces*N_PER_F;i++)
  { qdbg("face_elt%d: %f\n",i,face_v[i]);
  }
  for(i=0;i<tfaces*N_PER_TF&&i<100;i++)
  { qdbg("tface_v[%d]: %f\n",i,tface_v[i]);
  }
#endif
  // Info bursts
qdbg("Bursts=%d\n",bursts);
  for(i=0;i<bursts;i++)
  { qdbg("Burst @%d, count %d, mtlid %d\n",burstStart[i],
      burstCount[i],burstMtlID[i]);
  }
  return TRUE;
}

/**********************
* IMPORT VRML (SHAPE) *
**********************/
static void MyGetString(QFile *f,qstring &s)
// Tokenizer; separates word based on ',' and whitespace
{
  char buf[256],*d=buf,c;
  int  i;
  
  f->SkipWhiteSpace();
  for(i=0;i<sizeof(buf);i++)
  {
    c=f->GetChar();
    // ',' is a separator
    if(c==','&&i>0)
    { f->UnGetChar(c);
      break;
    }
    // Separators (whitespace)
    if(c==' '||c==10||c==13||c==9)break;
    *d++=c;
  }
  *d=0;
  s=buf;
}
static void GetVRMLToken(QFile *f,qstring &s)
// Skips comments and such
{
 retry:
  MyGetString(f,s);
  if(s=="DEF")
  {
    // Skip name creation
    f->GetString(s);
    goto retry;
  }
}
static int CountCommas(QFile *f)
// Returns the #commas that are found from here to the first ']'
// closing token.
// This is used to precalculate the number of coordinate points,
// normals and texCoords
{
  int offset,n;
  char c;
  qstring s;
  
  offset=f->Tell();
  n=1;                // Coordinates is n+
  while(1)
  {
    c=f->GetChar();
    if(f->IsEOF())break;
    if(c==',')
    { n++;
      // Was it an unnecessary comma at the end?
      f->GetString(s);
      if(s=="]")
      {
        // Indeed, the comma was last in the list, don't count it
        // as a vertex
        n--;
        break;
      }
    }
    if(c==']')break;
  }
  f->Seek(offset);
  return n;
}
static int CountMinusOnes(QFile *f)
// Returns the # '-1's that are found from here to the first ']'
// closing token.
// This is used to precalculate the number of faces
{
  int offset,n;
  qstring s;
  
  offset=f->Tell();
  n=0;                // All polygons are postfixed with "-1"
  while(1)
  {
    MyGetString(f,s);
    if(f->IsEOF())break;
    if(s=="-1")n++;
    else if(s=="]")break;
  }
  f->Seek(offset);
  return n;
}
static int CountMinuses(QFile *f)
// Returns the # '-'s that are found from here to the first ']'
// closing token.
// This is used to precalculate the number of faces
{
  int offset,n;
  char c;
  qstring s;
  
  offset=f->Tell();
  n=0;                // All polygons are postfixed with "-1"
  while(1)
  {
    c=f->GetChar();
    if(f->IsEOF())break;
    if(c=='-')n++;
    else if(c==']')break;
  }
  f->Seek(offset);
  return n;
}
static int CountIndices(QFile *f)
// Returns the # indices that are found from here to the first ']'
// closing token.
// This is used to precalculate the number of coord/normal/tc indices
{
  int offset,n;
  qstring s;
  
  offset=f->Tell();
  n=0;
  while(1)
  {
    MyGetString(f,s);
    if(f->IsEOF())break;
    if(s=="]")break;
    if(s!="-1"&&s!=","&&s!="[")
    { //qdbg("index '%s'\n",(cstring)s);
      n++;
    }
  }
  f->Seek(offset);
  return n;
}
static bool ReadUntilToken(QFile *f,cstring token)
// Read on until token 's' is found
{
  qstring s;
  
  while(1)
  {
    f->GetString(s);
    if(f->IsEOF())break;
    if(s==token)return TRUE;
  }
  // Token not found
  return FALSE;
}

bool DGeob::ImportVRML(QFile *f)
// Read a geometry node from the VRML file
// Assumes the pointer is at the opening { of the object
{
  qstring cmd,ns;
  int     i,j,k;
  int     n,lastOffset;
  float   x,y,z;
  float   r,g,b;
  int     v1,v2,v3;
  int     v[16];          	// Large polygons are possible
  int     nVertices;		// Number of vertices in geob
  float  *ptrVertex;
  DMaterial *curMat;                      // Current material being edited
  int        curMatIndex,curSubMatIndex;  // Tree-ing materials 1 deep
  
qdbg("DGeob: importVRML\n");
  
  // Init
  nVertices=0;

  while(1)
  {
    lastOffset=f->Tell();
    GetVRMLToken(f,cmd);
    // End shape loading by EOF (no more shapes) or other shape
    if(f->IsEOF())break;
    if(cmd=="Shape")
    {
      // Reading into another shape, restore state for parent reader
      f->Seek(lastOffset);
      break;
    }
    
    // Geometry
    if(cmd=="coord")
    {
      int n,off;
      
      // coord [DEF NAME] Coordinate { point [ ... ] }
      // Pre-count upcoming #coordinates
      n=CountCommas(f);
      vertices=n;
      vertex=(float*)qcalloc(sizeof(float)*vertices*N_PER_V);
      if(!vertex)qerr("?vertex");
qdbg("coord; %d coordinates\n",vertices);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vertices
      for(n=0;n<vertices;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        MyGetString(f,ns); z=atof(ns);
        vertex[n*N_PER_V+0]=x;
        vertex[n*N_PER_V+1]=y;
        vertex[n*N_PER_V+2]=z;
//qdbg("  vertex(%f,%f,%f)\n",x,y,z);
        // Skip comma
        MyGetString(f,ns);
      }
    } else if(cmd=="normal")
    {
      int n,off;
      
      // normal [DEF NAME] Normal { vector [ ... ] }
      // Pre-count upcoming #coordinates
      n=CountCommas(f);
      nrmVectors=n;
      nrmVector=(float*)qcalloc(sizeof(float)*nrmVectors*3);
      if(!nrmVector)qerr("?nrmVector");
qdbg("normal; %d vectors\n",nrmVectors);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vectors
      for(n=0;n<nrmVectors;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        MyGetString(f,ns); z=atof(ns);
        nrmVector[n*3+0]=x;
        nrmVector[n*3+1]=y;
        nrmVector[n*3+2]=z;
//qdbg("  nrmVector(%f,%f,%f)\n",x,y,z);
        // Skip comma
        MyGetString(f,ns);
      }
    } else if(cmd=="texCoord")
    {
      int n,off;
      
      // texCoord [DEF NAME] TextureCoordinate { point [ ... ] }
      // VRML uses 2D texCoords (UV; ASE uses 3D: UVW)
      // Pre-count upcoming #coordinates
      n=CountCommas(f);
      tvertices=n;
      tvertex=(float*)qcalloc(sizeof(float)*tvertices*N_PER_TV);
      if(!tvertex)qerr("?tvertex");
qdbg("texCoord; %d coords\n",tvertices);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vectors
      for(n=0;n<tvertices;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        tvertex[n*N_PER_TV+0]=x;
        tvertex[n*N_PER_TV+1]=y;
//qdbg("  tvertex(%f,%f)\n",x,y);
        // Skip comma
        MyGetString(f,ns);
      }

    // Actual indexed coordinate data
    //
    } else if(cmd=="coordIndex")
    {
      int n,off;
      
      // coordIndex [ 0,1,2,-1,... ]
      // Pre-count upcoming #coordinates
      n=CountIndices(f);
      // Remember for burst (at end of read)
      nVertices=n;
      // Number of faces is as yet unknown
      faces=0;
      face_v_elements=n*N_PER_V;
      face_v=(float*)qcalloc(sizeof(float)*n*N_PER_V);
      if(!face_v)qerr("?face_v");
qdbg("coordIndex; %d indices\n",n);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read faces, 'j' is maintained while reading indices
      ptrVertex=face_v;
      for(j=0;j<n;)
      {
        // Read polygon; we could just read and store every index!=-1,
        // but if I find a file which mixes 3- and 4-point poly's for
        // example, this method will ease adding the possibility.
        // For now, I assume all polygons in a geometry node all
        // have the same number of vertices per polygon.
        // Each geometry node may have its own #vertices per polygon.
        // (as seen in one of SCGT's SKBOX.wrl files)
        for(i=0;i<16;i++)
        {
          MyGetString(f,ns); v[i]=atoi(ns); MyGetString(f,ns);
          if(v[i]==-1)break;
          if(f->IsEOF())break;
          // Index read
          j++;
        }
        // 'i' contains the number of vertices for this face
        burstVperP[0]=i;
        // Copy the vertex data into the face vertex array
        for(k=0;k<i;k++)
        {
          // Vertex is XYZ
          *ptrVertex++=vertex[v[k]*N_PER_V+0];
          *ptrVertex++=vertex[v[k]*N_PER_V+1];
          *ptrVertex++=vertex[v[k]*N_PER_V+2];
//if(k==0)qdbg("  face (");
//else    qdbg(",");
//qdbg("%d",v[k]);
        }
//qdbg(")\n");
        // Read a face
        faces++;
//qdbg("    j=%d, n=%d\n",j,n);
      }
    } else if(cmd=="normalIndex")
    {
      int n,off;
      
      // normalIndex [ 0,1,2,-1,... ]
      // Assumes normalPerVertex==TRUE
      // Pre-count upcoming #coordinates
      n=CountIndices(f);
      // Number of faces is as yet unknown
      face_nrm_elements=n*N_PER_V;
      face_nrm=(float*)qcalloc(sizeof(float)*n*N_PER_V);
      if(!face_nrm)qerr("?face_nrm");
qdbg("normalIndex; %d indices\n",n);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read faces, 'j' is maintained while reading indices
      ptrVertex=face_nrm;
      k=0;
      for(j=0;j<n;)
      {
        MyGetString(f,ns); v1=atoi(ns); MyGetString(f,ns);
        if(f->IsEOF())break;
        // Don't insert breaks (-1)
        if(v1==-1)
        { //if(k>0)qdbg(")\n");
          k=0;
          continue;
        } else
        {
//if(k==0)qdbg("  normal (");
//else    qdbg(",");
//qdbg("%d",v1);
          k++;
          // Actual index read
          j++;
        }
        for(i=0;i<N_PER_V;i++)
        {
          *ptrVertex++=nrmVector[v1*N_PER_V+i];
        }
      }
//qdbg(")\n");

    } else if(cmd=="texCoordIndex")
    {
      int n,off;
      
      // texCoordIndex [ 0,1,2,-1,... ]
      // #texture faces as yet unknown
      tfaces=0;
      // Pre-count upcoming #coordinates
      n=CountIndices(f);
      tface_v_elements=n*2;
      tface_v=(float*)qcalloc(sizeof(float)*n*2);
      if(!tface_v)qerr("?tface_v");
qdbg("texCoordIndex; %d indices\n",n);

      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read faces, 'j' is maintained while reading indices
      ptrVertex=tface_v;
      k=0;
      for(j=0;j<n;)
      {
        MyGetString(f,ns); v1=atoi(ns); MyGetString(f,ns);
        if(f->IsEOF())break;
        // Don't insert breaks (-1)
        if(v1==-1)
        { //if(k>0)qdbg(")\n");
          k=0;
          continue;
        } else
        {
//if(k==0)qdbg("  texCoord (");
//else    qdbg(",");
//qdbg("%d",v1);
          k++;
          // Actual index read
          j++;
        }
        // Copy texture coordinates in
        for(i=0;i<N_PER_TV;i++)
        {
          *ptrVertex++=tvertex[v1*N_PER_TV+i];
        }
      }
//qdbg(")\n");

    }
  }
  
  // VRML geometry nodes have only 1 material burst
  bursts=1;
  burstStart[0]=0;
  burstCount[0]=nVertices*3;
  
  // Info bursts
qdbg("Bursts=%d\n",bursts);
  for(i=0;i<bursts;i++)
  { qdbg("Burst @%d, count %d, mtlid %d\n",burstStart[i],
      burstCount[i],burstMtlID[i]);
  }
  
  // Material ID is set by parent reader
  return TRUE;
 fail:
  return FALSE;
}

/******
* DOF *
******/
bool DGeob::ExportDOF(QFile *f)
// Export the geob into a file
// It doesn't include the original vertices, just the faces.
{
  int i,len,off,off2;

  f->Write("GOB0",4);
  off=f->Tell();
  len=0; f->Write(&len,sizeof(len));
  f->Write(&faces,sizeof(faces));
  f->Write(&tfaces,sizeof(tfaces));
  f->Write(&paintFlags,sizeof(paintFlags));
  // Write sizes of paint arrays
  f->Write(&face_v_elements,sizeof(int));
  f->Write(&face_nrm_elements,sizeof(int));
  f->Write(&tface_v_elements,sizeof(int));
  // Write necessary arrays
  f->Write(face_v,face_v_elements*sizeof(float));
  f->Write(face_nrm,face_nrm_elements*sizeof(float));
  f->Write(tface_v,tface_v_elements*sizeof(float));
  
  // Write bursts
  f->Write(&bursts,sizeof(bursts));
  f->Write(burstStart,bursts*sizeof(int));
  f->Write(burstCount,bursts*sizeof(int));
  f->Write(burstMtlID,bursts*sizeof(int));
  f->Write(burstVperP,bursts*sizeof(int));
  
  // Others
  f->Write(&materialRef,sizeof(materialRef));

  // Record size of chunk
  off2=f->Tell();
  f->Seek(off);
  len=off2-off;
  f->Write(&len,sizeof(len));
  
  // Return to previous write position
  f->Seek(off2);
  
  return TRUE;
}
bool DGeob::ImportDOF(QFile *f)
// Read a geob from a file
{
  char buf[8];
  int  len;
  
  // Destroy any old geob
  Destroy();
  
  // Read GEOB identifier ("GOB0")
  f->Read(buf,4);
  f->Read(&len,sizeof(len));
  
  f->Read(&faces,sizeof(faces));
  f->Read(&tfaces,sizeof(tfaces));
  f->Read(&paintFlags,sizeof(paintFlags));
  
  // Read sizes of paint arrays
  f->Read(&face_v_elements,sizeof(int));
  f->Read(&face_nrm_elements,sizeof(int));
  f->Read(&tface_v_elements,sizeof(int));

qdbg("faces=%d, tfaces=%d\n",faces,tfaces);

  // Read necessary arrays
  face_v=(float*)qcalloc(face_v_elements*sizeof(float));
  if(!face_v)
  {
    qwarn("DGeob:ImportDOF(); out of memory to read face_v");
    return FALSE;
  }
  f->Read(face_v,face_v_elements*sizeof(float));
  face_nrm=(float*)qcalloc(face_nrm_elements*sizeof(float));
  if(!face_nrm)
  {
    qwarn("DGeob:ImportDOF(); out of memory to read face_v");
    return FALSE;
  }
  f->Read(face_nrm,face_nrm_elements*sizeof(float));
  
  // Read texture faces
  if(tface_v_elements)
  {
    tface_v=(float*)qcalloc(tface_v_elements*sizeof(float));
    if(!tface_v)
    {
      qwarn("DGeob:ImportDOF(); out of memory to read face_v");
      return FALSE;
    }
    f->Read(tface_v,tface_v_elements*sizeof(float));
  } else tface_v=0;
  
  // Read bursts
  f->Read(&bursts,sizeof(bursts));
  f->Read(burstStart,bursts*sizeof(int));
  f->Read(burstCount,bursts*sizeof(int));
  f->Read(burstMtlID,bursts*sizeof(int));
  f->Read(burstVperP,bursts*sizeof(int));
  
  // Others
  f->Read(&materialRef,sizeof(materialRef));
  
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

/****************
* Triangulation *
****************/
void DGeob::Triangulate()
// Make all polygons triangles
// Still bugs
{
  int i,v,total;
  float *newFaceV,*newTFaceV;
  float *sFace,*dFace,*sTFace,*dTFace;
  
qdbg("Triangulate\n");
//return;

  // Count total of new vertices
  total=0;
  for(i=0;i<bursts;i++)
  {
    switch(burstVperP[i])
    {
      case 3: total+=burstCount[i]; break;
      case 4: total+=burstCount[i]*6/4; break;
      // Unsupported
      default: total+=burstCount[i]; break;
    }
  }
qdbg("  total count of triangulated mesh: %d\n",total);

  // Allocate new arrays
  newFaceV=(float*)qcalloc(total*sizeof(float)*N_PER_V);
  if(!newFaceV)
  {
    qwarn("DGeob:Triangulate; not enough memory (newFaceV)");
    return;
  }
  newTFaceV=(float*)qcalloc(total*sizeof(float)*N_PER_TV);
  if(!newTFaceV)
  {
    qfree(newFaceV);
    qwarn("DGeob:Triangulate; not enough memory (newTFaceV)");
    return;
  }
  
  // Start copying
  sFace=face_v;
  dFace=newFaceV;
  sTFace=tface_v;
  dTFace=newTFaceV;
  
  // Scale arrays to triangle data (tesselate)
DumpArray("sFace/face_v",sFace,burstCount[0]*N_PER_V,3);
DumpArray("sTFace/tface_v",sTFace,burstCount[0]*N_PER_TV,2);
  for(i=0;i<bursts;i++)
  {
    // Already triangles?
    if(burstVperP[i]==3)
    {
      // Vanilla copy
      for(v=0;v<burstCount[i];v++)
      {
        // Copy face vertex
        *dFace++=*sFace; sFace++;
        *dFace++=*sFace; sFace++;
        *dFace++=*sFace; sFace++;
        // Copy tface vertices
        *dTFace++=*sTFace; sTFace++;
        *dTFace++=*sTFace; sTFace++;
      }
      continue;
    } else if(burstVperP[i]==4)
    {
      // Make 4 into 3; from 0123 create 012 and 230
      // Vanilla copy
      for(v=0;v<burstCount[i]/4;v++)
      {
        // Copy face vertices
        // 012
        memcpy(dFace,&sFace[0],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[1],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[2],3*sizeof(float)); dFace+=3;
        // 230
        memcpy(dFace,&sFace[2],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[3],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[0],3*sizeof(float)); dFace+=3;
        sFace+=4*3;
        // Copy tface vertices; 012
        memcpy(dTFace,&sTFace[0],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[1],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[2],2*sizeof(float)); dTFace+=2;
        // 230
        memcpy(dTFace,&sTFace[2],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[3],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[0],2*sizeof(float)); dTFace+=2;
        sTFace+=4*2;
      }
      // From 4 to 6 vertices
      burstCount[i]=burstCount[i]*6/4;
      burstVperP[i]=3;
    }
  }
  // Move over to new vertex arrays
  qfree(face_v);
  qfree(tface_v);
  face_v=newFaceV;
  tface_v=newTFaceV;
}

void DGeob::Paint()
{
  int i,first;

  // If the object has been put in a display list, use that
  if(list)
  {
    glCallList(list);
    return;
  }
  
  // Generate a display list for the object
  list=glGenLists(1);
  glNewList(list,GL_COMPILE_AND_EXECUTE);
  
#ifdef OBS
qdbg("DGeob::Paint (this=%p), master geode=%p\n",this,geode);
qdbg("bursts=%d\n",bursts);
#endif
  
  // Faces
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  // Draw all bursts
  for(i=0;i<bursts;i++)
  {
#ifdef OBS
qdbg("paint burst %d, mref=%d, count=%d, VperP=%d\n",
i,materialRef,burstCount[i],burstVperP[i]);
qdbg("geode=%p, matRef=%d\n",geode,materialRef);
qdbg("geode->mat[ref]=%p\n",geode->material[materialRef]);
#endif
    // Install material in OpenGL engine
    DMaterial *mat=GetMaterialForID(burstMtlID[i]);
    if(mat)
    {
      mat->PrepareToPaint();
//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    }
    
// Test wireframe
//glPolygonMode(GL_FRONT,GL_LINE);

    glEnable(GL_CULL_FACE);
    //glEnable(GL_DEPTH_TEST);
#ifdef ND_LIGHT
    if(1)
    { glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
    }
    glShadeModel(GL_SMOOTH);
#endif
//glShadeModel(GL_FLAT);
glShadeModel(GL_SMOOTH);

    //glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    first=burstStart[i];
    glVertexPointer(3,GL_FLOAT,0,&face_v[first]);
//DumpArray("face_v",&face_v[first],burstCount[i],3);
//DumpArray("normal array",&face_nrm[first],18,3);
    glNormalPointer(GL_FLOAT,0,&face_nrm[first]);
    if(tface_v)
    { 
      int firstTFace;
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      // Assume all polygons are the same VperP
      firstTFace=first/burstVperP[i]*2;
//qdbg("  burst: firstTFace=%d from first=%d\n",firstTFace,first);
      glTexCoordPointer(2,GL_FLOAT,0,&tface_v[firstTFace]);
    }
    //glDisable(GL_BLEND);
    
    // Decide how to draw
    if(burstVperP[i]==3)
    { // Most common; triangles
      glDrawArrays(GL_TRIANGLES,0,burstCount[i]/3);
    } else if(burstVperP[i]==4)
    {
      // Quads; used by SCGT for the skybox for example
      glDrawArrays(GL_QUADS,0,burstCount[i]);
    } else
    {
      // Assume N-polygon; hopefully only 1 polygon is defined
      glDrawArrays(GL_POLYGON,0,burstCount[i]);
    }
  }
  // Turn off
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisable(GL_TEXTURE_2D);
  
  glEndList();
}

/*********
* DGeode *
*********/
#undef  DBG_CLASS
#define DBG_CLASS "DGeode"

DGeode::DGeode(cstring fname)
  : DObject()
{
  materials=0;
  geobs=0;
}
DGeode::DGeode(DGeode *master,int flags) : DObject()
// Create a geode based on another geode
// flags&1; don't reference materials; create a copy
{
  int i;

  // Clone the geometric objects
  geobs=master->GetNoofGeobs();
  for(i=0;i<master->GetNoofGeobs();i++)
  {
    // Clone geob
    geob[i]=new DGeob(master->geob[i],flags);
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
  {
    if(geob[i])delete geob[i];
  }
  for(i=0;i<materials;i++)
  {
    if(material[i])delete material[i];
  }
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
  QTRACE("Material not found in %d materials",materials);
  return 0;
}
void DGeode::SetMapPath(cstring path)
// Set image loading path
// Pass 0 for 'path' to reset the default loading path
{
  if(path)pathMap=path;
  else    pathMap="";
}

/****************************
* Importing ASE file format *
****************************/
bool DGeode::ImportASE(cstring fname)
// A very flat importer for a subset of .ASE files
// Doesn't support multiple material depths
{
  DBG_C("ImportASE")
  DBG_ARG_S(fname)
  
  QFile *f;
  qstring cmd,ns;
  int i,n;
  float x,y,z;
  float r,g,b;
  int v1,v2,v3;
  DMaterial *curMat=0;             // Current material being edited
  int curMatIndex,curSubMatIndex;  // Tree-ing materials 1 deep
  int curFace;

  QTRACE("DGeode::ImportASE\n");

  f=new QFile(fname);

  QTRACE("  f=%p\n",f);
  // File was found?
  if(!f->IsOpen())
  {
qdbg("  !open\n");
    QTRACE("Can't import ASE '%s'",fname);
    return FALSE;
  }

  // Search for vertices and information
  while(1)
  {
    f->GetString(cmd);
    if(f->IsEOF())break;
//qdbg("cmd='%s'\n",(cstring)cmd);
    
    // Geom objects
    //
    if(cmd=="*GEOMOBJECT")
    {
      bool r;
      DGeob *o;
      // Create geob
      if(geobs==MAX_GEOB)
      { qerr("Too many geomobjects in ASE, max=%d",MAX_GEOB);
        break;
      }
      o=new DGeob(this);
      geob[geobs]=o;
qdbg("new Geob @%p\n",geob[geobs]);
      geobs++;
      r=o->ImportASE(f);
      if(!r)goto fail;
    
    // Materials
    } else if(cmd=="*MATERIAL_COUNT")
    {
      f->GetString(ns); n=atoi(ns);
      materials=n;

qdbg("%d materials\n",materials);
      for(i=0;i<n;i++)
      { material[i]=new DMaterial(); //qcalloc(sizeof(DMaterial));
      }
    } else if(cmd=="*MATERIAL")
    {
      f->GetString(ns); n=atoi(ns);
      curMatIndex=n;
      curMat=material[curMatIndex];
qdbg("  material %d (@%p)\n",n,curMat);
    } else if(cmd=="*NUMSUBMTLS")
    {
      f->GetString(ns); n=atoi(ns);
      curMat->submaterials=n;
qdbg("%d submaterials\n",n);
      for(i=0;i<n;i++)
      { curMat->submaterial[i]=new DMaterial();
      }
    } else if(cmd=="*SUBMATERIAL")
    {
      f->GetString(ns); n=atoi(ns);
      curSubMatIndex=n;
qdbg("    submaterial %d\n",n);
      curMat=material[curMatIndex]->submaterial[curSubMatIndex];
    //
    // Material properties
    //
    } else if(cmd=="*BITMAP")
    {
      char buf[256];
      
      f->GetString(ns);
      // Go from PC full path to attempted SGI path
      sprintf(buf,"images/%s",QFileBase(ns));
qdbg("  material image %s (base %s)\n",buf,(cstring)ns);
      curMat->AddBitMap(buf);
    } else if(cmd=="*UVW_U_TILING")
    { f->GetString(ns);
      if(curMat)curMat->uvwUtiling=atof(ns);
    } else if(cmd=="*UVW_V_TILING")
    { f->GetString(ns);
      if(curMat)curMat->uvwVtiling=atof(ns);
    } else if(cmd=="*UVW_U_OFFSET")
    { f->GetString(ns);
      if(curMat)curMat->uvwUoffset=atof(ns);
    } else if(cmd=="*UVW_V_OFFSET")
    { f->GetString(ns);
      if(curMat)curMat->uvwVoffset=atof(ns);
    } else if(cmd=="*UVW_ANGLE")
    { f->GetString(ns);
      if(curMat)curMat->uvwAngle=atof(ns);
    } else if(cmd=="*MATERIAL_NAME")
    { f->GetString(ns);
      if(curMat)
        curMat->name=ns;
    } else if(cmd=="*MATERIAL_CLASS")
    { f->GetString(ns);
      if(curMat)
        curMat->className=ns;
    } else if(cmd=="*MATERIAL_AMBIENT")
    { f->GetString(ns); r=atof(ns);
      f->GetString(ns); g=atof(ns);
      f->GetString(ns); b=atof(ns);
      if(curMat)
      {
        curMat->emission[0]=r;
        curMat->emission[1]=g;
        curMat->emission[2]=b;
        curMat->emission[3]=1.0;
        //curMat->ambient[0]=r;
        //curMat->ambient[1]=g;
        //curMat->ambient[2]=b;
        //curMat->ambient[3]=1.0;
      }
    } else if(cmd=="*MATERIAL_DIFFUSE")
    { f->GetString(ns); r=atof(ns);
      f->GetString(ns); g=atof(ns);
      f->GetString(ns); b=atof(ns);
      if(curMat)
      {
#ifdef SCGT_FIX_DIFFUSE_COLOR
        // SCGT uses black materials
        if(r==0.0&&g==0.0&&b==0.0)
        {
          // No black for clarity please
          r=g=b=1;
        }
#endif
        curMat->diffuse[0]=r;
        curMat->diffuse[1]=g;
        curMat->diffuse[2]=b;
        curMat->diffuse[3]=1.0;
      }
    } else if(cmd=="*MATERIAL_SPECULAR")
    { f->GetString(ns); r=atof(ns);
      f->GetString(ns); g=atof(ns);
      f->GetString(ns); b=atof(ns);
      if(curMat)
      {
        curMat->specular[0]=r;
        curMat->specular[1]=g;
        curMat->specular[2]=b;
        curMat->specular[3]=1.0;
      }
    } else if(cmd=="*MATERIAL_SHINE")
    { // May need to use SHINESTRENGTH instead!
      f->GetString(ns); x=atof(ns);
      if(curMat)
        curMat->shininess=x*128.0;
    } else if(cmd=="*MATERIAL_TRANSPARENCY")
    { // Transparency is defined as the alpha component of DIFFUSE
      f->GetString(ns); x=atof(ns);
      if(curMat)
      { curMat->diffuse[3]=x;
        curMat->transparency=x;
      }
      // From dpoly.cpp, it really uses glColor4f(1,1,1,x) to
      // do transparency with GL_MODULATE texture env mode
      // So this might not be the way to do it.
    }
  }
  delete f; f=0;
  
qdbg("geobs=%d\n",geobs);
  return TRUE;
 fail:
  if(f)delete f;
  return FALSE;
}

/**************
* VRML IMPORT *
**************/
bool DGeode::ImportVRML(cstring fname)
// A very flat importer for a subset of .VRML files
// Files created from .VRL (SCGT) mainly.
{
  QFile *f;
  qstring cmd,ns;
  int i;
  float x,y,z;
  float r,g,b;
  int v1,v2,v3;
  DMaterial *curMat=0;             // Current material being edited
  int curFace;
  
  f=new QFile(fname);

  // File was found?
  if(!f->IsOpen())
  { QTRACE("Can't import VRML '%s'",fname);
    return FALSE;
  }

  // Search for vertices and information
  while(1)
  {
    GetVRMLToken(f,cmd);
    if(f->IsEOF())break;
//qdbg("cmd='%s'\n",(cstring)cmd);
    
    // Geom objects (shapes)
    //
    if(cmd=="geometry")
    {
      // Actually, this is halfway a Shape node
      // The materials though are defined before the geometry node,
      // and because of ASE conformity, materials are not local
      // to geometry nodes for VRML, but global to the scene
      // (although never shared in VRML files, but they can
      // in ASE)
      bool r;
      DGeob *o;
      // Create geob
      if(geobs==MAX_GEOB)
      { qerr("Too many shapes in VRML, max=%d",MAX_GEOB);
        break;
      }
      o=new DGeob(this);
      geob[geobs]=o;
qdbg("new Geob @%p\n",geob[geobs]);
      geobs++;
      r=o->ImportVRML(f);
      if(!r)goto fail;
      
      // Only 1 material per VRML geometry node
      // Other burst vars were set in ImportVRML() before
      o->burstMtlID[0]=materials-1;
      o->materialRef=materials-1;
      
    // Materials
    //
    } else if(cmd=="material")
    {
      // Skip name
      f->GetString(ns);
      
      if(materials==MAX_MATERIAL)
      { qerr("Too many materials in VRML, max=%d",MAX_GEOB);
        break;
      }
      material[materials]=new DMaterial();
      curMat=material[materials];
      materials++;

    // Material properties
    //
    } else if(cmd=="url")
    {
      // Assumes url is in a 'texture' node
      char buf[256];
      
      f->GetString(ns);
      // Go from PC full path to attempted SGI path
      sprintf(buf,"images/%s",QFileBase(ns));
qdbg("  material image %s (base %s)\n",buf,(cstring)ns);
      curMat->AddBitMap(buf);
    } else if(cmd=="ambientIntensity")
    { // VRML uses 1 float (gray)
      f->GetString(ns);
      r=g=b=atof(ns);
//r=g=b=1.0;
      if(curMat)
      {
        curMat->emission[0]=r;
        curMat->emission[1]=g;
        curMat->emission[2]=b;
        curMat->emission[3]=1.0;
        //curMat->ambient[0]=r;
        //curMat->ambient[1]=g;
        //curMat->ambient[2]=b;
        //curMat->ambient[3]=1.0;
      }
    } else if(cmd=="diffuseColor")
    { f->GetString(ns); r=atof(ns);
      f->GetString(ns); g=atof(ns);
      f->GetString(ns); b=atof(ns);
//r=g=b=1.0;
      if(curMat)
      {
        curMat->diffuse[0]=r;
        curMat->diffuse[1]=g;
        curMat->diffuse[2]=b;
        curMat->diffuse[3]=1.0;
      }
    } else if(cmd=="specularColor")
    { f->GetString(ns); r=atof(ns);
      f->GetString(ns); g=atof(ns);
      f->GetString(ns); b=atof(ns);
      if(curMat)
      {
        curMat->specular[0]=r;
        curMat->specular[1]=g;
        curMat->specular[2]=b;
        curMat->specular[3]=1.0;
      }
    } else if(cmd=="shininess")
    { // Not sure whether this is the right token
      f->GetString(ns); x=atof(ns);
      if(curMat)
        curMat->shininess=x*128.0;
    } else if(cmd=="transparency")
    { // Transparency is defined as the alpha component of DIFFUSE
      f->GetString(ns); x=atof(ns);
      if(curMat)
      { curMat->diffuse[3]=x;
        curMat->transparency=x;
      }
      // From dpoly.cpp, it really uses glColor4f(1,1,1,x) to
      // do transparency with GL_MODULATE texture env mode
      // So this might not be the way to do it.
    }
  }
  delete f; f=0;
  
qdbg("geobs=%d\n",geobs);
  return TRUE;
 fail:
  if(f)delete f;
  return FALSE;
}

/********
* PAINT *
********/
void DGeode::_Paint()
{
  int i;
#ifdef OBS
qdbg("==> DGeode:_Paint(); this=%p\n",this);
for(i=0;i<materials;i++)
qdbg("      mat[%d]=%p\n",i,material[i]);
#endif

  for(i=0;i<geobs;i++)
  {
    geob[i]->Paint();
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
void DGeode::Triangulate()
// Triangulate all polygons, if possible
{
  int i;
  for(i=0;i<geobs;i++)
    geob[i]->Triangulate();
}

/******
* DOF *
******/
bool DGeode::ExportDOF(QFile *f)
// Export the geode into a file
{
  int i;
  
  // Header
  f->Write("DOF0",4);
  // Write materials
  f->Write("MATS",4);
  f->Write(&materials,sizeof(materials));
  for(i=0;i<materials;i++)
  {
    if(!material[i]->ExportDOF(f))
      return FALSE;
  }
  
  // Write geobs
  f->Write("GEOB",4);
  f->Write(&geobs,sizeof(geobs));
  for(i=0;i<geobs;i++)
  {
    if(!geob[i]->ExportDOF(f))
      return FALSE;
  }
  return TRUE;
}
bool DGeode::ImportDOF(QFile *f)
// Read the geode from a file
{
  char buf[8];
  int  i;
  
  // Destory current geometry
  
  // Header
  f->Read(buf,4);        // DOF0
  // Chunks
 next_chunk:
  f->Read(buf,4); buf[4]=0;
  if(f->IsEOF())
  { // That's all
    return TRUE;
  }
  
  if(!strncmp(buf,"MATS",4))
  {
    // Read materials
    f->Read(&materials,sizeof(materials));
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
    for(i=0;i<geobs;i++)
    {
      geob[i]=new DGeob(this);
      if(!geob[i]->ImportDOF(f))
        return FALSE;
    }
  }
  goto next_chunk;
}
