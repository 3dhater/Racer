/*
 * RSkidMarkManager - manages all skid marks on a track
 * 07-10-01: Created!
 * 13-12-01: Support for replay skidding; forcing strips to be used
 * (c) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/texture.h>
DEBUG_ENABLE

// Minimum distance to next skid point
#define MIN_DIST_SQUARED    0.01

RSkidMarkManager::RSkidMarkManager()
{
  int i;

  maxPoint=RMGR->GetGfxInfo()->GetInt("fx.skid_mark_buffer");
  if(!RMGR->GetGfxInfo()->GetInt("fx.skid_mark_enable"))
    maxPoint=0;
  if(maxPoint)
  {
//qdbg("SMM: %d points per strip\n",maxPoint);
    // Allocate arrays
    for(i=0;i<MAX_STRIP;i++)
    {
      //point[i]=new DVector3[maxPoint];
      stripV[i]=(float*)qcalloc(2*3*maxPoint*sizeof(float));
      //if(point[i]==0||stripV[i]==0)
      if(stripV[i]==0)
      {
        qwarn("RSkidMarkManager: out of memory");
        //QFREE(point[i]);
        QFREE(stripV[i]);
        maxPoint=0;
        break;
      }
    }
  }
  // Init member variables
  strips=0;
  texture=0;
  nextStrip=0;
  for(i=0;i<MAX_STRIP;i++)
  {
    stripStart[i]=0;
    stripSize[i]=0;
    stripInUse[i]=FALSE;
  }

  // Get skid texture
  DBitMapTexture *tbm;
  QImage img("data/images/skidmark.bmp");
  if(img.IsRead())
  {
    tbm=new DBitMapTexture(&img);
    texture=tbm;
  }
}
RSkidMarkManager::~RSkidMarkManager()
{
  int i;
  for(i=0;i<MAX_STRIP;i++)
  {
    //QFREE(point[i]);
    QFREE(stripV[i]);
  }
  QDELETE(texture);
}

void RSkidMarkManager::Clear()
// Clear all skid mark strips
{
  int i;

  strips=0;
  // As 'strips' isn't actually used, clear all strips the hard way
  for(i=0;i<MAX_STRIP;i++)
  {
    stripInUse[i]=0;
    stripSize[i]=0;
  }
}

/*******************
* Strip generation *
*******************/
int RSkidMarkManager::StartStrip()
// Begin a skidmark strip
{
  int n;
  int min=9999,minIndex=0,i;

  // Get a new strip
  for(i=0;i<MAX_STRIP;i++)
  {
    // Don't use strips that are currently being worked on
    // (otherwise a wheel might think it still can add to this strip
    // for example)
    if(stripInUse[i])
      continue;
    if(stripSize[i]<min)
    {
      minIndex=i; min=stripSize[i];
#ifdef ND_USE0
      // Almost empty? Then use it.
      if(stripSize[i]<2)
        break;
#endif
    }
  }
  n=minIndex;
#ifdef OBS
qdbg("RSMM:StartStrip %d (size %d)\n",n,stripSize[n]);
for(i=0;i<MAX_STRIP;i++)
{if(stripSize[i]<stripSize[n])
  qdbg("%d size %d, inuse %d\n",i,stripSize[i],stripInUse[i]);
}
#endif
  stripSize[n]=0;
  stripStart[n]=0;
  stripInUse[n]=TRUE;

#ifdef OBS
  // Find next strip to use for a future call
  if(++nextStrip>=MAX_STRIP)
    nextStrip=0;
#endif

  // Return strip to use
  return n;
}
void RSkidMarkManager::StartStripForced(int n)
// For replays; force the use of a skid strip. Otherwise complex things
// happen when replaying back halfway through a replay.
// This ain't perfect, but hopefully nice enough.
{
  // Avoid overflows in case of different Racer versions using a different
  // maximum amount of strips.
  if(n>=MAX_STRIP)
    n=MAX_STRIP-1;

  stripSize[n]=0;
  stripStart[n]=0;
  stripInUse[n]=TRUE;
}
int RSkidMarkManager::AddPoint(int n,DVector3 *v)
// A skid mark point is submitted.
// Returns the strip number. This strip number MAY CHANGE if the current
// strip is full and another one needs to be started
{
  // Avoid overflows in case of different Racer versions using a different
  // maximum amount of strips.
  if(n>=MAX_STRIP)
    n=MAX_STRIP-1;

  DVector3 *lastV=0,vSide,vDiff,vUp(0,1,0);
  int size=stripSize[n];

//qdbg("RSMM:AddPoint(strip %d)\n",n);
//v->DbgPrint("v");
  if(size>=maxPoint)
  {
    // Start a new strip
//qdbg("Start new strip!\n");
    stripInUse[n]=FALSE;
    n=StartStrip();
    size=0;
  } else if(size>0)
  {
    //lastV=&point[n][size-1];
    lastV=&stripLastV[n];
    // Check distance to last skid point (must be at least some distance away)
    if(lastV->SquaredDistanceTo(v)<MIN_DIST_SQUARED)
    {
      // Too close
//qdbg("  too close\n");
      return n;
    }
  }

//if(lastV)lastV->DbgPrint("lastV");
  // Add point
  //*newV=*v;

  if(size>0)
  {
    // Get side vector by crossing (last-new) and up vector
    vDiff.x=v->x-lastV->x;
    vDiff.y=v->y-lastV->y;
    vDiff.z=v->z-lastV->z;
    vSide.Cross(&vDiff,&vUp);
    // Create points on both sides
    vSide.Normalize();
    vSide.Scale(0.1);
#ifdef OBS
    // Loan vDiff
    vDiff=*lastV;
    vDiff.Add(&vSide);
#endif
//qdbg("side0=%f,%f,%f\n",lastV->x+vSide.x,lastV->y+vSide.y,lastV->z+vSide.z);
//vSide.DbgPrint("vSide");
    size--;
    stripV[n][size*2*3+0]=lastV->x+vSide.x;
    stripV[n][size*2*3+1]=lastV->y+vSide.y;
    stripV[n][size*2*3+2]=lastV->z+vSide.z;
    stripV[n][size*2*3+3]=lastV->x-vSide.x;
    stripV[n][size*2*3+4]=lastV->y-vSide.y;
    stripV[n][size*2*3+5]=lastV->z-vSide.z;
    size++;

  }
  // Use this new position for the next hook point
  stripLastV[n]=*v;
  stripSize[n]++;

  // Return most current strip number
  return n;
}
void RSkidMarkManager::EndStrip(int n)
{
  // Avoid overflows in case of different Racer versions using a different
  // maximum amount of strips.
  if(n>=MAX_STRIP)
    n=MAX_STRIP-1;

//qdbg("EndStrip(%d)\n",n);
  // Mark strip is out of use, so a new strip may reuse this one
  stripInUse[n]=FALSE;
}
void RSkidMarkManager::EndAllStrips()
// Stops all strips. This is used when a replay ends, to make sure
// no skid strips is in the making (possible creating a skidmark to
// the restored car state)
{
  int i;
  for(i=0;i<MAX_STRIP;i++)
    EndStrip(i);
}

