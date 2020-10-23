/*
 * RReplay
 * 22-02-01: Created!
 * NOTES:
 * - Used for generic and ghost car replays
 * - Uses variable-sized frames for replay events within the time of the frame.
 * - 'rewindFrame' is used to indicate a ptr at which point we should rewind
 * the replay pointer to the start of the buffer. While rewindFrame=0, the
 * replay hasn't been completely filled yet (not rewound).
 * BUGS:
 * - Probably doesn't work on non-Intel due to endianess problems.
 * TODO:
 * - Doesn't store wheel rotZ/Y and suspLen[] yet
 * - Replay back using replay
 * - Ghost car using RMGR->replayGhost
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#define DEFAULT_BUFFERSIZE    5000000
#define CARNAME_LEN           64          // For storing car names

RReplay::RReplay()
{
  // Default buffer
  buffer=0;
  bufferSize=DEFAULT_BUFFERSIZE;
  firstFrame=currentFrame=currentData=rewindFrame=0;
  Reset();
}
RReplay::~RReplay()
{
  Destroy();
}

/*********
* Create *
*********/
bool RReplay::Create()
// Create the replay buffer
{
  Destroy();
  bufferSize=RMGR->GetMainInfo()->GetInt("replay.size",1000000);
//qdbg("Replay buffer size=%dK\n",bufferSize/1024);
  buffer=qcalloc(bufferSize);
  if(!buffer)return FALSE;

  // Start filling 'er up
  Reset();

  return TRUE;
}
void RReplay::Destroy()
{
  QFREE(buffer);
  // Avoid frames being entered
  firstFrame=currentFrame=currentData=0;
}

/**********
* Attribs *
**********/
void RReplay::SetSize(int size)
// Declare the size of the replay buffer
{
  if(buffer)
  { qwarn("RReplay::SetSize() called AFTER creation");
    Destroy();
  }
  bufferSize=size;
}
int RReplay::GetReplaySize()
// Return the replay memory size
{
  if(currentFrame>firstFrame)
  {
    // Easy calculation
    return (char*)currentFrame-(char*)firstFrame;
  } else
  {
    // Split into two parts
    return 0;
  }
}

/*******
* Info *
*******/
int RReplay::GetFirstFrameTime()
// Returns the time (in millisecs) for the first frame of the replay
{
  // Find first time in replay
  void *frame=GetFirstFrame();
  if(!frame)return 0;
  return GetFrameTime(frame);
}
int RReplay::GetLastFrameTime()
// Returns the time (in millisecs) for the LAST frame of the replay.
// Note this is an EXPENSIVE function; don't use it in inner loops.
{
  void *frame,*prevFrame=0;
  bool  first=TRUE;

  // Find last frame in replay
  for(frame=GetFirstFrame();frame;frame=GetNextFrame(frame))
  {
//qdbg("Find frame of time %d\n",RMGR->replay->GetFrameTime(frame));
    // Skip on zero-timed frame
    if(!first)
    { if(RMGR->replay->GetFrameTime(frame)==0)break;
    } else first=FALSE;
    prevFrame=frame;
  }
//qdbg("frame=%p, prevFrame=%d\n",frame,prevFrame);
//qdbg("  time of prevFrame=%d\n",RMGR->replay->GetFrameTime(prevFrame));
  // Was there a previous frame?
  if(!prevFrame)return 0;
  // Return time of last frame
  return GetFrameTime(prevFrame);
}

