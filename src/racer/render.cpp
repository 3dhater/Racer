/*
 * Render - optimizing large rendering batches
 * 08-04-2001: Created! (21:12:01)
 * NOTES:
 * - Sorts by material only
 * FUTURE:
 * - Sort by texture and perhaps some others
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#define MAX_GEOB 100
#define MAX_MAT  1000
//#define MAX_GEOB 100

struct GeobNode
{
  GeobNode *next;
  DGeob    *geob;
  int       burst;
};

// Future: class
static DMaterial *mat[MAX_MAT];       // Different materials
static DTexture  *tex[MAX_MAT];       // Textures
static GeobNode  *list[MAX_MAT];      // List of geobs for a material
static GeobNode   node[MAX_GEOB];     // Prepare nodes
static int    mats;                   // Also the #lists
static int    geobs;
//static DGeob *geob[MAX_GEOB];

#ifdef OBS
static int FindGeobList(DMaterial *findMat)
// Find geob list for material, and creates new list of none is found
// Returns the index of the list, or -1 if no space is left for a
// new material list
{
  int i;
  for(i=0;i<mats;i++)
  {
    if(mat[i]==findMat)return i;   // list[i];
  }
  // Prepare new list, if there's still space
  if(mats==MAX_MAT)return -1;
  list[mats]=0;
  mats++;
  return mats-1;
}
#endif
static int FindGeobList(DTexture *findTex)
// Find geob list for material, and creates new list of none is found
// Returns the index of the list, or -1 if no space is left for a
// new material list
{
  int i;
  for(i=0;i<mats;i++)
  {
    if(tex[i]==findTex)return i;
  }
  // Prepare new list, if there's still space
  if(mats==MAX_MAT)return -1;
  list[mats]=0;
  mats++;
  return mats-1;
}

void RenderAdd(DGeode *geode)
{
  int i,j,n,m;
  GeobNode *aNode;
  DGeob *aGeob;
  DMaterial *aMaterial;
  DTexture *aTexture;
  
//qdbg("RenderAdd\n");
  n=geode->GetNoofGeobs();
//qdbg("  geobs=%d\n",n);
  for(i=0;i<n;i++)
  {
    if(geobs>=MAX_GEOB)break;
    aGeob=geode->geob[i];
//qdbg("geob[%d]; %d bursts\n",i,aGeob->bursts);
    for(j=0;j<aGeob->bursts;j++)
    {
      // Find list of nodes for the burst material
      //node=FindGeobList(aGeob->GetMaterialForID(aGeob->burstMtlID[j]));
      aMaterial=aGeob->GetMaterialForID(aGeob->burstMtlID[j]);
      aTexture=aMaterial->GetTexture(0);
      m=FindGeobList(aTexture);
      // Out of space?
      if(m==-1)return;
      
      // Get a node from the node pool and fill it with info
      aNode=&node[geobs];
      geobs++;
      aNode->geob=aGeob;
      aNode->burst=j;
      aNode->next=list[m];
      
      // Hang node in front
      list[m]=aNode;
      mat[m]=aMaterial;
      tex[m]=aTexture;
    }
  }
}

void RenderReset()
{
  geobs=mats=0;
}

void RenderFlush()
// Do the actual rendering (optimized)
{
  int i,j;
  GeobNode *node;
  
  // Paint
//qdbg("RenderFlush()\n");

  // All geobs have vertex arrays
  glEnableClientState(GL_VERTEX_ARRAY);
  for(i=0;i<mats;i++)
  {
//qdbg("mat[%d]=%p\n",i,mat[i]);
    mat[i]->PrepareToPaint();
    for(node=list[i];node;node=node->next)
    {
//qdbg("  node %p\n",node);
      node->geob->PaintOptimized(0,0);
    }
  }
  glDisableClientState(GL_VERTEX_ARRAY);
  // Get ready for next batch
  RenderReset();
}
