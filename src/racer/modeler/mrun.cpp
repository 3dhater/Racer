/*
 * Modeler - made for Racer but generically usable actually
 * 05-08-00: Created!
 * 12-11-00: First attempt to do DOF exporting/importing
 * NOTES:
 * - A bit of a mess (derived from Racer itself)
 * (C) MarketGraph/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/global.h>
#include <d3/fps.h>
#include <qlib/timer.h>
#include <qlib/trackball.h>
#include <qlib/dxjoy.h>
#include <GL/glu.h>
DEBUG_ENABLE

// Trackball? (uses quaternions for more intuitive rotations)
//#define USE_TRACKBALL

#define DRW        Q_BC
#define CAR_FNAME  "data/cars/devtest/car.ini"
#define CONTROLS_FILE   "controls.ini"

#define DEBUG_INI  "debug.ini"

#define PROFILE(n)

#define CHECK_MODEL if(!model){ErrNoModel();return;}

// Modeler vars
enum Mode
{
  MODE_MODEL,
  MODE_MATERIAL,
  MODE_MAX
};

// GUI
QRadio *radMode[MODE_MAX];
cstring modeName[MODE_MAX]=
{
  "Model","Material"
};
int mode;
QRect repDefaultView(50,50,900,615);        // Default report view
int   repLines=28;

enum { MAX_IO=6 };
cstring  ioName[MAX_IO]={ "Import (A)SE","Import VRML",
  "Import (3)DO","Import (O)BJ","(I)mport DOF","(E)xport DOF" };
QButton *butIO[MAX_IO];
enum { MAX_MODIFY=12 };
QButton *butModify[MAX_MODIFY];

// Graphics
DGeode *model;
DBoundingBox *bbox;
DFPS         *fps;

// Material selection (flashes the current material)
QTimer *tmrFlash;
int     curMat;

// Model handling
#ifdef USE_TRACKBALL
QTrackBall *tb;
#endif

// Hack
char keyState[256];

// Any
enum Flags
{
  FLASH_CUR_MAT=1,        // Flash the currently selected material?
  DRAW_WIREFRAME=2
};
int flags=FLASH_CUR_MAT;

// Errors
QMessageHandler defQErr;

// Proto
void OptimizeIndices();

void exitfunc()
{
  int i;
  for(i=0;i<MAX_IO;i++)QDELETE(butIO[i]);
  for(i=0;i<MAX_MODIFY;i++)QDELETE(butModify[i]);
  if(model)QDELETE(model);
#ifdef USE_TRACKBALL
  QDELETE(tb);
#endif
  QDELETE(info);
  QDELETE(RMGR);

#ifdef Q_USE_FMOD
  // Should probably be done by RAudio
  FSOUND_Close();
#endif
}

void ErrNoModel()
{
  QMessageBox("Error","No model present.");
}

/********
* MENUS *
********/
void SetupMenus()
{
  QRect r;
  int   i;
  
  // IO
  r.x=Q_BC->GetX()-2; r.y=Q_BC->GetHeight()+Q_BC->GetY()+10;
  r.wid=150; r.hgt=35;
  for(i=0;i<MAX_IO;i++)
  {
    butIO[i]=new QButton(QSHELL,&r,ioName[i]);
    r.x+=r.wid+10;
  }
  
  // Modes
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10;
  r.y=Q_BC->GetY()+2;
  r.wid=150; r.hgt=20;
  for(i=0;i<MODE_MAX;i++)
  {
    radMode[i]=new QRadio(QSHELL,&r,(string)modeName[i],1000);
    r.y+=r.hgt;
  }
  radMode[0]->SetState(TRUE);
  
  r.y+=5;
  new QLabel(QSHELL,&r,"Operations");
  r.y+=30;
  
  // Modify/ops
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10;
  //r.y=Q_BC->GetY()-2;
  r.hgt=35;
  for(i=0;i<MAX_MODIFY;i++)
  {
    butModify[i]=new QButton(QSHELL,&r,"--op--");
    r.y+=r.hgt+8;
  }
}

/*********
* ERRORS *
*********/
void apperr(string s)
{
  QRect r(100,100,500,150);
  QMessageBox("Error",s,0,&r);
  if(defQErr)defQErr(s);
}
void MsgNote(cstring s)
{
  QRect r(100,100,500,150);
  QMessageBox("Note",s,0,&r);
}
void MsgError(cstring s)
{
  QRect r(100,100,500,150);
  QMessageBox("Error",s,0,&r);
}

