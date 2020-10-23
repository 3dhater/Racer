/*
 * DGeode support - importing VRML (3D Studio Max, MASpuce,...)
 * 26-12-00: Detached from dgeob.cpp/dgeode.cpp.
 * NOTES:
 * - VRML uses quite a flexible input format. All data here
 * is read much simplier, but this means things have to be optimized
 * to not waste graphics power when drawing. Export to DOF after that.
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/app.h>
#ifdef linux
#include <stdlib.h>
#endif
#include <d3/matrix.h>
#include <d3/geode.h>
DEBUG_ENABLE

// Indexed face sets instead of flat big arrays?
#define USE_INDEXED_FACES

#ifdef OBS
#define N_PER_V     3        // Coordinates per vertex
#define N_PER_F     9        // Coordinates per face (triangles)

#define N_PER_TV    2        // Coordinates used per texture vertex (2D maps)
#define N_PER_TF    6        // Tex Coordinates per tex face
#endif

#undef  DBG_CLASS
#define DBG_CLASS "DGeobVRML_flat"

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

/*************
* Tokenizing *
*************/
static void MyGetString(QFile *f,qstring &s)
// Tokenizer; separates word based on ',' and whitespace
{
  char buf[256],*d=buf,c;
  int  i;

  f->SkipWhiteSpace();
  for(i=0;i<sizeof(buf)-1;i++)
  {
    c=f->GetChar();
	if(f->IsEOF())break;
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
//qdbg("GetVRMLToken(f=%p,s=%p\n",f,&s);
  MyGetString(f,s);
  if(s=="DEF")
  {
    // Skip name creation
    f->GetString(s);
    goto retry;
  }
//qdbg("  GetVRMLToken ret\n");
}
static int CountCommas(QFile *f,bool fVerbose=FALSE)
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
//if(c!=' '&&c!=10&&c!=13)if(fVerbose)qdbg("char '%c'\n",c);
    if(f->IsEOF())
    {
      qdbg("EOF\n");
      break;
    }
    if(c==',')
    { n++;
      // Was it an unnecessary comma at the end?
      MyGetString(f,s);
      if(s=="]")
      {
//qdbg("end by '%s'\n",(cstring)s);
        // Indeed, the comma was last in the list, don't count it
        // as a vertex
        n--;
        break;
      } //else if(fVerbose)qdbg("Non-ending string '%s'\n",(cstring)s);
    }
    if(c==']')
    {
      //qdbg("ending ]\n");
      break;
    }
  }
  f->Seek(offset);
  return n;
}
#ifdef OBS
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
#endif
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

/***********************
* Index array handling *
***********************/
static int CalcBurstSize(dindex *a,int offset,int n)
// Returns size of burst at offset 'offset'
// a=array[n] of dindex
{
  int i;
  for(i=offset;i<n;i++)
  {
//qdbg("a[%d]=0x%x\n",i,a[i]);
    if(a[i]==0xFFFF)break;
  }
  return i-offset;
}
dindex *DGV_TriangulateArray(dindex *a,int n,int *pCount)
// Create triangulated index array from 'a' (size n).
// 'a' contains separators (-1) to indicate polygon ends.
// Returns #created indices in '*pCount'
// All data is converted into triangles.
{
  int i,j,curIndex;
  int len;
  dindex *d;
  
  // Allocate destination array, get enough space
//qdbg("DGV_TriangulateArray(); n=%d\n",n);
  d=(dindex*)qcalloc(sizeof(dindex)*n*40);
qdbg("DGV_Triangulate: allocating %d\n",n*4);
  curIndex=0;
  
  for(i=0;i<n;i++)
  {
    // Get #indices for this polygon
    len=CalcBurstSize(a,i,n);
//qdbg("Index %d, len %d\n",i,len);
    if(len==3)
    {
      // Direct copy
      for(j=0;j<3;j++)
      {
        d[curIndex]=a[i+j];
        curIndex++;
      }
    } else if(len>=4)
    {
      // Generic algorithm to triangle an n-gon
      dindex *dp=&d[curIndex];
      for(j=0;j<len-2;j++)
      {
        *dp++=a[i+0]; *dp++=a[i+j+1]; *dp++=a[i+j+2];
      }
      curIndex+=(len-2)*3;
    } else
    { qwarn("DGeodImportVRML; non-3/4/6/8/16-sided polygon found (%d sides)",
        len);
    }
    i+=len;
  }
qdbg("DGV_Triangulate: used %d\n",curIndex);
  // Return size in 'pCount'
  *pCount=curIndex;
  // Return array
  return d;
}
static dindex *ReadIndices(QFile *f,int *pCount)
// Read a load of index data, and returns an array
{
  int i,n;
  qstring ns;
  dindex *index;
  char buf[256];
  
//f->Read(buf,80); buf[80]=0; qdbg("ReadIndices at '%s'\n",buf);
  n=CountCommas(f /*,TRUE*/);
//qdbg("Commas: %d\n",n);

  index=(dindex*)qcalloc(sizeof(dindex)*n);
  // Skip '['
  MyGetString(f,ns);
  // Read all indices
  for(i=0;i<n;i++)
  {
    MyGetString(f,ns);
//qdbg("ns='%s'\n",(cstring)ns);
    // End of index array?
    if(ns=="]")break;
    index[i]=atoi(ns);
    MyGetString(f,ns);
    // Safety
    if(f->IsEOF())break;
  }
//qdbg("ReadIndices; count=%d\n",n);
  *pCount=n;
  return index;
}

