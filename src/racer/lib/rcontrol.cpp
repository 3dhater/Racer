/*
 * Racer - Controls
 * 05-10-00: Created!
 * 19-10-00: Implemented for real and actually using it.
 * 18-12-00: Preset files in data/ctrlsets (use Config to set)
 * 16-04-01: Support for slider axes.
 * 11-07-01: Linearity support.
 * 23-10-01: Keyboard support started.
 * 26-01-02: POV support (for Freeshifting later on).
 * NOTES:
 * - Keeps the logic for obtaining controller input, whether from joysticks,
 * mice or keyboards.
 * - Is quite a bit based on the number 1000 (hardcoded ranges).
 * FUTURE:
 * - Keyboard support (prizes for those who get a best lap using keys!)
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/keys.h>
#include <ctype.h>
DEBUG_ENABLE

// Default directory for presets
#define CTRLSET_DIR   "data/ctrlsets"

// Default preset file (hopefully present in CTRLSET_DIR)
#define DEF_CTRL_FILE	"genjoy.ini"

// Kerb effects extremes
#define MIN_PERIOD   10000
#define MAX_PERIOD   1000000

/***********
* RCONTROL *
***********/
RControl::RControl()
{
  joy=0;
  axis=button=pov=-1;
  min=0; max=1000;
  base=0; range=0;
  flags=0;
  value=0;
  key=0;
  keyAxisValue=0;
  sensitivityRise=5;
  sensitivityFall=50;
  linearity=1;
}
RControl::~RControl()
{
}

extern char keyState[256];

static bool KeyIsPressed(int key)
// Returns true if 'key' is currently depressed.
{
  return (bool)keyState[key&255];
}