/********
* Modes *
********/
static void SetOpButtons(cstring names[])
// Install new names in the buttnos
{
  int i;
  for(i=0;;i++)
  {
    if(!names[i])break;
    butModify[i]->SetText(names[i]);
  }
  for(;i<MAX_MODIFY;i++)
  {
    butModify[i]->SetText("---");
    butModify[i]->Invalidate();
  }
}

void SetMode(int newMode)
{
  int i;
  cstring  modelOp[MAX_MODIFY]=
  { "Enlighten model","Scale","Scale x100","Scale /100",
    "Optimize (Sh-O)","Generate normals","Toggle burst viewing",
    "Center to origin",
    "*Set specularity","Set shininess","Information",0
  };
  cstring  matOp[MAX_MODIFY]=
  {
    "Enable envmapping","Disable envmapping",
    "Enable transparency","Disable transparency",
    "Set shininess","Set specular color","Set ambient color",
    "Set diffuse color","Set emissive color","Flip faces",0
  };
  
  mode=newMode;
  switch(mode)
  {
    case MODE_MODEL   : SetOpButtons(modelOp); break;
    case MODE_MATERIAL: SetOpButtons(matOp); break;
    default: qerr("SetMode(%d) out of range",mode);
  }
  // Set correct radio button
  for(i=0;i<MODE_MAX;i++)
  {
    radMode[i]->SetState(i==mode);
    radMode[i]->Invalidate();
  }
}

void NYI()
{
  QMessageBox("Error","Not yet implemented.");
}

/******
* I/O *
******/
void RethinkBoundingBox()
{
  DBox *b;
  model->GetBoundingBox(bbox->GetBox());
  b=bbox->GetBox();
qdbg("Bounding box: min(%f,%f,%f)-max(%f,%f,%f)\n",
b->min.x,b->min.y,b->min.z,b->max.x,b->max.y,b->max.z);
}
void ModelDestroy()
{
  QDELETE(model);
  // Reset other attribs
  curMat=0;
}
bool ModelLoadASE(cstring fname)
{
  ModelDestroy();
  QCV->Select();
  model=new DGeode(fname);
  if(!DGeodeImportASE(model,fname))
  { qerr("Can't load model '%s'",fname);
    return FALSE;
  }
  RethinkBoundingBox();
  return TRUE;
}
void ModelSelectASE()
{
  char fname[1024];
  if(!QDlgFile("Import model...",info,"modsel",fname))return;
  ModelLoadASE(fname);
}
bool ModelLoadVRML(cstring fname)
{
  ModelDestroy();
  QCV->Select();
  model=new DGeode(fname);
  if(!DGeodeImportVRML(model,fname))
  { qerr("Can't load model '%s'",fname);
    return FALSE;
  }
//model->ScaleVertices(100,100,100);  // $DEV
  RethinkBoundingBox();
  return TRUE;
}
void ModelSelectVRML()
{
  char fname[1024];
  if(!QDlgFile("Import VRML model...",info,"modselvrml",fname))return;
  ModelLoadVRML(fname);
}

bool ModelLoadWAVE(cstring fname)
{
  //NYI(); return FALSE;

  ModelDestroy();
  QCV->Select();
  model=new DGeode(fname);
  if(!DGeodeImportWAVE(model,fname))
  { qerr("Can't load model '%s'",fname);
    return FALSE;
  }
  RethinkBoundingBox();
  return TRUE;
}
void ModelSelectWAVE()
{
  char fname[1024];
  if(!QDlgFile("Import model...",info,"modselwave",fname))return;
  ModelLoadWAVE(fname);
}

bool ModelLoad3DO(cstring fname)
{
  ModelDestroy();
  QCV->Select();
  model=new DGeode(fname);
  if(!DGeodeImport3DO(model,fname))
  { qerr("Can't load model '%s'",fname);
    return FALSE;
  }
//model->ScaleVertices(.1,.1,.1);  // $DEV
  RethinkBoundingBox();
  return TRUE;
}
void ModelSelect3DO()
{
  char fname[1024];
  if(!QDlgFile("Import 3DO model...",info,"modsel3do",fname))return;
  ModelLoad3DO(fname);
}

