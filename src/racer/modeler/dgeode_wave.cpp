/*
 * DGeode support - importing WaveFront models
 * 19-09-01: Created!
 * NOTES:
 * - Based on an example model in GDMag, dec '98.
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#pragma hdrstop
#include <qlib/app.h>
#ifdef linux
#include <stdlib.h>
#endif
#include <d3/matrix.h>
#include <d3/geode.h>
#include <qlib/debug.h>
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

/***************
* Wave helpers *
***************/
static void StripSpaces(string s)
{
  int i,len;
  len=strlen(s);
  for(i=0;i<len;i++)
  {
    if(s[i]==' '||s[i]==9)continue;
    // Strip those
    if(i>0)
      memmove(s,&s[i],len-i);
    break;
  }
}
static void CountElements(QFile *f,int count[4])
// Count all occurences of 'v', 'vt', 'vn' and 'f'.
// Stores results in 'count' (in the order as stated above)
{
  char buf[256];
  int  i;
  
  // Reset counters
  for(i=0;i<4;i++)
  {
    count[i]=0;
  }
  while(1)
  {
    f->GetLine(buf,sizeof(buf));
    if(f->IsEOF())break;
    StripSpaces(buf);
    if(strncmp(buf,"v ",2))count[0]++;
    if(strncmp(buf,"vt ",2))count[1]++;
    if(strncmp(buf,"vn ",2))count[2]++;
    if(strncmp(buf,"f ",2))count[3]++;
  }
}

#ifdef OBS
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
#endif

/***********************
* Index array handling *
***********************/
#ifdef OBS
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
static dindex *DGV_TriangulateArray(dindex *a,int n,int *pCount)
// Create triangulated index array from 'a' (size n).
// 'a' contains separators (-1) to indicate polygon ends.
// Returns #created indices in '*pCount'
// All data is converted into triangles.
{
  int i,j,curIndex;
  int len;
  dindex *d;
  
  // Allocate destination array, get enough space
qdbg("DGV_TriangulateArray(); n=%d\n",n);
  d=(dindex*)qcalloc(sizeof(dindex)*n*4);
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
    } else if(len==4)
    {
      // Split into 2 triangles
      d[curIndex+0]=a[i+0];
      d[curIndex+1]=a[i+1];
      d[curIndex+2]=a[i+2];
      d[curIndex+3]=a[i+0];
      d[curIndex+4]=a[i+2];
      d[curIndex+5]=a[i+3];
      curIndex+=2*3;
    } else if(len==5)
    {
      // 5-sided polygon; used in Oval1's TRK53.wrl for example
      // Split into 3 triangles, the easy way
      dindex *dp=&d[curIndex];
      *dp++=a[i+0]; *dp++=a[i+1]; *dp++=a[i+2];
      *dp++=a[i+0]; *dp++=a[i+2]; *dp++=a[i+3];
      *dp++=a[i+0]; *dp++=a[i+3]; *dp++=a[i+4];
      curIndex+=3*3;
    } else if(len==6)
    {
      // 6-sided polygon; used in Oval1's SIDC35.wrl for example
      // Split into 4 triangles, the easy way
      dindex *dp=&d[curIndex];
      *dp++=a[i+0]; *dp++=a[i+1]; *dp++=a[i+2];
      *dp++=a[i+0]; *dp++=a[i+2]; *dp++=a[i+3];
      *dp++=a[i+0]; *dp++=a[i+3]; *dp++=a[i+4];
      *dp++=a[i+0]; *dp++=a[i+4]; *dp++=a[i+5];
      curIndex+=4*3;
    } else if(len==7)
    {
      // 7-sided polygon; have never seen it, but here you go
      // Split into 5 triangles, the easy way
      dindex *dp=&d[curIndex];
      *dp++=a[i+0]; *dp++=a[i+1]; *dp++=a[i+2];
      *dp++=a[i+0]; *dp++=a[i+2]; *dp++=a[i+3];
      *dp++=a[i+0]; *dp++=a[i+3]; *dp++=a[i+4];
      *dp++=a[i+0]; *dp++=a[i+4]; *dp++=a[i+5];
      *dp++=a[i+0]; *dp++=a[i+5]; *dp++=a[i+6];
      curIndex+=5*3;
    } else if(len==8)
    {
      // 8-sided polygon; used in Oval1's SKBOX.wrl for example
      // Split into 6 triangles, the easy way
      dindex *dp=&d[curIndex];
      *dp++=a[i+0]; *dp++=a[i+1]; *dp++=a[i+2];
      *dp++=a[i+0]; *dp++=a[i+2]; *dp++=a[i+3];
      *dp++=a[i+0]; *dp++=a[i+3]; *dp++=a[i+4];
      *dp++=a[i+0]; *dp++=a[i+4]; *dp++=a[i+5];
      *dp++=a[i+0]; *dp++=a[i+5]; *dp++=a[i+6];
      *dp++=a[i+0]; *dp++=a[i+6]; *dp++=a[i+7];
      curIndex+=6*3;
    } else if(len==16)
    {
      // 16-sided polygon; used in Carrera's SKBOX.wrl for example
      // Split into 14 triangles, the easy way
      dindex *dp=&d[curIndex];
      *dp++=a[i+0]; *dp++=a[i+1]; *dp++=a[i+2];
      *dp++=a[i+0]; *dp++=a[i+2]; *dp++=a[i+3];
      *dp++=a[i+0]; *dp++=a[i+3]; *dp++=a[i+4];
      *dp++=a[i+0]; *dp++=a[i+4]; *dp++=a[i+5];
      *dp++=a[i+0]; *dp++=a[i+5]; *dp++=a[i+6];
      *dp++=a[i+0]; *dp++=a[i+6]; *dp++=a[i+7];

      *dp++=a[i+0]; *dp++=a[i+7]; *dp++=a[i+8];
      *dp++=a[i+0]; *dp++=a[i+8]; *dp++=a[i+9];
      *dp++=a[i+0]; *dp++=a[i+9]; *dp++=a[i+10];
      *dp++=a[i+0]; *dp++=a[i+10]; *dp++=a[i+11];
      *dp++=a[i+0]; *dp++=a[i+11]; *dp++=a[i+12];
      *dp++=a[i+0]; *dp++=a[i+12]; *dp++=a[i+13];
      *dp++=a[i+0]; *dp++=a[i+13]; *dp++=a[i+14];
      *dp++=a[i+0]; *dp++=a[i+14]; *dp++=a[i+15];
      curIndex+=14*3;
    } else
    { qwarn("DGeodImportVRML; non-3/4/6/8/16-sided polygon found (%d sides)",
        len);
    }
    i+=len;
  }
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
#endif

