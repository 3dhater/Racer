/*
 * DGeode - ASE format importing
 * 26-12-00: Detached from dgeob.cpp/dgeode.cpp.
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/app.h>
#include <d3/matrix.h>
#include <d3/geode.h>
#ifdef linux
// Linux needs atof() prototype
#include <stdlib.h>
#endif
DEBUG_ENABLE

// Indexed face sets instead of flat big arrays?
#define USE_INDEXED_FACES

// Dump end results to debug output?
//#define OPT_DUMP_AFTER_IMPORT

#define N_PER_V     3        // Coordinates per vertex
#define N_PER_F     9        // Coordinates per face (triangles)

#define N_PER_TV    2        // Coordinates used per texture vertex (2D maps)
#define N_PER_TF    6        // Tex Coordinates per tex face

#undef  DBG_CLASS
#define DBG_CLASS "DGeob"

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

/*************
* ASE Import *
*************/
bool DGeobImportASE(DGeob *g,QFile *f)
// Read a GEOMOBJECT from the ASE file
// Assumes the pointer is at the opening { of the object
{
  DBG_C_FLAT("DGeobImportASE")
  
  int nest;
  qstring cmd,ns;
  int i,n;
  float x,y,z;
  float cr,cg,cb;
  int v1,v2,v3;
  DMaterial *curMat;           // Current material being edited
  int curMatIndex,curSubMatIndex;  // Tree-ing materials 1 deep
#ifndef USE_INDEXED_FACES
  int vnrmIndex;               // Normal vertex index
#endif
  int curMtlID;
  int curFace;
  int curNormalFace;           // Last viewed normal face definition
  int curNormalVertex;         // Current normal vertex definition
  DMatrix4 matNode;
  DVector3 vec,vecT;
  dfloat *tempTVertex=0;
  dfloat *tempVColor=0;
  dfloat *tempVertex=0;

qdbg("DGeob: import (geob=@%p)\n",g);
  nest=0;
  curMtlID=-1;
  curFace=0;
  curNormalFace=0;
  curNormalVertex=0;
#ifndef USE_INDEXED_FACES
  vnrmIndex=0;
#endif
  matNode.SetIdentity();
  
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
//qdbg("%d vertices\n",n);
      g->vertices=n;
      // Don't allocate array yet; we will probably need
      // more space because not every vertex will have the same
      // tvertex values. For this we should just be pessimistic
      // and allocate a separate index for now for EVERY vertex
      // in EVERY face.
      // After MESH_NUMVERTEX, MESH_NUMFACES should follow...
      // Allocate temporary vertex storage though to contain
      // the original vertex array in the ASE file (to hand
      // out later when loading the faces).
      tempVertex=(float*)qcalloc(sizeof(float)*g->vertices*3);
      if(!tempVertex)qerr("?tempVertex");
    } else if(cmd=="*MESH_VERTEX")
    { f->GetString(ns); n=Eval(ns);
      f->GetString(ns); x=atof(ns);
      f->GetString(ns); y=atof(ns);
      f->GetString(ns); z=atof(ns);
      // 3DS Max to OpenGL axis system
      tempVertex[n*N_PER_V+0]=x;
      tempVertex[n*N_PER_V+1]=z; //z;
      tempVertex[n*N_PER_V+2]=-y; //-y;
//qdbg("vertex %d: %f %f %f\n",n,x,y,z);
    } else if(cmd=="*MESH_NUMFACES")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d faces\n",n);
qdbg("%d vertices\n",g->vertices);
      // Only triangles are expected (for ASE files)
#ifndef USE_INDEXED_FACES
      faces=n;
      face_v_elements=faces*N_PER_F;
      face_v=(float*)qcalloc(sizeof(float)*faces*N_PER_F);
      if(!face_v)qerr("?face_v");
      face_nrm_elements=faces*N_PER_F;
      face_nrm=(float*)qcalloc(sizeof(float)*faces*N_PER_F);
      if(!face_nrm)qerr("?face_nrm");
#else
      // Allocate worst case set of (indexed) vertices
      g->vertices=n*3;
      g->vertex=(float*)qcalloc(sizeof(float)*g->vertices*3);
      if(!g->vertex)qerr("?vertex");
//qdbg("g->vertex size is %d\n",g->vertices*3);
      // Indexed face set normals
      g->normals=g->vertices;
      g->normal=(float*)qcalloc(sizeof(float)*g->normals*3);
      if(!g->normal)qerr("?normal");
