/*
 * DGeode - 3DO format importing
 * 25-05-01: Created!
 * 04-11-01: Improved to add 0x821 polygons, bugfixes in geode creation.
 * NOTES:
 * - Based on GPL Paint's GPLTDO class which loads the .3do
 * (c) 2001 MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/gpltdo.h>
#include <qlib/app.h>
#include <d3/matrix.h>
#include <d3/geode.h>
DEBUG_ENABLE

// Indexed face sets? (always set this, 4-11-01)
#define USE_INDEXED_FACES

// 3DO has multiple levels of detail; define USE_LOWEST_DETAIL to
// always follow the lowest-detail tree. If not defined, the highest
// detail tree is followed (recommended; we want some quality).
//#define USE_LOWEST_DETAIL

// If 'USE_ADD_POLY' is not defined, no polygons will be generated
// This can cut down on debug output to get a closer look.
#define USE_ADD_POLY

// If 'USE_GEOB_DUMP' is defined, every geob will be printed once created.
//#define USE_GEOB_DUMP

// Depth of texture name stack
#define MAX_DEP    100
// #vertex data elements in the arrays
#define MAX_UNITS  1000

// Print out tree on debug output?
#define VERBOSE    1

// The model
static GPLTDO *tdo;

// Texture stack (the .3do file is a tree)
struct StackEntry
{
  // What texture?
  qstring texName;
  dindex  *index;
  dfloat  *vertex;
  dfloat  *tvertex,*normal;
  int      indices,vertices,normals,tvertices;
  // Allocated units for each of the arrays
  int      allocatedUnits;
};
static StackEntry stackEntry[MAX_DEP];
static int        stackDepth;
// Primitives
static long      *prim;
static DGeode    *geode;

// Import from dgeode_vrml.cpp
dindex *DGV_TriangulateArray(dindex *a,int n,int *pCount);

/********
* Stack *
********/
static void StackPush(cstring texName)
{
  StackEntry *se;
  char buf[256];
  
qdbg("StackPush(%s)\n",texName);
  if(stackDepth==MAX_DEP)
  {
    qwarn("Texture name stack overflow");
    return;
  }
  se=&stackEntry[stackDepth];
//qdbg("  se=%p, stackDepth=%d\n",se,stackDepth);
  // Allocate space to hold vertex data
  int n=MAX_UNITS;
//qdbg("1\n");
  se->allocatedUnits=n;
//qdbg("2\n");
#ifdef OBS
  se->index=0;
qdbg("2.1\n");
#endif
  se->index=(dindex*)qcalloc(n*sizeof(dindex));
//qdbg("3\n");
  // Assume .BMP files
  sprintf(buf,"images/%s.bmp",texName);
  se->texName=buf;
  se->vertex=(dfloat*)qcalloc(n*sizeof(dfloat));
  se->tvertex=(dfloat*)qcalloc(n*sizeof(dfloat));
  se->normal=(dfloat*)qcalloc(n*sizeof(dfloat));
  se->indices=se->vertices=se->normals=se->tvertices=0;
  stackDepth++;
//qdbg("  index=%p, vertex=%p, tv=%p, normal=%p\n",se->index,se->vertex,
//se->tvertex,se->normal);
//qdbg("  stack entry set up\n");
}
static void DumpGeob(DGeob *g)
{
  dfloat *v,*tv;
  dindex *index;
  int i,n,in;
  
  v=g->vertex;
  tv=g->tvertex;
  index=g->index;
  n=g->indices;
  for(i=0;i<n;i++)
  {
    in=index[i];
    qdbg("index[%d]=%d\n",i,in);
    v=&g->vertex[in*3];
    qdbg("  vertex=(%f,%f,%f)\n",v[0],v[1],v[2]);
    tv=&g->tvertex[in*2];
    qdbg("  tvertex=(%f,%f)\n",tv[0],tv[1]);
  }
}

