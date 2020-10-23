/*
 * DTPath; D3's path definition with splines for better animation
 * 06-08-1999: Created! (based on gppath.cpp)
 * NOTES:
 * - Supersedes GamePlanets' gppath.cpp
 * - This class connects an array of DTransformations to generate
 * flight paths for the D3 environment.
 * (C) MG/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/path.h>
#include <qlib/opengl.h>
#include <qlib/file.h>
#include <qlib/app.h>
#include <bstring.h>
DEBUG_ENABLE

/**********
* DT Path *
**********/
DTPath::DTPath()
{
  maxkey=100;
  keys=0;
  key=(DTKeyFrame*)qcalloc(maxkey*sizeof(DTKeyFrame));
  if(!key)qerr("DTPath; no mem");
}
DTPath::~DTPath()
{
  QFREE(key);
}

bool DTPath::CreateKey(int frame,DTransformation *state,int flags)
// Create a key for 'state' at the spec'd frame
{
  int i;
  // First see if the key already exists
  for(i=0;i<keys;i++)
    if(key[i].frame==frame)
    { // Found it! Copy in new state
      //qdbg("DTPath::CreateKey(); replace old key frame %d\n",frame);
      key[i].state=*state;
      key[i].flags=flags;
      return TRUE;
    }
  // Add the key in a new spot
  if(keys==maxkey)return FALSE;		// No space!
  //qdbg("DTPath::CreateKey(); new key @ frame %d\n",frame);
  key[keys].frame=frame;
  key[keys].flags=flags;
  key[keys].state=*state;
  keys++;
  SortKeys();
  return TRUE;
}
bool DTPath::RemoveKey(int frame)
// Returns TRUE if key succesfully removed.
// No need to sort afterwards.
{
  int i;
  // Find the keyframe
  for(i=0;i<keys;i++)
    if(key[i].frame==frame)
    { for(;i<keys-1;i++)
      { key[i]=key[i+1];
      }
      keys--;
      return TRUE;
    }
  return FALSE;
}
void DTPath::SortKeys()
{
  int i,j;
  DTKeyFrame t;
/*
  qdbg("DTPath::SortKeys(): before\n");
  for(i=0;i<keys;i++)
    qdbg("F%d(X%d) ",key[i].frame,key[i].state.x);
  qdbg("\n");
*/

  for(i=0;i<keys;i++)
  { for(j=i+1;j<keys;j++)
    { if(key[i].frame>key[j].frame)
      { t=key[i];
        key[i]=key[j];
        key[j]=t;
      }
    }
  }
/*
  qdbg("DTPath::SortKeys(): after\n");
  for(i=0;i<keys;i++)
    qdbg("F%d(X%d) ",key[i].frame,key[i].state.x);
  qdbg("\n");
*/
}
void DTPath::Reset()
{
  keys=0;
}

void DTPath::GetKeyIndexes(int frame,int *k1,int *k2,int *k3,int *k4)
// Method taken from class CMSpline
{
  int i,k;

#ifdef ND_CATMULL_EXTRACT
  // Get control values/points; make sure they exist
  if(i>0)p1=p[i-1]; else p1=p[0];
  p2=p[i];
  // Next points
  if(pType[i]==CMS_LINEAR)
  { // Linear knot
    //p3=p2; p4=p2;
    p1=p2;
  }
  // Curve
  if(i+1<points)
  { p3=p[i+1];
    if(pType[i+1]==CMS_LINEAR)
    { p4=p3;
      goto skip4;
    }
  } else p3=p[points-1];
  if(i+2<points)p4=p[i+2]; else p4=p[points-1];
#endif

  // Find keyframe that precedes 'frame'
  if(frame<=key[0].frame)
  { k=0;
  } else
  { for(i=1;i<keys;i++)
    { if(key[i].frame>=frame)
        break;
    }
    k=i-1;
  }
  // Deduce previous point
  if(k>0)*k1=k-1; else *k1=k;
  // Current point
  *k2=k;
  // Next points
  if(key[k].flags&LINEAR)
  { // Linear knot
    *k1=k;
  }
  if(k+1<keys)
  { *k3=k+1;
    if(key[k].flags&LINEAR)
    { *k4=*k3;
      return;
    }
  } else *k3=keys-1;
  if(k+2<keys)*k4=k+2; else *k4=keys-1;
}

dfloat DTPath::CalcCM(dfloat p1,dfloat p2,dfloat p3,dfloat p4,
  dfloat t,dfloat t2,dfloat t3)
{
  return -.5*t3*p1+1.5*t3*p2-1.5*t3*p3+.5*t3*p4
     +t2*p1-2.5*t2*p2+2*t2*p3-.5*t2*p4
     -.5*t*p1+.5*t*p3
     +p2;
}