void RControl::Update()
{
  int v;
  rfloat vf,vMax=1000.0f;
  
  if(!joy)return;
//if(joy==RR_CONTROLLER_MOUSE)return;
  
#ifdef OBS
if(joy==RR_CONTROLLER_MOUSE)
qdbg("mouse control Update; button=%d, axis=%d\n",button,axis);
if(joy==RR_CONTROLLER_KEYBOARD)
qdbg("keyboard control Update; button=%d, axis=%d\n",button,axis);
#endif
  if(axis>=0)
  {
    // Axis range
    if(joy==RR_CONTROLLER_MOUSE)
    {
      int x,y;
      app->GetMouseXY(&x,&y);
      // Scale to -1000..1000
      x-=RMGR->resWidth/2;
      x=x*2000/RMGR->resWidth;
      //y=RMGR->resHeight-1-y-RMGR->resHeight/2;
      y-=RMGR->resHeight/2;
      y=y*2000/RMGR->resHeight;
      switch(axis)
      {
        case AXIS_X:  v=x; break;
        case AXIS_Y:  v=y; break;
        default: v=0; break;
      }
//qdbg("mouse control Update; v=%d, x=%d,y=%d\n",v,x,y);
    } else if(joy==RR_CONTROLLER_KEYBOARD)
    {
//qdbg("key %d is pressed?; rise from %d\n",key&255,keyAxisValue);
      // Simulate axis for keyboard key
      if(KeyIsPressed(key))
      {
//qdbg("key 0x%x is pressed?; rise from %d\n",key,keyAxisValue);
        keyAxisValue+=sensitivityRise;
        if(keyAxisValue>max)
          keyAxisValue=max;
      } else
      {
        // Falling has a different speed
        keyAxisValue-=sensitivityFall;
        if(keyAxisValue<min)
          keyAxisValue=min;
      }
      // Transfer simulated axis to 'v' for linearity later on
      v=keyAxisValue;
    } else
    {
      // Joystick
      switch(axis)
      {
        case AXIS_X      : v=joy->x; break;
        case AXIS_Y      : v=joy->y; break;
        case AXIS_Z      : v=joy->z; break;
        case AXIS_RX     : v=joy->rx; break;
        case AXIS_RY     : v=joy->ry; break;
        case AXIS_RZ     : v=joy->rz; break;
        case AXIS_SLIDER0: v=joy->slider[0]; break;
        case AXIS_SLIDER1: v=joy->slider[1]; break;
        //case AXIS_POV    : v=joy->pov; break;
        default: v=0; break;
      }
    }
    // Keep within range
    if(v<min)v=min;
    else if(v>max)v=max;
    // Bring up to 0-base
    v-=base;
    if(range!=1000&&range!=0)
    {
      // Scale to virtual range (1000)
      v=v*1000/range;
    }
    // Adjust for range
    //...
  } else if(button>=0)
  {
    // Look at button
    if(joy==RR_CONTROLLER_MOUSE)
    {
      int b=(app->GetMouseButtons()>>1);
      if(b&(1<<button))v=1000;
      else             v=0;
//qdbg("button %d? b=%d, v=%d\n",button,b,v);
    } else if(joy==RR_CONTROLLER_KEYBOARD)
    {
      // On/off button support for keyboard
      if(KeyIsPressed(key))
        v=max;
      else
        v=min;
    } else
    {
      // Joystick
      if(joy->button[button])v=1000;
      else v=0;
    }
  } else if(pov>=0)
  {
    // Look at point-of-view hat
    // Only has meaning for a joystick/wheel.
    if(joy==RR_CONTROLLER_MOUSE)
    {
      v=0;
    } else if(joy==RR_CONTROLLER_KEYBOARD)
    {
      v=0;
    } else
    {
      // Joystick; note that 'max' contains
      // the value at which this control is
      // activated.
      if(joy->pov[pov]==max)v=1000;
      else                  v=0;
    }
  } else
  {
    // No axis or button? Keys then.
    if(joy==RR_CONTROLLER_KEYBOARD)
    {
      // On/off button support for keyboard
      if(KeyIsPressed(key))v=max;
      else                 v=0;
    } else
    {
      v=0;
    }
  }

  // Negation
  if(flags&RControl::NEGATE)v=1000-v;
  
  // Linearity; 0=non-linear (heavily curved), 1=linear
  // First normalize the value to 0..1
  vf=((rfloat)v)/vMax;
  // Then apply a x^3 curve for non-linear axes
  vf=vMax*(linearity*vf+(1.0f-linearity)*(vf*vf*vf));
  
  // Store value
  value=(int)vf;
//qdbg("RControl:Update; value=%d\n",value);
}

/************
* RCONTROLS *
************/
RControls::RControls()
  : QObject()
{
  int i;

  Name("RControls");

  joys=0;
  for(i=0;i<MAX_CONTROLLER_TYPE;i++)
    control[i]=0;
  for(i=0;i<MAX_CONTROLLER;i++)
  {
    joy[i]=0;
    fxAlign[i]=fxFriction[i]=fxKerb[i]=fxRumble[i]=0;
    flags[i]=0;
    ffMaxTorque[i]=ffMaxForce[i]=ffDelay[i]=0;
    kerbMagnitudeFactor[i]=kerbPeriodFactor[i]=1;
  }
}
RControls::~RControls()
{
  int i;
  // Delete all effects
  for(i=0;i<MAX_CONTROLLER;i++)
  {
    QDELETE(fxAlign[i]);
    QDELETE(fxFriction[i]);
    QDELETE(fxKerb[i]);
    QDELETE(fxRumble[i]);
  }
  for(i=0;i<MAX_CONTROLLER_TYPE;i++)
    QDELETE(control[i]);
  for(i=0;i<joys;i++)
  {
    if(joy[i]!=0&&joy[i]!=RR_CONTROLLER_MOUSE&&joy[i]!=RR_CONTROLLER_KEYBOARD)
    { QDELETE(joy[i]);
    }
  }
}

void RControls::LoadController(QInfo *info,cstring path,RControl *ctl,
  int defNegate)