static void StackPop()
{
  StackEntry *se;
qdbg("StackPop()\n");
  if(stackDepth==0)
  {
    qwarn("Texture name stack underflow");
    return;
  }
  stackDepth--;
  se=&stackEntry[stackDepth];
qdbg("  index=%p, vertex=%p, tv=%p, normal=%p\n",se->index,se->vertex,
se->tvertex,se->normal);
  // Create a geob from the entry
  DGeob *g;
  g=new DGeob(geode);
  g->bursts=1;
  g->burstStart[0]=0;
  g->burstCount[0]=se->indices*3;
  g->burstMtlID[0]=0;
  g->burstVperP[0]=3;
  g->vertex=se->vertex;
  g->vertices=se->vertices/3;
  g->index=se->index;
  g->indices=se->indices;
  g->tvertex=se->tvertex;
  g->tvertices=se->vertices/3;
  // No normals in the .3do file
  g->normal=0;
  g->normals=0;
  // Material
  g->materialRef=geode->materials;
  geode->geob[geode->geobs]=g;
  geode->geobs++;

#ifdef USE_GEOB_DUMP
  DumpGeob(g);
#endif

  // Add a material for this geob
  DMaterial *m;
  m=new DMaterial();
  m->AddBitMap(se->texName);
  geode->material[geode->materials]=m;
  geode->materials++;

#ifdef OBS_BUG
  QDELETE(se->normal);
  QDELETE(se->tvertex);
  QDELETE(se->vertex);
  QDELETE(se->index);
#endif
}
static int StackGetDepth()
{
  return stackDepth;
}
static StackEntry *StackGetTop()
// Returns top element
{
  if(stackDepth==0)return 0;
  return &stackEntry[stackDepth-1];
}

/*****************
* Primitive node *
*****************/
static cstring GetString(long offset)
{
  return &tdo->strnData[offset];
}
static void SetColor(long rgba)
{
  float r,g,b,a;
  r=(rgba&0xFF)/255.;
  g=((rgba&0xFF00)>>8)/255.;
  b=((rgba&0xFF0000)>>16)/255.;
  a=1.0;
  
r=r+g+b+a;
  //qdbg("%x color => %f %f %f\n",rgba&0xFFFFFF,r,g,b);
  //glColor4f(r,g,b,a);
}
static void AddPoly(int vertices,long *vertexNumber,long *texPair)
// Triangulate a polygon and add to the model
// If 'texPair' is 0, no texture is used.
{
  int i;
  StackEntry *se=StackGetTop();
  GPL_XYZ *xyz;
  dindex  srcIndex[200];        // Fat polygons were found in wheels!
  float  *tex;
 
  if(!se)
  {
    qwarn("AddPoly(%d) without texture stack present",vertices);
    // Get a texture
    StackPush("predef0");
    se=StackGetTop();
    //return;
  }

#ifndef USE_ADD_POLY
  return;
#endif
  
//qdbg("AddPoly(%d)\n",vertices);

  tex=(float*)texPair;
  for(i=0;i<vertices;i++)
  {
    // XYZ coordinates; note the coordinate juggling
    xyz=&tdo->xyz[vertexNumber[i]];
//qdbg("v[%d]=(%f,%f,%f)\n",i,xyz->x,xyz->y,xyz->z);
//qdbg("  t[%d]=(%f,%f)\n",i,tex[i*2],tex[i*2+1]);
    se->vertex[se->vertices+0]=xyz->x;
    se->vertex[se->vertices+1]=xyz->z;
    se->vertex[se->vertices+2]=-xyz->y;
    if(tex)
    {
      // Texture coordinates; note that Y is reversed (!)
      se->tvertex[(se->vertices/3)*2+0]=tex[i*2+0];
      se->tvertex[(se->vertices/3)*2+1]=1.0f-tex[i*2+1];
    }
    // Prepare index array for triangulisation
    srcIndex[i]=se->vertices/3;
    //se->index[se->indices]=se->vertices/3;
    se->vertices+=3;
    //se->indices++;
  }
  // Create tirangulated indices
  dindex *triIndices;
  int     count;
  triIndices=DGV_TriangulateArray(srcIndex,vertices,&count);
//qdbg("v's=%d => tri'd=%d, triIndices=%p\n",vertices,count,triIndices);
  // Add to index array
  for(i=0;i<count;i++)
  {
    se->index[se->indices]=triIndices[i];
//qdbg("  index[%d]=%d\n",se->indices,triIndices[i]);
    se->indices++;
  }
qdbg("se->vertices=%d\n",se->vertices);
  if(se->vertices>=MAX_UNITS)
  {
    qerr("Vertex overflow; crash may be the result");
  }
  QFREE(triIndices);
}

