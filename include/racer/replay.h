// racer/replay.h - Replay support

#ifndef __RACER_REPLAY_H
#define __RACER_REPLAY_H

#include <racer/car.h>
#include <d3/quat.h>

#define RREPLAY_VERSION_MINOR    1   // Revision minor
#define RREPLAY_VERSION_MAJOR    1   // When different, it's incompatible

// Replay global info
struct RReplayHeader
{
  char  id[4];                  // RPLY
  short versionMinor,           // Latest version (of the replay format)
        versionMajor;
  char  trackName[64];          // Track being driven
  int   interval;               // Storing time of replay frames
  // Cars, timing info etc
  int   cars;
  // followed by carName[64]*cars
};

// Replay records for the car
enum RReplayTypes
// OBSOLETE
{
  RRT_RESTART,                  // Restart replay at start of buffer
  RRT_TIME,                     // Timing (start of replay frame)
  RRT_CAR			// A car record
};

enum RReplayCommand
// Tiny packed commands that are stored in a replay.
// Note these may also be used in network communications, as
// network and replays have similar goals.
{
  RC_SETCAR,                    // Set current car; followed by short carIndex
  RC_CARPOSITION,               // Set position; DVector3 pos
  RC_CAR,                       // Pos, or, suspY, wheelRotX/XA, steer pack
  RC_REWIND,                    // End of replay; restart at start of buffer
  RC_SMOKE,                     // Smoke particle spawn
  RC_SKID_START,                // Start of skid strip (short stripnr)
  RC_SKID_POINT,                // Point of skidmark (stripnr, vec3)
  RC_SKID_STOP,                 // End of skid strip
  RC_SKID_VOL,                  // Skid volume
  RC_NEWCAR,                    // Car enters game
  RC_LAP,                       // Crossing start/finish
  RC_TIMELINE_CROSS,            // Crossing a timeline
  RC_xxx
};

struct RReplayHeaderRecord
// This header is presented before a 'frame' of events are stored
// More or less other records may follow, until the next RReplayHeader
// OBSOLETE
{
  char type,                    // RRT_TIME
       pad[3];
  int  time;			// Absolute time in msecs
};

struct RReplayCarRecord
// Contains the state of a car, needed for a nice replay
// OBSOLETE
{
  char type,			// Record type, always RRT_CAR for this struct
       index;			// Car index in scene
  char steering,                // Steering wheel angle, reduced to 0..255
       throttle,                // Throttle pedal 0..255
       brakes,                  // Brake pedal 0..255
       clutch;                  // Clutch pedal 0..255
  DQuaternion quat;             // Orientation of vehicle
  DVector3    pos;              // Linear position of vehicle
  float       rpm;              // Engine speed
  char wRotZ[RCar::MAX_WHEEL],  // Wheel roll (its normal rotation direction)
       wRotY[RCar::MAX_WHEEL];  // Wheel headings
  float suspLen[RCar::MAX_WHEEL]; // Size of suspension
};

struct RReplaySmokeRecord
// A smoke particle
{
  DVector3 pos,vel;
  dfloat   lifeTime;
};

class RReplay
// A replay.
// Looks a bit like QNMessage (see libQN); serializes events into
// packets (called frames here) which make up the replay.
{
 public:
  enum Flags
  {
    FRAME_STARTED=1             // A frame has been started for recording?
  };

 protected:
  // The buffer
  void *buffer;
  int   bufferSize;

  // Frames
  void *firstFrame;             // Start of replay
  void *currentFrame;           // Ptr to start of current frame being filled
  void *currentData;            // Ptr to current fill point
  void *rewindFrame;            // Frame ptr at which to rewind

  // Special laps
  void *ghostFirstFrame,        // Ghost (best) lap
       *ghostLastFrame,
       *lastLapFirstFrame,      // Last driven lap
       *lastLapLastFrame;

  // Playback
  void *playFrame,              // Last frame from which to interpolate to...
       *playNextFrame;          // the future frame
  int   playFrameTime,          // Time of last playback frame
        lastPlayTime;           // Last time that Goto() was called for

  // Storage
  int   flags;

 public:
  RReplay();
  ~RReplay();

  // Attributes
  void   SetSize(int size);
  int    GetSize(){ return bufferSize; }
  //int    GetUsedSize(){ return sizeUsed; }
  void *GetBuffer(){ return buffer; }
  void *GetFirstFrame(){ return firstFrame; }
  void *GetCurrentFrame(){ return currentFrame; }
  int   GetReplaySize();

  // Creation
  bool Create();
  void Destroy();

  // I/O
  bool Load(cstring fname);
  bool LoadHeader(cstring fname,RReplayHeader *header);
  bool Save(cstring fname,int tStart=-1,int tEnd=-1);

  // Data storing
  void Reset();
  void BeginFrame(int t=-1);
  void AddChunk(const void *p,int size);
  void AddShort(short n);
  void EndFrame();

  // Replay keeping (all 3 OBSOLETE)
  void AddHeaderRecord();
  void AddCarRecord(RCar *car);
  void AddSceneRecord(RScene *scene);

  // Convenience functions for the replay commands (RC_xxx)
  void RecSetCar(int id);
  void RecCarPosition(RCar *car);
  void RecCar(RCar *car);
  void RecSmoke(DVector3 *pos,DVector3 *vel,dfloat lifeTime);
  void RecSkidStart(int stripNr);
  void RecSkidPoint(int stripNr,DVector3 *pos);
  void RecSkidStop(int stripNr);
  void RecNewCar(cstring carName,int carIndex);

  // Playback
  short GetFrameLength(void *frame);
  int   GetFrameTime(void *frame);
  void *GetNextFrame(void *frame);
  bool  Goto(int time);
  void  PlayFrame(void *frame,int t,bool isFuture=FALSE);

  // Replay info
  void *FindFrame(int time);
  int   GetFirstFrameTime();
  int   GetLastFrameTime();
};

#endif