//qdbg("g->normal size is %d\n",g->vertices*3);
#endif

      g->indices=n*3;
      g->index=(dindex*)qcalloc(sizeof(dindex)*g->indices);
      if(!g->index)qerr("DGeob:ImportASE out of memory (index)");
    } else if(cmd=="*MESH_FACE")
    { f->GetString(ns); n=atoi(ns);
      curFace=n;
//qdbg("MESH_FACE %d\n",n);
      // Read vertex indices
      f->GetString(ns); f->GetString(ns); v1=atoi(ns);
      f->GetString(ns); f->GetString(ns); v2=atoi(ns);
      f->GetString(ns); f->GetString(ns); v3=atoi(ns);
      // For the worst case, we must create entirely new sets
      // of vertices and such
      g->vertex[n*3*3+0]=tempVertex[v1*3+0];
      g->vertex[n*3*3+1]=tempVertex[v1*3+1];
      g->vertex[n*3*3+2]=tempVertex[v1*3+2];
      g->vertex[n*3*3+3]=tempVertex[v2*3+0];
      g->vertex[n*3*3+4]=tempVertex[v2*3+1];
      g->vertex[n*3*3+5]=tempVertex[v2*3+2];
      g->vertex[n*3*3+6]=tempVertex[v3*3+0];
      g->vertex[n*3*3+7]=tempVertex[v3*3+1];
      g->vertex[n*3*3+8]=tempVertex[v3*3+2];
      // Create indices
      g->index[n*3+0]=n*3+0;
      g->index[n*3+1]=n*3+1;
      g->index[n*3+2]=n*3+2;
#ifdef OBS
      // Store indices
      g->index[n*3+0]=v1;
      g->index[n*3+1]=v2;
      g->index[n*3+2]=v3;
#endif
#ifndef USE_INDEXED_FACES
      // Store direct vertex information (old way)
      for(i=0;i<N_PER_V;i++)
      {
        face_v[n*N_PER_F+0+i]=vertex[v1*N_PER_V+i];
        face_v[n*N_PER_F+3+i]=vertex[v2*N_PER_V+i];
        face_v[n*N_PER_F+6+i]=vertex[v3*N_PER_V+i];
      }
#endif
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
//n=0; // Always MtlID 0
      if(n!=curMtlID)
      {
        // New material ID range
        if(g->bursts>=DGeob::MAX_FACE_BURST)
        { qwarn("DGeode:ImportASE(): max face burst (%d) maxed out",g->bursts);
        } else
        { g->burstStart[g->bursts]=curFace*3*3;     // Assumes tris
          g->burstCount[g->bursts]=0;
          g->burstMtlID[g->bursts]=n;
          g->burstVperP[g->bursts]=3;               // Assumes tris only
          g->bursts++;
          curMtlID=n;
//qdbg("new burst; bursts=%d\n",bursts);
        }
      }
      g->burstCount[g->bursts-1]+=3*3;      // Count #consecutive faces (tri's)
//qdbg("  updated burstCount=%d\n",burstCount[bursts-1]);
//if((n%1000)==0)qdbg("face %d: %d %d %d\n",n,v1,v2,v3);

    //
    // Normals
    //
    } else if(cmd=="*MESH_FACENORMAL")
    {
      // May do something with the face normal later
      //f->GetString(ns); curNormalFace=atoi(ns);
    } else if(cmd=="*MESH_VERTEXNORMAL")
    { 
      // A vertex normal; we don't use face normals
      // Assumes a MESH_FACENORMAL was just before this
      //qdbg("vertexNormal %d\n",vnrmIndex);
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); vec.x=atof(ns);
      f->GetString(ns); vec.y=atof(ns);
      f->GetString(ns); vec.z=atof(ns);
      // Transform normal through node matrix
      matNode.TransformVectorOr(&vec,&vecT);
      // Store in the indexed face set normal array
#ifdef ND_ORIGINAL_TRANSFORM
      g->normal[curNormalVertex*3+0]=-vecT.x;
      g->normal[curNormalVertex*3+1]=-vecT.z;
      g->normal[curNormalVertex*3+2]=vecT.y;
#endif
      g->normal[curNormalVertex*3+0]=vecT.x;
      g->normal[curNormalVertex*3+1]=vecT.z;
      g->normal[curNormalVertex*3+2]=-vecT.y;
      curNormalVertex++;
