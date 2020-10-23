/*
 * RModel - model used for car components
 * 10-12-2000: Created!
 * NOTES:
 * (C) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
#include <d3/geode.h>
#include <qlib/app.h>
DEBUG_ENABLE

// Default ambient lighting (otherwise its very dark)
#define DEF_AMBIENT_LIGHT 0.9f

RModel::RModel(RCar *_car)
{
  car=_car;
  geode=0;
}
RModel::~RModel()
{
  QDELETE(geode);
}

bool RModel::Load(QInfo *info,cstring path,cstring modelTag)
// Read settings from 'info'
// 'path' indicates the original component path, i.e. 'body'
// Default 'modelTag' is 'model'; the name of the model. The car body
// for example has 2; 'model' (full car) and 'model_incar' (inside view).
// Default, if not passed, is 'model'.
// Expects DOF models.
{
  char buf[128],fname[256];
  int  i;

  if(!modelTag)modelTag="model";

  // Mind the textures
  QCV->Select();
  
  // Model
  sprintf(buf,"%s.%s.file",path,modelTag);
  info->GetString(buf,fname);
if(fname[0])qdbg("RModel:Load; file '%s' (buf=%s)\n",fname,buf);
  if(fname[0])
  {
    rfloat scaleX,scaleY,scaleZ;
    QFile *f;
    
    geode=new DGeode(0);
    // Remember filename for saves later
    fileName=RFindFile(fname,car->GetDir());

    // First try to find the normal file.
    // Trying to first facilitates easy testing with replacement files,
    // without the need to rebuild the archives.
qdbg("Archive try '%s' as %s\n",fname,fileName.cstr());
    f=new QFile(fileName);
    if(!f->IsOpen())
    {
      // Try to find the model in the archive
      QDELETE(f);
      f=car->GetArchive()->OpenFile(fname);
      if(!f)
      {
        qwarn("Model '%s' not found on disk or in archive",fname);
        goto fail;
      }
    }
    // Set path to find map files
//qdbg("  car dir=%s\n",car->GetDir());
    geode->SetMapPath(car->GetDir());
    if(!geode->ImportDOF(f))
    {
     fail:
      qwarn("Can't import '%s'",fname);
      QDELETE(geode);
      goto skip_geode;
    }
    // Use this geode as a part of a rendering tree
    geode->EnableCSG();

    // Light object
    for(i=0;i<geode->materials;i++)
    { geode->material[i]->ambient[0]=DEF_AMBIENT_LIGHT;
      geode->material[i]->ambient[1]=DEF_AMBIENT_LIGHT;
      geode->material[i]->ambient[2]=DEF_AMBIENT_LIGHT;
    }
    
    // Optional scaling
    sprintf(buf,"%s.%s.scale.x",path,modelTag);
    scaleX=info->GetFloat(buf,1.0);
    sprintf(buf,"%s.%s.scale.y",path,modelTag);
    scaleY=info->GetFloat(buf,1.0);
    sprintf(buf,"%s.%s.scale.z",path,modelTag);
    scaleZ=info->GetFloat(buf,1.0);
    if(scaleX!=1.0||scaleY!=1.0||scaleZ!=1.0)
    { geode->ScaleVertices(scaleX,scaleY,scaleZ);
    }
   skip_geode:
    QDELETE(f);
  }
  
  return TRUE;
}
bool RModel::Save()
// Save model
// Assumes Load() has been called already
{
  bool r;

  if(fileName.IsEmpty())
  { qwarn("RModel:Save() called without Load() first");
    return FALSE;
  }
qdbg("RModel:Save(); file=%s\n",fileName.cstr());
  QFile f(fileName,QFile::WRITE);
  r=geode->ExportDOF(&f);
  return r;
}

/**********
* Attribs *
**********/
bool RModel::IsLoaded()
// Returns TRUE if model has been loaded
// If so, you may call Paint() to paint it
{
  if(geode)return TRUE;
  return FALSE;
}

/********
* Paint *
********/
void RModel::Paint()
// Paint the 3D model
{
  if(geode)
  {
//glEnable(GL_LIGHTING);
//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

    //glPushMatrix();
//qdbg("RModel:Paint(); geode->Paint\n");
    geode->Paint();
    //glPopMatrix();
  }
}
