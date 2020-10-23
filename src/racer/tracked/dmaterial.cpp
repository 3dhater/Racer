/*
 * DMaterial - materials
 * 15-03-00: Created!
 * 16-11-00: Import/export for DOF files
 * 27-12-00: MAT0 format changed considerably; split into chunks.
 * 06-01-01: Optimization flags to reduce OpenGL state changes.
 * BUGS:
 * - Upon dtor, doesn't destroy tex's (mind the pool!)
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif
#include <d3/material.h>
#include <d3/global.h>
#include <d3/geode.h>
#include <qlib/app.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DMaterial"

// Envmapping rotation test
dfloat d3Yaw;

DMaterial::DMaterial()
  : flags(0),createFlags(0)
{
  DBG_C("ctor")

  int i;

  // Defaults (matches OpenGL defaults)
  for(i=0;i<4;i++)
  {
    ambient[i]=0.2f;
    diffuse[i]=0.8f;
    specular[i]=0;
    emission[i]=0;
  }
  diffuse[3]=1;
  emission[3]=1;
  specular[3]=1;
  shininess=0;
  transparency=0;
  blendMode=BLEND_OFF;

  textures=0;
  for(i=0;i<MAX_BITMAP_TEXTURE;i++)
  {
    tex[i]=0;
    texIsPrivate[i]=FALSE;
  }

  uvwUoffset=uvwVoffset=0.0;
  uvwUtiling=uvwVtiling=1.0;
  uvwBlur=uvwBlurOffset=0;
  uvwAngle=0;
  
  submaterials=0;
  for(i=0;i<MAX_SUB_MAT;i++)
    submaterial[i]=0;
  userData=0;
  
  // Always 1 layer (future will have 0 layers by default)
  layers=1;
  for(i=0;i<MAX_LAYER;i++)
    layer[i]=0;
}
DMaterial::DMaterial(DMaterial *master)
// Clone the material and submaterials
// Textures are REFERENCED, but submaterials are COPIED
{
  DBG_C("copy-ctor")

  int i;

  name=master->name;
  className=master->className;
  flags=master->flags;
  
  for(i=0;i<4;i++)
  {
    ambient[i]=master->ambient[i];
    diffuse[i]=master->diffuse[i];
    specular[i]=master->specular[i];
    emission[i]=master->emission[i];
  }
  shininess=master->shininess;
  transparency=master->transparency;
  blendMode=master->blendMode;

  // Take over textures by reference
  textures=master->textures;
  for(i=0;i<textures;i++)
    tex[i]=master->tex[i];

  // Take over materials by copy
  submaterials=master->submaterials;
  for(i=0;i<submaterials;i++)
    submaterial[i]=new DMaterial(master->submaterial[i]);
}

DMaterial::~DMaterial()
{
  DBG_C("dtor")

  int i;
  for(i=0;i<submaterials;i++)
    QDELETE(submaterial[i]);
  // Delete textures NOT from the pool (created by material)
  for(i=0;i<textures;i++)
    if(texIsPrivate[i])
      QDELETE(tex[i]);
  for(i=0;i<layers;i++)
    QDELETE(layer[i]);
}

/*************
* Attributes *
*************/
void DMaterial::SetDiffuseColor(float r,float g,float b,float a)
// Set RGBA of diffuse color
{
  diffuse[0]=r;
  diffuse[1]=g;
  diffuse[2]=b;
  diffuse[3]=a;
}
void DMaterial::SetSpecularColor(float r,float g,float b,float a)
// Set RGBA of specular color
{
  specular[0]=r;
  specular[1]=g;
  specular[2]=b;
  specular[3]=a;
}
void DMaterial::SetShininess(dfloat v)
// Set shininess (exponent) of material
{
  shininess=v;
}
void DMaterial::Enable(int newFlags)
{
  flags|=newFlags;
}
void DMaterial::Disable(int _flags)
{
  flags&=~_flags;
}

