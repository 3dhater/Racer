/*
 * Import SCGT, DOF
 * 03-01-2001: Created! (21:07:14)
 * NOTES:
 * - Does math than importing a bunch of SCGT files; also does batch
 * processing for other things.
 * - Should perhaps be split into 2 modules, 1 for SCGT, 1 for Racer
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/dir.h>
#include <qlib/debug.h>
DEBUG_ENABLE

enum FileFormat
{
  FFORMAT_VRML,
  FFORMAT_ASE
};

/*******
* Help *
*******/
#ifdef OBS
static void EnlightenModel(DGeode *model)
// Some models have poor diffuse lighting
// Esp. SCGT imported cars
{
  int i;
  for(i=0;i<model->materials;i++)
  {
    model->material[i]->SetDiffuseColor(1,1,1,1);
  }
}
#endif

/******************************
* Batch converting WRL -> DOF *
******************************/
static void PrepNode(RTV_Node *node)
// Node ctor; avoid just setting to 0, which will kill the qstring member
{
  node->type=0;
  // Default flags; make your own collision flags, since most is not
  //node->flags=RTV_Node::F_COLLIDE|RTV_Node::F_SURFACE;
  node->flags=0;
  node->pad[0]=node->pad[1]=0;
  node->geode=0;
}
static void ConvertModel(cstring geoName,cstring trkDir,cstring mapDir,
  int fileFormat)
// Convert a VRML or ASE model, processing on the fly
// Model 'geoName' in dir 'trkDir'.
// 'fileFormat' indicates VRML (0) or ASE (1) (FFORMAT_xxx)
{
  RTV_Node node;
  char buf[1024];
  
//qdbg("ConvertModel, mapDir='%s'\n",mapDir);

  // Prepare node
  PrepNode(&node);
  node.type=0;
  node.geode=new DGeode(0);
  // Read model
  node.geode->SetMapPath(mapDir);
  sprintf(buf,"%s/%s",trkDir,geoName);
  QCV->Select();                  // Make sure textures end in QCV
  if(fileFormat==FFORMAT_VRML)DGeodeImportVRML(node.geode,buf);
  else if(fileFormat==FFORMAT_ASE)DGeodeImportASE(node.geode,buf);
 
  // Make it shine
  DGeodeOptimizeIndices(node.geode);
  DGeodePackIndices(node.geode);
  //DGeodeRethinkNormals(node.geode);
  node.geode->DestroyNormals();
  
  // Write model in the same place as the maps
  sprintf(buf,"%s/%s",mapDir,geoName);
  // No ugly caps
  strlwr(buf);
  // New extension
  buf[strlen(buf)-4]=0;
  // Store filename without extension
  node.fileName=QFileBase(buf);
  strcat(buf,".dof");
//qdbg("Export to '%s'\n",buf);
  QFile f(buf,QFile::WRITE);
  node.geode->ExportDOF(&f);
  
  // Cleanup
  QDELETE(node.geode);
  // Clear texture pool, since this geode's texture have
  // been removed
  dglobal.texturePool->Clear();
}
void ConvertSCGTwrl()
// Import all .wrl files, do NOT add to the scene
// Expects files in <trackdir>/SCGT/wrl
{
  cstring findDir;
  char buf[1024],tdir[1024],mapDir[1024];
  QDirectory *dir;
  QFileInfo fi;
  int i,total,current,pass;
  // Progress
  QRect r(100,100,400,150);
  QProgressDialog *dlg;
  
  i=QMessageBox("Convert all SCGT WRL to DOF",
    "This may take a while, are you sure?");
  if(i!=IDOK)
    return;
    
  // Find the track directory
  findDir=RFindFile(".",track->GetTrackDir());
//qdbg("findDir='%s'\n",findDir);
  strcpy(tdir,findDir);
  // Cut off "."
  tdir[strlen(tdir)-2]=0;
  
  // Find all models in <tdir>/SCGT/wrl/*.wrl
  // Run it twice, first counting for the progress dialog
  strcpy(mapDir,tdir);
  strcat(tdir,"/SCGT/wrl");
  current=total=0;
  dlg=new QProgressDialog(QSHELL,&r,"Converting...",
    "Converting all WRL files to DOF format.");
  dlg->Create();
  for(pass=0;pass<2;pass++)
  {
    dir=new QDirectory(tdir);
    while(dir->ReadNext(buf,&fi))
    {
      // Filter
      if(strstr(buf,".wrl")==0&&strstr(buf,".WRL")==0)
        continue;
      if(pass==0)
      { // Just count
        total++;
      } else
      { // Actually convert
//qdbg("Model '%s'\n",buf);
        dlg->SetProgressText(buf);
        dlg->SetProgress(current,total);
        ConvertModel(buf,tdir,mapDir,FFORMAT_VRML);
        current++;
        if(!dlg->Poll())break;
      }
    }
    QDELETE(dir);
  }
  QDELETE(dlg);
}

