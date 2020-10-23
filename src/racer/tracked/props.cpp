/*
 * Model Property Editing
 * 10-04-2001: Created! (17:31:32)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// GUI
cstring  opNameProps[]=
{ // Track cam ops
  "Goto line start",
  "Toggle Flag: Sk(y)",
  "Toggle Flag: (C)ollide",
  "Toggle Flag: (S)urface",
  "Toggle Flag: (R)elative",
  //"Toggle Flag: Invisible",
  //"Toggle Flag: NoCulling",
  "Toggle Flag: No (Z)Buffer",
  "Toggle Flag: (D)ecal",
  //"Edit properties",
  0
};

// Selection (may often be 0)
RTV_Node *nodeProp;
int nodePropIndex=-1;

void PropsSetMode()
// Install us as the current mode
{
  SetOpButtons(opNameProps);
}

/********
* Stats *
********/
void PropsStats()
{
  char buf[40],buf2[40];
  cstring flagName[8]=
  {
    "Sky","Collide","Surface","Relative","Invisible","NoCulling","NoZBuffer",
    "Decal"
  };
  //int flagValue[8]={ F_SKY,F_COLLIDE,F_SURFACE,F_RELATIVE_POS,
    //F_INVISIBLE,
  int i;
  
  sprintf(buf,"Object: %s (%d out of %d)",
    nodeProp?nodeProp->fileName.cstr():"---",nodePropIndex,track->GetNodes());
  StatsAdd(buf);
  if(!nodeProp)
  {
    strcpy(buf,"---");
  } else
  {
    // Show flags
    buf[0]=0;
    for(i=0;i<8;i++)
    {
      if(nodeProp->flags&(1<<i))
      {
        strcat(buf,flagName[i]);
        strcat(buf," ");
      }
    }
    if(!buf[0])strcpy(buf,"<no flags set>");
  }
  sprintf(buf2,"Flags: %s",buf);
  StatsAdd(buf2);
}

/******
* Ops *
******/
static void ToggleFlag(int flag)
// Switch a flag
{
  if(!nodeProp)return;
  nodeProp->flags^=flag;
}
void PropsOp(int op)
{
  switch(op)
  {
    case 0: GotoPickPoint(); break;
    case 1: ToggleFlag(RTV_Node::F_SKY); break;
    case 2: ToggleFlag(RTV_Node::F_COLLIDE); break;
    case 3: ToggleFlag(RTV_Node::F_SURFACE); break;
    case 4: ToggleFlag(RTV_Node::F_RELATIVE_POS); break;
    case 5: ToggleFlag(RTV_Node::F_NO_ZBUFFER); break;
    case 6: ToggleFlag(RTV_Node::F_DECAL); break;
  }
}

/********
* Paint *
********/
void PropsPaint()
{

  // Any hilites?
  if(!nodeProp)return;
//qdbg("PropsPaint: prop=%p, model=%p\n",nodeProp,nodeProp->model);
  if(!nodeProp->geode)return;
//qdbg("PropsPaint: prop=%p\n",nodeProp);

  static int col;
  float fcol;
  DBox box;
  
  nodeProp->geode->GetBoundingBox(&box);
  
  // Paint cube
  col=(col+5)&511;
  if(col>255)
    fcol=((float)(511-col))/255.0f;
  else
    fcol=((float)col)/255.0f;
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glColor3f(fcol,fcol,fcol);
//box.min.DbgPrint("box min");
//box.max.DbgPrint("box max");
//qdbg("col=%d, fcol=%f\n",col,fcol);
  glBegin(GL_LINES);
    glVertex3f(box.min.x,box.min.y,box.min.z);
    glVertex3f(box.max.x,box.min.y,box.min.z);
    
    glVertex3f(box.min.x,box.min.y,box.min.z);
    glVertex3f(box.min.x,box.max.y,box.min.z);
    
    glVertex3f(box.min.x,box.min.y,box.min.z);
    glVertex3f(box.min.x,box.min.y,box.max.z);
    
    glVertex3f(box.max.x,box.min.y,box.min.z);
    glVertex3f(box.max.x,box.max.y,box.min.z);
    
    glVertex3f(box.max.x,box.min.y,box.min.z);
    glVertex3f(box.max.x,box.min.y,box.max.z);
    
    glVertex3f(box.max.x,box.max.y,box.min.z);
    glVertex3f(box.max.x,box.max.y,box.max.z);
    
    glVertex3f(box.min.x,box.max.y,box.min.z);
    glVertex3f(box.min.x,box.max.y,box.max.z);
    
    glVertex3f(box.min.x,box.max.y,box.min.z);
    glVertex3f(box.max.x,box.max.y,box.min.z);
    
    glVertex3f(box.min.x,box.max.y,box.max.z);
    glVertex3f(box.max.x,box.max.y,box.max.z);
    
    glVertex3f(box.min.x,box.min.y,box.max.z);
    glVertex3f(box.min.x,box.max.y,box.max.z);
    
    glVertex3f(box.max.x,box.min.y,box.max.z);
    glVertex3f(box.max.x,box.max.y,box.max.z);
    
    glVertex3f(box.min.x,box.min.y,box.max.z);
    glVertex3f(box.max.x,box.min.y,box.max.z);
  glEnd();
  glPopAttrib();
}

/*******
* Keys *
*******/
static void SelectProp(int n)
{
  RTV_Node *newNode;
  newNode=track->GetNode(n);
  // Check if the node exists, and if so, make it current
  if(newNode)
  {
    nodeProp=newNode;
    nodePropIndex=n;
  }
#ifdef OBS
  // Adjust range
  if(n<0)n=0;
  if(n>=track->GetNodes())n=track->GetNodes()-1;
  // Check range
  if(n<0||n>=track->GetNodes())return;
  
  // Set prop
  nodeProp=track->GetNode(n);
#endif
}
bool PropsKey(int key)
{
  if(key==QK_PAGEUP)
  {
    SelectProp(nodePropIndex-1);
  } else if(key==QK_PAGEDOWN)
  {
    SelectProp(nodePropIndex+1);
  } else if(key==QK_END)
  {
    SelectProp(track->GetNodes()-1);
  } else if(key==QK_HOME)
  {
    SelectProp(0);
  } else if(key==QK_Y)
  { ToggleFlag(RTV_Node::F_SKY);
  } else if(key==QK_C)
  { ToggleFlag(RTV_Node::F_COLLIDE);
  } else if(key==QK_S)
  { ToggleFlag(RTV_Node::F_SURFACE);
  } else if(key==QK_R)
  { ToggleFlag(RTV_Node::F_RELATIVE_POS);
  } else if(key==QK_Z)
  { ToggleFlag(RTV_Node::F_NO_ZBUFFER);
  } else if(key==QK_D)
  { ToggleFlag(RTV_Node::F_DECAL);
  }
  return FALSE;
}