/*********
* Chunks *
*********/
void RReplay::Reset()
// Restart at front of the buffer (empty buffer)
{
  firstFrame=currentFrame=buffer;
  currentData=currentFrame;
  rewindFrame=0;
  playFrame=playNextFrame=0;
  ghostFirstFrame=ghostLastFrame=0;
  lastLapFirstFrame=lastLapLastFrame=0;
  flags=0;
  playFrameTime=lastPlayTime=0;
}
void RReplay::AddChunk(const void *p,int size)
// Add a chunk of bytes to the buffer
{
//qdbg("AddChunk(size=%d); [currentData]=%d\n",
//size,(char*)currentData-(char*)buffer);
  // Check if frame was started
  if(!(flags&FRAME_STARTED))
  {
    // Auto-start frame
    BeginFrame();
  }

  // Are we overflowing the replay buffer?
  // This piece of code assumes the replay buffer can at least hold
  // some 2 frames of data, which should normally be true. For this,
  // a minimum replay buffer size may be present.
  if((char*)currentData+size>(char*)buffer+bufferSize)
  {
//qdbg("  replay buffer overflow\n");
    // Restart this frame at the start of the replay (this only
    // happens when the replay buffer overflows so almost never).
    int curSize;
    curSize=(char*)currentData-(char*)currentFrame;
    // We're going to overwrite the first frame(s), so advance the first
    // frame until enough space is created to move the last frame (at curFrame)
    // to the start of the replay buffer.
    // This should not loop infinitely in practice.
//qdbg("  firstFrame was %p\n",firstFrame);
    while((char*)firstFrame-(char*)buffer<curSize&&firstFrame!=0)
      firstFrame=GetNextFrame(firstFrame);
//qdbg("  firstFrame corrected to [%d] (buffer=%p)\n",
//(char*)firstFrame-(char*)buffer,buffer);
    // Remind that this is the position to rewind back to the start.
    rewindFrame=currentFrame;
    // Move the current frame over to its new location.
    // Copy the current frame's contents to the start of the replay buffer
    memcpy(buffer,currentFrame,curSize);
    currentFrame=buffer;
    currentData=(char*)buffer+curSize;
//qdbg("  new curFrame=%p, curData=%p\n",currentFrame,currentData);
    // Continue writing at the frame's new location...
  }

  // Are we overwriting any existing frame(s)?
//qdbg("Check curData=%p > firstFrame=%p, currentFrame=%p (<firstFrame?)\n",
//currentData,firstFrame,currentFrame);
  if((char*)currentData+size>=firstFrame&&rewindFrame!=0)
  {
//qdbg("  firstFrame must be pushed back\n");
    // We're about to overwrite the first replay frame
//qdbg("currentFrame=%p, rewindFrame=%p\n",currentFrame,rewindFrame);
//int c=0;
    while((char*)currentData+size>(char*)firstFrame&&firstFrame!=0)
    {
//qdbg("GetNextFrame(%p)=%p\n",firstFrame,GetNextFrame(firstFrame));
//if(++c==50)break;
      firstFrame=GetNextFrame(firstFrame);
    }
    // Now what if 'firstFrame' overflows? This means we're about
    // to overwrite the last frame in the replay buffer, and we must start
    // from the top of the replay buffer again.
    if(!firstFrame)
    {
//qdbg("  firstFrame totally reset to start of replay buffer\n");
      firstFrame=buffer;
    }
  }

  // Store the chunk
  memcpy(currentData,p,size);
  currentData=((char*)currentData)+size;
}
void RReplay::AddShort(short n)
// Convenience function to add a short
{
  AddChunk(&n,sizeof(short));
}