/********************
* DOF import/export *
********************/
static void QFileDir(cstring s,qstring& d)
// Returns directory of full file path 's' in 'd'.
// For example '/home/mydir/myfile' returns '/home/mydir'
{
  int i;
  char buf[1024];

qdbg("QFileDir(%s)\n",s);
  strcpy(buf,s);
  for(i=strlen(buf);i>=0;i--)
  {
    if(buf[i]=='/'||buf[i]=='\\')
    {
      buf[i]=0;
qdbg("  buf='%s'\n",buf);
      d=buf;
      return;
    }
  }
  d="";
}
bool ModelLoadDOF(cstring fname)
{
  QFile f(fname);
  qstring mapDir;
  
  ModelDestroy();
  QCV->Select();
  model=new DGeode(fname);
  QFileDir(fname,mapDir);
  model->SetMapPath(mapDir);
qdbg("ModelLoadDOF(); mapDir=%s\n",mapDir.cstr());
  if(!model->ImportDOF(&f))
  { qerr("Can't load model '%s'",fname);
    return FALSE;
  }
//model->ScaleVertices(100,100,100);  // $DEV
  RethinkBoundingBox();
  return TRUE;
}
void ModelSelectDOF()
{
  char fname[1024];
  if(!QDlgFile("Import DOF model...",info,"modseldof",fname))return;
  ModelLoadDOF(fname);
}
void ModelExportDOF()
{
  char fname[1024];
  QFile *f;

  CHECK_MODEL;
  
  if(!QDlgFile("Export to DOF...",info,"exportdof",fname))return;
  f=new QFile(fname,QFile::WRITE);
  if(!model->ExportDOF(f))
  { qerr("Can't export model '%s'",fname);
  }
  QDELETE(f);
}

/*********
* MODIFY *
*********/
void EnlightenModel()
// Some models have poor diffuse lighting
// Esp. SCGT imported cars
{
  int i;
  CHECK_MODEL;
  
  for(i=0;i<model->materials;i++)
  {
    model->material[i]->SetDiffuseColor(1,1,1,1);
  }
  model->DestroyLists();
}
void ScaleModel()
{
  QVector3 scale;
  cstring sScale="Scaling";
  char buf[20];
  int  r;

  CHECK_MODEL;
  
  strcpy(buf,"1.0");
  r=QDlgString(sScale,"Enter scale X factor",buf,sizeof(buf));
  if(r!=IDOK)
  {
qdbg("IDOK=%d, r=%d\n",IDOK,r);
    return;
  }
  scale.x=atof(buf);
  strcpy(buf,"1.0");
  if(QDlgString(sScale,"Enter scale Y factor",buf,sizeof(buf))!=IDOK)
    return;
  scale.y=atof(buf);
  strcpy(buf,"1.0");
  if(QDlgString(sScale,"Enter scale Z factor",buf,sizeof(buf))!=IDOK)
    return;
  scale.z=atof(buf);
  
qdbg("Scale %f,%f,%f\n",scale.x,scale.y,scale.z);
  model->ScaleVertices(scale.x,scale.y,scale.z);
  RethinkBoundingBox();
}
void Scale100()
{
  CHECK_MODEL;
  model->ScaleVertices(100,100,100);
  RethinkBoundingBox();
}
void Scale1_100()
{
  CHECK_MODEL;
  model->ScaleVertices(.01f,.01f,.01f);
  RethinkBoundingBox();
}
void OptimizeIndices()
{
  char buf[256];
  int  r1,r2,r3;
  
  CHECK_MODEL;

  r1=DGeodeOptimizeIndices(model);
  r2=DGeodePackIndices(model);
  r3=DGeodeOptimizeBursts(model);
  sprintf(buf,
    "%d indices eq'd, %d vertices and %d bursts removed.",
    r1,r2,r3);
  QMessageBox("Operation completed",buf);
}
void GenerateNormals()
{
  CHECK_MODEL;
  DGeodeRethinkNormals(model);
  QMessageBox("Operation completed","Normals (re)generated.");
}
void ToggleBurstView()
{
  CHECK_MODEL;
  model->SetViewBursts(!model->IsViewBurstsEnabled());
}
void CenterModel()
{
  DBox box;
  DVector3 v;
  
  CHECK_MODEL;
  
  model->GetBoundingBox(&box);
box.DbgPrint("Center");
  box.GetCenter(&v);
qdbg("Center %f,%f,%f\n",v.x,v.y,v.z);
  v.x=-v.x;
  v.y=-v.y;
  v.z=-v.z;
  model->Translate(v.x,v.y,v.z);
  RethinkBoundingBox();
}
void SetSpecularity()
{
}

