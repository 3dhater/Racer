/*
 * DGeode - WRL exporting (VRML2.0)
 * 10-01-2002: Created! (16:44:10)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

/**********
* Helpers *
**********/
static int FWrite(QFile *f,cstring s)
{
  return f->Write(s,strlen(s));
}
static void WriteHeader(QFile *f)
{
  FWrite(f,"#VRML V2.0 utf8\n\n");
  FWrite(f,"# Produced by QLib's VRML export, version 1\n\n");
}

/**************
* Export geob *
**************/
static bool ExportMaterial(QFile *f,DMaterial *m)
{
  char buf[256];
  
  if(!m)
  {
    qwarn("No material to export.\n");
    
  }
  
  FWrite(f,"      appearance Appearance {\n");
  FWrite(f,"        material Material {\n");
  sprintf(buf,"        diffuseColor %.3f %.3f %.3f\n",
    m->diffuse[0],m->diffuse[1],m->diffuse[2]);
  FWrite(f,buf);
  // Ambient intensity is really a full RGB color in a geob,
  // but VRML only stores a single (gray) value. We select
  // the first (Red) component.
  sprintf(buf,"        ambientIntensity %.3f\n",m->emission[0]);
  FWrite(f,buf);
  sprintf(buf,"        specularColor %.3f %.3f %.3f\n",
    m->specular[0],m->specular[1],m->specular[2]);
  FWrite(f,buf);
  sprintf(buf,"        shininess %.3f\n",m->shininess/128.0f);
  FWrite(f,buf);
  sprintf(buf,"        transparency %.3f\n",m->transparency);
  FWrite(f,buf);
  FWrite(f,"        }\n");
  
  // Write textures
  DTexture *tex=m->GetTexture(0);
  if(tex)
  {
    FWrite(f,"        texture ImageTexture {\n");
    sprintf(buf,"          url \"%s\"\n",tex->GetName());
    FWrite(f,buf);
    FWrite(f,"        }\n");
  }
  FWrite(f,"      }\n");
  
  return TRUE;
}

bool DGeobExportVRML(QFile *f,DGeob *g,int index,cstring prefix)
// Export a geob to a VRML(2.0 format) file.
// Creates a descriptive name from 'index' and 'prefix'
{
  char objName[40];
  char buf[256],buf2[256];
  int  i,j;
  
  // Give it a name (not in the geob)
  sprintf(objName,"%s_%02d",prefix,index+1);
  
  // Transform
  sprintf(buf,"DEF %s Transform {\n",objName);
  FWrite(f,buf);
  FWrite(f,"  children [\n");
  FWrite(f,"    Shape {\n");
  
  // Appearance
  ExportMaterial(f,g->GetMaterialForID(0));
  
  // Geometry
  sprintf(buf,"      geometry DEF %s-FACES IndexedFaceSet {\n",objName);
  FWrite(f,buf);
  
  // Default geometry values
  FWrite(f,"         ccw TRUE\n");
  FWrite(f,"         solid TRUE\n");
  
  // Coordinates (vertices)
  sprintf(buf,"        coord DEF %s-COORD Coordinate { point [\n",objName);
  FWrite(f,buf);
  for(i=0;i<g->vertices;i++)
  {
    if((i%3)==0)strcpy(buf,"         ");
    else buf[0]=0;
    sprintf(buf2,"%.3f %.3f %.3f",
      g->vertex[i*3+0],g->vertex[i*3+1],g->vertex[i*3+2]);
    strcat(buf,buf2);
    if(i<g->vertices-1)
      strcat(buf,",");
    if((i%3)==2)strcat(buf,"\n");
    else        strcat(buf," ");
    FWrite(f,buf);
  }
  FWrite(f,"        ] }\n");

  // Normals
  sprintf(buf,"        normal Normal { vector [\n");
  FWrite(f,buf);
  for(i=0;i<g->normals;i++)
  {
    if((i%3)==0)strcpy(buf,"         ");
    else buf[0]=0;
    sprintf(buf2,"%.3f %.3f %.3f",
      g->normal[i*3+0],g->normal[i*3+1],g->normal[i*3+2]);
    strcat(buf,buf2);
    if(i<g->normals-1)
      strcat(buf,",");
    if((i%3)==2)strcat(buf,"\n");
    else        strcat(buf," ");
    FWrite(f,buf);
  }
  FWrite(f,"        ] }\n");
  FWrite(f,"         normalPerVertex TRUE\n");

  // Texcoords
  sprintf(buf,"        texCoord DEF %s-TEXCOORD"
    " TextureCoordinate { point [\n",objName);
  FWrite(f,buf);
  for(i=0;i<g->tvertices;i++)
  {
    if((i%3)==0)strcpy(buf,"         ");
    else buf[0]=0;
    sprintf(buf2,"%.3f %.3f",g->tvertex[i*2+0],g->tvertex[i*2+1]);
    strcat(buf,buf2);
    if(i<g->tvertices-1)
      strcat(buf,",");
    if((i%3)==2)strcat(buf,"\n");
    else        strcat(buf," ");
    FWrite(f,buf);
  }
  FWrite(f,"        ] }\n");

  // Coordinate/texCoord/normal indices (all look the same)
  for(j=0;j<3;j++)
  {
    switch(j)
    {
      case 0: FWrite(f,"        coordIndex [\n"); break;
      case 1: FWrite(f,"        texCoordIndex [\n"); break;
      case 2: FWrite(f,"        normalIndex [\n"); break;
    }
    for(i=0;i<g->indices;i++)
    {
      if((i%12)==0)strcpy(buf,"         ");
      else         buf[0]=0;
      sprintf(buf2,"%d, ",g->index[i]);
      strcat(buf,buf2);
      // Every tri is a polygon
      if((i%3)==2)
      {
        strcat(buf,"-1");
        // Another index coming up?
        if(i<g->indices-1)
          strcat(buf,", ");
      }
      // Line delimiter
      if((i%12)==11)strcat(buf,"\n");
      FWrite(f,buf);
    }
    FWrite(f,"        ]\n");
  }
  
  FWrite(f,"       }\n");
  FWrite(f,"     }\n");
  FWrite(f,"   ]\n");
  FWrite(f,"}\n");
  return TRUE;
}

/***************
* Export geode *
***************/
bool DGeodeExportVRML(DGeode *g,cstring fname)
// Export a geode to a VRML(2.0 format) file.
// Returns TRUE if succesful.
{
  QFile *f;
  int i;
  cstring fileBase;
  char buf[256],*s;
  
  f=new QFile(fname,QFile::WRITE);
  if(!f->IsOpen())
  {
    qwarn("DGeodeExportVRML(); can't open file '%s' for writing",fname);
    return FALSE;
  }

  strcpy(buf,QFileBase(fname));
  s=(string)QFileExtension(buf);
  if(s)*s=0;
  WriteHeader(f);
  
  for(i=0;i<g->geobs;i++)
  {
    if(!DGeobExportVRML(f,g->geob[i],i,buf))
    {
     fail:
      QDELETE(f);
      return FALSE;
    }
  }
  QDELETE(f);
  return TRUE;
}