/**********
* RECORDS *
**********/
#ifdef OBS
void RReplay::AddHeaderRecord()
// Add a timing record for a new timeframe
{
  RReplayHeaderRecord rec;
  rec.type=RRT_TIME;
  rec.time=RMGR->time->GetSimTime();
  AddChunk(&rec,sizeof(rec));
}
void RReplay::AddCarRecord(RCar *car)
// Add replay data for the car
{
  RReplayCarRecord rec;
  int i;

  rec.type=RRT_CAR;
  rec.index=car->GetIndex();
  // Controls are all stored in 0..250 (controls vary from 0..1000)
  rec.steering=(RMGR->controls->control[RControls::T_STEER_LEFT]->value-
    RMGR->controls->control[RControls::T_STEER_RIGHT]->value)/4;
  rec.throttle=RMGR->controls->control[RControls::T_THROTTLE]->value/4;
  rec.brakes=RMGR->controls->control[RControls::T_BRAKES]->value/4;
  rec.clutch=RMGR->controls->control[RControls::T_CLUTCH]->value/4;
  // Copy body orientation
  rec.quat.x=car->GetBody()->GetRotPosQ()->x;
  rec.quat.y=car->GetBody()->GetRotPosQ()->y;
  rec.quat.z=car->GetBody()->GetRotPosQ()->z;
  rec.quat.w=car->GetBody()->GetRotPosQ()->w;
  // Copy body position
  rec.pos=*car->GetBody()->GetLinPos();
  rec.rpm=car->GetEngine()->GetRPM();
  // Wheels
  for(i=0;i<car->GetWheels();i++)
  { rec.wRotZ[i]=0;
    rec.wRotY[i]=0;
  }
  // Suspensions
  for(i=0;i<car->GetWheels();i++)
  {
    rec.suspLen[i]=0;
  }
  AddChunk(&rec,sizeof(rec));
}
void RReplay::AddSceneRecord(RScene *scene)
// Add replay data for all the things in the scene
{
  int i;
  // Add all cars
  for(i=0;i<scene->GetCars();i++)
    AddCarRecord(scene->GetCar(i));
}
#endif

void RReplay::BeginFrame(int t)
// Start recording a replay frame.
// If 't'=-1, then the current simulation time will be put in the frame
// header. If not, then 't' is used instead. This is used in RScene to
// prerecord the next frame's time (in advance), so that statistics may
// read out the total time length of the replay, for example, without
// the need to search BACK for the previously recorded frame.
// The frame IS empty though (and not stored when saved).
{
//qdbg("RReplay:BeginFrame(); t=%d\n",RMGR->time->GetSimTime());

  // Mark start BEFORE entering data
  flags|=FRAME_STARTED;

  // Insert placeholder for the frame length
  AddShort(0);
  // Record the time of this frame's contents
  if(t==-1)t=RMGR->time->GetSimTime();
  AddChunk(&t,sizeof(t));
}
void RReplay::EndFrame()
{
//qdbg("RReplay:EndFrame()\n");
  unsigned short len;

  if(!(flags&FRAME_STARTED))
  {
    // Nothing done
    return;
  }

  // Close frame
  len=(char*)currentData-(char*)currentFrame;
//qdbg("  replay frame len=%d\n",len);
  if(len<=sizeof(short)+sizeof(int))
  {
    // Nothing was actually written; cancel header
    currentData=currentFrame;
  } else
  {
    // Set frame length now that it's done
    *(short*)currentFrame=len;
  }
  flags&=~FRAME_STARTED;

  // Prepare for the next frame
  currentFrame=currentData;
//qdbg("Replay size: %dK\n",((char*)currentFrame-(char*)firstFrame)/1024);
}