void SetShininess()
// To get specular highlights on the model
{
  char buf[20];
  int  i;
  dfloat v;
  
  CHECK_MODEL;
  
  strcpy(buf,"0.0");
  if(!QDlgString("Shininess for ALL materials (!)",
    "Enter shininess (0..128)",buf,sizeof(buf)))
    return;
  v=atof(buf);
  
  // Set it in the model
  for(i=0;i<model->materials;i++)
  {
    model->material[i]->SetSpecularColor(1,1,1);
    model->material[i]->SetShininess(v);
//model->material[i]->Enable(DMaterial::NO_TEXTURE_ENABLING);
  }
  model->DestroyLists();
}
void Information()
// A bunch of info about the model
{
  FILE *fp;
  int   i,j,total;
  DGeob *geob;
  
  fp=fopen("report.txt","w");
  if(!fp)
  {
    MsgError("Can't open report file");
    return;
  }
  if(!model)
  {
    fprintf(fp,"No model loaded.\n");
  } else
  {
    fprintf(fp,"\nNumber of geobs   : %d\n",model->geobs);
    // Estimate total #polygons
    total=0;
    for(i=0;i<model->geobs;i++)
      total+=model->geob[i]->indices/3;
    fprintf(fp,"Number of polygons: %d (estimated)\n",total);
    DBox     box;
    DVector3 size;
    model->GetBoundingBox(&box);
    box.GetSize(&size);
    fprintf(fp,"Size              : X %.3f, Y %.3f, Z %.3f\n",
      size.x,size.y,size.z);
    fprintf(fp,"\n");
    for(i=0;i<model->geobs;i++)
    {
      geob=model->geob[i];
      fprintf(fp,"\nGeob %d:\n\n",i);
      fprintf(fp,"  Vertices: %d, indices: %d\n",geob->vertices,
        geob->indices);
      fprintf(fp,"  TVertices: %d, tfaces: %d\n",geob->tvertices,
        geob->tfaces);\
      fprintf(fp,"  Normals present: %s\n",geob->normal?"YES":"NO");
      fprintf(fp,"  Vertex colors present: %s\n",geob->vcolor?"YES":"NO");
      fprintf(fp,"  Bursts: %d, materialref %d\n",
        geob->bursts,geob->materialRef);
      for(j=0;j<geob->bursts;j++)
      {
        fprintf(fp,"    burst %d: count %d, mtlid %d\n",
          j,geob->burstCount[j],geob->burstMtlID[j]);
      }
    }
    fprintf(fp,"\nNumber of materials: %d\n",model->materials);
    for(i=0;i<model->materials;i++)
    {
      DMaterial *m=model->material[i];
      fprintf(fp,"Material %d (%s)\n",i,m->GetName());
      fprintf(fp,"  ambient      %.2f,%.2f,%.2f , %.2f\n",m->ambient[0],
        m->ambient[1],m->ambient[2],m->ambient[3]);
      fprintf(fp,"  diffuse      %.2f,%.2f,%.2f , %.2f\n",m->diffuse[0],
        m->diffuse[1],m->diffuse[2],m->diffuse[3]);
      fprintf(fp,"  specular     %.2f,%.2f,%.2f , %.2f\n",m->specular[0],
        m->specular[1],m->specular[2],m->specular[3]);
      fprintf(fp,"  emission     %.2f,%.2f,%.2f , %.2f\n",m->emission[0],
        m->emission[1],m->emission[2],m->emission[3]);
      fprintf(fp,"  shininess    %.2f\n",m->shininess);
      fprintf(fp,"  transparency %.2f\n",m->transparency);
      fprintf(fp,"  uvw U offset %.2f\n",m->uvwUoffset);
      fprintf(fp,"  uvw V offset %.2f\n",m->uvwVoffset);
      fprintf(fp,"  uvw U tiling %.2f\n",m->uvwUtiling);
      fprintf(fp,"  uvw V tiling %.2f\n",m->uvwVtiling);
#ifdef FUTURE
      fprintf(fp,"  uvwAngle     %.2f\n",m->uvwAngle);
      fprintf(fp,"  uvwBlur      %.2f\n",m->uvwBlur);
      fprintf(fp,"  uvwBlurOffset %.2f\n",m->uvwBlurOffset);
#endif
      fprintf(fp,"  submaterials: %d, layers %d\n",m->submaterials,m->layers);
      fprintf(fp,"\n");
    }
    fprintf(fp,"\n");
  }
  fclose(fp);
  QDlgViewFile("report.txt",repLines,"Report","Model information",
    &repDefaultView);
}

/*******************
* Model operations *
*******************/
void ModelOp(int n)
{
  switch(n)
  {
    case 0: EnlightenModel(); break;
    case 1: ScaleModel(); break;
    case 2: Scale100(); break;
    case 3: Scale1_100(); break;
    case 4: OptimizeIndices(); break;
    case 5: GenerateNormals(); break;
    case 6: ToggleBurstView(); break;
    case 7: CenterModel(); break;
    case 8: SetSpecularity(); break;
    case 9: SetShininess(); break;
    case 10: Information(); break;
    default: qwarn("ModelOp(%d) out of range",n);
  }
}

