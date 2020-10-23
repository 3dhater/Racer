/*
 * Splines
 * 03-06-2001: Created! (14:56:37)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// Debug info in FindNextVertex()?
//#define VERBOSE_FNV

// GUI
cstring  opNameSpline[]=
{
  "(G)oto line start",
  "(A)dd longitudinal spline",
  "(R)emove last spline",
  "Find (N)ext cross-section",
  0
};

void SplineSetMode()
// Install us as the current mode
{
  SetOpButtons(opNameSpline);
}

/**********
* Helpers *
**********/
static void FindNextVertex(DVector3 *vFrom,DVector3 *vTo,
  DVector3 *vPrevious,DVector3 *vClosest)
// Given a from and to vertex, this function searches for the closest
// next vertex along the line 'from'->'to'.
// If you have just 1 line, take a virtual direction by calculating
// the normal from the left to right side of the road (using an UP
// vector as the 2nd input vector). In fact, this may always be
// the best approach, rather than taking a line through the last 2
// points.
{
  int i,j;
  RTV_Node *trackNode;
  int       g,tri,nTris,n;
  dfloat   *pVertex;
  //dindex *pIndex;
  DGeode   *geode;
  DGeob    *geob;
#ifdef OBS
  dfloat  v0[3],v1[3],v2[3];
  dfloat  t,u,v;
#endif
  DVector3 dirLat,dirLon,vTest,vPrevStore;
  dfloat   minDist,minDotLat,minDotLon;
  dfloat   curLength,curDotLat,curDotLon;
  
  if(!vPrevious)
  {
    // Generate a previous vertex based on a flat 2D-like track
    DVector3 vToDir,vUp(0,1,0);
    vToDir=*vTo;
    vToDir.Subtract(vFrom);
    vPrevious=&vPrevStore;
    vPrevious->Cross(&vUp,&vToDir);
  }
  
  // Calculate direction vectors
  dirLat=*vTo;
  dirLat.Subtract(vFrom);
  dirLon=*vPrevious;
  
  minDist=1e10;
  minDotLat=minDotLon=1e10;

#ifdef VERBOSE_FNV
qdbg("FindNextVertex\n");
vFrom->DbgPrint("vFrom");
vTo->DbgPrint("vTo");
vPrevious->DbgPrint("vPrevious");
dirLat.DbgPrint("dirLat");
dirLon.DbgPrint("dirLon");
#endif

  // For all nodes
  for(i=0;i<track->GetNodes();i++)
  {
    trackNode=track->GetNode(i);
    geode=trackNode->geode;
    
    // For all geobs, check every triangle
    for(g=0;g<geode->geobs;g++)
    {
      geob=geode->geob[g];
      pVertex=geob->vertex;
      if(!pVertex)continue;
      nTris=geob->indices/3;
#ifdef VERBOSE_FNV
qdbg("geob %d; nTris=%d\n",g,nTris);
#endif
      // For all triangles, check every vertex
      for(tri=0;tri<nTris;tri++)
      {
        for(j=0;j<3;j++)
        {
          // Get test vertex in vFrom's coordinate system
          n=geob->index[tri*3+j];
          pVertex=&geob->vertex[n*3];
          vTest.x=pVertex[0]-vFrom->x;
          vTest.y=pVertex[1]-vFrom->y;
          vTest.z=pVertex[2]-vFrom->z;
          
          // Calculate projection onto from->to line
          curDotLat=dirLat.Dot(vTest);
          curDotLon=dirLon.Dot(vTest);

#ifdef VERBOSE_FNV
qdbg("  tri %d, vertex %d (%f,%f,%f)\n",tri,j,vTest.x,vTest.y,vTest.z);
#endif
          //if(vFrom->SquaredDistanceTo(&vTest)<minDist)
          curLength=vTest.LengthSquared();
          if(curLength<minDist&&curLength>D3_EPSILON)
          {
#ifdef VERBOSE_FNV
qdbg("    dist=%f, minDist=%f\n",curLength,minDist);
qdbg("    curDotLon=%f, minDotLon=%f\n",curDotLon,minDotLon);
#endif
            // Closer vertex?
            if(curDotLon<D3_EPSILON)
            {
              // Vertex is behind the direction from the line from->to
              // Don't consider it, because it's where we just came from
            } else
            {
              // Best candidate sofar
              minDist=curLength;
              // Reset best projection length to current one (curDotLon
              // may be larger than minDotLon, but the vertex is closer
              // so take this projection length as the new reference)
              minDotLon=curDotLon;
              // Store closest vertex
              vClosest->x=vTest.x+vFrom->x;
              vClosest->y=vTest.y+vFrom->y;
              vClosest->z=vTest.z+vFrom->z;
#ifdef VERBOSE_FNV
vTest.DbgPrint("vClosest sofar");
#endif
            } // else it is probably a vertex on the same location!
          }
        }
      }
    }
  }
#ifdef VERBOSE_FNV
qdbg("--- end of FindNextVertex()\n");
#endif
}