/************************
* Recording convenience *
************************/
void RReplay::RecSetCar(int id)
// Set the current car to 'id'
{
//qdbg("RReplay:RecSetCar(%d)\n",id);
  short a[2]={ RC_SETCAR,id };
  AddChunk(a,sizeof(a));
}
void RReplay::RecCarPosition(RCar *car)
// Record car position. Obsolete; use RecCar() instead.
{
//qdbg("RReplay:RecCarPosition(%p)\n",car);
  AddShort(RC_CARPOSITION);
  AddChunk(car->GetPosition(),sizeof(DVector3));
}
void RReplay::RecCar(RCar *car)
// Record car most used attributes that probably change every replay frame
// Position, orientation, susp Y values, wheel rotX and rotX speed, steering
{
  int   i;
  float v[RCar::MAX_WHEEL];
  int   wheels=car->GetWheels();

//qdbg("RReplay:RecCar(%p)\n",car);
  AddShort(RC_CAR);
  AddChunk(car->GetPosition(),sizeof(DVector3));
  AddChunk(car->GetBody()->GetRotPosQ(),sizeof(DQuaternion));
  v[0]=car->GetSteer()->GetAngle();
  AddChunk(&v[0],sizeof(float));
  for(i=0;i<wheels;i++)
    //v[i]=car->GetSuspension(i)->GetLength();
    v[i]=car->GetWheel(i)->GetPosition()->y;
  AddChunk(v,sizeof(float)*wheels);
  for(i=0;i<wheels;i++)
    v[i]=car->GetWheel(i)->GetRotation();
  AddChunk(v,sizeof(float)*wheels);
  for(i=0;i<wheels;i++)
  {
    v[i]=car->GetWheel(i)->GetRotationV();
//qdbg("RReplay:RecCar(); wheel vel %d = %.3f\n",i,v[i]);
  }
  AddChunk(v,sizeof(float)*wheels);
  v[0]=car->GetEngine()->GetRPM();
  AddChunk(&v[0],sizeof(float));
  v[0]=car->GetVelocity()->Length();
  AddChunk(&v[0],sizeof(float));
}
//
// Smoke
//
void RReplay::RecSmoke(DVector3 *pos,DVector3 *vel,dfloat lifeTime)
{
  RReplaySmokeRecord rec;
  rec.pos=*pos;
  rec.vel=*vel;
  rec.lifeTime=lifeTime;
  AddShort(RC_SMOKE);
  AddChunk(&rec,sizeof(rec));
}
//
// Skidmarks
//
void RReplay::RecSkidStart(int n)
{
  short ns=n;
  AddShort(RC_SKID_START);
  AddChunk(&ns,sizeof(ns));
}
void RReplay::RecSkidPoint(int n,DVector3 *pos)
{
  short ns=n;

  AddShort(RC_SKID_POINT);
  AddChunk(&ns,sizeof(ns));
  AddChunk(pos,sizeof(*pos));
}
void RReplay::RecSkidStop(int n)
{
  short ns=n;
  AddShort(RC_SKID_STOP);
  AddChunk(&ns,sizeof(ns));
}
void RReplay::RecNewCar(cstring carName,int carIndex)
// New car enters field
{
  char buf[CARNAME_LEN];

  memset(buf,0,sizeof(buf));
  strncpy(buf,carName,sizeof(buf)-1);
  AddShort(RC_NEWCAR);
  AddChunk(buf,sizeof(buf));
  AddChunk(&carIndex,sizeof(carIndex));
}

/***********
* Playback *
***********/
void *RReplay::GetNextFrame(void *frame)
// Returns pointer to next frame
// (may go circular in the future)
{
  short len;
  len=*(short*)frame;
  // A frame with zero length is probably a frame being built but not
  // yet ended.
  if(!len)return 0;
  frame=((char*)frame)+len;
  // Check for last frame that is being written (end of replay)
  // Make sure to check this before the rewindFrame! (they may be the same)
  if(frame==currentFrame)return 0;
  // Check for rewind frame (restart at start of buffer)
  if(frame==rewindFrame)return buffer;
  return frame;
}
short RReplay::GetFrameLength(void *frame)
// Returns length of 'frame'
{
  return *(short*)frame;
}
int RReplay::GetFrameTime(void *frame)
// Returns time at which 'frame' was recorded
{
  return *(int*)((char*)frame+sizeof(short));
}
void *RReplay::FindFrame(int t)
// Find the frame starting at or just beyond 'time'.
// Returns 0 if there is no such frame
{
  void *fromFrame=firstFrame;
  void *playFrame=0;
  int   len,ft;

  while(1)
  {
    if(fromFrame==currentFrame||fromFrame==0)
    {
      // Reached the end of the replay
      break;
    }
    // Get frame time
//qdbg("fromFrame=%p\n",fromFrame);
    len=GetFrameLength(fromFrame);
    ft=GetFrameTime(fromFrame);
    if(ft>t)break;

    // Best guess yet
    playFrame=fromFrame;
    // Search on
    fromFrame=GetNextFrame(fromFrame);
  }
  return playFrame;
}