/*********
* Layers *
*********/
bool DMaterial::AddLayer(DMatLayer *newLayer)
{
  if(layers>=MAX_LAYER)
  { qwarn("DMaterial::AddLayer(); max (%d) reached",layers);
    return FALSE;
  }
  layer[layers]=newLayer;
  layers++;
  return TRUE;
}

/***********************
* Material Layer Class *
***********************/
#undef  DBG_CLASS
#define DBG_CLASS "DMatLayer"

DMatLayer::DMatLayer()
{
  tex=0;
  texType=TEX_DIFFUSE;
  transparency=1.0f;
}
DMatLayer::DMatLayer(DTexture *_tex,int type)
  : tex(_tex), texType(type)
{
  transparency=1.0f;
}
DMatLayer::~DMatLayer()
{
}
bool DMatLayer::PrepareToPaint()
{
  //QShowGLErrors("DMatLayer::PrepareToPaint()");
  // Install texture
  if(tex)
  {
    tex->Select();
  }
  if(texType==TEX_SPHERE_ENV)
  {
    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
    //glTexGenfv(GL_S,GL_SPHERE_MAP,0);
    //glTexGenfv(GL_T,GL_SPHERE_MAP,0);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
  }
  // Blend with pixels from previous layer
#ifdef GL_EXT_blend_color
  if(GetCurrentQGLContext()->IsExtensionSupported(
     QGLContext::EXT_blend_color))
  {
    //QShowGLErrors("PREBLEND DMatLayer::PrepareToPaint");
    //{const GLubyte *s=glGetString(GL_EXTENSIONS);qerr("glExt=%s",s);}
    glBlendFunc(GL_CONSTANT_COLOR,GL_ONE_MINUS_CONSTANT_COLOR);
    // Following line gives error 1282 (invalid op)
    // on Windows/nVidia6.50/Geforce2MX
    glBlendColor(transparency,transparency,transparency,transparency);
    glEnable(GL_BLEND);
    
    // Try additive if possible
    if(GetCurrentQGLContext()->IsExtensionSupported(
       QGLContext::ARB_texture_env_add))
      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_ADD);
    //QShowGLErrors("POSTBLEND DMatLayer::PrepareToPaint");
    //glBlendColor(.1,.5,.2,1);
    //glDisable(GL_TEXTURE_GEN_S);
    //glDisable(GL_TEXTURE_GEN_T);
    //glDisable(GL_DEPTH_TEST);
  }
#endif

  return TRUE;
}

#undef  DBG_CLASS
#define DBG_CLASS "DMaterial"

/******************
* Bitmap textures *
******************/
bool DMaterial::AddBitMap(cstring fname,int where)
// Adds the bitmap to the material
{
  DBG_C("AddBitMap")
  DTexture *t;
  bool fTransparent;
  
//qdbg("DMaterial:AddBitMap()\n");
  // Max reached?
  if(textures==MAX_BITMAP_TEXTURE)
    return FALSE;

  if(textures==0)
  {
    // Set name of material to texture name, to
    // recognize later on a bit
    name=QFileBase(fname);
//qdbg("DMaterial::AddBitMap(); name=%s\n",name.cstr());
  }

  if(dglobal.flags&DGlobal::USE_TEXTURE_POOL)
  {
    // Check pool first, to save bitmap loading (for Racer tracks
    // for example)
    t=dglobal.texturePool->Find(QFileBase(fname));
//qdbg("Check pool: '%s' = tex(%p)\n",QFileBase(fname),t);
    if(t)
    {
      tex[textures]=t;
      // Default is to repeat textures
      tex[textures]->SetWrap(DTexture::REPEAT,DTexture::REPEAT);
      texIsPrivate[textures]=FALSE;
      textures++;
      return TRUE;
    }
  }
  
  // Load the image
  QImage *img=0;
  QBitMap *bm=0;

  fTransparent=FALSE;
  if(!QFileExists(fname))
  {
    qwarn("Can't load material texture image '%s'\n",fname);
    // Supply our own
   supply_own:
    QColor c(255,0,0);
    bm=new QBitMap(32,1,1);
    bm->SetColorAt(0,0,&c);
  } else
  {
    // Try and load image
    img=new QImage(fname);
#ifdef OBS
qdbg("img '%s'\n",fname);
{char buf[128];
  sprintf(buf,"/tmp/%s",QFileBase(fname));
  img->Write(buf,QBMFORMAT_TGA);
}
#endif
    if(!img->IsRead())
    {
      // Supply easy to recognize image
      QDELETE(img);
      goto supply_own;
    }
    // For .BMP images, key out purple (Racer)
//qdbg("dmaterial; keypurple=%d\n",img->KeyPurple());
    if(img->KeyPurple())
    {
      // Key colors found, make sure texture is known to be transparent
      fTransparent=TRUE;
    }
  }
  // Create texture from image
  if(bm)tex[textures]=new DBitMapTexture(bm);
  else
  { tex[textures]=new DBitMapTexture(img);
    if(fTransparent)
      tex[textures]->DeclareTransparent();
  }
  texIsPrivate[textures]=TRUE;

  // Record filename to allow for saving DMaterials
  // only store file part, NOT the path
  tex[textures]->SetName(QFileBase(fname));
  // Default is to repeat textures
  tex[textures]->SetWrap(DTexture::REPEAT,DTexture::REPEAT);
  QDELETE(img);
  QDELETE(bm);
  
  if(dglobal.flags&DGlobal::USE_TEXTURE_POOL)
  {
    // Add image name to the pool
    dglobal.texturePool->Add(QFileBase(fname),tex[textures]);
  }
  
  // Another texture
  textures++;
  return TRUE;
}

