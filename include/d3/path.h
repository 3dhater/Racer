// d3/path.h

#ifndef __D3_PATH_H
#define __D3_PATH_H

#include <qlib/types.h>
// DTransformation
#include <d3/object.h>

// DTPath is a collection of DTransformations connected keyframed using splines
// Originally taken from GPPath, this class fits D3 better.
#define DTPATH_VERSION  1               // File format; using fields
#define DTPATH_VERSION_FRAMES   1       // Old frame-based format

// Keyframe flags (obsolete since 14-9-1999; use DTPath::LINEAR etc)
#define DTP_KF_LINEAR     1            // Linear knot
#define DTP_KF_CURRENT    2            // Use current state instead of predef

struct DTKeyFrame
{ int frame;                            // At what frame?
  int flags;				// Linear? etc.
  DTransformation state;		// The state at this key
};

class DTPath
// 3D keyframed path made out of DTransformations
// Could be a template one day.
// Or perhaps split into DController (QController) types, which
// have their own key framing per controller.
{
  int         maxkey;
  int         keys;             // Number of keyframes
  DTKeyFrame *key;                      // Keyframes

 public:
  enum KeyFlags
  { LINEAR=1,			// Linear knot (from here on)
    CURRENT=2,			// Use current state instead of predef'd?
  };

 public:
  DTPath();
  ~DTPath();

  // Key admin
  void SortKeys();
  bool CreateKey(int frame,DTransformation *state,int flags=0);
  bool RemoveKey(int frame);
  int  GetNoofKeys(){ return keys; }
  void Reset();

  // Retrieve a state at a certain frame (interpolated)
 private:
  void GetKeyIndexes(int frame,int *k1,int *k2,int *k3,int *k4);
  float CalcCM(float p1,float p2,float p3,float p4,
    float t,float t2,float t3);
 public:
  void GetState(int frame,DTransformation *state);

  // Keyframe functionality
  int  GetPrevKeyFrame(int frame);
  int  GetNextKeyFrame(int frame);
  bool IsKeyFrame(int frame);
  int  GetKeyFrameFlags(int frame);
  void SetKeyFrameFlags(int frame,int flags);
  int  GetMaxKeyFrame();
  void Reverse();
  void ScaleKeys(int num,int den=1,int from=-1,int to=-1);
  void ShiftKeys(int n,int from,int to=-1);
  void SetScaleID();            // Set all scales to 1/1/1

  // I/O
  bool Load(string fname);
  bool Save(string fname);
};

#endif