bool RReplay::Goto(int t)
// Setup the situation at time 't'
{
  int   i;
  short len;
  void *fromFrame;

qdbg("RReplay:Goto(%d); this=%p\n",t,this);

  // Any replay frames?
  if(firstFrame==currentFrame)
  {
//qdbg("No replay frames\n");
    return FALSE;
  }

  // Restart searching the entire replay
  // (future: cache last play frame and search from there)
  playFrame=0;

  if(!playFrame)
  {
    // Find the frame that is at time 't' or just beyond
    playFrame=FindFrame(t);
#ifdef OBS
    fromFrame=firstFrame;
    while(1)
    {
      if(fromFrame==currentFrame||fromFrame==0)
      {
        // Reached the end of the replay
        break;
      }
      // Get frame time
//qdbg("fromFrame=%p\n",fromFrame);
      len=GetFrameLength(fromFrame);
      ft=GetFrameTime(fromFrame);
      if(ft>t)break;
  
      // Best guess yet
      playFrame=fromFrame;
      // Search on
      fromFrame=GetNextFrame(fromFrame);
    }
#endif
  }

  if(!playFrame)
  { // No frame to play
qdbg("No frame to play\n");
    return FALSE;
  }
  // Get future frame
//qdbg("GetFutureFrame, playFrame=%p\n",playFrame);
  len=GetFrameLength(playFrame);
  playNextFrame=GetNextFrame(playFrame);
  if(!playNextFrame)
    return FALSE;
 
//qdbg("found frame %p with time %d\n",playFrame,GetFrameTime(playFrame));
  if(playNextFrame)
  {
    PlayFrame(playFrame,t,FALSE);
    PlayFrame(playNextFrame,t,TRUE);

    // Update car shadows; deduce it from the current situation.
    // Note that a wheel's PreAnimate() calculates the surface info, so
    // we can deduce the shadow height.
    int j;
    RCar *car;
    for(i=0;i<RMGR->scene->GetCars();i++)
    {
      car=RMGR->scene->GetCar(i);
      for(j=0;j<car->GetWheels();j++)
        car->GetWheel(j)->PreAnimate();
    }
  }
  return TRUE;
}