/************
* Materials *
************/
void MatAddSphereMap()
{
  QImage img("../data/images/smap_purple.tga");
  //QImage img("../data/images/smap_flowers.tga");
  DBitMapTexture *tex;
  DMatLayer *layer;
  int i;
  
  CHECK_MODEL;

qdbg("Adding sphere map\n");

  // Create spherical map texture
  tex=new DBitMapTexture(&img);
  // Create material layer with the sphere map
  layer=new DMatLayer(tex,DMatLayer::TEX_SPHERE_ENV);
  layer->transparency=0.4f;
  // Add it to the materials of the model
  for(i=0;i<model->materials;i++)
  {
    model->material[i]->AddLayer(layer);
  }
}
bool MatEditColor(dfloat col[4],cstring title)
// Helper function to edit a color.
// Returns color in 'col' and TRUE if it was OK'ed.
// Returns FALSE if the action was cancelled.
{
  int i;
  cstring compName[4]={ "RED","GREEN","BLUE","ALPHA" };
  char buf[256],buf2[256];
  dfloat newCol[4];

  for(i=0;i<4;i++)
  {
    sprintf(buf2,"Enter %s component value (0..255)",compName[i]);
    sprintf(buf,"%d",(int)(col[i]*255.0f));
    if(QDlgString(title,buf2,buf,sizeof(buf))!=IDOK)
    {
      return FALSE;
    }
    newCol[i]=((dfloat)Eval(buf))/255.0f;
  }
  // Confirm color
  for(i=0;i<4;i++)
    col[i]=newCol[i];
  return TRUE;
}

/******************
* Material colors *
******************/
static void MatEditSpecular()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  if(MatEditColor(mat->specular,"Edit specular color"))
  {
  }
}
static void MatEditAmbient()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  if(MatEditColor(mat->ambient,"Edit ambient color"))
  {
  }
}
static void MatEditDiffuse()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  if(MatEditColor(mat->diffuse,"Edit diffuse color"))
  {
  }
}
static void MatEditEmissive()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  if(MatEditColor(mat->emission,"Edit emissive color"))
  {
  }
}

/**********************
* Material operations *
**********************/
static void MatEnableEnv()
// Enable environment mapping on current material
{
  DTexture *texEnv;
  DMaterial *mat;
  
  CHECK_MODEL;

  texEnv=RMGR->GetEnvironmentTexture();
  if(!texEnv)
  {
    MsgError("No environment texture set in gfx.ini");
    return;
  }
  
  DMatLayer *layer;
  int i;
  
  mat=model->material[curMat];
  if(!mat)return;
  
  // Currently, an extra layer is only used for envmapping. A bit
  // rough, as it should check whether this is not just an extra
  // shader layer.
  if(mat->layers>1)
  {
    MsgNote("Environment layer already present");
    return;
  }
  
  // Create material layer with the sphere map
  layer=new DMatLayer(texEnv,DMatLayer::TEX_SPHERE_ENV);
  layer->transparency=0.3f;
  // Add it to the material
  mat->AddLayer(layer);
  // Remember to request the envmap when loading
  mat->createFlags|=DMaterial::REQUEST_ENV_MAP;
}
static void MatDisableEnv()
// Remove environment mapping for a material
{
  DMaterial *mat;
  
  CHECK_MODEL;

  mat=model->material[curMat];
  if(!mat)return;
  
  // Currently, an extra layer is only used for envmapping. A bit
  // rough, as it should check whether this is not just an extra
  // shader layer.
  if(mat->layers<=1)
  {
    MsgNote("Environment mapping wasn't enabled");
    return;
  }
  
  // Delete layer
  QDELETE(mat->layer[1]);
  mat->layers--;
  
  // Remember for loading
  mat->createFlags&=~DMaterial::REQUEST_ENV_MAP;
}
void MatSelect(int mat)
// Select material #n
{
  int i;

  CHECK_MODEL;

  if(mat>=0&&mat<model->materials)
  {
qdbg("MatSelect(%d)\n",mat);
    curMat=mat;
    // Deselect all
    for(i=0;i<model->materials;i++)
    {
      model->material[i]->DisableSelect();
    }
    if(flags&FLASH_CUR_MAT)
    {
      // Select current to flash
      model->material[curMat]->EnableSelect();
    }
  }
}
static void MatEnableTransparency()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;
  
  mat->flags&=~DMaterial::NO_BLEND_PROPERTIES;
  mat->blendMode=DMaterial::BLEND_SRC_ALPHA;
  mat->EnableTransparency();
}
static void MatDisableTransparency()
{
  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;
  
  //mat->flags&=~DMaterial::NO_BLEND_PROPERTIES;
  mat->blendMode=DMaterial::BLEND_OFF;
  mat->DisableTransparency();
}
static void MatSetShininess()
{
  char  buf[20];
  float v;

  DMaterial *mat;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  strcpy(buf,"0.0");
  if(!QDlgString("Material shininess","Enter shininess (0..128)",
    buf,sizeof(buf)))
    return;
  v=atof(buf);

  mat->SetSpecularColor(1,1,1);
  mat->SetShininess(v);
  // Regenerate display lists
  model->DestroyLists();
} 
static void MatFlipNormals()
// Needed for 3DO objects; faces seem to have varying ordering
{
  int i,j;
  DMaterial *mat,*matBurst;
  CHECK_MODEL;
  mat=model->material[curMat];
  if(!mat)return;

  // Search all geobs for the current material
  for(i=0;i<model->geobs;i++)
  {
    DGeob *geob=model->geob[i];
    for(j=0;j<geob->bursts;j++)
    {
      matBurst=geob->GetMaterialForID(geob->burstMtlID[j]);
      if(mat==matBurst)
      {
qdbg("Flip nrm geob %d, burst %d\n",i,j);
        geob->FlipFaces(j);
      }
    }
  }
}
void MatOp(int n)
{
  switch(n)
  {
    case 0: MatEnableEnv(); break;
    case 1: MatDisableEnv(); break;
    case 2: MatEnableTransparency(); break;
    case 3: MatDisableTransparency(); break;
    case 4: MatSetShininess(); break;
    case 5: MatEditSpecular(); break;
    case 6: MatEditAmbient(); break;
    case 7: MatEditDiffuse(); break;
    case 8: MatEditEmissive(); break;
    case 9: MatFlipNormals(); break;
    default: qwarn("MatOp(%d) out of range",n);
  }
}