bool DGeobImportVRML(DGeob *g,QFile *f)
// Read a geometry node from the VRML file
// Assumes the pointer is at the opening { of the object
// Start by loading the actual data, then, to get an indexed primitive,
// it expands the array to accomodate sets of vertex-normal-texcoord.
// As it probably has too many members by then, it optimizes
// as a last step. Use the DOF format for much faster loading.
// This function should be separated into dgeob_vrml.cpp (flat).
{
  DBG_C_FLAT("DGeobImportVRML")
  
  qstring cmd,ns;
  int     i,j,k;
  int     n,lastOffset;
  float   x,y,z;
  float   cr,cg,cb;
  int     v1,v2,v3;
  int     v[16];          	// Large polygons are possible
  float  *ptrVertex=0;
  DMaterial *curMat=0;                    // Current material being edited
  int        curMatIndex,curSubMatIndex;  // Tree-ing materials 1 deep
  // Temporary loading
  dfloat *tempNormal=0;
  dfloat *tempTVertex=0;
  dfloat *tempVertex=0;
  int     tempVertices,tempNormals,tempTVertices;
  dindex *tempVertexIndex=0,*tempNormalIndex=0,*tempTexIndex=0;
  int     curIndex,lastIndex;
  
//qdbg("DGeobImportVRML()\n");
  
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
    //
    if(cmd=="coord")
    {
      int n,off;
      
      // coord [DEF NAME] Coordinate { point [ ... ] }
      // Pre-count upcoming #coordinates
      n=CountCommas(f);
      tempVertices=n;
      tempVertex=(float*)qcalloc(sizeof(float)*tempVertices*3);
      if(!tempVertex)goto nomem;
//qdbg("coord; %d coordinates\n",tempVertices);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vertices
      for(n=0;n<tempVertices;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        MyGetString(f,ns); z=atof(ns);
        tempVertex[n*3+0]=x;
        tempVertex[n*3+1]=y;
        tempVertex[n*3+2]=z;
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
      tempNormals=n;
      tempNormal=(float*)qcalloc(sizeof(float)*tempNormals*3);
      if(!tempNormal)goto nomem;
//qdbg("normal; %d vectors\n",tempNormals);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vectors
      for(n=0;n<tempNormals;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        MyGetString(f,ns); z=atof(ns);
        tempNormal[n*3+0]=x;
        tempNormal[n*3+1]=y;
        tempNormal[n*3+2]=z;
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
      tempTVertices=n;
      tempTVertex=(float*)qcalloc(sizeof(float)*tempTVertices*2);
      if(!tempTVertices)goto nomem;
//qdbg("texCoord; %d coords\n",tempTVertices);
      // Find opening bracket
      if(!ReadUntilToken(f,"["))goto fail;
      // Read vectors
      for(n=0;n<tempTVertices;n++)
      {
        MyGetString(f,ns); x=atof(ns);
        MyGetString(f,ns); y=atof(ns);
        tempTVertex[n*2+0]=x;
        tempTVertex[n*2+1]=y;
//qdbg("  tvertex(%f,%f)\n",x,y);
        // Skip comma
        MyGetString(f,ns);
      }

    // Actual indexed coordinate data
    //
    } else if(cmd=="coordIndex")
    {
      dindex *a,*ao;
      dfloat *pVertex;
      int n,no,v;
      
      // Format: coordIndex [ 0,1,2,-1,... ]
      a=ReadIndices(f,&n);
      ao=DGV_TriangulateArray(a,n,&no);
//DumpArray("coordIndex",ao,no,3);
      QFREE(a);
      
      // Create index array
      g->indices=no;
//qdbg("indices: %d\n",g->indices);
      //g->index=(dindex*)qcalloc(sizeof(dindex)*g->indices);
      g->index=ao;
      // Create non-optimized arrays for upcoming vertices, normals etc.
      // Although this leads to inefficient storage NOW, it is flexible,
      // and an optimization pass is done later for efficiency.
      g->vertices=g->indices;
      g->normals=g->indices;
      g->tvertices=g->indices;
      g->vertex=(float*)qcalloc(sizeof(float)*g->indices*3);
      g->normal=(float*)qcalloc(sizeof(float)*g->indices*3);
      g->tvertex=(float*)qcalloc(sizeof(float)*g->indices*2);
      if(g->vertex==0||g->normal==0||g->tvertex==0)
      {nomem:
        qerr("DGeob:ImportVRML; not enough memory");
        goto fail;
      }
      
      // Copy vertices in
      pVertex=g->vertex;
      for(i=0;i<g->indices;i++)
      {
        v=ao[i];
        for(j=0;j<3;j++)
          *pVertex++=tempVertex[v*3+j];
      }
      // Flatten all vertices (a later optimization will
      // pack matching vertices again)
      for(i=0;i<g->indices;i++)
        g->index[i]=i;
        
      // Always triangular data
      g->burstVperP[0]=3;
      
    } else if(cmd=="normalIndex")
    {
      dindex *a,*ao;
      dfloat *pVertex;
      int n,no,v;
      
      // Format: normalIndex [ 0,1,2,-1,... ]
      // Assumes normalPerVertex==TRUE
      a=ReadIndices(f,&n);
      ao=DGV_TriangulateArray(a,n,&no);
      QFREE(a);
      
      // Copy normals in
      pVertex=g->normal;
      for(i=0;i<no;i++)
      {
        v=ao[i];
        for(j=0;j<3;j++)
          *pVertex++=tempNormal[v*3+j];
      }
      QFREE(ao);
      
    } else if(cmd=="texCoordIndex")
    {
      dindex *a,*ao;
      dfloat *pVertex;
      int n,no,v;
      
      // Format: texCoordIndex [ 0,1,2,-1,... ]
      //
      a=ReadIndices(f,&n);
      ao=DGV_TriangulateArray(a,n,&no);
      QFREE(a);
      
      if(no!=g->indices)
      { qerr("DGeob:ImportVRML; #texcoords (%d)!= #indices (%d)",n,g->indices);
        goto fail;
      }
//qdbg("texCoordIndex; %d indices\n",no);

      // Copy texcoords in
      pVertex=g->tvertex;
      for(i=0;i<no;i++)
      {
        v=ao[i];
        for(j=0;j<2;j++)
          *pVertex++=tempTVertex[v*2+j];
      }
      QFREE(ao);
      
    }
  }
  
  // VRML geometry nodes have only 1 material burst
  g->bursts=1;
  g->burstStart[0]=0;
  g->burstCount[0]=g->indices*3;
  g->burstMtlID[0]=-1;
  
  // Info bursts
#ifdef OBS
qdbg("Bursts=%d\n",g->bursts);
  for(i=0;i<g->bursts;i++)
  { qdbg("Burst @%d, count %d, mtlid %d\n",g->burstStart[i],
      g->burstCount[i],g->burstMtlID[i]);
  }
#endif

#ifdef OBS
 DumpArray("tempVertex",tempVertex,tempVertices*3,3);
 DumpArray("tempNormal",tempNormal,tempNormals*3,3);
 DumpArray("tempTVertex",tempTVertex,tempTVertices*2,2);
 DumpArray("vertex",g->vertex,g->vertices*3,3);
 DumpArray("normal",g->normal,g->normals*3,3);
 DumpArray("tvertex",g->tvertex,g->tvertices*2,2);
 DumpArray("index",g->index,g->indices,1);
#endif

  // Cleanup
  QFREE(tempVertex);
  QFREE(tempNormal);
  QFREE(tempTVertex);
  
  // Material ID is set by parent reader
  return TRUE;
  
 fail:
  // Cleanup
  QFREE(tempVertex);
  QFREE(tempNormal);
  QFREE(tempTVertex);
  return FALSE;
}

/**************
* VRML IMPORT *
**************/
bool DGeodeImportVRML(DGeode *g,cstring fname)
// A very flat importer for a subset of .VRML files
// Files created from .VRL (SCGT) mainly.
{
  DBG_C_FLAT("DGeodeImportVRML")
  
  QFile *f;
  qstring cmd,ns;
  int i;
  float x,y,z;
  float cr,cg,cb;
  int v1,v2,v3;
  DMaterial *curMat=0;             // Current material being edited
  int curFace;
  
qdbg("ImportVRML(%s)\n",fname);
  f=new QFile(fname);

  // File was found?
  if(!f->IsOpen())
  { QTRACE_PRINT("Can't import VRML '%s'",fname);
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
      if(g->geobs==DGeode::MAX_GEOB)
      { qerr("Too many shapes in VRML, max=%d",DGeode::MAX_GEOB);
        break;
      }
      o=new DGeob(g);
      g->geob[g->geobs]=o;
      g->geobs++;
      r=DGeobImportVRML(o,f);
      if(!r)goto fail;
      
      // Only 1 material per VRML geometry node
      // Other burst vars were set in ImportVRML() before
      o->burstMtlID[0]=g->materials-1;
      o->materialRef=g->materials-1;
      
    // Materials
    //
    } else if(cmd=="material")
    {
      // Skip name
      f->GetString(ns);
      
      if(g->materials==DGeode::MAX_MATERIAL)
      { qerr("Too many materials in VRML, max=%d",DGeode::MAX_MATERIAL);
        break;
      }
      g->material[g->materials]=new DMaterial();
      curMat=g->material[g->materials];
      g->materials++;

    // Material properties
    //
    } else if(cmd=="url")
    {
      // Assumes url is in a 'texture' node
      char buf[256];
      
      f->GetString(ns);
      g->FindMapFile(ns,buf);
#ifdef OBS
      // Go from PC full path to attempted SGI path
      sprintf(buf,"images/%s",QFileBase(ns));
#endif
//qdbg("  material image %s (base %s)\n",buf,(cstring)ns);
      curMat->AddBitMap(buf);
    } else if(cmd=="ambientIntensity")
    { // VRML uses 1 float (gray)
      f->GetString(ns);
      cr=cg=cb=atof(ns);
//r=g=b=1.0;
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
    } else if(cmd=="diffuseColor")
    { f->GetString(ns); cr=atof(ns);
      f->GetString(ns); cg=atof(ns);
      f->GetString(ns); cb=atof(ns);
//r=g=b=1.0;
      if(curMat)
      {
        curMat->diffuse[0]=cr;
        curMat->diffuse[1]=cg;
        curMat->diffuse[2]=cb;
        curMat->diffuse[3]=1.0;
      }
    } else if(cmd=="specularColor")
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
  QDELETE(f);
  
//qdbg("geobs=%d\n",g->geobs);
#ifdef OBS
{DBox b;g->GetBoundingBox(&b);
qdbg("DGeode_VRML: bv radius=%f\n",b.GetRadius());}
#endif
  return TRUE;
 fail:
  QDELETE(f);
  return FALSE;
}

