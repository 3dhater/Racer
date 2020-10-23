/*
 * DGeob support - optimizations
 * 26-12-00: Detached from dgeob.cpp. In end applications, these functions
 * should not be called. Instead, this should be part of preprocessing
 * your models (and exporting the optimized geobs to DOF).
 * NOTES:
 * - These functions are flat so they're not linked when not used.
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/matrix.h>
DEBUG_ENABLE

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
* Optimizing *
*************/
int DGeobOptimizeIndices(DGeob *g)
// Find vertices that are the same, and merge them
// This may result in fewer indices being used (VRL files from SCGT
// for example), and this again results in a better working RethinkNormals()
// (otherwise shared vertices won't be noticed)
// Returns the number of reduced indices.
// NOTE: no arrays are shrunk after this optimization yet. Use
// DGeobPackIndices() for that.
{
  int i,j,k,b;
  int vs;
  int count,               // Number of vertices optimized out
      countTexMisses;      // #vertices missed because of ineqaul texcoords
  float v1[3],v2[3];
  float *vbase;
  
//qdbg("DGeob:OptimizeIndices()\n");
  count=countTexMisses=0;
  // Optimize per burst; if we combine indices through burst, problems
  // may arise with texture coordinates that differ. (?)
  //for(b=0;b<bursts;b++)
  {
    //vs=burstCount[b]/3;
    vs=g->vertices;
#ifdef OBS
qdbg("  burst 0; %d vertices to check\n",vs);
qdbg("  indices=%d, burstCount[0]=%d\n",g->indices,g->burstCount[0]);
#endif
    // Calculate base of first vertex to check
    //vbase=&vertex[burstStart[b]];
    vbase=g->vertex;
    
    for(i=0;i<vs;i++)
    {
      // Get vertex
      v1[0]=vbase[i*3+0];
      v1[1]=vbase[i*3+1];
      v1[2]=vbase[i*3+2];
      for(j=i+1;j<vs;j++)
      {
        // Vertex equal?
        v2[0]=vbase[j*3+0];
        v2[1]=vbase[j*3+1];
        v2[2]=vbase[j*3+2];
        if(v1[0]==v2[0]&&v1[1]==v2[1]&&v1[2]==v2[2])
        {
//qdbg("  vertex %d==vertex %d (%f,%f,%f)\n",i,j,v1[0],v1[1],v1[2]);
          // Check for texture vertices to match; if not
          // (the ferp4 has non-matching ones) than don't weld,
          // because this will mess up texturing.
          if(g->tvertex!=0&&
             (g->tvertex[i*2+0]!=g->tvertex[j*2+0]||
              g->tvertex[i*2+1]!=g->tvertex[j*2+1]))
          {
//qdbg("  differing texcoords though; %f,%f vs %f,%f\n",
//tvertex[i*2+0],tvertex[i*2+1],tvertex[j*2+0],tvertex[j*2+0]);
            countTexMisses++;
            continue;
          }

          // Check for vertex normals to match; if not
          // (the Impr22 has non-matching ones) than don't weld,
          // because this will mess up smoothing.
          if(g->normal!=0&&
             (g->normal[i*3+0]!=g->normal[j*3+0]||
              g->normal[i*3+1]!=g->normal[j*3+1]||
              g->normal[i*3+2]!=g->normal[j*3+2]))
          {
            countTexMisses++;
            continue;
          }
//if(!count)qdbg("  weldable vertices found.\n");

          // Check vertex colors as well? Probably not needed.
          //...

          count++;
          // Change all references to vertex j to vertex i
          for(k=0;k<g->indices;k++)
          {//qdbg("k=%d\n",k);
            if(g->index[k]==j)
              g->index[k]=i;
          }
        }
      }
    }
  }
#ifdef OBS
qdbg("Report: %d vertices optimized out, %d missed (texcoord)\n",
count,countTexMisses);
#endif
#ifdef OBS
 DumpArray("vertex",g->vertex,g->vertices*3,3);
 DumpArray("normal",g->normal,g->normals*3,3);
 DumpArray("tvertex",g->tvertex,g->tvertices*2,2);
 DumpArray("index",g->index,g->indices,1);
#endif
  return count;
}