/******************************
* Batch converting ASE -> DOF *
******************************/
void ConvertASE()
// Import all .ase files, do NOT add to the scene
// Expects files in <trackdir>/ase
{
  cstring findDir;
  char buf[1024],tdir[1024],mapDir[1024];
  QDirectory *dir;
  QFileInfo fi;
  int i,total,current,pass;
  // Progress
  QRect r(100,100,400,150);
  QProgressDialog *dlg;
  
  i=QMessageBox("Convert all ASE files in <trackdir>/ase",
    "This may take a while, are you sure?");
  if(i!=IDOK)
    return;
    
  // Clear the texture pool to avoid bad textures
  dglobal.texturePool->Clear();

  // Find the track directory
  findDir=RFindFile(".",track->GetTrackDir());
//qdbg("findDir='%s'\n",findDir);
  strcpy(tdir,findDir);
  // Cut off "."
  tdir[strlen(tdir)-2]=0;
  
  // Find all models in <tdir>/ase/*.ase
  // Run it twice, first counting for the progress dialog
  strcpy(mapDir,tdir);
  strcat(tdir,"/ase");
  current=total=0;
  dlg=new QProgressDialog(QSHELL,&r,"Converting...",
    "Converting all ASE files to DOF format.");
  dlg->Create();
  for(pass=0;pass<2;pass++)
  {
    dir=new QDirectory(tdir);
qdbg("Perhaps '%s'\n",buf);
    while(dir->ReadNext(buf,&fi))
    {
      // Filter
      if(strstr(buf,".ase")==0&&strstr(buf,".ASE")==0)
        continue;
      if(pass==0)
      { // Just count
        total++;
      } else
      { // Actually convert
//qdbg("Model '%s'\n",buf);
        dlg->SetProgressText(buf);
        dlg->SetProgress(current,total);
        ConvertModel(buf,tdir,mapDir,FFORMAT_ASE);
        current++;
        if(!dlg->Poll())break;
      }
    }
    QDELETE(dir);
  }
  QDELETE(dlg);
}

/************
* Filtering *
************/
static char filterPattern[256];
bool FilterRequest()
// Sets up filter
{
  QRect r(100,100,400,200);
  
  // Default filter
  if(!filterPattern[0])
    strcpy(filterPattern,"TRK2*");
  if(!QDlgString("Import DOF files","Enter filter expression:",
    filterPattern,sizeof(filterPattern)))
    return FALSE;
  return TRUE;
}
bool FilterMatch(cstring fname)
// Returns TRUE if 'fname' is in the filter
{
  // Matches filter?
  if(!QMatch(filterPattern,fname))return FALSE;
  return TRUE;
}


/****************************
* Batch importing DOF files *
****************************/
static void AddModel(cstring geoName,cstring trkDir,cstring mapDir)
// Adds a VRML model, processing on the fly
// Model 'geoName' in dir 'trkDir'.
{
  RTV_Node node;
  char buf[1024];
  
//qdbg("AddModel(%s,%s)\n",geoName,trkDir);
//qdbg("AddModel mapDir='%s'\n",mapDir);

  // Prepare node
  PrepNode(&node);
  node.type=0;
  node.geode=new DGeode(0);
  
  // Read model
  node.geode->SetMapPath(mapDir);
  sprintf(buf,"%s/%s",trkDir,geoName);
  QCV->Select();                  // Make sure textures end in QCV
  {
    QFile f(buf);
    node.geode->ImportDOF(&f);
  }
  
  // Write model in the same place as the maps
  sprintf(buf,"%s/%s",mapDir,geoName);
  // No ugly caps
  strlwr(buf);
  // New extension
  buf[strlen(buf)-4]=0;
  // Store filename without extension
  node.fileName=QFileBase(buf);
  
  // Add model to track
  track->AddNode(&node);
  // Recreate culler on next occasion to include new model
  track->DestroyCuller();
}
void ImportDOFs()
// Import all .wrl files, add to the scene, and process
// accordingly
// Expects files in <trackdir>/SCGT
{
  char buf[1024],tdir[1024],mapDir[1024];
  QDirectory *dir;
  QFileInfo fi;
  // Progress
  int   total,current,pass;
  QRect r(100,100,400,150);
  QProgressDialog *dlg;
  
  if(!FilterRequest())return;
  
  strcpy(tdir,track->GetTrackDir());
qdbg("tdir='%s'\n",tdir);
  
  // Find all models in <tdir>/*.dof
  strcpy(mapDir,tdir);
  
  dlg=new QProgressDialog(QSHELL,&r,"Importing...",
    "Importing matching DOF files.");
  dlg->Create();
  current=total=0;
  for(pass=0;pass<2;pass++)
  {
    dir=new QDirectory(tdir);
    while(dir->ReadNext(buf,&fi))
    {
//qdbg("Perhaps '%s'\n",buf);
      // Filter
      if(strstr(buf,".dof")==0&&strstr(buf,".DOF")==0)
        continue;
      if(!FilterMatch(buf))continue;
      if(pass==0)
      { // Just count
        total++;
      } else
      { // Actually add the model
        dlg->SetProgressText(buf);
        dlg->SetProgress(current,total);
        AddModel(buf,tdir,mapDir);
        // Show progress
        PaintTrack();
        current++;
        if(!dlg->Poll())break;
      }
    }
    QDELETE(dir);
  }
  QDELETE(dlg);
}

