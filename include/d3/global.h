// d3/global.h - global settings

#ifndef __D3_GLOBAL_H
#define __D3_GLOBAL_H

// Don't use std::map for the texture pool?
#define DTP_NO_MAP

#include <d3/texture.h>
#include <d3/batch.h>
#ifndef DTP_NO_MAP
#include <map>
#endif
// Avoid the <string> include on Linux
// because at some installations it doesn't go
// into the STL namespace.
//#include <string>

struct DPrefs
{
  enum Flags
  {
    USE_DISPLAY_LISTS=1		// Generate display lists?
  };

  // Preferred filtering
  int minFilter,maxFilter;
  // Preferred texture generation
  int wrapS,wrapT,wrapP,wrapQ;
  // Preferred environment mode
  int envMode;

  // Misc. flags
  int flags;
  // Questions about prefs
  bool UseMipMapping();

  DPrefs();
  ~DPrefs();

  void Reset();
};

// Callback for texture pool finding (pattern matching)
typedef void (*cbTexturePoolFind)(DTexture*);

class DTexturePool
// A collection of textures, often shared
// between geodes
{
#ifdef DTP_NO_MAP
  // Explicit array (bug was found on Linux)
  char     **texName;
  DTexture **texPtr;
  int        allocated,count;
#else
  // Although weird, this seemed to work on Win32/SGI
  std::map<std::string,DTexture*> pool;
#endif

 public:
  DTexture *Find(cstring name);
  bool      Add(cstring name,DTexture *texture);
 
 public:
  DTexturePool();
  ~DTexturePool();

  void Clear();

  // High level
  int FindAll(cstring pattern,cbTexturePoolFind cbFunc=0);
};

struct DGlobal
// Global settings for the D3 environment
{
 public:
  enum Flags
  {
    USE_TEXTURE_POOL=1,		// First look at texture pool?
    USE_BATCH=2,                // Use batch rendering?
    XXX
  };

 public:
  int flags;
  DPrefs prefs;
  // Texture sharing
  DTexturePool *texturePool;
  // Environment mapping
  DTexture *texEnv;              // Default environment map

 protected:
  // Batch rendering
  DBatch *batch;

 public:
  DGlobal();
  ~DGlobal();

  DBatch *GetBatch(){ return batch; }

  // Texture sharing
  void EnableTexturePool(){ flags|=USE_TEXTURE_POOL; }
  void DisableTexturePool(){ flags&=~USE_TEXTURE_POOL; }
  bool AreTexturesPooled()
  { if(flags&USE_TEXTURE_POOL)return TRUE; return FALSE; }

  // Environment texturing
  void SetDefaultEnvironmentMap(DTexture *tex);
  DTexture *GetDefaultEnvironmentMap(){ return texEnv; }

  // Batch rendering
  void EnableBatchRendering(){ flags|=USE_BATCH; }
  void DisableBatchRendering(){ flags&=~USE_BATCH; }
  bool IsBatchRendering(){ if(flags&USE_BATCH)return TRUE; return FALSE; }

  // Handy string ops
  cstring FilterN2S(int n);
  int     FilterS2N(cstring name);
};

extern DGlobal dglobal;

#endif