/*********
* EVENTS *
*********/
bool event(QEvent *e)
{
  int i;

  PROFILE(RProfile::PROF_EVENTS);

  DTransformation *tf;
  static int drag,dragX,dragY;

  if(model)tf=model->GetTransformation();
  else     tf=0;
  
#ifdef OBS
if(e->type!=QEvent::MOTIONNOTIFY)
qdbg("event %d; @%d,%d\n",e->type,e->x,e->y);
#endif

  if(e->type==QEvent::BUTTONPRESS)
  {
    //if(!drag)
    if(e->win==Q_BC)
    { drag|=(1<<(e->n-1));
      dragX=e->x; dragY=e->y;
    }
  } else if(e->type==QEvent::BUTTONRELEASE)
  {
    drag&=~(1<<(e->n-1));
  } else if(e->type==QEvent::MOTIONNOTIFY)
  {
    if(!tf)return FALSE;
    
#ifdef USE_TRACKBALL
    if(drag&1)
    { //qdbg("move %d,%d -> %d,%d\n",dragX,dragY,e->x,e->y);
      tb->Movement(dragX,dragY,e->x,e->y);
    }
#else
    // 2 mouse button version
    if((drag&(1|4))==(1|4))
    { tf->x+=(e->x-dragX)*400/720;
      tf->y-=(e->y-dragY)*400/576;
    } else if(drag&1)
    {
      tf->xa+=((float)(e->y-dragY))*300./576.;
      tf->ya+=((float)(e->x-dragX))*300./720.;
    } else if(drag&4)
    {
      tf->za+=(e->y-dragY)*300./576.;
      tf->z+=(e->x-dragX);
    }
#endif
    dragX=e->x; dragY=e->y;
    
    // Copy transformation matrix to its bounding box
    *bbox->GetTransformation()=*tf;
    return TRUE;
  }

  if(e->type==QEvent::CLICK)
  {
    if(e->win==butIO[0])
    {
      ModelSelectASE();
    } else if(e->win==butIO[1])
    {
      ModelSelectVRML();
    } else if(e->win==butIO[2])
    {
      ModelSelect3DO();
    } else if(e->win==butIO[3])
    {
      ModelSelectWAVE();
    } else if(e->win==butIO[4])
    {
      ModelSelectDOF();
    } else if(e->win==butIO[5])
    {
      ModelExportDOF();
    }
    
    // Check mode selection
    for(i=0;i<MODE_MAX;i++)
    {
      if(e->win==radMode[i])
        SetMode(i);
    }
    
    // Check operation buttons
    for(i=0;i<MAX_MODIFY;i++)
    {
      if(e->win==butModify[i])
      {
        switch(mode)
        {
          case MODE_MODEL   : ModelOp(i); break;
          case MODE_MATERIAL: MatOp(i); break;
        }
      }
    }
  }
  if(e->type==QEvent::MOTIONNOTIFY)
  {
    //MapXYtoCtl(e->x,e->y);
  } else if(e->type==QEvent::KEYPRESS)
  {
    if(e->n==QK_ESC)
      app->Exit(0);
    else if(e->n==QK_R)
    { // Report transformation 'matrix'
      if(tf)
        qdbg("tf: @%f,%f,%f angle %f,%f,%f scale %f,%f,%f\n",
          tf->x,tf->y,tf->z,tf->xa,tf->ya,tf->za,tf->xs,tf->ys,tf->zs);
      else
        qdbg("No transformation yet.\n");
    } else if(e->n==QK_M)
    {
      // Mode switch
      switch(mode)
      {
        case MODE_MODEL   : SetMode(MODE_MATERIAL); break;
        case MODE_MATERIAL: SetMode(MODE_MODEL); break;
      }
    } else if(e->n==QK_A)
    {
      // Import ASE
      ModelSelectASE();
    } else if(e->n==QK_O)
    {
      // Import WAVE
      ModelSelectWAVE();
    } else if(e->n==QK_3)
    {
      // Import 3DO
      ModelSelect3DO();
    } else if(e->n==QK_I)
    {
      // Import DOF
      ModelSelectDOF();
    } else if(e->n==QK_E)
    {
      // Export DOF
      ModelExportDOF();
    } else if(e->n==(QK_SHIFT|QK_O))
    {
      OptimizeIndices();
    } else if(e->n==QK_V)
    {
      QDlgViewFile("README.txt",repLines,"Report","Information",
        &repDefaultView);
#ifdef OBS
    } else if(e->n==QK_S)
    {
      // Sphere map
      MatAddSphereMap();
#endif
    }
    
    // Material mode
    //if(mode==MODE_MATERIAL)
    {
      if(e->n==QK_PAGEDOWN)
      {
        MatSelect(curMat+1);
      } else if(e->n==QK_PAGEUP)
      {
        MatSelect(curMat-1);
      } else if(e->n==QK_F)
      {
        // Toggle material flashing
        flags^=FLASH_CUR_MAT;
        MatSelect(curMat);
      } else if(e->n==QK_W)
      {
        // Toggle wireframe drawing
        flags^=DRAW_WIREFRAME;
      }
    }
  }

  // Return to other stuff
  PROFILE(RProfile::PROF_OTHER);

  return FALSE;
}