static int Split(string txt,int *v0,int *v1,int *v2)
// Split '1/2/3' -> v[3]
// May miss the slashes, for example '1//3' -> 1,0,3
// Returns 0 (in the future, the number of args found?)
{
//qdbg("Split(%s)\n",txt);
  string s;
  int   *va[3]={ v0,v1,v2 };
  int    n=0;
  
  for(s=strtok(txt,"/");s;s=strtok(0,"/"))
  {
    *va[n++]=atoi(s);
  }
  return 0;
}
static bool DGeobImportWAVE(DGeob *g,QFile *f)
// Read a geometry object.
{
  DBG_C_FLAT("DGeobImportWAVE")
  
  qstring ns;
  char    cmd[256],buf[256];
  int     i,j,k;
  int     n,lastOffset;
  float   x,y,z;
  float   cr,cg,cb;
  int     v1,v2,v3;
  float   vf[16];
  int     v[16];          	// Large polygons are possible
  float  *ptrVertex=0;
  DMaterial *curMat=0;                    // Current material being edited
  int        curMatIndex;
  // Temporary loading
  int     countOcc[4];                    // Counts v,vt,vn,f lines
  dfloat *tempNormal=0;
  dfloat *tempTVertex=0;
  dfloat *tempVertex=0;
  int     tempVertices,tempNormals,tempTVertices;
  dindex *tempVertexIndex=0,*tempNormalIndex=0,*tempTexIndex=0;
  int     curIndex,lastIndex;
  
//qdbg("DGeobImportWAVE()\n");
  
  while(1)
  {
    lastOffset=f->Tell();
    f->GetLine(cmd,sizeof(cmd));
qdbg("Geob cmd='%s'\n",cmd);

    // End shape loading by EOF (no more shapes) or other shape
    if(f->IsEOF())break;
    if(cmd[0]=='#')continue;
    if(cmd[0]=='g'&&cmd[1]==' ')
    {
      // Next object coming up
      f->Seek(lastOffset);
      break;
    } else if(cmd[0]=='v'&&cmd[1]==' ')
    { 
      // Vertices
      f->Seek(lastOffset);
      n=CountElements(f,"v ");
qdbg("%d vertices.\n",n);
      tempVertex=(dfloat*)qcalloc(n*3*sizeof(dfloat));
      tempVertices=n;
      for(i=0;i<n;i++)
      {
        f->GetLine(cmd,sizeof(cmd));
        sscanf(cmd,"%s %f %f %f",buf,&vf[0],&vf[1],&vf[2]);
        tempVertex[i*3+0]=vf[0];
        tempVertex[i*3+1]=vf[1];
        tempVertex[i*3+2]=vf[2];
qdbg("  v[%d]=%f, %f, %f\n",i,vf[0],vf[1],vf[2]);
      }
    } else if(cmd[0]=='v'&&cmd[1]=='t')
    { 
      // Texture vertices
      f->Seek(lastOffset);
      n=CountElements(f,"vt ");
qdbg("%d tvertices.\n",n);
      tempTVertex=(dfloat*)qcalloc(n*2*sizeof(dfloat));
      tempTVertices=n;
      for(i=0;i<n;i++)
      {
        f->GetLine(cmd,sizeof(cmd));
        sscanf(cmd,"%s %f %f",buf,&vf[0],&vf[1]);
        tempTVertex[i*2+0]=vf[0];
        tempTVertex[i*2+1]=vf[1];
qdbg("  vt[%d]=%f, %f\n",i,vf[0],vf[1]);
      }
    } else if(cmd[0]=='v'&&cmd[1]=='n')
    { 
      // Normals
      f->Seek(lastOffset);
      n=CountElements(f,"vn ");
qdbg("%d normals.\n",n);
      tempNormal=(dfloat*)qcalloc(n*3*sizeof(dfloat));
      tempNormals=n;
      for(i=0;i<n;i++)
      {
        f->GetLine(cmd,sizeof(cmd));
        sscanf(cmd,"%s %f %f %f",buf,&vf[0],&vf[1],&vf[2]);
        tempNormal[i*3+0]=vf[0];
        tempNormal[i*3+1]=vf[1];
        tempNormal[i*3+2]=vf[2];
qdbg("  vn[%d]=%f, %f, %f\n",i,vf[0],vf[1],vf[2]);      }
    } else if(cmd[0]=='f'&&cmd[1]==' ')
    { 
      // Faces
      f->Seek(lastOffset);
      n=CountElements(f,"f ");
qdbg("%d faces.\n",n);
      // Allocate worst-case polygons; let the optimizer do its work
      // later on
      g->indices=n*3;
      g->index=(dindex*)qcalloc(g->indices*sizeof(dindex));
      g->vertices=g->indices;
      g->tvertices=g->indices;
      g->normals=g->indices;
      g->vertex =(float*)qcalloc(sizeof(float)*g->indices*3);
      g->tvertex=(float*)qcalloc(sizeof(float)*g->indices*2);
      g->normal =(float*)qcalloc(sizeof(float)*g->indices*3);
      if(g->vertex==0||g->normal==0||g->tvertex==0)
      {nomem:
        qerr("DGeob:ImportVRML; not enough memory");
        goto fail;
      }
      
      // Copy vertices in
      for(i=0;i<n;i++)
      {
        // Get 'f' and points
        f->GetString(buf,sizeof(buf));
        for(k=0;k<3;k++)
        {
          // Get point def (for example "4/3/1")
          f->GetString(buf,sizeof(buf));
          Split(buf,&v[0],&v[1],&v[2]);
          // Convert to base0
          v[0]--; v[1]--; v[2]--;
//qdbg("v=%d/%d/%d\n",v[0],v[1],v[2]);
          if(v[0]>=tempVertices)
          {
            qwarn("Face vertex index %d > #vertices (%d)",v[0],tempVertices);
            v[0]=0;
          }
          // Copy vertices over
          for(j=0;j<3;j++)
          {
            g->vertex[i*9+k*3+j]=tempVertex[v[0]*3+j];
            if(tempNormal)
              g->normal[i*9+k*3+j]=tempNormal[v[2]*3+j];
          }
          // Copy tvertices over
          if(tempTVertex)
            for(j=0;j<2;j++)
              g->tvertex[i*6+k*2+j]=tempNormal[v[1]*2+j];
        }
      }
      // Create index array in the worst possible way
      for(i=0;i<g->indices;i++)
        g->index[i]=i;
    }
    

#ifdef OBS
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
      
    }
#endif

  }
  
  // WAVE geobs have only 1 material burst
  g->bursts=1;
  g->burstStart[0]=0;
  g->burstCount[0]=g->indices*3;
  g->burstVperP[0]=3;
  g->burstMtlID[0]=-1;
  
  // Info bursts
