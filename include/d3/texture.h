// dtexture.h - declaration

#ifndef __DTEXTURE_H
#define __DTEXTURE_H

#include <qlib/bitmap.h>
#include <qlib/opengl.h>
#include <qlib/dmpbuffer.h>
#include <qlib/dmbpool.h>

class DTexture
// Basic texture class; handles settings/options
{
 public:
  enum WrapOptions
  {
    CLAMP=GL_CLAMP,
    REPEAT=GL_REPEAT
  };
  enum Flags
  {
    HAS_TRANSPARENCY=1          // Has transparent pixels
  };
  enum EnvModes
  // Modes that specify how the texture is applied
  {
    MODULATE=GL_MODULATE,	// For lighting
    DECAL=GL_DECAL,		// Just place it
    BLEND=GL_BLEND,		// Blending with colors
    REPLACE=GL_REPLACE,		// Place it too?
    ADD=GL_ADD			// Becomes white soon
  };

 protected:
  // Class member variables
  QGLContext *gl;		// Context in which texture resides
  int     textureID;		// OpenGL texture ID
  int     flags;                // Generic flags
  int     wid,hgt;		// Size of texture source
  qstring name;			// Identifying the texture

  void CreateTexture();

 public:
  DTexture();
  virtual ~DTexture();

  void     SetSize(int wid,int hgt);
  void     SetName(cstring name);
  cstring  GetName(){ return name; }
  qstring *GetNameQ(){ return &name; }

  // Flags
  bool IsTransparent(){ if(flags&HAS_TRANSPARENCY)return TRUE; return FALSE; }
  void DeclareTransparent(){ flags|=HAS_TRANSPARENCY; }
  int  GetFlags(){ return flags; }

  int GetTexID(){ return textureID; }
  int GetWidth();
  int GetHeight();

  void Select();

  void SetWrap(int sWrap,int tWrap=REPEAT,int rWrap=REPEAT,int qWrap=REPEAT);
  void SetEnvMode(int mode);
};

class DBitMapTexture : public DTexture
// Texture based on a bitmap
{
 protected:
  QBitMap *bm;			// Source bitmap
  int      bwid,bhgt;		// Bitmap size (texture may be larger)

 public:
  DBitMapTexture(QBitMap *bm);
  ~DBitMapTexture();

  void FromBitMap(QBitMap *bm);
};

class DDrawableTexture : public DTexture
// Texture that gets its image from a drawable
{
 protected:
  QDrawable *draw;
 public:
  DDrawableTexture(QDrawable *draw);
  ~DDrawableTexture();

  void Refresh(bool front=FALSE);
};

class DMovieTexture : public DTexture
// Texture based on a movie; uses a pbuffer+dmbuffer to hold the texture
{
 protected:
  QMovie *movie;
  QDMBPool *pool;
  QDMPBuffer *pbuf;
  QDMBuffer  *dmbuf;
  int         mwid,mhgt;		// Movie size

 public:
  DMovieTexture(QMovie *mv);
  ~DMovieTexture();

  QMovie *GetMovie(){ return movie; }
  bool Render();

  bool SetCurFrame(int frame);
  bool Advance();
};

#endif