/***************
* Pack indices *
***************/
int DGeobPackIndices(DGeob *g)
// Pack the index array so unused index entries are removed.
// Call this function after DGeobOptimizeIndices(); that function
// will remove indices that are equal.
// For example, a rectangle in WRL will start out as:
//    0,1,2,3,4,5 (indices)
// After DGeobOptimizeIndices(), it will probably contain:
//    0,1,2,0,2,5 (index 3 and 4 were found to be equal to 0 and 2)
// Call PackIndices, and it will result in:
//    0,1,2,0,2,3 (the smallest possible set for an indexed primitive list)
// Returns size reduction of the vertex/normal/tvertex arrays.
// Assumptions: geob contains only 1 burst
{
  int i,j,k,thisIndex,unusedIndices,newSize;
  int expectedIndex,lowestIndexFound,maxIndex;
  dfloat *newVertex,*newNormal,*newTVertex,*newVColor;
  
//qdbg("DGeobPackIndices()\n");
#ifdef ND_HM
  if(g->bursts>1)
  { qwarn("DGeobPackIndices() works only on geobs with 1 burst");
    return;
  }
#endif
  
  // Find the max index in the list of indices
  maxIndex=-1;
  for(i=0;i<g->indices;i++)
    if(g->index[i]>maxIndex)
      maxIndex=g->index[i];
//qdbg("  maxIndex found=%d\n",maxIndex);

  // Find the number of unused indices
  unusedIndices=0;
  for(i=0;i<maxIndex;i++)
  {
    expectedIndex=i;
    for(j=0;j<g->indices;j++)
      if(g->index[j]==expectedIndex)goto index_found;
    unusedIndices++;
   index_found:;
  }
  // The new size of the indexed primitve array can now be calculated
  newSize=maxIndex+1-unusedIndices;
//qdbg("  Unused indices: %d, newSize=%d\n",unusedIndices,newSize);

  //
  // Allocate new arrays of information
  //
  newVertex=(dfloat*)qcalloc(sizeof(dfloat)*newSize*3);
  newNormal=(dfloat*)qcalloc(sizeof(dfloat)*newSize*3);
  newTVertex=(dfloat*)qcalloc(sizeof(dfloat)*newSize*2);
  newVColor=(dfloat*)qcalloc(sizeof(dfloat)*newSize*3);
  if(newVertex==0||newNormal==0||newTVertex==0||newVColor==0)
  {
    qerr("DGeobPackIndices() out of memory");
    return 0;
  }
  
  // Find all differing indices, make sure they exist
  for(i=0;i<newSize;i++)
  {
//qdbg("  search for index %d\n",i);
    expectedIndex=i;
    lowestIndexFound=-1;
    for(j=0;j<g->indices;j++)
    {
      thisIndex=g->index[j];
      // If the index was used, we can continue with the next index
      if(thisIndex==expectedIndex)goto index_ok;
      if(thisIndex>=expectedIndex&&thisIndex>lowestIndexFound)
        lowestIndexFound=thisIndex;
    }
//qdbg("  lowest next index found is %d\n",lowestIndexFound);
    // Change all references to 'lowestIndexFound' to 'expectedIndex'
    for(j=0;j<g->indices;j++)
      if(g->index[j]==lowestIndexFound)
        g->index[j]=expectedIndex;
    // Now make sure the index data is copied to its new spot in the
    // new arrays. Think of 'thisIndex' as 'sourceIndex' from here.
    thisIndex=lowestIndexFound;
    
   index_ok:
    // Copy info of lowest index found into free spot (expectedIndex)
    for(k=0;k<3;k++)
    {
      newVertex[i*3+k]=g->vertex[thisIndex*3+k];
      if(g->normal)
        newNormal[i*3+k]=g->normal[thisIndex*3+k];
      if(g->vcolor)
        newVColor[i*3+k]=g->vcolor[thisIndex*3+k];
      if(k<2&&g->tvertex)
      { /*qdbg("k=%d, i=%d, thisIndex=%d, g->tvertex=%p\n",
          k,i,thisIndex,g->tvertex);*/
        if(g->tvertex)
          newTVertex[i*2+k]=g->tvertex[thisIndex*2+k];
      }
    }
  }
  
  // Switch model to optimized arrays.
  // Note that the 'vertex' array must always exist
  QFREE(g->vertex);
  g->vertex=newVertex; g->vertices=newSize;
  if(g->normal)
  {
    QFREE(g->normal);
    g->normal=newNormal; g->normals=newSize;
  } else QFREE(newNormal);
  if(g->tvertex)
  {
    QFREE(g->tvertex);
    g->tvertex=newTVertex; g->tvertices=newSize;
  } else QFREE(newTVertex);
  if(g->vcolor)
  {
    QFREE(g->vcolor);
    g->vcolor=newVColor; g->vcolors=newSize;
  } else QFREE(newVColor);

  // Rebuild display list at next paint (if any)
  g->DestroyList();
  
#ifdef OBS
 //DumpArray("vertex",g->vertex,g->vertices*3,3);
 DumpArray("normal",g->normal,g->normals*3,3);
 //DumpArray("tvertex",g->tvertex,g->tvertices*2,2);
 //DumpArray("index",g->index,g->indices,1);
 qdbg("----------------\n");
#endif
 
  return unusedIndices;
}

