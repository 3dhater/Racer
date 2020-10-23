// d3/material.h

#ifndef __D3_MATERIAL_H
#define __D3_MATERIAL_H

#include <qlib/types.h>
#include <d3/types.h>

class DGeode;

class DMatLayer
// Layer inside a material
// Maps quite directly to multitexturing although multipass is possible
{
 public:
  enum TextureTypes
  {
    TEX_DIFFUSE,                       // Generic diffuse color map
    TEX_SPHERE_ENV                     // Spherical environment map
  };
  DTexture *tex;
  int       texType;                   // TEX_xxx
  dfloat    transparency;              // Visibility for upper layer

 public:
  DMatLayer();
  DMatLayer(DTexture *tex,int texType=TEX_DIFFUSE);
  ~DMatLayer();

  // Attribs
  DTexture *GetTexture(){ return tex; }
  int       GetTextureType(){ return texType; }

  // Setting attribs
  void SetTexture(DTexture *tex);
  void SetTextureType(int type);

  bool PrepareToPaint();
};

class DMaterial
// Based partly on 3D Studio Max' .ASE Export
{
 public:
  enum Flags
  {
    NO_TEXTURE_ENABLING=1,		// Assume texturing is already on
    NO_MATERIAL_PROPERTIES=2,		// Don't set material state
    NO_BLEND_PROPERTIES=4,		// Don't set blending state
    SELECTED=8,                         // Material is selected (editor)
    IS_TRANSPARENT=16,                  // Contains transparency (alpha)
    XXX
  };
  enum CreateFlags
  {
    REQUEST_ENV_MAP=1                   // Request environment mapping
  };
  enum MaxThings
  { MAX_BITMAP_TEXTURE=10,		// Bitmap texturing
    MAX_LAYER=10,                       // Max #material layers
    MAX_SUB_MAT=100			// Max #submaterials
  };
  enum TextureTypes
  { TEX_DIFFUSE				// Diffuse bitmap texture
  };
  // Import from QCanvas
  enum BlendModes
  { BLEND_OFF=0,                        // Don't blend
    BLEND_SRC_ALPHA=1,                  // Most common blend; use alpha
    BLEND_CONSTANT=2                    // Use constant color
  };

  qstring name;
  qstring className;
  int     flags;                        // 'Live' flags

  // Creation flags
  int    createFlags;
  QColor selColor;                      // Selection color (if selected)

  // Lighting (corresponds to OpenGL)
  dfloat ambient[4];
  dfloat diffuse[4];
  dfloat specular[4];
  dfloat emission[4];
  dfloat shininess;
  // Transparency is not standard OpenGL but may be implemented somehow
  dfloat transparency;
  // Blending (not supported by Max by default)
  int blendMode;

  // Bitmap textures
  DTexture *tex[MAX_BITMAP_TEXTURE];
  bool      texIsPrivate[MAX_BITMAP_TEXTURE];   // Allocate by material?
  int       textures;
  // UVW texture mapping
  dfloat uvwUoffset,uvwVoffset;
  dfloat uvwUtiling,uvwVtiling;
  dfloat uvwAngle,uvwBlur,uvwBlurOffset;

  // Layers (added on 21-05-01)
  DMatLayer *layer[MAX_LAYER];
  int        layers;

  // Tree
  DMaterial *submaterial[MAX_SUB_MAT];
  int submaterials;

  // User data
  void *userData;

 public:
  DMaterial();
  DMaterial(DMaterial *master);
  ~DMaterial();

  // Attribs
  cstring GetName(){ return name; }
  void    EnableSelect(){ flags|=SELECTED; }
  void    DisableSelect(){ flags&=~SELECTED; }
  bool    IsSelected(){ if(flags&SELECTED)return TRUE; return FALSE; }
  void    EnableTransparency(){ flags|=IS_TRANSPARENT; }
  void    DisableTransparency(){ flags&=~IS_TRANSPARENT; }
  bool    IsTransparent(){ if(flags&IS_TRANSPARENT)return TRUE; return FALSE; }
  void   *GetUserData(){ return userData; }
  void    SetUserData(void *ptr){ userData=ptr; }

  // Layer support
  DMatLayer *GetLayer(int n){ return layer[n]; }
  int        GetLayers(){ return layers; }
  bool       AddLayer(DMatLayer *layer);

  // Bitmap texturing
  bool AddBitMap(cstring fileName,int where=TEX_DIFFUSE);
  void SetTexture(int n,DTexture *tex);
  DTexture *GetTexture(int n);

  // Attribs
  void SetDiffuseColor(float r,float g,float b,float a=1.0);
  void SetSpecularColor(float r,float g,float b,float a=1.0);
  void SetShininess(dfloat shininess);
  void Enable(int flags);
  void Disable(int flags);

  // I/O
  bool ImportDOF(QFile *f,DGeode *geode=0);
  bool ExportDOF(QFile *f);

  // OpenGL link
  bool PrepareToPaint(int layer=0);
  void UnPrepare(int layer=0);
};

#endif