#ifdef OBS
      g->normal[n*3+0]=-vecT.x;
      g->normal[n*3+1]=-vecT.z;
      g->normal[n*3+2]=vecT.y;
#endif
#ifndef USE_INDEXED_FACES
      // Store as drawarray form (old)
      n=vnrmIndex;
      face_nrm[vnrmIndex*3+0]=-vecT.x;
      face_nrm[vnrmIndex*3+1]=-vecT.z;
      face_nrm[vnrmIndex*3+2]=vecT.y;
      vnrmIndex++;
//if((vnrmIndex%1000)==0)qdbg("facenrm %d: %d %d %d\n",n,v1,v2,v3);
#endif

    //
    // Texturing
    //
    } else if(cmd=="*MESH_NUMTVERTEX")
    { f->GetString(ns);
      n=Eval(ns);
qdbg("%d texture vertices\n",n);
      // Still, this worst case loader does 1 tvertex per vertex
      g->tvertices=g->vertices;
      if(n)
      {
        tempTVertex=(float*)qcalloc(sizeof(float)*n*N_PER_V);
        if(!tempTVertex)qerr("?tempTVertex");
        // 2D coordinates as end result
        // Note that for every vertex a new tvertex will be created
        // Optimization is done later (DGeodeOptimize())
        g->tvertex=(float*)qcalloc(sizeof(float)*g->vertices*2);
        if(!g->tvertex)qerr("?tvertex");
      }
    } else if(cmd=="*MESH_TVERT")
    {
      f->GetString(ns); n=Eval(ns);
      f->GetString(ns); x=atof(ns);
      f->GetString(ns); y=atof(ns);
      f->GetString(ns); z=atof(ns);
      tempTVertex[n*N_PER_V+0]=x;
      tempTVertex[n*N_PER_V+1]=y;
      tempTVertex[n*N_PER_V+2]=z;
//qdbg("tempTVertex %d: %f %f %f\n",n,x,y,z);
    } else if(cmd=="*MESH_NUMTVFACES")
    { f->GetString(ns);
      n=Eval(ns);
//qdbg("%d texture faces\n",n);
#ifndef USE_INDEXED_FACES
      tfaces=n;
      // Assumes all triangles here
      tface_v_elements=tfaces*N_PER_TF;
      tface_v=(float*)qcalloc(sizeof(float)*tface_v_elements);
      if(!tface_v)qerr("?tface_v");
#endif
    } else if(cmd=="*MESH_TFACE")
    { int burst;
      dfloat xScale,yScale,scale[2],offset[2];
      DMaterial *mat=0;
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); v1=atoi(ns);
      f->GetString(ns); v2=atoi(ns);
      f->GetString(ns); v3=atoi(ns);
      // UVW tiling
      burst=g->GetBurstForFace(n*9);    // Assume triangles; 3x3 floats/v
      if(burst!=-1)
      {
        mat=g->GetMaterialForID(g->burstMtlID[burst]);
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
//n,burst,g->burstMtlID[burst],scale[0],scale[1]);
#ifdef USE_INDEXED_FACES
      // Make sure the indexes of the texture vertices match
      // with the indices of the vertices themselves
      g->tvertex[n*3*2+0]=tempTVertex[v1*3+0];
      g->tvertex[n*3*2+1]=tempTVertex[v1*3+1];
      g->tvertex[n*3*2+2]=tempTVertex[v2*3+0];
      g->tvertex[n*3*2+3]=tempTVertex[v2*3+1];
      g->tvertex[n*3*2+4]=tempTVertex[v3*3+0];
      g->tvertex[n*3*2+5]=tempTVertex[v3*3+1];
#ifdef OBS
      for(i=0;i<N_PER_TV;i++)
      {
        g->tvertex[g->index[n*3+0]*2+i]=
          tempTVertex[v1*N_PER_V+i]*scale[i]+offset[i];
        g->tvertex[g->index[n*3+1]*2+i]=
          tempTVertex[v2*N_PER_V+i]*scale[i]+offset[i];
        g->tvertex[g->index[n*3+2]*2+i]=
          tempTVertex[v3*N_PER_V+i]*scale[i]+offset[i];
      }
#endif
#ifdef OBS
 qdbg("TFace %d; vertex indices=%d, %d, %d, v[]=%d,%d,%d\n",n,
 index[n*3+0],index[n*3+1],index[n*3+2],v1,v2,v3);
 DumpArray("tvertex",tvertex,tvertices*N_PER_TV,N_PER_TV);
#endif
#else
      // We use just 2D texture maps, no Z of texture vertex is used
      for(i=0;i<N_PER_TV;i++)
      {
        tface_v[n*N_PER_TF+0+i]=tempTVertex[v1*N_PER_V+i]*scale[i]+offset[i];
        tface_v[n*N_PER_TF+2+i]=tempTVertex[v2*N_PER_V+i]*scale[i]+offset[i];
        tface_v[n*N_PER_TF+4+i]=tempTVertex[v3*N_PER_V+i]*scale[i]+offset[i];
      }
#endif
 if((n%1000)==0)qdbg("tface %d: %d %d %d\n",n,v1,v2,v3);

    // Vertex colors (very optional)
    } else if(cmd=="*MESH_NUMCVERTEX")
    { f->GetString(ns); n=Eval(ns);
qdbg("%d color vertices\n",n);
      // Worst case loader does 1 vcolor per vertex
      g->vcolors=g->vertices;
      if(n)
      {
        tempVColor=(float*)qcalloc(sizeof(float)*n*3);
        if(!tempVColor)qerr("?tempVColor");
        // Note that for every vertex a new vcolor will be created
        // Optimization is done later (DGeodeOptimize())
        g->vcolor=(float*)qcalloc(sizeof(float)*g->vertices*3);
        if(!g->vcolor)qerr("?vcolor");
      }
    } else if(cmd=="*MESH_VERTCOL")
    {
      // Read RGB (vertex color)
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); x=atof(ns);
      f->GetString(ns); y=atof(ns);
      f->GetString(ns); z=atof(ns);
      tempVColor[n*3+0]=x;
      tempVColor[n*3+1]=y;
      tempVColor[n*3+2]=z;
    } else if(cmd=="*MESH_CFACE")
    {
      f->GetString(ns); n=atoi(ns);
      f->GetString(ns); v1=atoi(ns);
      f->GetString(ns); v2=atoi(ns);
      f->GetString(ns); v3=atoi(ns);
//qdbg("CFACE %d\n",n);

      // Assumes indexed faces
      // Make sure the indexes of the texture vertices match
      // with the indices of the vertices themselves
      g->vcolor[n*3*3+0]=tempVColor[v1*3+0];
      g->vcolor[n*3*3+1]=tempVColor[v1*3+1];
      g->vcolor[n*3*3+2]=tempVColor[v1*3+2];
      g->vcolor[n*3*3+3]=tempVColor[v2*3+0];
      g->vcolor[n*3*3+4]=tempVColor[v2*3+1];
      g->vcolor[n*3*3+5]=tempVColor[v2*3+2];
      g->vcolor[n*3*3+6]=tempVColor[v3*3+0];
      g->vcolor[n*3*3+7]=tempVColor[v3*3+1];
      g->vcolor[n*3*3+8]=tempVColor[v3*3+2];
//if((n%10)==0)qdbg("cface %d: %d %d %d\n",n,v1,v2,v3);
    } else if(cmd=="*MATERIAL_REF")
    { f->GetString(ns); n=atoi(ns);
      g->materialRef=n;
    }
  }

  // End of importing this GEOMOBJECT
