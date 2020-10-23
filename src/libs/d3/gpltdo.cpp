/*
 * GPL 3DO file
 * 09-07-00: Created! (18:00:47)
 * 25-05-01: Moved to Racer's Modeler (from csource/gpl/paint)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/gpltdo.h>
#include <qlib/app.h>
#include <sys/types.h>
//#include <netinet/in.h>
DEBUG_ENABLE

// Paint room boxes?
//#define PAINT_ROOMS

// Paint vertices as white dots?
#define PAINT_VERTICES

// Paint primitives? (polygons etc)
#define PAINT_PRIMITIVES

#define VERBOSE        1

GPLTDO::GPLTDO(cstring _fname)
{
  fname=_fname;
  fp=fopen(fname,"rb");
  xyz=0;
  xyzs=0;
  plane=0;
  planes=0;
  strnSize=0;
  strnData=0;
  xa=ya=za=0.0;
}
GPLTDO::~GPLTDO()
{
  QFREE(plane);
  QFREE(prim);
  QFREE(xyz);
  QFREE(strnData);
  fclose(fp);
}

/**********
* Reading *
**********/
static void cvtShorts(ushort *buffer,long n)
// From Paul Haeberli source code
{
#ifdef __sgi
  short i;
  long nshorts = n>>1;
  unsigned short swrd;

  for(i=0; i<nshorts; i++)
  { swrd = *buffer;
    *buffer++ = (swrd>>8) | (swrd<<8);
  }
#endif
}
static void cvtLongs(long *buffer,long n)
{
#ifdef __sgi
  int i;
  long nlongs = n /*>>2*/;
  unsigned long lwrd;

  for(i=0; i<nlongs; i++)
  { lwrd = buffer[i];
    buffer[i]=((lwrd>>24)          |
              (lwrd>>8 & 0xff00)   |
              (lwrd<<8 & 0xff0000) |
              (lwrd<<24)           );
  }
#endif
}
bool GPLTDO::Read()
// Read all
{
  long size,version;
  char id[5];
  
  // Seek past header
  fseek(fp,12,SEEK_SET);
  
  // Read chunks
 next_id:
  fread(&id,1,4,fp); id[4]=0;
  if(feof(fp))goto do_eof;
  fread(&version,1,4,fp);
  fread(&size,1,sizeof(size),fp); cvtLongs(&size,1);
  printf("ID '%s', size %d\n",id,size);
  // Round to multiple of 4
  if(size&3)size+=4-(size&3);
  
  if(!strcmp(id,"SZYX"))
  {
    xyzs=size/sizeof(GPL_XYZ);
    printf("Vertices: %d\n",xyzs);
    xyz=(GPL_XYZ*)qcalloc(xyzs*sizeof(GPL_XYZ));
    fread(xyz,xyzs,sizeof(GPL_XYZ),fp);
    cvtLongs((long*)xyz,xyzs*4);
#ifdef OBS
    for(int i=0;i<xyzs;i++)
    { printf("vert%d=%f %f %f\n",i,xyz[i].x,xyz[i].y,xyz[i].z);
    }
    QQuickSave("vertex.dmp",xyz,xyzs*sizeof(GPL_XYZ));
#endif
  } else if(!strcmp(id,"NALP"))
  {
    planes=size/sizeof(GPL_Plane);
    printf("Planes: %d\n",planes);
    plane=(GPL_Plane*)qcalloc(planes*sizeof(GPL_Plane));
    fread(plane,planes,sizeof(GPL_Plane),fp);
    cvtLongs((long*)plane,planes*4);
    printf("plane0=%f %f %f %f\n",plane[0].a,plane[0].b,plane[0].c,
      plane[0].d);
  } else if(!strcmp(id,"MIRP"))
  {
    primSize=size;
    printf("Primitives size: %d\n",primSize);
    prim=(long*)qcalloc(primSize);
    fread(prim,1,primSize,fp);
    cvtLongs(prim,primSize/sizeof(long));
  } else if(!strcmp(id,"NRTS"))
  {
    strnSize=size;
    printf("Strings: %d\n",size);
    strnData=(char*)qcalloc(size);
    fread(strnData,strnSize,1,fp);
  } else
  { // Skip
    printf("Unhandled chunk; skipping.\n");
    fseek(fp,size,SEEK_CUR);
  }
  goto next_id;
 do_eof:
  return TRUE;
}

/***********
* Painting *
***********/
void GPLTDO::PaintVertex(GPL_XYZ *v)
{
//qdbg("v(%f,%f,%f)\n",v->x,v->y,v->z);
  glVertex3f(v->x,v->y,v->z);
}
void GPLTDO::Paint()
{
  QCanvas *cv;
  int i;
  
  cv=QCV;
  cv->Select();
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#ifdef OBS_NO_FONTS
  cv->SetFont(FONT(0));
  cv->Set2D();
  cv->Text(fname,10,10);
#endif
  cv->Set3D();
  //ya+=1;
  za+=1;
  xa=-70;
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
glDisable(GL_CULL_FACE);
  //glTranslatef(0,-10,-1000);
  glTranslatef(0,-10,900);         // LFTBRIDG.3do
#ifdef OBS
  glTranslatef(0,-10,ZINI->GetInt("object.distance"));
#endif
  glRotatef(xa,1,0,0);
  glRotatef(ya,0,1,0);
  glRotatef(za,0,0,1);
#ifdef PAINT_VERTICES
  glColor4f(1,1,1,1);
  glBegin(GL_POINTS);
  for(i=0;i<xyzs;i++)
  { PaintVertex(&xyz[i]);
  }
  glEnd();
#endif
#ifdef PAINT_PRIMITIVES
  PaintPrimitives();
#endif
}

