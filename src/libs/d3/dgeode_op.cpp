/*
 * DGeode support - misc operations
 * 26-12-00: Detached from dgeode.cpp.
 * BUGS:
 * - Triangulate doesn't work
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/app.h>
#include <d3/matrix.h>
#include <d3/geode.h>
DEBUG_ENABLE

// Indexed face sets instead of flat big arrays?
#define USE_INDEXED_FACES

/****************
* Triangulation *
****************/
void DGeobTriangulate(DGeob *g)
// Make all polygons triangles
// Still bugs
{
  qerr("DGeobTriangulate() NYI");
#ifdef ND_FUTURE
  int i,v,total;
  float *newFaceV,*newTFaceV;
  float *sFace,*dFace,*sTFace,*dTFace;
  
qdbg("Triangulate\n");
//return;

  // Count total of new vertices
  total=0;
  for(i=0;i<bursts;i++)
  {
    switch(burstVperP[i])
    {
      case 3: total+=burstCount[i]; break;
      case 4: total+=burstCount[i]*6/4; break;
      // Unsupported
      default: total+=burstCount[i]; break;
    }
  }
qdbg("  total count of triangulated mesh: %d\n",total);

  // Allocate new arrays
  newFaceV=(float*)qcalloc(total*sizeof(float)*N_PER_V);
  if(!newFaceV)
  {
    qwarn("DGeob:Triangulate; not enough memory (newFaceV)");
    return;
  }
  newTFaceV=(float*)qcalloc(total*sizeof(float)*N_PER_TV);
  if(!newTFaceV)
  {
    QFREE(newFaceV);
    qwarn("DGeob:Triangulate; not enough memory (newTFaceV)");
    return;
  }
  
  // Start copying
  sFace=face_v;
  dFace=newFaceV;
  sTFace=tface_v;
  dTFace=newTFaceV;
  
  // Scale arrays to triangle data (tesselate)
 DumpArray("sFace/face_v",sFace,burstCount[0]*N_PER_V,3);
 DumpArray("sTFace/tface_v",sTFace,burstCount[0]*N_PER_TV,2);
  for(i=0;i<bursts;i++)
  {
    // Already triangles?
    if(burstVperP[i]==3)
    {
      // Vanilla copy
      for(v=0;v<burstCount[i];v++)
      {
        // Copy face vertex
        *dFace++=*sFace; sFace++;
        *dFace++=*sFace; sFace++;
        *dFace++=*sFace; sFace++;
        // Copy tface vertices
        *dTFace++=*sTFace; sTFace++;
        *dTFace++=*sTFace; sTFace++;
      }
      continue;
    } else if(burstVperP[i]==4)
    {
      // Make 4 into 3; from 0123 create 012 and 230
      // Vanilla copy
      for(v=0;v<burstCount[i]/4;v++)
      {
        // Copy face vertices
        // 012
        memcpy(dFace,&sFace[0],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[1],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[2],3*sizeof(float)); dFace+=3;
        // 230
        memcpy(dFace,&sFace[2],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[3],3*sizeof(float)); dFace+=3;
        memcpy(dFace,&sFace[0],3*sizeof(float)); dFace+=3;
        sFace+=4*3;
        // Copy tface vertices; 012
        memcpy(dTFace,&sTFace[0],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[1],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[2],2*sizeof(float)); dTFace+=2;
        // 230
        memcpy(dTFace,&sTFace[2],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[3],2*sizeof(float)); dTFace+=2;
        memcpy(dTFace,&sTFace[0],2*sizeof(float)); dTFace+=2;
        sTFace+=4*2;
      }
      // From 4 to 6 vertices
      burstCount[i]=burstCount[i]*6/4;
      burstVperP[i]=3;
    }
  }
  // Move over to new vertex arrays
  QFREE(face_v);
  QFREE(tface_v);
  face_v=newFaceV;
  tface_v=newTFaceV;
#endif
}

