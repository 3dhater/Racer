/*
 * DScene - a collection of geodes
 * 03-01-01: Created!
 * NOTES:
 * - Not much use in itself, but useful to create trees from
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/scene.h>
DEBUG_ENABLE

DScene::DScene()
{
  geodes=0;
}
DScene::~DScene()
{
}

bool DScene::AddGeode(DGeode *g)
// Returns FALSE if out of space.
{
  if(geodes==MAX_GEODE)return FALSE;
  geode[geodes]=g;
  geodes++;
  return TRUE;
}