void RReplay::PlayFrame(void *frame,int t,bool isFuture)
// Act out the contents in this frame.
// 't' is the target time.
// If 'isFuture'=TRUE, this is the oncoming frame, and some values
// of that future frame must be interpolated
{
  short len,cmdLen;
  short id;
  int   i,n,curCar=0;
  RCar *car=0;
  // Don't use void*, but char* to make arithmetic easier
  char *pData;
  // Interpolation
  float inter,tf;            // inter=0..1, tf=0..nextFrameTime (in secs)
  DVector3 *v;
  char      buf[100];

//qdbg("RReplay:PlayFrame()\n");

  len=GetFrameLength(frame);
  if(isFuture)
  {
    // Calculate how far we're from this frame and the next
    tf=float(t-GetFrameTime(playFrame))/1000.0;
    inter=float(t-GetFrameTime(playFrame))/
      float(GetFrameTime(frame)-GetFrameTime(playFrame));
  } else inter=tf=0;              // To be safe

  // Skip header and start interpreting
  pData=(char*)frame+sizeof(short)+sizeof(int);
  len-=sizeof(short)+sizeof(int);
  while(len>0)
  {
    // Get command id
    id=*(short*)pData;
    len-=sizeof(short);
    pData=pData+sizeof(short);
//qdbg("PlayFrame: id %d\n",id);
    // Interpret id; note that ALL ids must be recognized here
    // for this to work!
    switch(id)
    {
      case RC_SETCAR:
        curCar=*(short*)pData;
        pData+=sizeof(short); len-=sizeof(short);
        // Check car index
        //...
//qdbg("curCar=%d\n",curCar);
        // Retrieve car pointer
        car=RMGR->scene->GetCar(curCar);
        break;
      case RC_CARPOSITION:
        if(car)
        {
          DVector3 v,*cp=car->GetPosition();
          memcpy(&v,pData,sizeof(DVector3));
          if(!isFuture)
            *cp=v;
          else
          { cp->x+=inter*(v.x-cp->x);
            cp->y+=inter*(v.y-cp->y);
            cp->z+=inter*(v.z-cp->z);
          }
        }
//qdbg("id = carPos\n");
        pData+=sizeof(DVector3);
        len-=sizeof(DVector3);
        break;
      case RC_CAR:
        // Car attribute pack
        {
        DVector3 v,*cp=car->GetPosition();
        DQuaternion quat;
        float val,val2,*vp;

        n=car->GetWheels();
        cmdLen=sizeof(DVector3)+sizeof(DQuaternion)+sizeof(float)+
          n*3*sizeof(float)+sizeof(float)+sizeof(float);

        // Position
        memcpy(&v,pData,sizeof(DVector3));
        if(!isFuture)*cp=v;
        else
        { cp->x+=inter*(v.x-cp->x);
          cp->y+=inter*(v.y-cp->y);
          cp->z+=inter*(v.z-cp->z);
#ifdef RR_ODE
          dBodySetPosition(car->GetBody()->GetODEBody(),cp->x,cp->y,cp->z);
#endif
        }
        // Orientation
        memcpy(&quat,pData+sizeof(DVector3),sizeof(DQuaternion));
        if(!isFuture)*car->GetBody()->GetRotPosQ()=quat;
        else
        {
          DQuaternion q2;
          car->GetBody()->GetRotPosQ()->Slerp(&quat,inter,&q2);
          car->GetBody()->GetRotPosM()->FromQuaternion(&q2);
#ifdef ND_IN_QUAT_ITSELF
          car->GetBody()->GetRotPosQ()->Slerp(&quat,inter,
            car->GetBody()->GetRotPosQ());
          // Create 3D matrix, which is actually used in painting
          car->GetBody()->GetRotPosM()->FromQuaternion(
           car->GetBody()->GetRotPosQ());
           //car->GetBody()->GetRotPosQ()->LengthSquared());
#endif
#ifdef RR_ODE
          dBodySetQuaternion(car->GetBody()->GetODEBody(),(dReal*)&q2);
#endif
        }
        // Steering
        val=*(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion));
        car->GetSteer()->SetAngle(
          car->GetSteer()->GetAngle()+
          inter*(val-car->GetSteer()->GetAngle()));
        car->ApplySteeringToWheels();

        // Suspension lengths
        vp=(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion)+sizeof(float));
        if(!isFuture)
          for(i=0;i<n;i++)
          {
            //car->GetSuspension(i)->SetLength(vp[i]);
            car->GetWheel(i)->GetPosition()->y=vp[i];
          }
        else
          for(i=0;i<n;i++)
          {
            car->GetWheel(i)->GetPosition()->y+=
              inter*(vp[i]-car->GetWheel(i)->GetPosition()->y);
            /*car->GetSuspension(i)->SetLength(
              car->GetSuspension(i)->GetLength()+
              inter*(vp[i]-car->GetSuspension(i)->GetLength()));*/
          }

        // Wheel rotations
        if(!isFuture)
        {
          vp=(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion)+
            sizeof(float)+n*sizeof(float));
          for(i=0;i<n;i++)
          {
            car->GetWheel(i)->GetRotationVector()->x=vp[i];
          }
        } else
        { // Predict wheel rotation based on wheel spin velocity
          vp=(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion)+
            sizeof(float)+2*n*sizeof(float));
          for(i=0;i<n;i++)
          {
            car->GetWheel(i)->GetRotationVector()->x+=tf*vp[i];
//qdbg("tf=%.3f, vp[%d]=%f\n",tf,i,vp[i]);
          }
        }

        // Engine RPM
        val=*(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion)+
          sizeof(float)+n*3*sizeof(float));
        if(!isFuture)
        {
          car->GetEngine()->SetRPM(val);    // For the views
        } else
        {
          val2=car->GetEngine()->GetRPM();
          car->GetEngine()->SetRPM(val+(val-val2)*inter);
        }

        // Audio of engine
        car->GetRAPEngine()->SetCurrentSpeed(val);

        // Car speed
        val=*(float*)(pData+sizeof(DVector3)+sizeof(DQuaternion)+
          sizeof(float)+n*3*sizeof(float)+sizeof(float));
        if(!isFuture)
        {
          car->GetVelocity()->z=val; // Just make the vector the correct length
        } else
        {
          val2=car->GetVelocity()->z;
          car->GetVelocity()->z=val+(val-val2)*inter;
        }

        pData+=cmdLen; len-=cmdLen;
        }
        break;
      case RC_SMOKE:
        // Avoid playing smoke multiple times
        {
//qdbg("RC_SMOKE\n");
        RReplaySmokeRecord *rec=(RReplaySmokeRecord*)pData;
        if(isFuture==FALSE&&GetFrameTime(frame)!=playFrameTime)
        {
//qdbg("Smoke at %f/%f/%f\n",rec->pos.x,rec->pos.y,rec->pos.z);
          RMGR->pgSmoke->Spawn(&rec->pos,&rec->vel,rec->lifeTime);
        }
        pData+=sizeof(*rec);
        len-=sizeof(*rec);
        }
        break;
      case RC_SKID_START:
        // Start skid strip
        // Avoid playing skids multiple times
        if(isFuture==FALSE&&GetFrameTime(frame)!=playFrameTime)
        {
          n=*(short*)pData;
          // To avoid mixups in the strips, force this strip to be used,
          // no matter what strips are in use. Be careful though with
          // varying versions where the number of strips may have changed.
//qdbg("skid strip %d force start\n",n);
          RMGR->smm->StartStripForced(n);
        }
        pData+=sizeof(short);
        len-=sizeof(short);
        break;
      case RC_SKID_POINT:
        // Avoid playing skids multiple times
        // Problem: which skid strip to put it in?
        if(isFuture==FALSE&&GetFrameTime(frame)!=playFrameTime)
        {
          n=*(short*)pData;
          v=(DVector3*)(pData+sizeof(short));
          RMGR->smm->AddPoint(n,v);
        }
        pData+=sizeof(short)+sizeof(DVector3);
        len-=sizeof(short)+sizeof(DVector3);
        break;
      case RC_SKID_STOP:
        // Stop skid strip
        // Avoid playing skids multiple times
        if(isFuture==FALSE&&GetFrameTime(frame)!=playFrameTime)
        {
          n=*(short*)pData;
//qdbg("skid strip %d STOP\n",n);
          RMGR->smm->EndStrip(n);
        }
        pData+=sizeof(short);
        len-=sizeof(short);
        break;
      case RC_NEWCAR:
        // New car enters field
        // Avoid entering a car multiple times
        strncpy(buf,pData,CARNAME_LEN);
        n=*(short*)(pData+CARNAME_LEN);
