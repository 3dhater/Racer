/*
 * DMaterial - materials
 * 15-03-00: Created!
 * 16-11-00: Import/export for DOF files
 * NOTES:
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#pragma hdrstop
#include <d3/material.h>
#include <d3/geode.h>
#include <qlib/app.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DMaterial"

DMaterial::DMaterial()
{
  DBG_C("ctor")

  int i;

  // Defaults (matches OpenGL defaults)
  for(i=0;i<4;i++)
  {
    ambient[i]=0.2;
    diffuse[i]=0.8;
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
  uvwUoffset=uvwVoffset=0.0;
  uvwUtiling=uvwVtiling=1.0;
  uvwBlur=uvwBlurOffset=0;
  uvwAngle=0;
  
  submaterials=0;
}
DMaterial::DMaterial(DMaterial *master)
// Clone the material and submaterials
// Textures are REFERENCED, but submaterials are COPIED
{
  DBG_C("copy-ctor")

  int i;

  name=master->name;
  className=master->className;

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
    if(submaterial[i])
      delete submaterial[i];
}

/************
* Modifying *
************/
void DMaterial::SetDiffuseColor(float r,float g,float b,float a)
{
  diffuse[0]=r;
  diffuse[1]=g;
  diffuse[2]=b;
  diffuse[3]=a;
}

bool DMaterial::AddBitMap(cstring fname,int where)
// Adds the bitmap to the material
{
  DBG_C("AddBitMap")

  //char fileName[256];
  
  // Max reached?
  if(textures==MAX_BITMAP_TEXTURE)
    return FALSE;

//qdbg("DMat:AddBM(%s)\n",fname);

#ifdef OBS
  QImage img(fname);
  if(!img.IsRead())
    return FALSE;
#endif
  QImage *img;
  img=new QImage(fname);
  //QCV->Select();
  tex[textures]=new DBitMapTexture(img);
  // Record filename to allow for saving DMaterials
  // only store file part, NOT the path
  tex[textures]->SetName(QFileBase(fname));
  // Default is to repeat textures
  tex[textures]->SetWrap(DTexture::REPEAT,DTexture::REPEAT);
  delete img;
  textures++;
  return TRUE;
}

DTexture *DMaterial::GetTexture(int n)
{
  return tex[n];
}
void DMaterial::SetTexture(int n,DTexture *_tex)
{
  DBG_C("SetTexture")
  DBG_ARG_I(n)
  DBG_ARG_P(_tex)

  tex[n]=_tex;
}

void DMaterial::PrepareToPaint()
// Use this material for painting OpenGL polygons
{
  DBG_C("PrepareToPaint")

  if(textures)
  {
    // Diffuse map
//qdbg("DMat:PTP; tex %p select (this=%p)\n",tex[0],this);
    if(tex[0])tex[0]->Select();
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  } else
  { glDisable(GL_TEXTURE_2D);
  }
  // Set lighting/material
#ifdef OBS
qdbg("PTP; ambient=%f,%f,%f\n",ambient[0],ambient[1],ambient[2]);
qdbg("PTP; diffuse=%f,%f,%f\n",diffuse[0],diffuse[1],diffuse[2]);
qdbg("PTP; specular=%f,%f,%f\n",specular[0],specular[1],specular[2]);
qdbg("PTP; emission=%f,%f,%f\n",emission[0],emission[1],emission[2]);
qdbg("PTP; shininess=%f\n",shininess);
#endif
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

  // Blending
  if(blendMode==BLEND_OFF)
  { glDisable(GL_BLEND);
  } else if(blendMode==BLEND_SRC_ALPHA)
  { glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  } else
  { qwarn("DMaterial:PrepareToPaint; unsupported blend mode %d",blendMode);
    glDisable(GL_BLEND);
  }
//glDisable(GL_BLEND);
}

/******
* I/O *
******/
bool DMaterial::ExportDOF(QFile *f)
// Write the material into a file
{
  DBG_C("ExportDOF")

  int i,len,off,off2;
  
  f->Write("MAT0",4);
  off=f->Tell();
  f->Write(&len,sizeof(len));
  f->Write(name);
  f->Write(className);
  f->Write(ambient,sizeof(ambient));
  f->Write(diffuse,sizeof(diffuse));
  f->Write(specular,sizeof(specular));
  f->Write(emission,sizeof(emission));
  f->Write(&shininess,sizeof(shininess));
  f->Write(&transparency,sizeof(transparency));
  f->Write(&blendMode,sizeof(blendMode));
  f->Write(&uvwUoffset,sizeof(uvwUoffset));
  f->Write(&uvwVoffset,sizeof(uvwVoffset));
  f->Write(&uvwUtiling,sizeof(uvwUtiling));
  f->Write(&uvwVtiling,sizeof(uvwVtiling));
  f->Write(&uvwAngle,sizeof(uvwAngle));
  f->Write(&uvwBlur,sizeof(uvwBlur));
  f->Write(&uvwBlurOffset,sizeof(uvwBlurOffset));
  // Textures
  f->Write(&textures,sizeof(textures));
  // Names of textures
  for(i=0;i<textures;i++)
  {
    f->Write(*tex[i]->GetNameQ());
  }
  
  // Submaterials
  f->Write(&submaterials,sizeof(submaterials));
  for(i=0;i<submaterials;i++)
  {
    submaterial[i]->ExportDOF(f);
  }
  
  // Record size of chunk
  off2=f->Tell();
  f->Seek(off);
  len=off2-off;
  f->Write(&len,sizeof(len));
  
  // Return to previous write position
  f->Seek(off2);
  
  return TRUE;
}

bool DMaterial::ImportDOF(QFile *f,DGeode *geode)
// Read the material from a file
{
  DBG_C("ImportDOF")
  
  int i,len,n;
  char buf[8];
  
  f->Read(buf,4);     // MAT0
  f->Read(&len,sizeof(len));
  f->Read(name);
  f->Read(className);
  f->Read(ambient,sizeof(ambient));
  f->Read(diffuse,sizeof(diffuse));
  f->Read(specular,sizeof(specular));
  f->Read(emission,sizeof(emission));
  f->Read(&shininess,sizeof(shininess));
  f->Read(&transparency,sizeof(transparency));
  f->Read(&blendMode,sizeof(blendMode));
  f->Read(&uvwUoffset,sizeof(uvwUoffset));
  f->Read(&uvwVoffset,sizeof(uvwVoffset));
  f->Read(&uvwUtiling,sizeof(uvwUtiling));
  f->Read(&uvwVtiling,sizeof(uvwVtiling));
  f->Read(&uvwAngle,sizeof(uvwAngle));
  f->Read(&uvwBlur,sizeof(uvwBlur));
  f->Read(&uvwBlurOffset,sizeof(uvwBlurOffset));
  // Textures
  f->Read(&n,sizeof(n));
  // Names of textures
  textures=0;
  for(i=0;i<n;i++)
  {
    qstring s;
    f->Read(s);
qdbg("DMat:ImportDOF; material '%s'\n",(cstring)s);
    if(geode)
    {
      // Use the geode's path
      char fname[256];
      sprintf(fname,"%s/%s",(cstring)geode->GetMapPath(),(cstring)s);
      AddBitMap(fname);
    } else
    { // Relative to current directory (default)
      AddBitMap(s);
    }
  }
  
  // Submaterials
  f->Read(&submaterials,sizeof(submaterials));
  for(i=0;i<submaterials;i++)
  {
    submaterial[i]=new DMaterial();
    submaterial[i]->ImportDOF(f,geode);
  }
  
  return TRUE;
}
