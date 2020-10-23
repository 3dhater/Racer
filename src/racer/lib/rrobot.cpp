/*
 * RRobot - AI driver
 * 25-04-2001: Created! (22:44:32)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "RRobot"

/*******
* ctor *
*******/
RRobot::RRobot(RCar *_car)
  : car(_car)
{
  DBG_C("ctor")
}
RRobot::~RRobot()
{
  DBG_C("dtor")
}

/************
* Decisions *
************/
void RRobot::Decide()
{
  DBG_C("Decide")
}
