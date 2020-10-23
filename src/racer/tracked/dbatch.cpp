/*
 * DBatch - optimizing large rendering batches
 * 08-04-01: Created! (21:12:01)
 * 28-04-01: Support for transparent objects.
 * NOTES:
 * - Sorts by material only
 * FUTURE:
 * - Sort by texture and perhaps some others
 * - Transparency support; 
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/batch.h>
DEBUG_ENABLE

DBatch::DBatch()
{
  flags=0;
  Reset();
}
DBatch::~DBatch()
{
}

/*********************
* Finding a material *
*********************/
int DBatch::FindGeobList(DTexture *findTex)
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

/********
* Reset *
********/
void DBatch::Reset()
{
  geobs=mats=0;
  // Clear batch state flags
  flags&=~(HAS_TRANSPARENT_GEOBS);
}

/******************
* Adding geometry *
******************/
void DBatch::Add(DGeode *geode)
// Add all geode primitives for future rendering
{
  int i,j,n,m;
  GeobNode *aNode;
  DGeob *aGeob;
  DMaterial *aMaterial;
  DTexture *aTexture;
  
qdbg("DBatch:Add(%p)\n",geode);
  n=geode->GetNoofGeobs();
qdbg("  geobs=%d\n",n);
  for(i=0;i<n;i++)
  {
    if(geobs>=MAX_GEOB)break;
    aGeob=geode->geob[i];
qdbg("geob[%d]; %d bursts\n",i,aGeob->bursts);
    for(j=0;j<aGeob->bursts;j++)
    {
      // Find list of nodes for the burst material
      //node=FindGeobList(aGeob->GetMaterialForID(aGeob->burstMtlID[j]));
      aMaterial=aGeob->GetMaterialForID(aGeob->burstMtlID[j]);
qdbg("aMat=%p\n",aMaterial);
      aTexture=aMaterial->GetTexture(0);
qdbg("aTex=%p\n",aTexture);
      // Won't sort untextured geobs
      if(!aTexture)continue;

      m=FindGeobList(aTexture);
      // Out of space?
      if(m==-1)return;
      
      // Get a node from the node pool and fill it with info
      aNode=&node[geobs];
      geobs++;
      aNode->geob=aGeob;
      aNode->burst=j;
      if(aTexture->IsTransparent())
      {
        aNode->flags=IS_TRANSPARENT;
        // Remember to split drawing into transparent and opaque
        flags|=HAS_TRANSPARENT_GEOBS;
      } else
      {
        aNode->flags=0;
      }
      aNode->next=list[m];
      
      // Hang node in front
      list[m]=aNode;
      mat[m]=aMaterial;
      tex[m]=aTexture;
    }
  }
}
void DBatch::Flush()
// Actually send the things to be rendered to the graphics pipeline
{
  int i;
  GeobNode *node;
  
  // Paint
qdbg("DBatch::Flush()\n");

  // Save some affected settings
  glPushAttrib(GL_DEPTH_BUFFER_BIT);
  glPushAttrib(GL_ENABLE_BIT);

  // All geobs have vertex arrays
  glEnableClientState(GL_VERTEX_ARRAY);

  if(flags&HAS_TRANSPARENT_GEOBS)
  {
qdbg("  transparent detected\n");
    // Split drawing into opaque and transparent objects

    // Draw opaque objects
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    for(i=0;i<mats;i++)
    {
      if(tex[i]->IsTransparent())continue;
qdbg("  opaque mat %d out of %d\n",i,mats);
      mat[i]->PrepareToPaint();
      for(node=list[i];node;node=node->next)
      {
        node->geob->PaintOptimized(0,0);
      }
    }

    // Draw transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER,0.5);
    //glDepthMask(GL_FALSE);
    for(i=0;i<mats;i++)
    {
      if(!tex[i]->IsTransparent())continue;
qdbg("  mat %d out of %d\n",i,mats);
      mat[i]->PrepareToPaint();
      for(node=list[i];node;node=node->next)
      {
        node->geob->PaintOptimized(0,0);
      }
    }
    glDisable(GL_ALPHA_TEST);
  } else
  {
//qdbg("  all opaque\n");
    // All objects are opaque
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
  }
  glDisableClientState(GL_VERTEX_ARRAY);

  glPopAttrib();
  glPopAttrib();

  // Get ready for next batch
  Reset();
}