DTexture *DMaterial::GetTexture(int n)
{
  QASSERT(n>=0&&n<MAX_BITMAP_TEXTURE);
  // Texture present?
  if(n>=textures)return 0;
  // Return the texture
  return tex[n];
}
void DMaterial::SetTexture(int n,DTexture *_tex)
{
  DBG_C("SetTexture")
  DBG_ARG_I(n)
  DBG_ARG_P(_tex)

  tex[n]=_tex;
}

/*******************
* Preparing OpenGL *
*******************/
bool DMaterial::PrepareToPaint(int layerIndex)
// Select layer 'layerIndex' for painting.
// If 'layerIndex'==-1, all layers are attempted at once.
// It's a bit raw; we must have a better Shader handler.
// Returns FALSE if there's no need to paint this layer,
// since it was done with multitexture.
{
  DBG_C("PrepareToPaint")
  
  if(IsSelected())
  {
    // Special material to identify material
    float diffuse[4];
    diffuse[0]=((float)selColor.GetR())/255.0f;
    diffuse[1]=((float)selColor.GetG())/255.0f;
    diffuse[2]=((float)selColor.GetB())/255.0f;
    diffuse[3]=((float)selColor.GetA())/255.0f;
    // Only draw 1 layer?
    if(layerIndex>0)
      return TRUE;
    glDisable(GL_TEXTURE_2D);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
    return TRUE;
  }
  
  // Special case; 1 normal layer and 1 envmap
  // (Racer cars for example)
  if(layers==2&&dglobal.texEnv)
  {
    if(layer[1]->GetTextureType()==DMatLayer::TEX_SPHERE_ENV)
    {
      // Use multitexture?
#ifdef GL_ARB_multitexture
      if(GetCurrentQGLContext()->IsExtensionSupported(
         QGLContext::ARB_multitexture)&&
         GetCurrentQGLContext()->IsExtensionSupported(
         QGLContext::ARB_texture_env_add))
      {
        if(layerIndex>0)
        {
          // No need to do this layer; done already
          // using multitexturing
          return FALSE;
        }
        glActiveTextureARB(GL_TEXTURE1_ARB);
        // Install envmap texture
        glEnable(GL_TEXTURE_2D);
        dglobal.texEnv->Select();
        glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
        glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        // Blend with previous layer
        //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_ZERO,GL_SRC_COLOR);
        //glBlendFunc(GL_ZERO,GL_SRC_ALPHA);
        //glBlendFunc(GL_CONSTANT_COLOR,GL_ONE_MINUS_CONSTANT_COLOR);
        //glBlendColor(0.1,0.1,0.1,0.1);
        //glEnable(GL_BLEND);
        glDisable(GL_BLEND);
        if(IsTransparent())
        {
          // DECAL gives better results (even correct?)
          glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
        } else
        {
          // Add reflection; using DECAL would darken the colors
          glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_ADD);
        }
        //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
        //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_BLEND);
        //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
//#ifdef ND_ROTATE_TEST
        // Rotate yaw angle
        static float a;
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        a=d3Yaw;
        glTranslatef(0.5,0,0.5);
        glRotatef(a,0,1,0);
        //a+=0.1f;
        glMatrixMode(GL_MODELVIEW);
//#endif

		    glActiveTextureARB(GL_TEXTURE0_ARB);
      }
#endif
    }
  }

  if(layerIndex>0)
  {
    // Real layers; pass on to layer
    return layer[layerIndex]->PrepareToPaint();
  }

  // Texturing
  if(textures)
  {
    // Diffuse image texture map
//qdbg("DMat:PTP; tex %p select (this=%p)\n",tex[0],this);
    if(tex[0])tex[0]->Select();
    if(!(flags&NO_TEXTURE_ENABLING))
      glEnable(GL_TEXTURE_2D);
#ifdef OBS
{
int n; glGetTexEnviv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,&n);
qdbg("DMat:PTP; envmode=0x%x, MODULATE=%x, REPLACE=%x\n",
n,GL_MODULATE,GL_REPLACE);
}
#endif
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    
  } else
  {
#ifdef OBS
qdbg("DMaterial:PrepareToPaint(); no tex, enabling=%d\n",
flags&NO_TEXTURE_ENABLING);
#endif
    if(!(flags&NO_TEXTURE_ENABLING))
      glDisable(GL_TEXTURE_2D);
  }