qdbg("RC_NEWCAR: '%s' index %d\n",buf,n);
        if(RMGR->scene->GetCars()<=n)
        {
          // This is indeed a new car; create it
qdbg("  indeed; create the new car\n");
          RCar *car=new RCar(buf);
          RMGR->scene->AddCar(car);
        }
        pData+=CARNAME_LEN+sizeof(int);
        len-=CARNAME_LEN+sizeof(int);
        break;
      default:
        qerr("Replay command id %d unknown!",id);
        // We'd like to cut the replay here to avoid trashing
        break;
    }
  }
//qdbg("-- end playback frame\n");
  // Remember we played this frame (for one-time effects like smoke)
  if(isFuture==FALSE)
  {
    playFrameTime=GetFrameTime(frame);

    // Process any continues replay data (smoke)
    tf=(float(t)-float(lastPlayTime))/1000.0;
    if(tf>0)
    {
      // Move forward
//qdbg("Update %.2f secs\n",tf);
      RMGR->pm->Update(tf);
    } else
    {
      // Reverse play; kill some effects we don't want to play backwards
//qdbg("Kill particles\n");
      // But we may still playback in reverse for some effects, however wrong.
      // Smoke particles still look ok, even in reverse.
      RMGR->pm->Update(-tf);
    }

    // Remember last time rendered
    lastPlayTime=t;
  }
}