void DTPath::GetState(int frame,DTransformation *state)
// Retrieve the state at frame 'frame'
// Interpolates between keyframes, and always comes up with an answer
{
  int i,n,d;
  if(!keys)
  { // No decidable state; use default
   def:
    //qdbg("DTPath::GetState(); default\n");
    state->Reset();
    return;
  } else if(keys==1)
  { // Only 1 keyframe; not much choice
   use_first:
    //qdbg("DTPath::GetState(); first key\n");
    *state=key[0].state;
    return;
  }

  // Negative frame reset to start (first keyframe)
  if(frame<0)frame=0;

  int k1,k2,k3,k4;			// Key numbers for CM spline
  DTransformation *p[4];
  // Retrieve key indexes to spline between
  GetKeyIndexes(frame,&k1,&k2,&k3,&k4);

  if(frame>key[keys-1].frame)
  { // Right of defined range; use last
    *state=key[keys-1].state;
    return;
  }
 
//qdbg("DTPath:GetState(frame %d); key indexes %d,%d,%d,%d\n",frame,k1,k2,k3,k4);

  // Calculate distance across segment
  d=key[k3].frame-key[k2].frame;
  dfloat t,t2,t3;
  if(!d)t=0;
  else  t=((dfloat)(frame-key[k2].frame))/d;
//qdbg("progress in [%d,%d] (frame %d) is %f\n",key[k1].frame,key[k2].frame,frame,t);
  t2=t*t; t3=t2*t;
  p[0]=&key[k1].state; p[1]=&key[k2].state;
  p[2]=&key[k3].state; p[3]=&key[k4].state;

  // Interpolate parameters using splines
  *state=key[k2].state;
  state->x=CalcCM(p[0]->x,p[1]->x,p[2]->x,p[3]->x,t,t2,t3);
  state->y=CalcCM(p[0]->y,p[1]->y,p[2]->y,p[3]->y,t,t2,t3);
  state->z=CalcCM(p[0]->z,p[1]->z,p[2]->z,p[3]->z,t,t2,t3);
  state->xa=CalcCM(p[0]->xa,p[1]->xa,p[2]->xa,p[3]->xa,t,t2,t3);
  state->ya=CalcCM(p[0]->ya,p[1]->ya,p[2]->ya,p[3]->ya,t,t2,t3);
  state->za=CalcCM(p[0]->za,p[1]->za,p[2]->za,p[3]->za,t,t2,t3);
  state->xs=CalcCM(p[0]->xs,p[1]->xs,p[2]->xs,p[3]->xs,t,t2,t3);
  state->ys=CalcCM(p[0]->ys,p[1]->ys,p[2]->ys,p[3]->ys,t,t2,t3);
  state->zs=CalcCM(p[0]->zs,p[1]->zs,p[2]->zs,p[3]->zs,t,t2,t3);
  // Don't interpolate poly size
  //...
  //qdbg("  cm from %d,%d,%d,%d = %d\n",p[0]->x,p[1]->x,p[2]->x,p[3]->x,state->x);
#ifdef OBS_LINEAR
  // Linear interpolation
  if(frame<=key[0].frame)goto use_first;	// Left of defined range
  for(i=1;i<keys;i++)
  { if(key[i].frame>=frame)
      break;
  }
  if(i==keys)
  { // Right of defined range; use last
    *state=key[keys-1].state;
    return;
  }
  // Frame is now between key[i-1] and key[i]. i>0
  if(key[i].frame==frame)
  { // Right on the spot
    *state=key[i].state;
    return;
  }
  // Interpolate between key[i-1] and key[i]
//printf("ipolate between frame %d and frame %d\n",key[i-1].frame,key[i].frame);
  d=key[i].frame-key[i-1].frame;
  n=frame-key[i-1].frame;
//printf("d=%d, n=%d\n",d,n);
  // d = distance between keys, n = n/d on the road
  DTransformation *pi1,*pi2;
  *state=key[i-1].state;		// Remove when all interpol done
  pi1=&key[i-1].state; pi2=&key[i].state;
  state->x=pi1->x+n*(pi2->x-pi1->x)/d;
  state->y=pi1->y+n*(pi2->y-pi1->y)/d;
  state->z=pi1->z+n*(pi2->z-pi1->z)/d;
#ifdef USE_QUAT
  for(i=0;i<4;i++)
  { state->quat[i]=pi1->quat[i]+n*(pi2->quat[i]-pi1->quat[i])/d;
  }
#else
  state->xa=pi1->xa+n*(pi2->xa-pi1->xa)/d;
  state->ya=pi1->ya+n*(pi2->ya-pi1->ya)/d;
  state->za=pi1->za+n*(pi2->za-pi1->za)/d;
#endif
#endif
}

