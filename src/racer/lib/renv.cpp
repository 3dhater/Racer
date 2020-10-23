/*
 * REnv - environment
 * 06-08-00: Created!
 * (c) Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/info.h>
DEBUG_ENABLE

#define ENV_NAME      "env.ini"

REnvironment::REnvironment()
{
  airDensity=0;
  gravity=10;
  Load(ENV_NAME);
}
REnvironment::~REnvironment()
{
}

bool REnvironment::Load(cstring fileName)
{
  airDensity=RMGR->GetMainInfo()->GetFloat("environment.air_density");
  gravity=RMGR->GetMainInfo()->GetFloat("environment.gravity");
  return TRUE;
}