// Read a controller config from the info file
// If 'defNegate' is true, the default negation parameters is true.
// This is used for the clutch; v0.4.8 used the clutch a bit
// different so cars didn't pull away by default with the old
// control set files.
{
  char buf[256],buf2[256];
  int index;
  qstring s;

  // Get controller or notice if none is used for this control
  // (may be keyboard still)
  sprintf(buf,"%s.controller",path);
  index=info->GetInt(buf);
  if(index<0)
  { 
    qwarn("Controller index<0 (%d)",index);
    ctl->joy=0;
    goto no_ctl;
  } else
  { if(index>joys-1)
    { qwarn("Controller index %d out of range 0..%d",index,joys-1);
      index=joys-1;
    }
    ctl->joy=joy[index];
  }
  // Axis
  sprintf(buf,"%s.axis",path);
  info->GetString(buf,s);
  if(s=="x")ctl->axis=RControl::AXIS_X;
  else if(s=="y")ctl->axis=RControl::AXIS_Y;
  else if(s=="z")ctl->axis=RControl::AXIS_Z;
  else if(s=="rx")ctl->axis=RControl::AXIS_RX;
  else if(s=="ry")ctl->axis=RControl::AXIS_RY;
  else if(s=="rz")ctl->axis=RControl::AXIS_RZ;
  else if(s=="slider0")ctl->axis=RControl::AXIS_SLIDER0;
  else if(s=="slider1")ctl->axis=RControl::AXIS_SLIDER1;
  //else if(s=="pov")ctl->axis=RControl::AXIS_POV;
  else
  { //qwarn("Unknown controller axis type for '%s'",buf);
    ctl->axis=RControl::AXIS_NONE;
  }
  
  // Axis range
  int min,max;
  sprintf(buf,"%s.min",path);
  min=info->GetInt(buf,0);
  sprintf(buf,"%s.max",path);
  max=info->GetInt(buf,1000);

  // Calculate base axis value and range
  if(min<max)
  {
    ctl->base=min;
    ctl->range=max-min;
    ctl->flags&=~RControl::NEGATE;
  } else
  {
    int t;
    ctl->base=max;
    ctl->range=min-max;
    ctl->flags|=RControl::NEGATE;
    // Swap min/max
    t=min;
    min=max;
    max=t;
  }
  ctl->min=min;
  ctl->max=max;
/*qdbg("RControls:LoadController: min=%d, max=%d, base=%d,"
" range=%d, flags=%d\n",
ctl->min,ctl->max,ctl->base,ctl->range,ctl->flags);*/

  // Button?
  int button;
  sprintf(buf,"%s.button",path);
  button=info->GetInt(buf,-1);
  ctl->button=button;
  // Take precedence over axis if a button is specified
  if(ctl->button!=-1)
    ctl->axis=RControl::AXIS_NONE;
//qdbg("ctl->button=%d\n",button);

  // POV?
  int pov;
  sprintf(buf,"%s.pov",path);
  pov=info->GetInt(buf,-1);
  ctl->pov=pov;
  // Take precedence over others
  if(ctl->pov!=-1)
  {
    ctl->axis=RControl::AXIS_NONE;
    ctl->button=-1;
  }

  // Manual negate?
  sprintf(buf,"%s.negate",path);
  if(info->GetInt(buf,defNegate))
    ctl->flags|=RControl::NEGATE;

  // Modifiers
  sprintf(buf,"%s.linearity",path);
  ctl->linearity=info->GetFloat(buf,1);

  // Keyboard support
  sprintf(buf,"%s.sensitivity_rise",path);
  ctl->sensitivityRise=info->GetInt(buf,5);
  sprintf(buf,"%s.sensitivity_fall",path);
  ctl->sensitivityFall=info->GetInt(buf,50);
  sprintf(buf,"%s.key",path);
  info->GetString(buf,buf2);
  if(!buf2[0])
  { // No key
    ctl->key=0;
  } else if(buf2[1]==0)
  { // Single char key
qdbg("RControl: key=%s\n",buf2);
    switch(tolower(buf2[0]))
    {
      case 'a': ctl->key=QK_A; break;
      case 'b': ctl->key=QK_B; break;
      case 'c': ctl->key=QK_C; break;
      case 'd': ctl->key=QK_D; break;
      case 'e': ctl->key=QK_E; break;
      case 'f': ctl->key=QK_F; break;
      case 'g': ctl->key=QK_G; break;
      case 'h': ctl->key=QK_H; break;
      case 'i': ctl->key=QK_I; break;
      case 'j': ctl->key=QK_J; break;
      case 'k': ctl->key=QK_K; break;
      case 'l': ctl->key=QK_L; break;
      case 'm': ctl->key=QK_M; break;
      case 'n': ctl->key=QK_N; break;
      case 'o': ctl->key=QK_O; break;
      case 'p': ctl->key=QK_P; break;
      case 'q': ctl->key=QK_Q; break;
      case 'r': ctl->key=QK_R; break;
      case 's': ctl->key=QK_S; break;
      case 't': ctl->key=QK_T; break;
      case 'u': ctl->key=QK_U; break;
      case 'v': ctl->key=QK_V; break;
      case 'w': ctl->key=QK_W; break;
      case 'x': ctl->key=QK_X; break;
      case 'y': ctl->key=QK_Y; break;
      case 'z': ctl->key=QK_Z; break;
      default: qwarn("Unknown key '%s'",buf2); break;
    }
  } else if(!strcmp(buf2,"left"))ctl->key=QK_LEFT;
  else if(!strcmp(buf2,"right"))ctl->key=QK_RIGHT;
  else if(!strcmp(buf2,"up"))ctl->key=QK_UP;
  else if(!strcmp(buf2,"down"))ctl->key=QK_DOWN;
  else if(!strcmp(buf2,"space"))ctl->key=QK_SPACE;
  else qwarn("Unknown/unsupported key '%s'",buf2);

no_ctl:;
  // Check for keys...
}