#ifdef OBS
{glEnable(GL_TEXTURE_2D);float v[4]={0.1,0.2,0.8};
glMaterialfv(GL_FRONT,GL_DIFFUSE,v);return TRUE;}
#endif

  // Set lighting/material properties
  if(!(flags&NO_MATERIAL_PROPERTIES))
  {
qdbg("PTP; ambient=%f,%f,%f,%f\n",ambient[0],ambient[1],ambient[2],ambient[3]);
qdbg("PTP; diffuse=%f,%f,%f,%f\n",diffuse[0],diffuse[1],diffuse[2],diffuse[3]);
qdbg("PTP; specular=%f,%f,%f,%f\n",specular[0],specular[1],specular[2],specular[3]);
qdbg("PTP; emission=%f,%f,%f,%f\n",emission[0],emission[1],emission[2],emission[3]);
qdbg("PTP; shininess=%f\n",shininess);
//ambient[0]=ambient[1]=ambient[2]=0.8f;
//diffuse[0]=diffuse[1]=diffuse[2]=0.4f;
    glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
    glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
#ifdef OBS
{float v[4]={ 1,1,1,1 };
 glMaterialfv(GL_FRONT,GL_SPECULAR,v);
}
#endif
    glMaterialfv(GL_FRONT,GL_EMISSION,emission);
    glMaterialf(GL_FRONT,GL_SHININESS,shininess);
//glMaterialf(GL_FRONT,GL_SHININESS,128);
//glDisable(GL_TEXTURE_2D);
//glDisable(GL_BLEND);
  }
  
  // Blending
  if(!(flags&NO_BLEND_PROPERTIES))
  {
//qdbg("DMat:Prep(); blendMode=%d\n",blendMode);
    if(blendMode==BLEND_OFF)
    { glDisable(GL_BLEND);
    } else if(blendMode==BLEND_SRC_ALPHA)
    { glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glColor4f(1,1,1,1);
      //glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
      //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
      //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
      glDepthMask(GL_FALSE);
      //glDisable(GL_CULL_FACE);
      //glDisable(GL_BLEND);
    } else
    { qwarn("DMaterial:PrepareToPaint; unsupported blend mode %d",blendMode);
      glDisable(GL_BLEND);
    }
//glDisable(GL_BLEND);
  }
  return TRUE;
}
void DMaterial::UnPrepare(int layerIndex)
// Undo mode changes done by PrepareToPaint()
{
  // Special case; 1 normal layer and 1 envmap
  // (Racer cars for example)
  if(layers==2)
  {
    if(layer[1]->GetTextureType()==DMatLayer::TEX_SPHERE_ENV)
    {
      // Use multitexture?
#ifdef GL_ARB_multitexture
      // No work done in layer 1
      if(layerIndex>0)return;

      if(GetCurrentQGLContext()->IsExtensionSupported(
         QGLContext::ARB_multitexture)&&
         GetCurrentQGLContext()->IsExtensionSupported(
         QGLContext::ARB_texture_env_add))
      {
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glDisable(GL_TEXTURE_2D);
        glActiveTextureARB(GL_TEXTURE0_ARB);
      }
#endif
    }
  }

  if(textures)
  {
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
  }
  if(blendMode==BLEND_SRC_ALPHA)
  {
    // Restore tex environment
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    //glEnable(GL_CULL_FACE);
  }
}