int DTPath::GetPrevKeyFrame(int frame)
// Get keyframe number BEFORE 'frame'
{
  int i;
  if(!keys)return 0;
  for(i=keys-1;i>=0;i--)
  { if(key[i].frame<frame)
      return key[i].frame;
  }
  return key[0].frame;
}
int DTPath::GetNextKeyFrame(int frame)
// Get keyframe number AFTER 'frame'
{
  int i;
  if(!keys)return 0;
  for(i=0;i<keys;i++)
  { if(key[i].frame>frame)
      return key[i].frame;
  }
  // Otherwise return last keyframe
  return key[keys-1].frame;
}
bool DTPath::IsKeyFrame(int frame)
// Is the frame a keyframe?
{
  int i;
  for(i=0;i<keys;i++)
  { if(key[i].frame==frame)
      return TRUE;
  }
  return FALSE;
}
int DTPath::GetKeyFrameFlags(int frame)
// Returns flags of key on frame 'frame'
{
  int i;
  for(i=0;i<keys;i++)
  { if(key[i].frame==frame)
      return key[i].flags;
  }
  return 0;
}
void DTPath::SetKeyFrameFlags(int frame,int flags)
// Set flags of key on frame 'frame'
{
  int i;
  for(i=0;i<keys;i++)
  { if(key[i].frame==frame)
      key[i].flags=flags;
  }
}
int DTPath::GetMaxKeyFrame()
// Get last frame for which a key is defined
{
  if(!keys)return 0;
  return key[keys-1].frame;		// Sorted, so take last key def
}

void DTPath::Reverse()
// Reverses the path
{
  int i;
  int length;

  // Get length of path
  length=GetMaxKeyFrame();
  for(i=0;i<keys;i++)
  { key[i].frame=length-key[i].frame;
  }
  // Shuffle all keys to order it right again
  SortKeys();
}

void DTPath::ScaleKeys(int num,int den,int from,int to)
// Scale the path
// Currently only a full scale is supported, not a range
// (must shift keys beyond 'to' etc)
{
  if(from!=-1||to!=-1)
  { qerr("DTPath::ScaleKeys() doesn't support from/to args yet");
    return;
  }
  int i;

  // Scale every key
  for(i=0;i<keys;i++)
  { key[i].frame=key[i].frame*num/den;
  }
}

void DTPath::ShiftKeys(int n,int from,int to)
// Shift keys a number of frames
// 'to' arg is not implemented yet
{
  if(to!=-1)
  { qerr("DTPath::ShiftKeys() doesn't support to arg yet");
    return;
  }
  int i;

  // Shift every key
  for(i=0;i<keys;i++)
  { if(key[i].frame>=from)
      key[i].frame+=n;
  }
}

void DTPath::SetScaleID()
{
  int i;
  for(i=0;i<keys;i++)
  {
    key[i].state.xs=1;
    key[i].state.ys=1;
    key[i].state.zs=1;
  }
}

/******
* I/O *
******/
bool DTPath::Save(string fname)
{
  QFile *f;
  int n;

  f=new QFile(fname,QFile::WRITE);
  if(!f->IsOpen())
  { QDELETE(f); return FALSE; }
  n=DTPATH_VERSION;
  f->Write(&n,sizeof(n));
  n=keys;
  f->Write(&n,sizeof(n));
  int i;
  for(i=0;i<keys;i++)
  { f->Write(&key[i],sizeof(key[i]));
  }
  QDELETE(f);
  return TRUE;
}
bool DTPath::Load(string fname)
{
  QFile *f;
  int n,i,version;

  f=new QFile(fname);
  if(!f->IsOpen())
  { QDELETE(f); return FALSE; }

  // Version
  f->Read(&n,sizeof(n));
  version=n;
  if(n!=DTPATH_VERSION)
  { 
    // Accept older known versions
    if(n==DTPATH_VERSION_FRAMES)
      goto ver_ok;
    qwarn("DTPath::Load(); wrong version (%d instead of %d)",n,DTPATH_VERSION);
    QDELETE(f); return FALSE;
  }
 ver_ok:

  // Load keys
  f->Read(&n,sizeof(n));
  keys=n;
  if(keys<0)keys=0; else if(keys>maxkey)keys=maxkey;
  for(i=0;i<keys;i++)
  { f->Read(&key[i],sizeof(key[i]));
  }
  QDELETE(f);

  // Post-load
  if(version==DTPATH_VERSION_FRAMES)
  { // This version used frames as a timebase, now using fields
    qwarn("Old flight file found; converting from frames to fields");
    ScaleKeys(2);
  }
  return TRUE;
}