bool RControls::LoadConfig(cstring fname)
// Read controls configuration file
// 'fname' may be 0, in which case a default controller file is searched for
// in data/ctrlsets
{
  QInfo *info;
  int i;
  char buf[256];
  int  fMouse,fKeyboard;

  if(!fname)
  {
    // Search for selected controller file
    RMGR->GetMainInfo()->GetString("ini.controls",buf,DEF_CTRL_FILE);
qdbg("RControls: indirection to '%s'\n",buf);
    info=new QInfo(RFindFile(buf,CTRLSET_DIR));
  } else
  {
    // Explicit controller file
    qwarn("RControls: explicit controller file given; this is obsolete");
    info=new QInfo(RFindFile(fname));
  }

  joys=info->GetInt("controllers.count");
  if(joys>MAX_CONTROLLER)
  {
    qwarn("Too many controllers (max=%d) specified in controller file (%s)",
      MAX_CONTROLLER,fname);
    joys=MAX_CONTROLLER;
  }

  // Open all used controllers
//qlog(QLOG_INFO,"RControls:LoadConfig(); open all controllers\n");
  for(i=0;i<joys;i++)
  {
    int index;
    
    // Check for special controllers, like the mouse or keyboard
    sprintf(buf,"controllers.controller%d.mouse",i);
    fMouse=info->GetInt(buf);
    sprintf(buf,"controllers.controller%d.keyboard",i);
    fKeyboard=info->GetInt(buf);
    if(fMouse)
    {
      joy[i]=RR_CONTROLLER_MOUSE;
    } else if(fKeyboard)
    {
      joy[i]=RR_CONTROLLER_KEYBOARD;
    } else
    {
      // Game controller
      sprintf(buf,"controllers.controller%d.index",i);
      index=info->GetInt(buf);
      // Flags
      sprintf(buf,"controllers.controller%d.force_feedback",i);
      if(info->GetInt(buf))
      {
        flags[i]|=FORCE_FEEDBACK;
        // Get FF settings
        sprintf(buf,"controllers.controller%d.max_torque",i);
        ffMaxTorque[i]=info->GetInt(buf);
        sprintf(buf,"controllers.controller%d.max_force",i);
        ffMaxForce[i]=info->GetInt(buf);
        sprintf(buf,"controllers.controller%d.delay",i);
        ffDelay[i]=info->GetInt(buf);
        sprintf(buf,"controllers.controller%d.kerb_magnitude_factor",i);
        kerbMagnitudeFactor[i]=info->GetFloat(buf,1);
        sprintf(buf,"controllers.controller%d.kerb_period_factor",i);
        kerbPeriodFactor[i]=info->GetFloat(buf,1);
      }
      // Open joystick
      joy[i]=new QDXJoy(index);
//qlog(QLOG_INFO,"Open joy %d\n",i);
      if(!joy[i]->Open())
      { qerr("Can't open controller %d (gamecontroller index %d)",i,index);
      }
    }
  }

  // Read all controls
  cstring ctlName[MAX_CONTROLLER_TYPE]=
  { "steerleft","steerright","throttle","brakes","shiftup","shiftdown",
    "clutch","handbrake","starter","horn"
  };
  for(i=0;i<MAX_CONTROLLER_TYPE;i++)
  {
    control[i]=new RControl();
    if(!control[i])break;
    sprintf(buf,"controls.%s",ctlName[i]);
    // Load controller; default negate=TRUE for
    // the clutch (otherwise the clutch will be turned off)
    LoadController(info,buf,control[i],i==T_CLUTCH);
  }

  // Acquire all controllers
  for(i=0;i<joys;i++)
  {
    // Open only real game controllers
    if(joy[i]==RR_CONTROLLER_MOUSE||
       joy[i]==RR_CONTROLLER_KEYBOARD)continue;
    
    if(flags[i]&FORCE_FEEDBACK)
    {
      if(!joy[i]->IsForceFeedback())
      {
        qwarn("Controller %d can't do force feedback; disabling FF",i);
        flags[i]&=~FORCE_FEEDBACK;
      } else
      {
        // Use the force, Luke!
        // Disable autocentering BEFORE acquiring device (!)
        joy[i]->DisableAutoCenter();
        fxAlign[i]=new QDXFFEffect(joy[i]);
        fxAlign[i]->SetupConstantForce();
        // Friction to avoid smooth movement of wheel
        sprintf(buf,"controllers.controller%d.friction",i);
        fxFriction[i]=new QDXFFEffect(joy[i]);
        fxFriction[i]->SetupFrictionForce(info->GetInt(buf));
        //fxFriction[i]->Start();
#ifdef RACER_OBS
        // Inertia effects are not used anymore since v0.4.8
        sprintf(buf,"controllers.controller%d.inertia",i);
        fxInertia[i]=new QDXFFEffect(joy[i]);
        fxInertia[i]->SetupInertiaForce(info->GetInt(buf));
        //fxInertia[i]->Start();
#endif
        // Kerb effects
        fxKerb[i]=new QDXFFEffect(joy[i]);
        fxKerb[i]->SetupPeriodicForce(QDXFFEffect::TRIANGLE);
        //fxKerb[i]->Start();
#ifdef FUTURE
        // Rumble effects (dirt)
        fxRumble[i]=new QDXFFEffect(joy[i]);
        fxRumble[i]->SetupPeriodicForce(QDXFFEffect::RANDOM);
        fxRumble[i]->Start();
#endif
      }
    }
    // Acquire joystick before starting effects
    joy[i]->Acquire();
    if(flags[i]&FORCE_FEEDBACK)
    {
      fxAlign[i]->Start();
      fxFriction[i]->Start();
      fxKerb[i]->Start();
      //fxRumble[i]->Start();
    }
  }
 
  QDELETE(info);
  return TRUE;
}