#ifdef OPT_DUMP_AFTER_IMPORT
 DumpArray("index",g->index,g->indices,3);
 DumpArray("vertex",g->vertex,g->vertices*N_PER_V,N_PER_V);
 DumpArray("face",g->face_v,g->face_v_elements,N_PER_F);
 DumpArray("face_nrm",g->face_nrm,g->face_nrm_elements,N_PER_F);
 DumpArray("tface",g->tface_v,g->tface_v_elements,N_PER_TV);
 DumpArray("tvertex",g->tvertex,g->tvertices*N_PER_TV,N_PER_TV);
 DumpArray("vcolor",g->vcolor,g->vcolors*3,3);
#endif
#ifdef OBS_DBG
  for(i=0;i<faces*N_PER_F;i++)
  { qdbg("face_elt%d: %f\n",i,face_v[i]);
  }
  for(i=0;i<tfaces*N_PER_TF&&i<100;i++)
  { qdbg("tface_v[%d]: %f\n",i,tface_v[i]);
  }
#endif
#ifdef OBS
  // Info bursts
qdbg("Bursts=%d\n",g->bursts);
  for(i=0;i<g->bursts;i++)
  { qdbg("Burst @%d, count %d, mtlid %d\n",g->burstStart[i],
      g->burstCount[i],g->burstMtlID[i]);
  }