/******
* I/O *
******/
bool DMaterial::ExportDOF(QFile *f)
// Write the material into a file
// ASSUMES: dfloat==float (otherwise PC ordering will fail on MIPS)
{
  DBG_C("ExportDOF")

  int i,len,off,off2,n;
  
  f->Write("MAT0",4);
  off=f->Tell();
  len=0; n=QHost2PC_L(len);
  f->Write(&n,sizeof(n));
  
  // Header
  {
    int pos,posNext;
    f->Write("MHDR",4);
    pos=f->Tell();
    len=QHost2PC_L(0); f->Write(&len,sizeof(len));
    f->Write(name);
    f->Write(className);
    posNext=f->Tell();
    f->Seek(pos); len=QHost2PC_L(posNext-pos-4); f->Write(&len,sizeof(len));
    f->Seek(posNext);
  }

  // Colors
  f->Write("MCOL",4);
  len=(4*4+1)*sizeof(dfloat); n=QHost2PC_L(len); f->Write(&n,sizeof(n));
  QHost2PC_FA(ambient,4);
  QHost2PC_FA(diffuse,4);
  QHost2PC_FA(specular,4);
  QHost2PC_FA(emission,4);
  QHost2PC_FA(&shininess,1);
  f->Write(ambient,sizeof(ambient));
  f->Write(diffuse,sizeof(diffuse));
  f->Write(specular,sizeof(specular));
  f->Write(emission,sizeof(emission));
  f->Write(&shininess,sizeof(shininess));
  QPC2Host_FA(ambient,4);
  QPC2Host_FA(diffuse,4);
  QPC2Host_FA(specular,4);
  QPC2Host_FA(emission,4);
  QPC2Host_FA(&shininess,1);
  
  // UVW mapping
  f->Write("MUVW",4);
  len=7*sizeof(dfloat); n=QHost2PC_L(len); f->Write(&n,sizeof(n));
  QHost2PC_FA(&uvwUoffset,1);
  QHost2PC_FA(&uvwVoffset,1);
  QHost2PC_FA(&uvwUtiling,1);
  QHost2PC_FA(&uvwVtiling,1);
  QHost2PC_FA(&uvwAngle,1);
  QHost2PC_FA(&uvwBlur,1);
  QHost2PC_FA(&uvwBlurOffset,1);
  f->Write(&uvwUoffset,sizeof(uvwUoffset));
  f->Write(&uvwVoffset,sizeof(uvwVoffset));
  f->Write(&uvwUtiling,sizeof(uvwUtiling));
  f->Write(&uvwVtiling,sizeof(uvwVtiling));
  f->Write(&uvwAngle,sizeof(uvwAngle));
  f->Write(&uvwBlur,sizeof(uvwBlur));
  f->Write(&uvwBlurOffset,sizeof(uvwBlurOffset));
  QPC2Host_FA(&uvwUoffset,1);
  QPC2Host_FA(&uvwVoffset,1);
  QPC2Host_FA(&uvwUtiling,1);
  QPC2Host_FA(&uvwVtiling,1);
  QPC2Host_FA(&uvwAngle,1);
  QPC2Host_FA(&uvwBlur,1);
  QPC2Host_FA(&uvwBlurOffset,1);
  
  // Misc
  f->Write("MTRA",4);
  len=QHost2PC_L(1*sizeof(dfloat)+1*sizeof(long));
  f->Write(&len,sizeof(len));
  QHost2PC_FA(&transparency,1);
  QHost2PC_LA((unsigned long*)&blendMode,1);
  f->Write(&transparency,sizeof(transparency));
  f->Write(&blendMode,sizeof(blendMode));
  QPC2Host_FA(&transparency,1);
  QPC2Host_LA((unsigned long*)&blendMode,1);

  // Creation flags
  f->Write("MCFL",4);
  len=QHost2PC_L(1*sizeof(int));
  f->Write(&len,sizeof(len));
  QHost2PC_IA(&createFlags,1);
  f->Write(&createFlags,sizeof(createFlags));
  QHost2PC_IA(&createFlags,1);
//qdbg("DMat:ExportDOF(); createFlags=0x%x\n",createFlags);
  
  // Textures
  {
    int pos,posNext;
    f->Write("MTEX",4);
    pos=f->Tell();
    len=QHost2PC_L(0); f->Write(&len,sizeof(len));
    n=QHost2PC_L(textures); f->Write(&n,sizeof(n));
    // Names of textures
    for(i=0;i<textures;i++)
    {
      f->Write(*tex[i]->GetNameQ());
    }
    posNext=f->Tell();
    f->Seek(pos); len=QHost2PC_L(posNext-pos-4); f->Write(&len,sizeof(len));
    f->Seek(posNext);
  }
  
  // Submaterials
  {
    int pos,posNext;
    f->Write("MSUB",4);
    pos=f->Tell();
    len=QHost2PC_L(0); f->Write(&len,sizeof(len));
    n=QHost2PC_L(submaterials); f->Write(&n,sizeof(n));
    for(i=0;i<submaterials;i++)
    {
      submaterial[i]->ExportDOF(f);
    }
    posNext=f->Tell();
    f->Seek(pos); len=QHost2PC_L(posNext-pos-4); f->Write(&len,sizeof(len));
    f->Seek(posNext);
  }
  
  f->Write("MEND",4);
  
  // Record size of material chunk
  off2=f->Tell();
  f->Seek(off);
  len=QHost2PC_L(off2-off-4); f->Write(&len,sizeof(len));
  f->Seek(off2);
  
  return TRUE;
}

