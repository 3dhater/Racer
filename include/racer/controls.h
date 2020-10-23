// racer/controls.h

#ifndef __RACER_CONTROLS_H
#define __RACER_CONTROLS_H

#include <qlib/dxjoy.h>

// Default maximum value for a valuator (axis)
#define RCONTROL_MAX_VALUE	1000

class RControl
{
 public:
  enum Flags
  {
    NEGATE=1,                   // Negate axis?
    MOUSE=2                     // Mouse control?
  };
  enum Axis
  { AXIS_NONE=-1,
    AXIS_X,AXIS_Y,AXIS_Z,       // Joystick or mouse
    AXIS_RX,AXIS_RY,AXIS_RZ,
    AXIS_SLIDER0,AXIS_SLIDER1   // 2 sliders (Logitech wheel uses SLIDER0)
    //AXIS_POV                    // POV hat (like a compass)
  };

  // What to use
  int     flags;
  QDXJoy *joy;
  int     axis,                 // From where to get the info?
          button,
          pov;                  // POV hats must match 'max'
  int     min,max;
  int     base,range;           // Base and range of axis values
  int     deadzone;		// Percentage
  rfloat  linearity;            // From 0..1, 1=fully linear, cubic function
  // Keyboard support
  int     key;                  // Key to use
  int     keyAxisValue;         // Position along simulated axis
  int     sensitivityRise,      // Speed of axis movement
          sensitivityFall;      // Lowering of value when key released

  // State
  int     value;		// The current value (with linearity applied)

 public:
  RControl();
  ~RControl();

  void Update();
};

// Magic value to indicate system mouse
#define RR_CONTROLLER_MOUSE ((QDXJoy*)-2)
// More magic to indicate the... keyboard (ouch!)
#define RR_CONTROLLER_KEYBOARD ((QDXJoy*)-3)

class RControls : public QObject
// Class that maintains all controls used to control
// the vehicle
{
 public:
  enum Max
  {
    MAX_CONTROLLER=4
  };
  enum Flags
  {
    FORCE_FEEDBACK=1
  };
  enum ControlTypes
  // Which kind of controls do we have?
  {
    T_STEER_LEFT,T_STEER_RIGHT,
    T_THROTTLE,T_BRAKES,
    T_SHIFTUP,T_SHIFTDOWN,
    T_CLUTCH,
    T_HANDBRAKE,
    T_STARTER,
    T_HORN,
    MAX_CONTROLLER_TYPE
  };

  RControl *control[MAX_CONTROLLER_TYPE];
  QDXJoy   *joy[MAX_CONTROLLER];
  int       joys;
  // Effects on the joysticks
  int          flags[MAX_CONTROLLER];
  QDXFFEffect *fxAlign[MAX_CONTROLLER];
  QDXFFEffect *fxFriction[MAX_CONTROLLER];
  QDXFFEffect *fxKerb[MAX_CONTROLLER];
  QDXFFEffect *fxRumble[MAX_CONTROLLER];
  // Preferences
  int          ffMaxTorque[MAX_CONTROLLER];
  int          ffMaxForce[MAX_CONTROLLER];
  int          ffDelay[MAX_CONTROLLER];
  rfloat kerbMagnitudeFactor[MAX_CONTROLLER],   // Influence of load
         kerbPeriodFactor[MAX_CONTROLLER];      // Influence of velocity

  // Global controller-wide preferences

 public:
  RControls();
  ~RControls();

  void LoadController(QInfo *info,cstring path,RControl *ctl,
    int defNegate=FALSE);
  bool LoadConfig(cstring fname=0);
  void Update();

  // FF support
  void SetFFSteerResponse(rfloat steeringTorque,rfloat kerbLoad,
    rfloat kerbVelocity);
};

#endif