#endif
  
  // Cleanup
  QFREE(tempTVertex);
  
  return TRUE;
}

/****************************
* Importing ASE file format *
****************************/
bool DGeodeImportASE(DGeode *g,cstring fname)
// A very flat importer for a subset of .ASE files
// Doesn't support multiple material depths
{
  DBG_C_FLAT("DGeodeImportASE")
  DBG_ARG_S(fname)
  
  QFile *f;
  qstring cmd,ns;
  int i,n;
  float x,y,z;
  float cr,cg,cb;
  int v1,v2,v3;
  DMaterial *curMat=0;             // Current material being edited
  int curMatIndex=0,curSubMatIndex;  // Tree-ing materials 1 deep
  int curFace;

  QTRACE_PRINT("DGeode::ImportASE\n");

  f=new QFile(fname);

  QTRACE_PRINT("  f=%p\n",f);
  // File was found?
  if(!f->IsOpen())
  {
qdbg("  !open\n");
    QTRACE_PRINT("Can't import ASE '%s'",fname);
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
      if(g->geobs==DGeode::MAX_GEOB)
      { qerr("Too many geomobjects in ASE, max=%d",DGeode::MAX_GEOB);
        break;
      }
      o=new DGeob(g);
      g->geob[g->geobs]=o;
qdbg("new Geob @%p\n",o);
      g->geobs++;
      r=DGeobImportASE(o,f);
      if(!r)goto fail;
    
    // Materials
    } else if(cmd=="*MATERIAL_COUNT")
    {
      f->GetString(ns); n=atoi(ns);
      if(n>DGeode::MAX_MATERIAL)
      {
        qwarn("ASE import; max #materials exceeded (%d, max %d)",
          n,DGeode::MAX_MATERIAL);
        n=DGeode::MAX_MATERIAL;
      }
      g->materials=n;

qdbg("%d materials\n",g->materials);
      for(i=0;i<n;i++)
      { g->material[i]=new DMaterial();
      }
    } else if(cmd=="*MATERIAL")
    {
      f->GetString(ns); n=atoi(ns);
      if(n>=g->materials)
      {
        qwarn("ASE import; material %d out of range (0..%d)",
          n,g->materials-1);
        n=g->materials-1;
      }
      curMatIndex=n;
      curMat=g->material[curMatIndex];
qdbg("  material %d (@%p)\n",n,curMat);
    } else if(cmd=="*NUMSUBMTLS")
    {
      f->GetString(ns); n=atoi(ns);
      // Check 'n'
      //...
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
      curMat=g->material[curMatIndex]->submaterial[curSubMatIndex];
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
    { f->GetString(ns); cr=atof(ns);
      f->GetString(ns); cg=atof(ns);
      f->GetString(ns); cb=atof(ns);
      if(curMat)
      {
        curMat->emission[0]=cr;
        curMat->emission[1]=cg;
        curMat->emission[2]=cb;
        curMat->emission[3]=1.0;
        //curMat->ambient[0]=r;
        //curMat->ambient[1]=g;
        //curMat->ambient[2]=b;
        //curMat->ambient[3]=1.0;
      }
    } else if(cmd=="*MATERIAL_DIFFUSE")
    { f->GetString(ns); cr=atof(ns);
      f->GetString(ns); cg=atof(ns);
      f->GetString(ns); cb=atof(ns);
      if(curMat)
      {
        curMat->diffuse[0]=cr;
        curMat->diffuse[1]=cg;
        curMat->diffuse[2]=cb;
        curMat->diffuse[3]=1.0;
      }
    } else if(cmd=="*MATERIAL_SPECULAR")
    { f->GetString(ns); cr=atof(ns);
      f->GetString(ns); cg=atof(ns);
      f->GetString(ns); cb=atof(ns);
      if(curMat)
      {
        curMat->specular[0]=cr;
        curMat->specular[1]=cg;
        curMat->specular[2]=cb;
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
      { curMat->diffuse[3]=1.0f-x;
        curMat->transparency=x;
      }
      // From dpoly.cpp, it really uses glColor4f(1,1,1,x) to
      // do transparency with GL_MODULATE texture env mode
      // So this might not be the way to do it.
    }
  }
  QDELETE(f);

//qdbg("geobs=%d\n",g->geobs);
  return TRUE;
 fail:
  QDELETE(f);
  return FALSE;
}