void RControls::Update()
// Poll all used controllers; don't use too often please,
// might be quite slow for old joysticks.
{
  int i;

  for(i=0;i<joys;i++)
  {
    if(joy[i]!=0&&joy[i]!=RR_CONTROLLER_MOUSE&&
       joy[i]!=RR_CONTROLLER_KEYBOARD&&joy[i]->IsOpen())
    {
      joy[i]->Poll();
    }
  }
  for(i=0;i<MAX_CONTROLLER_TYPE;i++)
  {
//qdbg("RControls:Update(); ctrltype %d\n",i);
    control[i]->Update();
  }
}

/*****************
* FORCE FEEDBACK *
*****************/
void RControls::SetFFSteerResponse(rfloat steeringTorque,rfloat kerbLoad,
  rfloat kerbVelocity)
// Set feedback forces
// 'steeringTorque' gives the torque that pulls the steering wheel (Nm)
// 'kerbLoad' is the total load of all wheels that are on kerbs
// 'kerbVelocity' is the speed of the wheels (approximately)
{
  int i;
  rfloat fx,fy;
  rfloat mag,period;
  static bool kerbOn;

  // Calculate steering force from torque
  // Find FF controllers
  for(i=0;i<joys;i++)
  {
    if(!(flags[i]&FORCE_FEEDBACK))continue;
    
    // Check for force to be valid
    if(!ffMaxTorque[i])continue;
    
    fx=steeringTorque*ffMaxForce[i]/ffMaxTorque[i];
    if(fx<-ffMaxForce[i])fx=-ffMaxForce[i];
    else if(fx>ffMaxForce[i])fx=ffMaxForce[i];
    fy=0;

#ifdef ND_TEST_MOUSE
{ // Test with mouse
    int x,y;
    app->GetMouseXY(&x,&y);
    fx=(x-400)*25;
//qlog(QLOG_INFO,"FF Mz; fx=%.2f, fy=%.2f (T=%.2f, MaxF=%d, maxT=%d\n",fx,fy,
//steeringTorque,ffMaxForce[i],ffMaxTorque[i]);
}
#endif

    // Update the aligning effect
    fxAlign[i]->UpdateConstantForce(fx,fy);

    // Update kerb effects
    mag=kerbLoad*kerbMagnitudeFactor[i];
    if(mag<0)mag=0;
    else if(mag>10000)mag=10000;
    // Calculate a period as the inverse of the velocity.
    // Note that 100 means 100m/s which is approximately
    // 360 km/h, quite a max speed for most vehicles.
    period=(100-kerbVelocity)*1000*kerbPeriodFactor[i];
#ifdef OBS
    if((rand()&63)==1)
    {
      qlog(QLOG_INFO,"period=%f, kerbVel=%f, kerbFactor=%f\n",period,kerbVelocity,
        kerbPeriodFactor[i]);
    }
#endif

    if(period<MIN_PERIOD)period=MIN_PERIOD;
    else if(period>MAX_PERIOD)period=MAX_PERIOD;

    // Note that DirectInput seems to restart the effect
    // at every update, even with DIEP_NORESTART
    if(kerbOn)
    {
      // Still on?
      if(mag<1)
      {
        // It's off
        fxKerb[i]->UpdatePeriodicForce(mag,period);
        kerbOn=FALSE;
//qlog(QLOG_INFO,"KerbFX off\n");
      } // else don't update
    } else
    {
      // Must turn on?
      if(mag>0)
      {
        // Turn on fx
        //mag=10000; period=100000;
        fxKerb[i]->UpdatePeriodicForce(mag,period);
        kerbOn=TRUE;
//qlog(QLOG_INFO,"KerbFX ON mag=%.2f, period=%.2f\n",mag,period);
      } // else don't update
    }
    // Always update
    //fxKerb[i]->UpdatePeriodicForce(mag,period);

    // Update rumble effects
    //...
  }
}