static void PaintPrim(long root,int dep)
// Recursive tree painter
{
  long type,subType;
  long off,offset[6];
  long plan[6];
  long v[8],vs;
  int  i,n;
  char buf[256];

  if(root==-1)return;
  root/=4;
  type=prim[root];
  if(VERBOSE)
  { qdbg("%*s",dep*1,"");
    qdbg("0x%5x: type 0x%x",root*4,type);
  }
  switch(type)
  { case 0x4:
      // Multi-child node
      n=prim[root+1];
      if(VERBOSE)qdbg(" multichild (%d children)\n",n);
      for(i=0;i<n;i++)
        PaintPrim(prim[root+i+2],dep+1);
      break;
    case 0x5:
      // Texture selection; has many subtypes
      subType=prim[root+2];
      off=prim[root+1];
      if(VERBOSE)qdbg(" subType 0x%x, offset 0x%x (texture select)\n",
        subType,off);
      if(prim[root+2]==5)
      {
        // Texture select
        if(VERBOSE)qdbg("Texture '%s'\n",GetString(prim[root+3]));
        StackPush(GetString(prim[root+3]));
      } else if(prim[root+2]==0x8)
      {
        if(VERBOSE)qdbg("T-5/8 int[2]=%x,%x,%x float=%f\n",
          prim[root+3],prim[root+4],prim[root+5]);
      } else if(prim[root+2]==0xC)
      {
        if(VERBOSE)qdbg("T-5/C int[3]=%x,%x,%x float=%f\n",
          prim[root+3],prim[root+4],prim[root+5],prim[root+6]);
      } else if(prim[root+2]==0x20)
      {
        // Select predefined texture; only found in wheels it seems
        // 5, offset, 32, int
        // 'int' -> 0 and 1 = tire wall, 2 = tire tread
        if(VERBOSE)qdbg("Predef'd texture %d\n",prim[root+3]);
        // Although the texture is not really found, push a representative
        // name.
        sprintf(buf,"predef%d",prim[root+3]);
        StackPush(buf);
      }
      PaintPrim(off,dep+1);
      if(prim[root+2]==5||prim[root+2]==0x20)
      //if(prim[root+2]==5)
      {
        // Pop the texture and write out a geob
        StackPop();
      }
      break;
    case 0x6:
      // 1 child, if ABOVE plane
      plan[0]=prim[root+1];
      if(VERBOSE)qdbg("  child (if above) with plane %d\n",plan[0]);
      PaintPrim(prim[root+2],dep+1);
      break;
    case 0x8:
      // Bi-directional plane offsets
      // <plane> <offset1-4>
      PaintPrim(prim[root+2],dep+1);
      PaintPrim(prim[root+3],dep+1);
      PaintPrim(prim[root+4],dep+1);
      PaintPrim(prim[root+5],dep+1);
      break;
    case 0x9:
      // Three child with plane
      plan[0]=prim[root+1];
      if(VERBOSE)qdbg("  3child with plane %d\n",plan[0]);
      PaintPrim(prim[root+2],dep+1);
      PaintPrim(prim[root+3],dep+1);
      PaintPrim(prim[root+4],dep+1);
      break;
    case 0xA:
      // Three child with plane (the same as T-9)
      plan[0]=prim[root+1];
      if(VERBOSE)qdbg("  3child with plane %d\n",plan[0]);
      PaintPrim(prim[root+2],dep+1);
      PaintPrim(prim[root+3],dep+1);
      PaintPrim(prim[root+4],dep+1);
      break;
    case 0xB:
      // Plane with 2 offsets
      plan[0]=prim[root+1];
      offset[0]=prim[root+2];
      offset[1]=prim[root+3];
      if(VERBOSE)qdbg(" plane %d, offset 0x%x-0x%x\n",
        plan[0],offset[0],offset[1]);
      PaintPrim(offset[0],dep+1);
      PaintPrim(offset[1],dep+1);
      break;
    case 0xD:
      // Transform child node (much like VRML)
      // <dx> <dy> <dz> <rx> <ry> <rz> <scale> <offset>
      // Follow the primitive
      float pos[3],rot[3];
      for(i=0;i<3;i++)
      {
        pos[i]=*(float*)(&prim[root+1+i]);
        rot[i]=*(float*)(&prim[root+4+i]);
      }
      if(VERBOSE)qdbg("  transform pos %f,%f,%f rot %f,%f,%f",
        pos[0],pos[1],pos[2],rot[0],rot[1],rot[2]);
      PaintPrim(prim[root+8],dep+1);
      break;
    case 0xF:
      // A volume with enclodes 3DO's
      break;
    case 0x10:
      // Number array; nothing to paint
      break;
    case 0x11:
      // Distance switch; n1, vertex, n3, ps, ps*(float dist,long offset)
      // If n1==0, the 'ps' (pairs) gives the number of dist/offset
      // pairs, depending on the distance of the viewer to 'vertex'
      // (which is an index ofcourse)
      // Follow last offset, which has the most detail
      n=prim[root+4];
      if(VERBOSE)qdbg(" distance switch, %d entries\n",n);
#ifdef USE_LOWEST_DETAIL
      // Use furthest tree
      PaintPrim(prim[root+6],dep+1);
#else
      // Follow tree with most detail; last array offset
      PaintPrim(prim[root+6+(n-1)*2],dep+1);
#endif
      break;
    case 0x19:
      // Room; 8 vertices defining a cube, and an offset
      for(i=0;i<8;i++)
        v[i]=prim[root+i+1];
      offset[0]=prim[root+9];
      if(VERBOSE)qdbg("  room, offset 0x%x\n",offset[0]);
      if(VERBOSE)qdbg("v=%d %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],
        v[4],v[5],v[6],v[7]);
#ifdef PAINT_ROOMS
      glColor4f(1,1,1,.1);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      glBegin(GL_QUADS);
        PaintVertex(&xyz[v[0]]);
        PaintVertex(&xyz[v[1]]);
        PaintVertex(&xyz[v[2]]);
        PaintVertex(&xyz[v[3]]);
        
        PaintVertex(&xyz[v[4]]);
        PaintVertex(&xyz[v[5]]);
        PaintVertex(&xyz[v[6]]);
        PaintVertex(&xyz[v[7]]);
        
        PaintVertex(&xyz[v[0]]);
        PaintVertex(&xyz[v[1]]);
        PaintVertex(&xyz[v[5]]);
        PaintVertex(&xyz[v[4]]);
        
        PaintVertex(&xyz[v[2]]);
        PaintVertex(&xyz[v[3]]);
        PaintVertex(&xyz[v[7]]);
        PaintVertex(&xyz[v[6]]);
        
        PaintVertex(&xyz[v[0]]);
        PaintVertex(&xyz[v[3]]);
        PaintVertex(&xyz[v[7]]);
        PaintVertex(&xyz[v[4]]);
        
        PaintVertex(&xyz[v[1]]);
        PaintVertex(&xyz[v[2]]);
        PaintVertex(&xyz[v[6]]);
        PaintVertex(&xyz[v[5]]);
      glEnd();
      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
#endif
      PaintPrim(offset[0],dep+1);
      break;
    // Actual drawing primitives
    case 0x815:
      // Line
      SetColor(prim[root+1]);
#ifdef OBS
      glBegin(GL_LINES);
        PaintVertex(&xyz[prim[root+2]]);
        PaintVertex(&xyz[prim[root+3]]);
      glEnd();
#endif
      break;
    case 0x81B:
      // Polygon, flat shaded
      SetColor(prim[root+1]);
      vs=prim[root+2];
      if(VERBOSE)qdbg(" flat poly, color 0x%x, %d vertices\n",prim[root+1],vs);
      AddPoly(vs,&prim[root+3],0);
#ifdef OBS
      qwarn("0x81B polygon with unsupported #vertices (%d)\n",vs);
#endif
      break;
    case 0x81C:
      // Polygon, gourad, not textured
      // <col> <nvertices> <vertices> <vert-colors>
      SetColor(prim[root+1]);
      vs=prim[root+2];
      if(VERBOSE)qdbg(" textured poly, color 0x%x, %d vertices\n",
        prim[root+2],vs);
      if(vs==4)
      {
        qwarn("0x81C polygon with unsupported #vertices (%d)\n",vs);
#ifdef OBS
        glBegin(GL_QUADS);
          PaintVertex(&xyz[prim[root+3]]);
          PaintVertex(&xyz[prim[root+4]]);
          PaintVertex(&xyz[prim[root+5]]);
          PaintVertex(&xyz[prim[root+6]]);
        glEnd();
#endif
      } else if(vs==5)
      { // Known problem!
        qwarn("0x81C polygon with unsupported #vertices (%d)\n",vs);
      } else
      { qwarn("0x81C polygon with unsupported #vertices (%d)\n",vs);
      }
      break;
    case 0x81F:
      // Polygon, textured, otherwise flat
      // 0, col, #v's, (x,y) pairings, vertices
      SetColor(prim[root+2]);
      vs=prim[root+3];
      if(VERBOSE)qdbg(" textured poly, color 0x%x, %d vertices\n",
        prim[root+2],vs);
      if(vs==4)
      {
        AddPoly(4,&prim[root+2*4+4],&prim[root+4]);
#ifdef OBS
        glBegin(GL_QUADS);
          PaintVertex(&xyz[prim[root+2*4+4]]);
          PaintVertex(&xyz[prim[root+2*4+5]]);
          PaintVertex(&xyz[prim[root+2*4+6]]);
          PaintVertex(&xyz[prim[root+2*4+7]]);
        glEnd();
#endif
      } else if(vs==5)
      { 
#ifdef OBS
        glBegin(GL_POLYGON);
          PaintVertex(&xyz[prim[root+2*5+4]]);
          PaintVertex(&xyz[prim[root+2*5+5]]);
          PaintVertex(&xyz[prim[root+2*5+6]]);
          PaintVertex(&xyz[prim[root+2*5+7]]);
          PaintVertex(&xyz[prim[root+2*5+8]]);
        glEnd();
#endif
      } else
      { qdbg("0x81F polygon with unsupported #vertices (%d)\n",vs);
      }
      break;
    case 0x820:
      // Polygon, textured, otherwise Gouraud
      // 0, col, #v's, (x,y) pairings, vertices, vcols
      SetColor(prim[root+2]);
      vs=prim[root+3];
      if(VERBOSE)qdbg(" textured poly, color 0x%x, %d vertices\n",
        prim[root+2],vs);
      if(vs==4)
      { 
#ifdef OBS
        glBegin(GL_QUADS);
          SetColor(prim[root+2*4+4+0]);
          PaintVertex(&xyz[prim[root+2*4+4]]);
          SetColor(prim[root+2*4+4+1]);
          PaintVertex(&xyz[prim[root+2*4+5]]);
          SetColor(prim[root+2*4+4+2]);
          PaintVertex(&xyz[prim[root+2*4+6]]);
          SetColor(prim[root+2*4+4+3]);
          PaintVertex(&xyz[prim[root+2*4+7]]);
        glEnd();
#endif
      } else if(vs==3)
      { 
#ifdef OBS
        glBegin(GL_TRIANGLES);
          SetColor(prim[root+2*3+3+0]);
          PaintVertex(&xyz[prim[root+2*3+4]]);
          SetColor(prim[root+2*3+3+1]);
          PaintVertex(&xyz[prim[root+2*3+5]]);
          SetColor(prim[root+2*3+3+2]);
          PaintVertex(&xyz[prim[root+2*3+6]]);
        glEnd();
#endif
      } else if(vs>4)
      { // Known problem!
        qwarn("0x820 polygon with unsupported #vertices (%d)\n",vs);
      } else
      { qwarn("0x820 polygon with unsupported #vertices (%d)\n",vs);
      }
      break;
    case 0x821:
      // Polygon, textured, specular, flat shading (?)
      // 0, col, #v's, (x,y) pairings, vertices, vertex normals
      SetColor(prim[root+2]);
      vs=prim[root+3];
      if(VERBOSE)qdbg(" textured/specular poly, color 0x%x, %d vertices\n",
        prim[root+2],vs);
      // Normals at &prim[root+4+2*vs+vs]
      AddPoly(vs,&prim[root+2*vs+4],&prim[root+4]);
#ifdef OBS_ONE_SIZE_FITS_ALL
      if(vs==4)
      { 
        // Normals at &prim[root+4+2*vs+vs]
        AddPoly(4,&prim[root+2*vs+4],&prim[root+4]);
      } else if(vs==3)
      { 
        qwarn("0x821 polygon with unsupported #vertices (%d)\n",vs);
#ifdef OBS
        glBegin(GL_TRIANGLES);
          SetColor(prim[root+2*3+3+0]);
          PaintVertex(&xyz[prim[root+2*3+4]]);
          SetColor(prim[root+2*3+3+1]);
          PaintVertex(&xyz[prim[root+2*3+5]]);
          SetColor(prim[root+2*3+3+2]);
          PaintVertex(&xyz[prim[root+2*3+6]]);
        glEnd();
#endif
      } else if(vs==6)
      { // Normals at &prim[root+4+2*vs+vs]
        AddPoly(vs,&prim[root+2*vs+4],&prim[root+4]);
      } else if(vs>4)
      { // Known problem! Still warn.
        qwarn("0x821 polygon with unsupported #vertices (%d)\n",vs);
      } else
      { qwarn("0x821 polygon with unsupported #vertices (%d)\n",vs);
      }
#endif
      break;
    default:
      //if(VERBOSE)
      qdbg(" (nyi t=0x%x)\n",type);
      break;
  }
}

/*****************
* Primtives root *
*****************/
static void ConvertPrimitives()
{
  // Root
  long root;
  root=*prim;
  if(VERBOSE)printf("Primitive root @ 0x%x\n",root);
  PaintPrim(root,0);
  if(VERBOSE)printf("--------------------\n");
}

/***********
* High end *
***********/
bool DGeodeImport3DO(DGeode *g,cstring fname)
// Imports a (Grand Prix Legends) .3do file
// Robustness is still to be decided; note that a .3do file is a
// tree, and a .3do file is NOT, so efficiency will not be terrific.
{
  DBG_C_FLAT("DGeodeImport3DO")
  DBG_ARG_S(fname)

  // Attempt to read the object
  tdo=new GPLTDO(fname);
  if(!tdo->fp)
  {
qdbg("Can't load '%s'\n",fname);
   fail:
    QDELETE(tdo);
    return FALSE;
  }
  if(!tdo->Read())
    goto fail;
  
  // Global var for easy access
  prim=tdo->prim;
  geode=g;
  
  // Init texture stack
  stackDepth=0;
  ConvertPrimitives();
  
  QDELETE(tdo);
  return TRUE;
}