/********
* Paint *
********/
void RSkidMarkManager::Paint()
{
  int i,j;
  int count=0;
//qdbg("RSMM:Paint()\n");

  //glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(.2,.2,.2,.4);

  glPolygonOffset(-1,-1);
  glEnable(GL_POLYGON_OFFSET_FILL);

  for(i=0;i<MAX_STRIP;i++)
  {
    if(stripSize[i]<=0)continue;
    count+=stripSize[i];
#ifdef OBS
    glBegin(GL_POINTS);
    for(j=0;j<stripSize[i];j++)
    {
      glVertex3f(point[i][j].x,point[i][j].y,point[i][j].z);
    }
    glEnd();
#endif

    //glVertexPointer(3,GL_FLOAT,0,stripV[i]);
    //glDrawArrays(GL_TRIANGLE_STRIP,0,stripSize[i-1]);

//#ifdef OBS_HAND_FEED
    glBegin(GL_TRIANGLE_STRIP);
    for(j=0;j<stripSize[i]-1;j++)
    {
//qdbg("RSMM: v(%f,%f,%f)\n",stripV[i][j*2*3+0],stripV[i][j*2*3+1],stripV[i][j*2*3+2]);
//qdbg("RSMM: v2(%f,%f,%f)\n",stripV[i][j*2*3+3],stripV[i][j*2*3+4],stripV[i][j*2*3+5]);
      glVertex3fv(&stripV[i][j*2*3+0]);
      glVertex3fv(&stripV[i][j*2*3+3]);
    }
    glEnd();
//#endif
  }
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDepthMask(GL_TRUE);
//qdbg("RSMM: %d total strip count\n",count);
}