qdbg("Bursts=%d\n",g->bursts);
  for(i=0;i<g->bursts;i++)
  { qdbg("Burst @%d, count %d, mtlid %d\n",g->burstStart[i],
      g->burstCount[i],g->burstMtlID[i]);
  }

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

/******************
* Material import *
******************/
static bool DGeodeImportMTLLIB(DGeode *g,cstring fname)
// Read a material library file
{
  DBG_C_FLAT("DGeodeImportMTLLIB")
  
  qstring ns;
  char    cmd[256],buf[256];
  int     i,j,k;
  int     n,lastOffset;
  float   vf[16];
  int     v[16];
  DMaterial *curMat=0;                    // Current material being edited
  QFile *f;
  
qdbg("DGeodeImportMTLLIB(%s)\n",fname);
  
  f=new QFile(fname);
  if(!f->IsOpen())
  { 
    // Failed to open
    QDELETE(f);
    return FALSE;
  }

  while(1)
  {
    lastOffset=f->Tell();
    f->GetLine(cmd,sizeof(cmd));
    StripSpaces(cmd);
qdbg("MTL cmd='%s'\n",cmd);

    // End shape loading by EOF (no more shapes) or other shape
    if(f->IsEOF())break;
    if(cmd[0]=='#')continue;
    if(!strncmp(cmd,"newmtl",6))
    { 
      // New material
      f->Seek(lastOffset);
      f->GetString(ns);
      f->GetString(ns);
      if(g->materials==DGeode::MAX_MATERIAL)
      { qerr("Too many materials in WAVE, max=%d",DGeode::MAX_MATERIAL);
        break;
      }
      g->material[g->materials]=new DMaterial();
      curMat=g->material[g->materials];
      g->materials++;
    } else if(curMat)
    {
      if(cmd[0]=='K'&&cmd[1]=='a')
      {
        // Ambient
        sscanf(cmd,"%s %f %f %f",buf,&vf[0],&vf[1],&vf[2]);
        vf[3]=0;
        for(j=0;j<4;j++)
          curMat->ambient[j]=vf[j];
      } else if(cmd[0]=='K'&&cmd[1]=='d')
      {
        // Diffuse
        sscanf(cmd,"%s %f %f %f",buf,&vf[0],&vf[1],&vf[2]);
        vf[3]=0;
        for(j=0;j<4;j++)
          curMat->diffuse[j]=vf[j];
      } else if(cmd[0]=='K'&&cmd[1]=='s')
      {
        // Specular
        sscanf(cmd,"%s %f %f %f",buf,&vf[0],&vf[1],&vf[2]);
        vf[3]=0;
        for(j=0;j<4;j++)
          curMat->specular[j]=vf[j];
      }
    }
  }
  QDELETE(f);
  return TRUE;
}