/********
* Stats *
********/
void SplineStats()
{
  char buf[40],buf2[40];
  sprintf(buf,"-- Spline info --");
  StatsAdd(buf);
}

/********
* Paint *
********/
void SplinePaint()
{
  track->PaintHiddenSplineTris();
#ifdef OBS
  int i,j;
  RSplineRep *sr=track->GetSplineRep();
  RSplineTri *st;
  
  // Paint triangles from the spline
  for(i=0;i<sr->GetTris();i++)
  {
    st=sr->GetTri(i);
    glColor3f(.5,1,.5);
    glBegin(GL_LINE_LOOP);
      for(j=0;j<3;j++)
      {
        glVertex3f(st->v[j].x,st->v[j].y,st->v[j].z);
      }
    glEnd();
  }
#endif
}

/******
* Ops *
******/
static void FindNextCrossSection()
// Give it a shot for what would be the next spline section
{
  DVector3 vLeftNext,vRightNext;
  DVector3 vRightHelp;
  
  // Find new left vertex
  FindNextVertex(&vPick,&vPickEnd,0,&vLeftNext);
  
  // Find new right vertex, first create a virtual search line
  vRightHelp=vPickEnd;
  vRightHelp.Subtract(&vPick);
  vRightHelp.Add(&vPickEnd);
  FindNextVertex(&vPickEnd,&vRightHelp,0,&vRightNext);
  
  // Use the new points as the new pick line
  vPick=vLeftNext;
  vPickEnd=vRightNext;
}
static void RemoveSplineSegment()
{
  RSplineRep *sr=track->GetSplineRep();
  
  // Remove last spline segment
  sr->RemoveLine();
  
  // Rebuild triangle list
  sr->BuildTriangles();
}
static void AddSplineSegment()
// Add current selection line as road spline
{
  RSplineRep *sr=track->GetSplineRep();
  
  // Check for space
  if(!sr->GetLines())
  {
    // Reserve some
    sr->Reserve(1000);
  }
  
  // Add it
  RSplineLine sl;
  memset(&sl,0,sizeof(sl));
  // Left/right side of the road (2 control points, 2 traces in GPL terms)
  sl.cp[0]=vPick;
  sl.cp[1]=vPickEnd;
  sr->AddLine(&sl);
  
  // Rebuild triangle list
  sr->BuildTriangles();
}
void SplineOp(int op)
{
  switch(op)
  {
    case 0: GotoPickPoint(); break;
    case 1: AddSplineSegment(); break;
    case 2: RemoveSplineSegment(); break;
    case 3: FindNextCrossSection(); break;
  }
}

/*******
* Keys *
*******/
bool SplineKey(int key)
{
  if(key==QK_PAGEUP)
  {
  } else if(key==QK_PAGEDOWN)
  {
  } else if(key==QK_A)
  {
    AddSplineSegment();
    FindNextCrossSection();
  } else if(key==QK_R)
  {
    RemoveSplineSegment();
  } else if(key==QK_N)
  {
    FindNextCrossSection();
  }
  return FALSE;
}
