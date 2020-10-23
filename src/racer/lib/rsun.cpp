/*
 * Racer - The sun
 * 10-10-01: Created!
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/sun.h>
DEBUG_ENABLE

RSun::RSun()
{
  int i;
  char buf[256];

  flags=ACTIVE;
  pos.SetToZero();
  vWin.SetToZero();
  whiteoutFactor=40000;           // Not a lot
  visibilityMethod=VM_NONE;

  // Read parameters
  visibilityMethod=RMGR->GetGfxInfo()->GetInt("fx.sun.visibility_method");
  RMGR->GetGfxInfo()->GetInts("fx.sun.texture",MAX_FLARE,flareTex);
  RMGR->GetGfxInfo()->GetFloats("fx.sun.length",MAX_FLARE,length);
  RMGR->GetGfxInfo()->GetFloats("fx.sun.size",MAX_FLARE,size);

  // Read the textures
  QCV->Select();
  for(i=0;i<MAX_TEX;i++)
  {
    QImage *img;
    sprintf(buf,"data/images/flare%d.bmp",i);
    //strcpy(buf,"data/images/logo.tga");
    img=new QImage(buf);
    if(!img->IsRead())
    { tex[i]=0;
    } else
    {
      DBitMapTexture *tbm;
      tbm=new DBitMapTexture(img);
//qdbg("tex[%d]=%p\n",i,tbm);
      tex[i]=tbm;
    }
//sprintf(buf,"/tmp/flare%d.tga",i);
//img->Write(buf,QBMFORMAT_TGA);
    QDELETE(img);
  }
}
RSun::~RSun()
{
  int i;

  for(i=0;i<MAX_TEX;i++)
    QDELETE(tex[i]);
}

void RSun::CalcWindowPosition()
// From the sun 3D location, derive the 2D window coordinates
// Assumes the OpenGL context is still 3D, so the correct 2D coordinate
// can be found. After switching the OpenGL context to 2D, call Paint()
// to actually draw the sun and its lens reflections.
{
  DVector3  z;
  QGLContext *gl=GetCurrentQGLContext();
  dfloat *m=gl->GetModelViewMatrix()->GetM();

  // Sun is defined?
  if(!(flags&ACTIVE))return;

//qdbg("RSun:CalcWindowPos()\n");
  // Determine is sun is in FRONT of us
  // FUTURE: get cached copy from dglobal; the track cams already know
  // exactly what the modelview matrix is, only the car cams need to
  // be enhanced so the matrix is known. All this to avoid a glGet(),
  // which seems esp. costly on Voodoo's (just a theory, as glGetError()
  // made a world of difference).
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
//mModelView->DbgPrint("ModelView");
//gl->Project(&pos,&vWin);
//return;
  // Extract the Z vector from the matrix. Alternative would be 0-1-2,
  // which seems to work just about in reverse.
  z.x=-m[2];
  z.y=-m[6];
  z.z=-m[10];
//z.DbgPrint("z2-6-10");
  //dot=z.Dot(pos);
//qdbg("  dot product=%f\n",dot);
  if(z.Dot(pos)<0)
  {
    // Sun is invisible
    flags&=~VISIBLE;
    // No need to project
    return;
  }
  flags|=VISIBLE;
#ifdef ND
  if(dot<0)qdbg("  -> Invisible\n");
  else     qdbg("  -> VISIBLE!\n");
#endif

#ifdef OBS_ALTERNATIVE_WORKS_REVERSED_SOMEWHAT
  // Alternative Z vector
  z.x=m[0];
  z.y=m[1];
  z.z=m[2];
z.DbgPrint("z012");
  dot=z.Dot(pos);
qdbg("  dot product=%f\n",dot);
  if(dot<0)qdbg("  -> Invisible\n");
  else     qdbg("  -> VISIBLE!\n");
#endif
  gl->Project(&pos,&vWin);

  if(visibilityMethod==VM_ZBUFFER)
  {
    // Read zbuffer at that pixel to determine is something was drawn there.
    // If so, assume that's opaque and hide the sun.
    GLfloat v=0;
    glReadPixels((int)vWin.x,(int)vWin.y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&v);
//qdbg("sun Z sample: %f\n",v);
    if(v<1.0f)
    {
      // Something was drawn in front of the sun
      flags&=~VISIBLE;
    }
  }
}

void RSun::Paint()
// Paints the sun, if in view, and flares.
// Method is to use the projected window coordinate, then from there draw
// a line through the viewport's center and draw some lens artifacts on route.
// Assumes CalcWindowPosition() has been called.
// Assumes a 2D view
{
  float factor;
  int   i;
  struct LensFX
  {
    DVector3 pos;
    DVector3 size;
  };
  LensFX fx,*p=&fx;
  DVector3 center,dir;
  int     *vp;

//qdbg("RSun:Paint()\n");

  // Sun is defined?
  if(!(flags&ACTIVE))return;

  // Sun is in front of us?
  if(!(flags&VISIBLE))return;

  // Calculate 2D center in viewport
  vp=QCV->GetGLContext()->GetViewport();

  // In view?
  if(vWin.x<-100||vWin.x>vp[0]+vp[2]+100)return;
  if(vWin.y<-100||vWin.y>vp[1]+vp[3]+100)return;

  center.x=vp[0]+vp[2]/2;
  center.y=vp[1]+vp[3]/2;
  center.z=0;
  vWin.z=0;
  // Calculate direction
  dir=center;
  dir.Subtract(&vWin);
  dir.Negate();
  //dir.Scale(2.8);

//vWin.DbgPrint("vWin");
//center.DbgPrint("center");
//dir.DbgPrint("dir");

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  glDepthMask(GL_FALSE);
  //glEnable(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
  //glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_ONE);
  //glBlendFunc(GL_ONE,GL_ONE);

  glEnable(GL_TEXTURE_2D);
  //glDisable(GL_TEXTURE_2D);

  glColor3f(1,1,.5);
  //glNormal3f(0,0,-1);

  //glBegin(GL_QUADS);
  for(i=0;i<MAX_FLARE;i++)
  {
    // Calculate position
#ifdef OBS
    p->pos.x=vWin.x+dir.x*i/MAX_TEX;
    p->pos.y=vWin.y+dir.y*i/MAX_TEX;
    p->pos.x=vWin.x+dir.x*length[i];
    p->pos.y=vWin.y+dir.y*length[i];
#endif
    p->pos.x=center.x+dir.x*length[i];
    p->pos.y=center.y+dir.y*length[i];
    p->size.x=128.0*size[i];
    p->size.y=128.0*size[i];
    // Select right texture (optimize texture selection)
    if(i==0||flareTex[i]!=flareTex[i-1])    // Note the conditional OR
    {
      tex[flareTex[i]]->Select();
    }

//qdbg("  i=%d -> pos=%f, %f\n",i,p->pos.x,p->pos.y);

    glPushMatrix();
    glTranslatef(p->pos.x,p->pos.y,0);
    if(i==0)
    {
      // First, main sun object gets rotated slightly according to the
      // left-right position onscreen
      glRotatef((center.x-p->pos.x)*0.1,0,0,1);
    }

    glBegin(GL_QUADS);
#ifdef OBS_TRANSLATE_HERE
    glTexCoord2f(0,0);
    glVertex2f(p->pos.x-p->size.x,p->pos.y+p->size.y);
    glTexCoord2f(0,1);
    glVertex2f(p->pos.x-p->size.x,p->pos.y-p->size.y);
    glTexCoord2f(1,1);
    glVertex2f(p->pos.x+p->size.x,p->pos.y-p->size.y);
    glTexCoord2f(1,0);
    glVertex2f(p->pos.x+p->size.x,p->pos.y+p->size.y);
#endif
    // Assume translated correctly
    glTexCoord2f(0,0);
    glVertex2f(-p->size.x,p->size.y);
    glTexCoord2f(0,1);
    glVertex2f(-p->size.x,-p->size.y);
    glTexCoord2f(1,1);
    glVertex2f(p->size.x,-p->size.y);
    glTexCoord2f(1,0);
    glVertex2f(p->size.x,p->size.y);
    glEnd();

    glPopMatrix();
  }

  // Determine white out factor
  factor=0.9f-(vWin.SquaredDistanceTo(&center)/whiteoutFactor);
//qdbg("factor=%f\n",factor);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  if(factor>0.01)
  {
    // Paint brightness polygon. If you look very much into the sun,
    // the screen brightens out.
    glColor3f(factor,factor,factor);
    glDisable(GL_TEXTURE_2D);
    //glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_ONE);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
    glBegin(GL_QUADS);
      glVertex2f(vp[0],vp[1]);
      glVertex2f(vp[0],vp[1]+vp[3]);
      glVertex2f(vp[0]+vp[2],vp[1]+vp[3]);
      glVertex2f(vp[0]+vp[2],vp[1]);
    glEnd();
  }

  // Restore state
  glDepthMask(GL_TRUE);
}