/***********
* HANDLING *
***********/
void SetupViewport(int w,int h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50.0,(GLfloat)w/(GLfloat)h,1.0,10000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0,0,-1000);
}

QDraw *GGetDrawable()
{
  return DRW;
}

QCanvas *GGetCV()
{
  return DRW->GetCanvas();
}
void GSwap()
{ DRW->Swap();
}

/********
* Stats *
********/
struct Stats
{
  int x,y;
} stats;
void StatsStart()
{
  stats.x=20; stats.y=20;
  QCV->Set3D();
  QCV->Set2D();
  QCV->SetColor(255,255,255);
  QCV->SetFont(app->GetSystemFont());
}
void StatsAdd(cstring s)
{
  QCV->Text(s,stats.x,stats.y);
  stats.y+=20;
//qdbg("StatsAdd(%s)\n",s);
}

static void HandleFlashing()
{
  char col[4];
  dfloat msF,v,vi;
  int ms;
  bool fInvert;
  DMaterial *mat;

  if(!model)return;
  if(!(flags&FLASH_CUR_MAT))return;

  mat=model->material[curMat];
  if(!mat)return;
  if(!mat->IsSelected())
    mat->EnableSelect();
    
  // Flash up & down
  ms=tmrFlash->GetMilliSeconds();
  if((ms/1000)&1)fInvert=TRUE;
  else           fInvert=FALSE;
  ms%=1000;
  if(fInvert)ms=1000-1-ms;
  msF=(dfloat)ms;
  
  v=msF/1000.0f*255.0f;
  vi=(int)v;
  mat->selColor.SetRGBA(255-vi,vi,vi,255);
//qdbg("flash $%x\n",mat->selColor.GetRGBA());
}

