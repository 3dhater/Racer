// racer/view.h

#ifndef __RVIEW_H
#define __RVIEW_H

#include <d3/types.h>
#include <d3/bbox.h>

class RView;
class RViews;

class RViewElt
// Element of a view
{
 public:
  enum Types
  {
    NONE=0,
    STATIC=1,
    ANGULAR=2,			// Rotating element
    TIME=3                      // A timer (mm:ss:mmm or hh:mm:ss)
  };
  enum Var
  {
    VAR_RPM,			// Car's RPM
    VAR_LAPTIME,                // Current laptime
    VAR_BESTTIME,               // Current best lap
    VAR_GEAR,                   // Current gear
    VAR_VELOCITY,               // Car velocity tach (perceived by gear)
    VAR_LAP,                    // Lap number
    VAR_VELOCITY_WC,            // Real world velocity (not the gearbox)
    VAR_GEARBOX_VEL             // Gearbox rotational velocity
  };
  enum Flags
  {
    IS3D=1,                     // View element should be drawn in full 3D
    FLAGS_xxx
  };

 protected:
  RView    *view;
  int       type;
  int       flags;
  DVector3  pos;
  rfloat    wid,hgt;			// Size of element
  DVector3  pivot;
  DTexture *tex;
  int       var;			// Which var to track
  rfloat    minValue,maxValue;
  rfloat    minAngle,maxAngle;

 public:
  RViewElt(RView *view);
  ~RViewElt();

  // Attribs
  RView *GetView(){ return view; }
  bool Is3D(){ if(flags&IS3D)return TRUE; return FALSE; }

  bool Load(QInfo *info,cstring path);

 private:
  rfloat GetVarValue();
  int    GetVarValueInt();
  rfloat GetVarAngle();
 public:
  void Paint();
};

class RView
// A collection of view elements, which in total constitutes
// a view for a camera (rpm-meters etc)
// Is always part of a RViews object
{
  enum Max
  {
    MAX_ELT=50
  };

  RViews   *views;
  RViewElt *elt[MAX_ELT];
  int       elts;
  qstring   name;
  
 public:
  RView(RViews *views);
  ~RView();

  RViews *GetViews(){ return views; }

  bool Load(QInfo *info,cstring path);
  
  // Attribs
  int   GetElts(){ return elts; }
  RViewElt *GetElt(int n){ return elt[n]; }
  
  void Paint2D();
  void Paint3D();
};

class RViews
// A collection of views
// Always belongs to a car
{
  enum Max
  {
    MAX_VIEW=10
  };
  RCar  *car;
  RView *view[MAX_VIEW];
  int    views;
 public:
  RViews(RCar *car);
  ~RViews();

  // Attribs
  int    GetViews(){ return views; }
  RView *GetView(int n){ return view[n]; }
  RCar  *GetCar(){ return car; }

  bool   Load(QInfo *info);
};

#endif