/******
* I/O *
******/
bool RReplay::Load(cstring fname)
{
  RReplayHeader hdr;
  int i,size;
  char buf[256];

qdbg("RReplay:Load(%s)\n",fname);

  size=QFileSize(fname);

  QFile f(fname);
  if(!f.IsOpen())
    return FALSE;
  f.Read(&hdr,sizeof(hdr));
  // Check version
  if(hdr.versionMajor!=RREPLAY_VERSION_MAJOR)
  {
    qwarn("Version of replay (%d.%d) is incompatible (need %d[.%d])",
      hdr.versionMajor,hdr.versionMinor,RREPLAY_VERSION_MAJOR,
      RREPLAY_VERSION_MINOR);
    return FALSE;
  }
  // Read car names
  for(i=0;i<hdr.cars;i++)
  {
    f.Read(buf,CARNAME_LEN);
    // In case of a stand-alone replay, load the car (if not already present)
    //...
  }

  // Skip 'DATA' id
  f.Read(buffer,4);

  // Read frames
  size-=sizeof(hdr);
  if(size>=bufferSize)
  {
    qwarn("Replay doesn't fit into buffer (replay=%d, buffer=%d bytes)",
      size,bufferSize);
    return FALSE;
  }
qdbg("  load %d bytes of replay data\n",size);
  f.Read(buffer,size);

  // Reset replay vars
  Reset();

  // Added frames should be at the end (although this shouldn't happen,
  // it may influence functions like GetNextFrame()).
  currentFrame=(char*)buffer+size;
  currentData=currentFrame;

  return TRUE;
} 
bool RReplay::LoadHeader(cstring fname,RReplayHeader *header)
// Convenience function to get only the header (for 'View replay',
// where you just quickly want to know the track and some data)
// Returns FALSE if file could not be openen (header is untouched)
// Doesn't check version info or anything.
{
qdbg("RReplay:LoadHeader(%s)\n",fname);

  QFile f(fname);
  if(!f.IsOpen())
    return FALSE;
  f.Read(header,sizeof(*header));
  return TRUE;
}
bool RReplay::Save(cstring fname,int tStart,int tEnd)
{
  RReplayHeader hdr;
  void *frame,*startFrame,*endFrame;
  int   i;
  char  buf[256];

  memset(&hdr,0,sizeof(hdr));
  strncpy(hdr.id,"RPLY",4);
  hdr.versionMinor=RREPLAY_VERSION_MINOR;
  hdr.versionMajor=RREPLAY_VERSION_MAJOR;
  strcpy(hdr.trackName,RMGR->GetTrack()->GetName());
  hdr.interval=RMGR->scene->replayInterval;
  // #cars, car names...
  hdr.cars=RMGR->scene->GetCars();

  QFile f(fname,QFile::WRITE);
  if(!f.IsOpen())
    return FALSE;
  // Write header
  f.Write(&hdr,sizeof(hdr));
  // Write cars
  for(i=0;i<hdr.cars;i++)
  {
    memset(buf,0,sizeof(buf));
    strcpy(buf,RMGR->scene->GetCar(i)->GetShortName());
    f.Write(buf,CARNAME_LEN);
  }

  // Write replay data
  f.Write("DATA",4);

  // Find start and end frame
  if(tStart==-1)startFrame=firstFrame;
  else          startFrame=FindFrame(tStart);
  if(tEnd==-1)endFrame=0;
  else        endFrame=FindFrame(tEnd);

  // Write every frame
  for(frame=startFrame;frame!=endFrame;frame=GetNextFrame(frame))
  {
    // Don't write too much; skip the last frame being filled, or if
    // the frame time is 0.
    if(frame==currentFrame||
       (frame!=firstFrame&&RMGR->replay->GetFrameTime(frame)==0))
      break;
    f.Write(frame,GetFrameLength(frame));
//qdbg("Save frame time=%d\n",RMGR->replay->GetFrameTime(frame));
  }
  return TRUE;
}