/**********
* Normals *
**********/
void DGeobRethinkNormals(DGeob *g)
// Re-generate normals
// Only works for triangle meshes
// Generates smoothed normals for the vertices in the 'normal' array
// (indexed face sets only)
{
  int i,j,tris;
  int b;                    // Burst
  int count;
  int v[3];                 // Vertex indices
  dfloat planeNormal[3];    // The plane normal

qdbg("DGeob:RethinkNormals() (geob=@%p)\n",g);

#ifdef OBS
  // First optimize, otherwise shared vertices might not be noticed,
  // and planes won't be averaged, but instead seen as separate
  OptimizeIndices();
//return;  //$DEV
#endif
  
//qdbg("  index=%p, indices=%d\n",index,indices);
//DumpArray("index",index,indices,3);
  b=0;
  tris=g->burstCount[b]/(3*3);
  
#ifdef OBS
for(i=0;i<normals;i++)
{
DVector3 nrm(&normal[i*3]);
qdbg("  normal pre %d=(%f,%f,%f)\n",i,nrm.x,nrm.y,nrm.z);
}
#endif

  // Clear all normals
qdbg("  normal=%p, normals=%d, tris=%d\n",g->normal,g->normals,tris);
  if(!g->normal)
  {
    // No normals were loaded or generated, allocate space
    g->normals=g->vertices;
    // Check for any normals to be generated
    if(!g->normals)
      return;
    g->normal=(dfloat*)qcalloc(g->normals*sizeof(dfloat)*3);
    if(!g->normal)
    { qerr("No memory for normals");
      return;
    }
  }
  for(i=0;i<g->normals*3;i++)
    g->normal[i]=0.0f;

  // Average all plane normals that touch vertex 'i'
  for(i=0;i<tris;i++)
  {
    // Create vectors of the triangle
    for(j=0;j<3;j++)
      v[j]=g->index[i*3+j];
    DVector3 v1(&g->vertex[v[0]*3]),
             v2(&g->vertex[v[1]*3]),
             v3(&g->vertex[v[2]*3]);
    // Create a plane for the triangle
    DPlane3 plane(v1,v2,v3);
#ifdef OBS
qdbg("tri %d; normal %f,%f,%f\n",i,plane.n.x,plane.n.y,plane.n.z);
qdbg("  indices=%d,%d,%d\n",v[0],v[1],v[2]);
qdbg("  v1=%f,%f,%f\n",v1.x,v1.y,v1.z);
qdbg("  v2=%f,%f,%f\n",v2.x,v2.y,v2.z);
qdbg("  v3=%f,%f,%f\n",v3.x,v3.y,v3.z);
DVector3 vv=v1-v3;
qdbg("  v1-v3=%f,%f,%f\n",vv.x,vv.y,vv.z);
vv=v1-v2;
qdbg("  v2-v3=%f,%f,%f\n",vv.x,vv.y,vv.z);
vv=(v1-v3)^(v1-v2);
qdbg("  crossprod=%f,%f,%f\n",vv.x,vv.y,vv.z);
#endif
    planeNormal[0]=plane.n.x;
    planeNormal[1]=plane.n.y;
    planeNormal[2]=plane.n.z;
    // Add plane normal to all vertices
    for(j=0;j<3;j++)
    {
      g->normal[v[0]*3+j]+=planeNormal[j];
      g->normal[v[1]*3+j]+=planeNormal[j];
      g->normal[v[2]*3+j]+=planeNormal[j];
    }
//if(i==10)exit(0);
  }
  // Normalize all normals
  for(i=0;i<g->normals;i++)
  {
    DVector3 nrm(&g->normal[i*3]);
//qdbg("  normal %d=(%f,%f,%f)\n",i,nrm.x,nrm.y,nrm.z);
    nrm.Normalize();
//qdbg("  normalized %d: (%f,%f,%f)\n",i,nrm.x,nrm.y,nrm.z);
    g->normal[i*3+0]=nrm.x;
    g->normal[i*3+1]=nrm.y;
    g->normal[i*3+2]=nrm.z;
  }
  
  // Regenerate display list
  g->DestroyList();
}

/*****************
* MODEL JUGGLING *
*****************/
void DGeodeTriangulate(DGeode *g)
// Triangulate all polygons, if possible
{
  int i;
  for(i=0;i<g->geobs;i++)
    DGeobTriangulate(g->geob[i]);
}
void DGeodeRethinkNormals(DGeode *g)
// Auto-generate normals for all geobs
{
  int i;
  for(i=0;i<g->geobs;i++)
    DGeobRethinkNormals(g->geob[i]);
}
