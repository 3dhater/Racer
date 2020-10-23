// racer/racer.h - Common include

#ifndef __RACER_RACER_H
#define __RACER_RACER_H

#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif

// Global Racer options
// Graphics API; OpenGL is default (QLib/D3 use OpenGL extensively)
#define RR_GFX_OGL
// Direct3D is not yet supported, but may be in the future
//#define RR_GFX_D3D

// Define RR_ODE to use Russell Smith's ODE (Open Dynamics Engine)
#define RR_ODE
#ifdef RR_ODE
// Use 'float' as the floating point type for ODE
#define dSINGLE
#endif

// Useful constants

#define RR_RAD_DEG_FACTOR	57.29578f	// From radians to degrees
#define RR_RAD2DEG              57.29578f	// A bit more descriptive

#ifndef PI
#define PI	3.14159265358979f
#endif

// Near-zero definitions
#define RR_EPSILON_VELOCITY	0.001		// Wheel velocity

// Include often used class definitions
#include <racer/types.h>
#include <racer/rigid.h>
#include <racer/model.h>
#include <racer/timeline.h>
#include <racer/manager.h>
#include <racer/car.h>
#include <racer/body.h>
#include <racer/steer.h>
#include <racer/susp.h>
#include <racer/engine.h>
#include <racer/camera.h>
#include <racer/view.h>
#include <racer/env.h>
#include <racer/audio.h>
#include <racer/gfx.h>
#include <racer/replay.h>
#include <racer/flat.h>
#include <racer/profile.h>
#include <racer/driver.h>
#include <racer/network.h>

// Shortcuts
#define RMSG  RMGR->console->printf

#endif