bool DMaterial::ImportDOF(QFile *f,DGeode *geode)
// Read the material from a file
{
  DBG_C("ImportDOF")
  
  int i,len,n;
  char buf[8];

//qlog(QLOG_INFO,"DMat:ImportDOF(f=%p)\n",f);
  f->Read(buf,4); buf[4]=0;     // MAT0
//qdbg("DMat:ImportDOF(); MAT0='%s'?\n",buf);
  f->Read(&len,sizeof(len)); len=QPC2Host_L(len);

  // Reset things that may not be in the file
  createFlags=0;
  
  // Read chunks
  while(1)
  {
    f->Read(buf,4); buf[4]=0;
    if(f->IsEOF())break;
    if(!strcmp(buf,"MEND"))break;
    // All non-MEND chunks have a length
    f->Read(&n,sizeof(n)); len=QPC2Host_L(n);
    if(!strcmp(buf,"MCOL"))
    { // Color
      f->Read(ambient,sizeof(ambient));
      f->Read(diffuse,sizeof(diffuse));
      f->Read(specular,sizeof(specular));
      f->Read(emission,sizeof(emission));
      f->Read(&shininess,sizeof(shininess));
      QPC2Host_FA(ambient,4);
      QPC2Host_FA(diffuse,4);
      QPC2Host_FA(specular,4);
      QPC2Host_FA(emission,4);
      QPC2Host_FA(&shininess,1);
    } else if(!strcmp(buf,"MHDR"))
    {
      f->Read(name);
      f->Read(className);
//qdbg("  name='%s', className='%s'\n",(cstring)name,(cstring)className);
    } else if(!strcmp(buf,"MTRA"))
    {
      // Extra attribs
      f->Read(&transparency,sizeof(transparency));
      f->Read(&blendMode,sizeof(blendMode));
      QPC2Host_FA(&transparency,1);
      QPC2Host_LA((unsigned long*)&blendMode,1);
      // Detect transparent material
      if(blendMode!=BLEND_OFF)
        EnableTransparency();
    } else if(!strcmp(buf,"MCFL"))
    {
      // Creation flags
      f->Read(&createFlags,sizeof(createFlags));
      QPC2Host_IA(&createFlags,1);
//qdbg("DMat:ImportDOF(); createFlags=0x%x\n",createFlags);
    } else if(!strcmp(buf,"MUVW"))
    {
      f->Read(&uvwUoffset,sizeof(uvwUoffset));
      f->Read(&uvwVoffset,sizeof(uvwVoffset));
      f->Read(&uvwUtiling,sizeof(uvwUtiling));
      f->Read(&uvwVtiling,sizeof(uvwVtiling));
      f->Read(&uvwAngle,sizeof(uvwAngle));
      f->Read(&uvwBlur,sizeof(uvwBlur));
      f->Read(&uvwBlurOffset,sizeof(uvwBlurOffset));
      QPC2Host_FA(&uvwUoffset,1);
      QPC2Host_FA(&uvwVoffset,1);
      QPC2Host_FA(&uvwUtiling,1);
      QPC2Host_FA(&uvwVtiling,1);
      QPC2Host_FA(&uvwAngle,1);
      QPC2Host_FA(&uvwBlur,1);
      QPC2Host_FA(&uvwBlurOffset,1);
    } else if(!strcmp(buf,"MTEX"))
    {
      // Textures
      f->Read(&n,sizeof(n));
      n=QPC2Host_L(n);
      // Names of textures
      textures=0;
      for(i=0;i<n;i++)
      {
        qstring s;
        f->Read(s);
//qdbg("DMat:ImportDOF; texture '%s'\n",(cstring)s);
        if(geode)
        {
          // Use the geode's path
          char fname[256];
          geode->FindMapFile(s,fname);
#ifdef OBS
          cstring path;
          path=(cstring)geode->GetMapPath();
          sprintf(fname,"%s/%s",path[0]?path:"images",(cstring)s);
#endif
          AddBitMap(fname);
        } else
        { // Relative to images/ (default)
          char fname[256];
          sprintf(fname,"images/%s",(cstring)s);
          AddBitMap(fname);
        }
      }
    } else if(!strcmp(buf,"MSUB"))
    {
      // Submaterials
      f->Read(&submaterials,sizeof(submaterials));
      submaterials=QPC2Host_L(submaterials);
//qdbg("  submats=%d\n",submaterials);
      for(i=0;i<submaterials;i++)
      {
        submaterial[i]=new DMaterial();
        submaterial[i]->ImportDOF(f,geode);
      }
    } else
    {
qdbg("Unknown chunk '%s'\n",buf);
      f->Seek(len,QFile::S_CUR);
    }
  }

  // Post-import
  
//qdbg("DMat:ImportDOF(); createFlags=%d\n",createFlags);
  if(createFlags&REQUEST_ENV_MAP)
  {
//qdbg("DMat:ImportDOF(); Req Env Map\n");
    // Try to add an environment map
    DMatLayer *layer;
    DTexture  *texEnv;
    texEnv=dglobal.GetDefaultEnvironmentMap();
    if(!texEnv)
    {
      static bool fReported;
      if(!fReported)
      {
        qwarn("DMaterial:ImportDOF(); can't create envmap; lacking default");
        fReported=TRUE;
      }
    } else
    {
      // Create material layer with the sphere map
      layer=new DMatLayer(texEnv,DMatLayer::TEX_SPHERE_ENV);
      layer->transparency=0.4f;
      // Add it to the material as a separate rendering layer
      AddLayer(layer);
    }
  }
  
  return TRUE;
}
