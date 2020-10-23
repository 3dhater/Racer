// trackball.h - virtual trackball for intuivitive rotations in a window

#ifndef __QLIB_TRACKBALL_H
#define __QLIB_TRACKBALL_H

#include <qlib/object.h>

class QTrackBall
{
 protected:
  float size;
  float quat[4];	// Quaternion representing rotation
  QRect rWindow;	// Window location

 public:
  QTrackBall(int wid,int hgt,int x=0,int y=0);
  ~QTrackBall();

  // Access to the quaternion
  void GetQuat(float *q);
  void SetQuat(float *q);

  void Movement(int sx,int sy,int dx,int dy);	// Moving it
  void BuildRotateMatrix(float *m);		// For OpenGL
};

#endif