/*********************************
* Batch processing existing DOFs *
*********************************/
static void ModelAddLighting(cstring geoName,cstring trkDir,cstring mapDir)
// Load & process DOF to add lighting (normals)
// Model 'geoName' in dir 'trkDir'.
{
  DGeode *model;
  char buf[1024];
  int i;
  
qdbg("ProcessModel(%s), mapDir='%s'\n",geoName,mapDir);

  // Prepare node
  model=new DGeode(0);
  
  // Read model
  model->SetMapPath(mapDir);
  sprintf(buf,"%s/%s",trkDir,geoName);
  QCV->Select();                  // Make sure textures end in QCV
  {
    QFile f(buf);
    model->ImportDOF(&f);
  }
  
  // Add lighting by adding normals
#ifdef OBS
  DGeodeOptimizeIndices(model);
  DGeodePackIndices(model);
#endif
  DGeodeRethinkNormals(model);
  // Enlighten model for lighting to work
  for(i=0;i<model->materials;i++)
  {
    model->material[i]->SetDiffuseColor(1,1,1,1);
    model->material[i]->emission[0]=0.8f;
  }
  
  // Write processed model back
  sprintf(buf,"%s/%s",trkDir,geoName);
  // No ugly caps
  strlwr(buf);
  QFile f(buf,QFile::WRITE);
  model->ExportDOF(&f);
  
  // Cleanup
  QDELETE(model);
}
void BatchAddLighting()
// Import all .dof files, do NOT add to the scene,
// but add lighting to the models.
// Expects files in <trackdir>/*.dof
{
  cstring findDir;
  char buf[1024],tdir[1024],mapDir[1024];
  QDirectory *dir;
  QFileInfo fi;
  int i,total,current,pass;
  // Progress
  QRect r(100,100,400,150);
  QProgressDialog *dlg;
  
  i=QMessageBox("Add lighting to all DOFs",
    "This may take a while, are you sure?");
  if(i!=IDOK)
    return;
    
  // Find the track directory
  findDir=RFindFile(".",track->GetTrackDir());
//qdbg("findDir='%s'\n",findDir);
  strcpy(tdir,findDir);
  // Cut off "."
  tdir[strlen(tdir)-2]=0;
  
  // Find all models in <tdir>
  // Run it twice, first counting for the progress dialog
  strcpy(mapDir,tdir);
  current=total=0;
  dlg=new QProgressDialog(QSHELL,&r,"Processing...",
    "Add lighting/normals to all DOF files.");
  dlg->Create();
  for(pass=0;pass<2;pass++)
  {
    dir=new QDirectory(tdir);
//qdbg("Perhaps '%s'\n",buf);
    while(dir->ReadNext(buf,&fi))
    {
      // Filter
      if(strstr(buf,".dof")==0&&strstr(buf,".DOF")==0)
        continue;
      if(pass==0)
      { // Just count
        total++;
      } else
      { // Actually convert
//qdbg("Model '%s'\n",buf);
        dlg->SetProgressText(buf);
        dlg->SetProgress(current,total);
        ModelAddLighting(buf,tdir,mapDir);
        current++;
        if(!dlg->Poll())break;
      }
    }
    QDELETE(dir);
  }
  QDELETE(dlg);
}

/******************************
* Batch converting DOF -> WRL *
******************************/
void ConvertDOF2WRL()
// Batch export all DOF files to <trackdir>/wrl
{
  char buf[1024];
  int  i,n;
  // Progress
  QRect r(100,100,400,150);
  QProgressDialog *dlg;
  
  // Protection
  sprintf(buf,"%s/track.ar",track->GetTrackDir());
  if(QFileExists(buf))
  {
    return;
  }
  
  i=QMessageBox("Export all DOF files to WRL",
    "This may take a while, are you sure?");
  if(i!=IDOK)
    return;

  dlg=new QProgressDialog(QSHELL,&r,"Processing...",
    "Converting DOF to WRL (in <trackdir>/wrl).");
  dlg->Create();
  
  n=track->GetNodes();
  for(i=0;i<n;i++)
  {
    RTV_Node *node=track->GetNode(i);
    sprintf(buf,"%s/wrl/%s.wrl",track->GetTrackDir(),node->fileName.cstr());
qdbg("Write '%s'\n",buf);
    dlg->SetProgressText(node->fileName);
    dlg->SetProgress(i,n);
    DGeodeExportVRML(node->geode,buf);
  }
  QDELETE(dlg);
}
