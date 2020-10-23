/*
 * Racer Splines (track representation using splines)
 * 17-06-01: Created!
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// When loading, add some extra line memory space
#define RESERVE_ADD   1000

#undef  DBG_CLASS
#define DBG_CLASS "RSplineRep"

/*********
* C/Dtor *
*********/
RSplineRep::RSplineRep()
{
  int i,j;
  
  line=0;
  tri=0;
  lines=0;
  tris=0;
  for(i=0;i<MAX_TRACE;i++)
    for(j=0;j<3;j++)
      splineHM[i][j]=0;
}
RSplineRep::~RSplineRep()
{
  int i,j;
  
  delete [] line; line=0;
  delete [] tri; tri=0;
  //QDELETE(line);
  //QDELETE(tri);
  for(i=0;i<MAX_TRACE;i++)
    for(j=0;j<3;j++)
      QDELETE(splineHM[i][j]);
}

/*********
* Memory *
*********/
bool RSplineRep::Reserve(int resLines)
// Reserve a certain amount of spline info
{

  if(line)
  {
    qwarn("RSplineRep:Reserve() called twice; old lines not yet copied over");
  }
  linesAllocated=resLines;
  //line=(RSplineLine*)qcalloc(linesAllocated*sizeof(RSplineLine));
  line=new RSplineLine[linesAllocated];
  lines=0;
//qdbg("RSplineRep:Reserve(); pre1\n");return FALSE;
  if(!line)
  { qerr("RSplineRep:Reserve() out of memory (line)");
    linesAllocated=0;
    return FALSE;
  }

//qdbg("RSplineLine=%d, %p-%p\n",sizeof(RSplineLine),&line[0],&line[1]);
  // Each line gives 2 triangles, except for the first line
  trisAllocated=2*linesAllocated;      // -1
//qdbg("trisAlloc'ed=%d, sizeof(Tri)=%d\n",trisAllocated,
//sizeof(RSplineTri));
  //tri=(RSplineTri*)qcalloc(trisAllocated*sizeof(RSplineTri));
  tri=new RSplineTri[trisAllocated];
//qdbg("RSplineTri=%d, %p-%p\n",sizeof(RSplineTri),&tri[0],&tri[1]);
  tris=0;
  if(!tri)
  { qerr("RSplineRep:Reserve() out of memory (tri)");
    trisAllocated=0;
    return FALSE;
  }
  return TRUE;
}

/******
* I/O *
******/
void RSplineRep::Load(QInfo *info)
// Read spline definitions.
// Also builds the triangle list and splines after loading (!)
{
  char buf[256];
  int  i,j,n;


  // Save line info
  n=info->GetInt("lines.count");

  // Reserve at least the line count
  Reserve(n+RESERVE_ADD);
//qdbg("RSplineRep:Load; this=%p\n",this);return;
  lines=n;
  
//qdbg("RSplineRep:pre2; this=%p\n",this);return;
  for(i=0;i<lines;i++)
  {
    // Restore control points
    for(j=0;j<2;j++)
    {
      sprintf(buf,"line%d.cp%d",i,j);
      info->GetFloats(buf,3,(float*)&line[i].cp[j]);
    }
  }
  // Automatically build triangle list and spline for convenience
//qdbg("RSplineRep:pre1; this=%p\n",this);return;
  BuildTriangles();
  BuildSpline();
}
void RSplineRep::Save(QInfo *info)
// Write spline definitions
{
  char buf[256],buf2[256];
  int  i,j;

  // Save line info
  info->SetInt("lines.count",lines);
  for(i=0;i<lines;i++)
  {
    // Store control points
    for(j=0;j<2;j++)
    {
      sprintf(buf,"line%d.cp%d",i,j);
      sprintf(buf2,"%f %f %f",line[i].cp[j].x,line[i].cp[j].y,line[i].cp[j].z);
      info->SetString(buf,buf2);
    }
  }
  // Don't save tri info, since this is derived from the line def's
}

/***********
* Building *
***********/
void RSplineRep::AddLine(RSplineLine *l)
{
  if(lines==linesAllocated)
  { Reserve(lines+50);
  }
  // Store line info
  line[lines]=*l;
  lines++;
}
void RSplineRep::RemoveLine()
// Remove last line
{
  if(lines>0)
  {
    lines--;
  }
}
void RSplineRep::BuildTriangles()
// Build a list of triangles from the spline lines
{
  int i;
  RSplineTri *t;
  
  // Create triangles; for every line, connect the left- and
  // rightmost points to the ones from the next spline (lateral) line.
  // This way, we get an efficient triangle 'net' which will give
  // us U/V coordinates for use in the spline later.
  // Note the careful ordering to get right-handed anti-clockwise
  // ordered polygons (they point up)
  tris=0;
//qdbg("BuildTriangles; lines=%d\n",lines);
  for(i=0;i<lines-1;i++)
  {
    t=&tri[i*2];
//qdbg("t=%p, tri=%p, allocated=%d, i*2=%d\n",t,tri,trisAllocated,i*2);
    // Leftmost
    t->v[0]=line[i].cp[0];
    // Rightmost
    t->v[1]=line[i].cp[1];
    // Next line leftmost
    t->v[2]=line[i+1].cp[0];
    
    // Other side triangle; note that the ordering makes U/V
    // coordinates behave almost like the first triangle; only
    // the V coordinate (which goes up from 0 to 1 as your approach
    // the 3rd vertex) is reversed. So every second triangle
    // in the list has a reversed V coordinate.
    t=&tri[i*2+1];
    t->v[0]=line[i+1].cp[1];
    t->v[1]=line[i+1].cp[0];
    t->v[2]=line[i].cp[1];
    
    // Got 2 more
    tris+=2;
  }
//qdbg("  tris=%d\n",tris);
}
void RSplineRep::BuildSpline()
// Build the (longitudinal) splines for the patch
{
  int i,j,t;
  
  // Deleted old splines
  for(i=0;i<MAX_TRACE;i++)
    for(j=0;j<3;j++)
      QDELETE(splineHM[i][j]);

  for(i=0;i<MAX_TRACE;i++)
  {
    for(j=0;j<3;j++)
    {
      splineHM[i][j]=new DHermiteSpline();
      // Don't close start & end (useful for partly built splines)
      splineHM[i][j]->DisableLoop();
    }
  }
  for(i=0;i<lines;i++)
  {
    // Add heights for the traces
    for(j=0;j<3;j++)
    {
      for(t=0;t<MAX_TRACE;t++)
      {
//qdbg("RSplineRep:BuildSpline(); line %d, cp %d; y=%f\n",j,i,line[i].cp[j].y);
        switch(j)
        {
          case 0: splineHM[t][j]->AddPoint(line[i].cp[t].x); break;
          case 1: splineHM[t][j]->AddPoint(line[i].cp[t].y); break;
          case 2: splineHM[t][j]->AddPoint(line[i].cp[t].z); break;
        }
      }
    }
  }
}