/**************
* WAVE IMPORT *
**************/
bool DGeodeImportWAVE(DGeode *g,cstring fname)
// An importer for .OBJ files (WaveFront)
{
  DBG_C_FLAT("DGeodeImportWAVE")
  
  QFile *f;
  qstring ns;
  char cmd[256];
  int i;
  int lastOffset;
  float x,y,z;
  float cr,cg,cb;
  int v1,v2,v3;
  DMaterial *curMat=0;             // Current material being edited
  int curFace;
  
qdbg("ImportWAVE(%s)\n",fname);
  f=new QFile(fname);

  // File was found?
  if(!f->IsOpen())
  { //QTRACE("Can't import WAVE '%s'",fname);
    return FALSE;
  }

  // Search for vertices and information
  while(1)
  {
    lastOffset=f->Tell();
    f->GetLine(cmd,sizeof(cmd));
    if(f->IsEOF())break;
    if(cmd[0]=='#')
    { 
      // Comment
      continue;
    }
qdbg("cmd='%s'\n",cmd);
    if(!strncmp(cmd,"mtllib",6))
    {
      // Read material library file(s)
      f->Seek(lastOffset);
      // Read 'mtllib'
      f->GetString(ns);
      // Read filename
      f->GetString(ns);
      // Create lib filename in same directory as model
      strcpy(cmd,fname);
      cstring s=QFileBase(cmd);
      strcpy((char*)s,ns);
      // Read the library
      DGeodeImportMTLLIB(g,cmd);
    }
    if(cmd[0]=='g'&&cmd[1]==' ')
    {
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
      r=DGeobImportWAVE(o,f);
      if(!r)goto fail;
    }

      
#ifdef OBS
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
#endif
  }
  
  QDELETE(f);
  
qdbg("geobs=%d\n",g->geobs);
  return TRUE;
 fail:
  QDELETE(f);
  return FALSE;
}