void idlefunc()
{
  QVector3 *v;
  float lightDir[3]={ 0,-.1,-1 };
  //float lightPos[4]={ 1,1,-1,0 };
  //float light0Pos[4]={ 0.0002,-0.0002,-1000,0 };
  //float light0Pos[4]={ 0.0002,-0.0002,1000,0 };
  float light0Pos[4]={ -1,1,1,0 };
  //float light1Pos[4]={ -0.0002,0.0002,-100,0 };
  //float light1Pos[4]={ -0.0002,0.0002,100,0 };
  float light1Pos[4]={ 1,-1,-1,0 };
  float m[16];
  char buf[256];

//qdbg("--------------\n");

  // Don't refresh while window is disabled. This happens
  // when a dialog gets on top, and redrawing doesn't work
  // correctly on a lot of buggy drivers on Win98.
  if(!Q_BC->IsEnabled())return;
  
  // Ongoing timing tasks
  HandleFlashing();
  
#ifdef WIN32
  // Task swapping grinds to a halt on Win32, duh!
  QNap(1);
#endif

  GGetCV()->Select();
  SetupViewport(DRW->GetWidth(),DRW->GetHeight());
  //GGetCV()->SetFont(app->GetSystemFont());
  //GGetCV()->Set2D();
  //GGetCV()->Set3D();
  glClearColor(.1,.1,.4,0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glEnable(GL_CULL_FACE);
  //glCullFace(GL_FRONT);
  //glFrontFace(GL_CW);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  
  // Specular fun
  QCV->GetGLContext()->SetSeparateSpecularColor(TRUE);

#ifdef USE_TRACKBALL
  // Rotate to trackball orientation
  tb->BuildRotateMatrix(m);
  glMultMatrixf(m);
#endif
  
  // Lighting (fixed in relation to the camera pos)
  glLightfv(GL_LIGHT0,GL_POSITION,light0Pos);
  glLightfv(GL_LIGHT1,GL_POSITION,light1Pos);
  {
    float v[4]={ 1,1,1,1 };
    float l1diffuse[4]={ .5,.5,.5,1 };
    glLightfv(GL_LIGHT1,GL_DIFFUSE,l1diffuse);
    glLightfv(GL_LIGHT1,GL_SPECULAR,l1diffuse);
  }
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);

  if(model)
  {
//qdbg("model pnt\n");
    //glTranslatef(0,0,-20);
    //QCV->Set3D();
    glPushMatrix();
//glDisable(GL_LIGHT0);
    model->EnableCSG();
//glShadeModel(GL_SMOOTH);

#ifdef OBS
  float ambient[]={ 0.2f,0.2f,0.2f,1.0f };
  float diffuse[]={ 0.8f,0.8f,0.8f,1.0f };
  float specular[]={ 0,0,0,1.0 };
  float emission[]={ 0,0,0,1.0 };
  float shininess=0;

  glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
  glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
  glMaterialfv(GL_FRONT,GL_EMISSION,emission);
  glMaterialf(GL_FRONT,GL_SHININESS,shininess);
#endif
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  if(flags&DRAW_WIREFRAME)
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  else
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    model->Paint();
    // The box
    bbox->Paint();
    glPopMatrix();
  }
  
#ifdef OBS
  if(rPrefs.bReflections)
  { // Reflections
    glFrontFace(GL_CW);
    glPushMatrix();
    glScalef(1,-1,1);
    //glPixelZoom(1,-1);
    if(model)model->Paint();
    glPopMatrix();
    glFrontFace(GL_CCW);
    //glPixelZoom(1,1);
    //glScalef(1,1,1);
  }
#endif

#ifdef OBS
  float ambient[]={ 0.2f,0.2f,0.2f,1.0f };
  float diffuse[]={ 0.8f,0.8f,0.8f,1.0f };
  float specular[]={ 0,0,0,1.0 };
  float emission[]={ 0,0,0,1.0 };
  float shininess=0;

  glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
  glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
  glMaterialfv(GL_FRONT,GL_EMISSION,emission);
  glMaterialf(GL_FRONT,GL_SHININESS,shininess);
#endif
  
  fps->FrameRendered();
  StatsStart();
  sprintf(buf,"FPS: %.2f",fps->GetFPS());
  StatsAdd(buf);

  GSwap();
}

void Setup()
{
  float v;
  
  //defQErr=QSetErrorHandler(apperr);

  // Create a Racer Manager, which is used in some places
  new RManager();
  RMGR->Create();
  
  // GUI
  SetupMenus();

  // Get window up
  app->RunPoll();
  app->SetIdleProc(idlefunc);
  app->SetExitProc(exitfunc);
  app->SetEventProc(event);
  
  // Graphics
  bbox=new DBoundingBox();
  bbox->EnableCSG();
  fps=new DFPS();

  // Misc
  tmrFlash=new QTimer();
  tmrFlash->Start();
  
  SetMode(MODE_MODEL);
#ifdef USE_TRACKBALL
  tb=new QTrackBall(Q_BC->GetWidth(),Q_BC->GetHeight());
#endif
}

void Run()
{
  Setup();
  app->Run();
}
