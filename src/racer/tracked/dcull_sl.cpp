/*
 * DCullerSphereList - culling a list based on sphere bounding volumes
 * 04-01-2001: Created! (22:28:00)
 * NOTES:
 * - Meant more as a test of the DCuller equations.
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/culler.h>
DEBUG_ENABLE

DCullerSphereList::DCullerSphereList()
{
  nodes=0;
  node=(DCSLNode*)qcalloc(MAX_NODE*sizeof(DCSLNode));
}
DCullerSphereList::~DCullerSphereList()
{
  QFREE(node);
}

/**********
* Destroy *
**********/
void DCullerSphereList::Destroy()
// Destroy culler
{
  nodes=0;
}

/***********
* Building *
***********/
bool DCullerSphereList::AddGeode(DGeode *g)
{
  DCSLNode *n;
  DBox box;
//qdbg("DCSL:AddGeode\n");

  if(nodes==MAX_NODE)return FALSE;
  n=&node[nodes];
//qdbg("add geode %p\n",g);
  n->geode=g;
  g->GetBoundingBox(&box);
//qdbg("box (%f,%f,%f)-(%f,%f,%f)\n",box.min.x,box.min.y,box.min.z,
//box.max.x,box.max.y,box.max.z);
  box.GetCenter(&n->center);
  n->radius=box.GetRadius();
  
  nodes++;
  
  return TRUE;
}

/***********
* Painting *
***********/
void DCullerSphereList::Paint()
{
qdbg("DCSL:Paint()\n");
  int i,count;
  DCSLNode *n;
  
  count=0;
  for(i=0;i<nodes;i++)
  {
    n=&node[i];
qdbg("  node %d; geode %p\n",i,n->geode);
    if(SphereInFrustum(&n->center,n->radius)!=OUTSIDE)
    { count++;
      if(dglobal.IsBatchRendering())
      {
        dglobal.GetBatch()->Add(n->geode);
      } else
      {
        // Paint immediate
        n->geode->Paint();
      }
    }
    //else qdbg(" !INSIDE\n");
  }
  if(dglobal.IsBatchRendering())
    dglobal.GetBatch()->Flush();
//qdbg("DCullerSphereList:Paint(); painted %d out of %d\n",count,nodes);
}
