/*
 * DGlobal - application-wide structures and settings
 * 10-01-01: Created!
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/global.h>
DEBUG_ENABLE

// Default #texture names cached
#define DTP_DEFAULT_COUNT   1000

/*********
* Global *
*********/
DGlobal::DGlobal()
{
  texturePool=new DTexturePool();
  batch=new DBatch();
  // No default environment texture map
  texEnv=0;
}
DGlobal::~DGlobal()
{
  QDELETE(texturePool);
  QDELETE(batch);
}
cstring DGlobal::FilterN2S(int n)
// Convert OpenGL filter to string
{
  switch(n)
  {
    case GL_LINEAR : return "GL_LINEAR";
    case GL_NEAREST: return "GL_NEAREST";
    case GL_LINEAR_MIPMAP_NEAREST : return "GL_LINEAR_MIPMAP_NEAREST";
    case GL_LINEAR_MIPMAP_LINEAR  : return "GL_LINEAR_MIPMAP_LINEAR";
    case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
    case GL_NEAREST_MIPMAP_LINEAR : return "GL_NEAREST_MIPMAP_LINEAR";
    default: qwarn("DGlobal::FilterN2S(%d) unknown",n);
             return "???";
  }
}
int DGlobal::FilterS2N(cstring s)
{
  if(!strcmp(s,"GL_LINEAR"))return GL_LINEAR;
  if(!strcmp(s,"GL_NEAREST"))return GL_NEAREST;
  if(!strcmp(s,"GL_LINEAR_MIPMAP_NEAREST"))return GL_LINEAR_MIPMAP_NEAREST;
  if(!strcmp(s,"GL_LINEAR_MIPMAP_LINEAR"))return GL_LINEAR_MIPMAP_LINEAR;
  if(!strcmp(s,"GL_NEAREST_MIPMAP_NEAREST"))return GL_NEAREST_MIPMAP_NEAREST;
  if(!strcmp(s,"GL_NEAREST_MIPMAP_LINEAR"))return GL_NEAREST_MIPMAP_LINEAR;
  qwarn("DGlobal::FilterS2N(%s) unknown",s);
  return GL_NEAREST;
}

/***********
* Env maps *
***********/
void DGlobal::SetDefaultEnvironmentMap(DTexture *tex)
{
  texEnv=tex;
}

/**************
* Preferences *
**************/
DPrefs::DPrefs()
// Default preferences
{
  Reset();
}
DPrefs::~DPrefs()
{
}

void DPrefs::Reset()
// Set default preferences
{
  minFilter=GL_LINEAR;
  maxFilter=GL_LINEAR;
  wrapS=wrapT=wrapP=wrapQ=DTexture::CLAMP;
  // Default env mode is replace, which means no lighting.
  // To use lighting, use DTexture::MODULATE for example.
  envMode=DTexture::REPLACE;
  flags=0;
}

bool DPrefs::UseMipMapping()
// Returns TRUE if prefs wants to use mipmapping
{
  switch(minFilter)
  {
    case GL_LINEAR_MIPMAP_NEAREST:
    case GL_LINEAR_MIPMAP_LINEAR:
    case GL_NEAREST_MIPMAP_NEAREST:
    case GL_NEAREST_MIPMAP_LINEAR:
      return TRUE;
  }
  return FALSE;
}

/***************
* DTexturePool *
***************/
DTexturePool::DTexturePool()
{
#ifdef DTP_NO_MAP
  allocated=1000;
  count=0;
  texName=(char**)qcalloc(sizeof(char*)*allocated);
  texPtr=(DTexture **)qcalloc(sizeof(DTexture*)*allocated);
#endif
}
DTexturePool::~DTexturePool()
{
#ifdef DTP_NO_MAP
  Clear();
  QFREE(texName);
  QFREE(texPtr);
#endif
}

bool DTexturePool::Add(cstring name,DTexture *texture)
{
//qdbg("DTexturePool:Add(%s,%p)\n",name,texture);
//qdbg("  name=%p\n",name);
#ifdef DTP_NO_MAP
  // Don't re-insert when already present
  if(Find(name))
    return TRUE;
  // Check for free space
  if(count>=allocated)
  { qwarn("DTexturePool:Add(); max reached (%d)",allocated);
    return FALSE;
  }
  // Append the texture
  texName[count]=qstrdup(name);
  texPtr[count]=texture;
  count++;
#else
  pool[(std::string)name]=texture;
#endif
  return TRUE;
}
DTexture *DTexturePool::Find(cstring name)
{
  int i;

//qdbg("DTexturePool:Find(%s)\n",name);
#ifdef DTP_NO_MAP
  for(i=0;i<count;i++)
  { if(texName[i][0]==name[0])
    {
      if(!strcmp(texName[i],name))
      {
        // Present
//qdbg("  present at index %d, ptr %p\n",i,texPtr[i]);
        return texPtr[i];
      }
    }
  }
//qdbg("  NOT found\n");
  return 0;
#else
  DTexture *t;
  t=pool[(std::string)name];
#ifdef OBS
static int cntMiss,cntHits;
qdbg("DTexturePool:Find(%s)=%p\n",name,t);
if(t)cntHits++; else cntMiss++;
qdbg("  hits: %d, misses: %d\n",cntHits,cntMiss);
#endif
  return t;
#endif
}

void DTexturePool::Clear()
// Reset entries that were stored
{
#ifdef DTP_NO_MAP
  int i;
  // Delete duplicated strings
  for(i=0;i<count;i++)
    QFREE(texName[i]);
  count=0;
#else
  pool.clear();
#endif
}