/********************
* Optimizing bursts *
********************/
int DGeobOptimizeBursts(DGeob *g)
// Find equal material bursts and combine them to reduce the number
// of bursts possibly.
// Returns the #bursts optimized out (removed).
{
  int i,j,k;
  bool    found;
  dindex *newIndex;
  int     newStart;
  int     newBursts,oldBursts;
  int     newBurstStart[DGeob::MAX_FACE_BURST];
  int     newBurstCount[DGeob::MAX_FACE_BURST];
  int     newBurstMtlID[DGeob::MAX_FACE_BURST];
  int     newBurstVperP[DGeob::MAX_FACE_BURST];

  // Anything to optimize?
  if(!g->indices)return 0;
  if(g->bursts==1)return 0;

  // First, equalize material ID's that end up at the same
  // material anyway (3D Studio Max cubes for example may expose
  // this behavior; each side gets its own material ID, but is then
  // unused by the artist)

  // For all bursts, find matching partners
  for(i=0;i<g->bursts;i++)
  {
    for(j=i+1;j<g->bursts;j++)
    {
      if(g->GetMaterialForID(g->burstMtlID[i])==
         g->GetMaterialForID(g->burstMtlID[j]))
      {
        // Equalize mtlID's; note this doesn't pack MtlID's,
        // so you may end up with 1 id that is 5 for example.
        g->burstMtlID[j]=g->burstMtlID[i];
      }
    }
  }

  // New index array
  newIndex=(dindex*)qcalloc(g->indices*sizeof(dindex));
  if(!newIndex)
  { qwarn("DGeobOptimizeBursts: out of memory");
    return 0;
  }
  newStart=newBursts=0;
  oldBursts=g->bursts;

qdbg("Indices=%d\n",g->indices);

  // For all bursts, find matching partners
  for(i=0;i<g->bursts;i++)
  {
    // Find all matching bursts, INCLUDING ITSELF (!)
    found=FALSE;
    for(j=i;j<g->bursts;j++)
    {
      // Check for already optimized burst
      if(g->burstMtlID[j]==-1)continue;
      if(g->burstMtlID[i]==g->burstMtlID[j])
      {
        // Combine into new set
        if(!found)
        {
qdbg("New burst (%d) for b%d and b%d (mtlID %d)\n",
newBursts+1,i,j,g->burstMtlID[i]);
          found=TRUE;
          // Create new burst
          newBursts++;
          newBurstMtlID[newBursts-1]=g->burstMtlID[i];
          // Start new sequence in 'newIndex'
          newBurstStart[newBursts-1]=newStart*3;
          // Contains 0 elements still; will be copied
          // shortly after this
          newBurstCount[newBursts-1]=0; // burstCount[i];
        }
        // Add the indices of this partner to the burst
qdbg("  adding %d indices\n",g->burstCount[j]/3);
        for(k=0;k<g->burstCount[j]/3;k++)
        {
          newIndex[newStart+k]=g->index[g->burstStart[j]/3+k];
        }
        newStart+=g->burstCount[j]/3;
        // Update the (new) burst count
        newBurstCount[newBursts-1]+=g->burstCount[j];

        // Invalidate old burst, so it won't be included next time.
        // Take care not too invalidate the starting burst yet though.
        if(i!=j)
          g->burstMtlID[j]=-1;
      }
    }
  }
qdbg("  at end: newStart=%d\n",newStart);

  // Switch to new set of bursts
  QFREE(g->index);
  g->index=newIndex;
  for(i=0;i<newBursts;i++)
  {
    g->burstStart[i]=newBurstStart[i];
    g->burstCount[i]=newBurstCount[i];
    g->burstMtlID[i]=newBurstMtlID[i];
    // Leave burstVperP; they should all be 3 anyway
    //g->burstVperP[i]=newBurstVperP[i];
  }
  g->bursts=newBursts;

  // Free resources
  //QFREE(newIndex);

  // Return the # of bursts removed
  return oldBursts-newBursts;
}

/*********
* DGeode *
*********/
int DGeodeOptimizeBursts(DGeode *g)
// Combine burst of equal material
{
//return 0;
  int i,total;
  for(i=total=0;i<g->geobs;i++)
    total+=DGeobOptimizeBursts(g->geob[i]);
  return total;
}
int DGeodeOptimizeIndices(DGeode *g)
// Weld equal vertices and such
{
  int i,total;
  for(i=total=0;i<g->geobs;i++)
    total+=DGeobOptimizeIndices(g->geob[i]);
  return total;
}
int DGeodePackIndices(DGeode *g)
// Pack array of indices, to get rid of indices that are not used
// Returns #indices that were removed.
{
  int i,total;
  for(i=total=0;i<g->geobs;i++)
    total+=DGeobPackIndices(g->geob[i]);
  return total;
}