/*************
* Primitives *
*************/
cstring GPLTDO::GetString(long offset)
{
  return &strnData[offset];
}
void GPLTDO::SetColor(long rgba)
{
  float r,g,b,a;
  r=(rgba&0xFF)/255.;
  g=((rgba&0xFF00)>>8)/255.;
  b=((rgba&0xFF0000)>>16)/255.;
  a=1.0;
  //qdbg("%x color => %f %f %f\n",rgba&0xFFFFFF,r,g,b);
  glColor4f(r,g,b,a);
}

void GPLTDO::PaintPrim(long root,int dep)
// Recursive tree painter
{
  long type,subType;
  long off,offset[6];
  long plan[6];
  long v[8],vs;
  int  i,n;
  
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
      // Has many subtypes
      subType=prim[root+2];
      if(VERBOSE)qdbg(" subType 0x%x\n",subType);
      off=prim[root+1];
      if(prim[root+2]==5)
      {
        // Texture select
        if(VERBOSE)qdbg("Texture '%s'\n",GetString(prim[root+3]));
      }
      PaintPrim(off,dep+1);
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
      //PaintPrim(prim[root+6+(n-1)*2],dep+1);
      // Use furthest tree
      PaintPrim(prim[root+6],dep+1);
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
      glBegin(GL_LINES);
        PaintVertex(&xyz[prim[root+2]]);
        PaintVertex(&xyz[prim[root+3]]);
      glEnd();
      break;
    case 0x81B:
      // Polygon, flat shaded
      SetColor(prim[root+1]);
      vs=prim[root+2];
      if(VERBOSE)qdbg(" flat poly, color 0x%x, %d vertices\n",prim[root+1],vs);
      if(vs==4)
      { glBegin(GL_QUADS);
          PaintVertex(&xyz[prim[root+3]]);
          PaintVertex(&xyz[prim[root+4]]);
          PaintVertex(&xyz[prim[root+5]]);
          PaintVertex(&xyz[prim[root+6]]);
        glEnd();
      } else if(vs==5)
      { // Known problem; can't draw 5-sides polys
      } else
      { qdbg("0x81B polygon with unsupported #vertices (%d)\n",vs);
      }
      break;
    case 0x81C:
      // Polygon, gourad, not textured
      // <col> <nvertices> <vertices> <vert-colors>
      SetColor(prim[root+1]);
      vs=prim[root+2];
      if(VERBOSE)qdbg(" textured poly, color 0x%x, %d vertices\n",
        prim[root+2],vs);
      if(vs==4)
      { glBegin(GL_QUADS);
          PaintVertex(&xyz[prim[root+3]]);
          PaintVertex(&xyz[prim[root+4]]);
          PaintVertex(&xyz[prim[root+5]]);
          PaintVertex(&xyz[prim[root+6]]);
        glEnd();
      } else if(vs==5)
      { // Known problem!
      } else
      { qdbg("0x81C polygon with unsupported #vertices (%d)\n",vs);
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
        glBegin(GL_QUADS);
          PaintVertex(&xyz[prim[root+2*4+4]]);
          PaintVertex(&xyz[prim[root+2*4+5]]);
          PaintVertex(&xyz[prim[root+2*4+6]]);
          PaintVertex(&xyz[prim[root+2*4+7]]);
        glEnd();
      } else if(vs==5)
      { 
        glBegin(GL_POLYGON);
          PaintVertex(&xyz[prim[root+2*5+4]]);
          PaintVertex(&xyz[prim[root+2*5+5]]);
          PaintVertex(&xyz[prim[root+2*5+6]]);
          PaintVertex(&xyz[prim[root+2*5+7]]);
          PaintVertex(&xyz[prim[root+2*5+8]]);
        glEnd();
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
      { glBegin(GL_QUADS);
          SetColor(prim[root+2*4+4+0]);
          PaintVertex(&xyz[prim[root+2*4+4]]);
          SetColor(prim[root+2*4+4+1]);
          PaintVertex(&xyz[prim[root+2*4+5]]);
          SetColor(prim[root+2*4+4+2]);
          PaintVertex(&xyz[prim[root+2*4+6]]);
          SetColor(prim[root+2*4+4+3]);
          PaintVertex(&xyz[prim[root+2*4+7]]);
        glEnd();
      } else if(vs==3)
      { glBegin(GL_TRIANGLES);
          SetColor(prim[root+2*3+3+0]);
          PaintVertex(&xyz[prim[root+2*3+4]]);
          SetColor(prim[root+2*3+3+1]);
          PaintVertex(&xyz[prim[root+2*3+5]]);
          SetColor(prim[root+2*3+3+2]);
          PaintVertex(&xyz[prim[root+2*3+6]]);
        glEnd();
      } else if(vs>4)
      { // Known problem!
      } else
      { qdbg("0x820 polygon with unsupported #vertices (%d)\n",vs);
      }
      break;
    default:
      //if(VERBOSE)
      qdbg(" (nyi t=0x%x)\n",type);
      break;
  }
}
void GPLTDO::PaintPrimitives()
{
  // Root
  long root;
  root=*prim;
  if(VERBOSE)printf("Primitive root @ 0x%x\n",root);
  PaintPrim(root,0);
  if(VERBOSE)printf("--------------------\n");
}

