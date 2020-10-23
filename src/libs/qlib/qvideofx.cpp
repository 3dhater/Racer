/*
 * QVideoFX - graphics contents effects
 * 05-10-96: Created!
 * 21-04-98: Support for free flight video effects
 * IMPORTANT:
 * - When adding a new effect:
 *   - categorize into Static, Free In and Free Out
 *   - IsFree() must handle effect
 *   - GetBoundingRect() for Free FX
 * BUGS:
 * - Members fSetup, bmDst and cv should NOT be static module-wise.
 *   (but static in the class)
 * NOTES:
 * - Uses the name 'win' where QDrawable (drw) is actually used (28-2-99)
 * - Platform independent source code.
 * (C) MarketGraph/RVG
 */

#include <qlib/videofx.h>
#include <qlib/app.h>
#include <qlib/canvas.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>		// sginap
#include <qlib/debug.h>
DEBUG_ENABLE

// The following should be static class members, but results
// in link errors (not found)
static bool fSetup;
static QBitMap *bmDst;          // Future bitmap after effects
static QBitMap *bmFix;          // Restore bitmap for free FX, quite dynamic
static QCanvas *cvFix;		// To paint in bmFix
static QCanvas *cvDst;		// To paint in bmDst
//static QCanvas *cv;

/*******
* FLAT *
*******/
QBitMap *QVideoFXGetBitMap()
{ return bmDst;
}
void QVideoFXRun()
// Run video effects only with future screen (the next frame)
// Must use this for performance reasons
{ QCanvas *cv=app->GetBC()->GetCanvas();
  QFXManager *fm=app->GetFXManager();
  QFXManager *vfm=app->GetVideoFXManager();
  QVideoFX *fx;
  QRect rRestore,rBounds,*r;
  bool first=TRUE;
  
  qdbg("QVideoFXRun()\n");
  // Get the future gel situation in the back buffer
  fm->Iterate(QFXManager::NO_SWAP);
  // Fix the future situation
  cv->ReadPixels(bmFix);
  // Get the total restore rectangle
  for(fx=(QVideoFX*)vfm->GetHead();fx;fx=(QVideoFX*)fx->next)
  { if(first)
    { fx->GetBoundingRect(&rRestore);
    } else
    { fx->GetBoundingRect(&rBounds);
      rRestore.Union(&rBounds);
    }
  }
  qdbg("Total restore=%d,%d %dx%d\n",rRestore.x,rRestore.y,
    rRestore.wid,rRestore.hgt);
#ifdef OBS
  cv->Blit(bmFix);
  //cv->Blit(bmDst);
  app->GetBC()->Swap();
  qdbg("This is the back buffer\n"); QNap(100);
  app->GetBC()->Swap();
#endif
  // Run video effects solely
  //vfm->Run();
  while(!vfm->IsDone())
  { // Restore bounding rectangle of all effects
    // (often, this is the entire screen)
    //qdbg("-Restore\n");QNap(100);
    cv->Blit(bmFix,rRestore.x,rRestore.y,rRestore.wid,rRestore.hgt,
      rRestore.x,rRestore.y);
    //qdbg("-VFM it\n");QNap(100);
    vfm->Iterate();
  }
}

/*********
* STATIC *
*********/
void QVideoFX::Setup(QCanvas *cv)
{ QDrawable *win=cv->GetDrawable();
  if(fSetup)return;
  qdbg("QVideoFX::Setup()\n");
  //bmDst=new QBitMap(win->GetDepth(),win->GetWidth(),win->GetHeight());
  bmDst=new QBitMap(32,win->GetWidth(),win->GetHeight());
  cvDst=new QCanvas();
  cvDst->UseBM(bmDst);
  printf("  <fxbitmap=%dx%d>\n",win->GetWidth(),win->GetHeight());
  bmFix=new QBitMap(32,win->GetWidth(),win->GetHeight());
  cvFix=new QCanvas();
  cvFix->UseBM(bmFix);
  //cv=new QCanvas(win);
  fSetup=TRUE;
}

QVideoFX::QVideoFX(QCanvas *_cv,int _flags)
  : QFX(_flags)
{
  cv=_cv;
  if(!cv)
  {
    if(!app->GetBC())
    { qerr("QVideoFX need BC");
    }
    cv=app->GetBC()->GetCanvas();
  }
  Setup(cv);

  r.x=r.y=r.wid=r.hgt=0;
  fxType=0;
  frame=fdelay=frames=0;
  srcBM=0;
  pi=0;
#ifdef OBS
  params=(QVideoFXParams*)qcalloc(sizeof(*params));
  if(!params)qerr("QVideoFX; out of memory");
#endif
  srcRect.x=srcRect.y=srcRect.wid=srcRect.hgt=0;
}

QVideoFX::~QVideoFX()
{
  //if(bmDst)delete bmDst;
  //if(cv)delete cv;
  //if(params)qfree(params);
  RemoveParticles();
}

/************
* PARTICLES *
************/
void QVideoFX::CreateParticles(int xParts,int yParts)
{
  QASSERT_V(xParts>0&&yParts>0);
  int total=xParts*yParts;
  pi=(QVParticle*)qcalloc(total*sizeof(QVParticle));
  if(!pi)
  { qerr("QVideoFX::CreateParticles(): out of memory");
    return;
  }
  xParticles=xParts;
  yParticles=yParts;
  
  // Create subdivision source parts
  int x,y,n,i,v1,v2;
  QVParticle *p;
  //QWindow *win=cv->GetWindow();

  n=PARTICLES;
  for(i=0;i<n;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    p->sx=srcRect.x+x*srcRect.wid/xParticles;
    p->sy=srcRect.y+y*srcRect.hgt/yParticles;
    // Width
    v1=p->sx;
    v2=srcRect.x+(x+1)*srcRect.wid/xParticles;
    p->wid=v2-v1;
    // Height
    v1=p->sy;
    v2=srcRect.y+(y+1)*srcRect.hgt/yParticles;
    p->hgt=v2-v1;
    // Init the rest
    p->frame=0;
    p->flags=0;
  }
}
void QVideoFX::RemoveParticles()
{
  if(pi)qfree(pi);
  pi=0;
}

void QVideoFX::CreateParticleDelay(int delayType,int delayFactor)
{
  int i,x,y;
  QVParticle *p;
  
  for(i=0;i<PARTICLES;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    // Delay
    if(delayType==QVFXDT_LINEAR)
    { p->delay=i*delayFactor;
    } else if(delayType==QVFXDT_SWEEP_LR_TB)
    { p->delay=x*(yParticles*delayFactor/2)+y*delayFactor;
    } else
    { qwarn("QVideoFX: delaytype %d not implemented\n",delayType);
    }
  }
}

bool QVideoFX::Define(int typ,QRect *_r,int _frames,int delay,QBitMap *sbm,
  QRect *sRect)
// Generic definition
// For Free FX, use the specific Define...() functions
// For Static FX, this should be enough
// _r = destination rectangle
{
  //printf("QVideoFX::Create\n");
  //printf("  bmbuffer=%p\n",bmDst->GetBuffer());
  fxType=typ;
  r.x=_r->x; r.y=_r->y;
  r.wid=_r->wid; r.hgt=_r->hgt;
  frames=_frames;
  fdelay=delay;
  frame=0;
  srcBM=sbm;
  if(srcBM)
  { if(sRect)
    { srcRect.x=sRect->x; srcRect.y=sRect->y;
      srcRect.wid=sRect->wid; srcRect.hgt=sRect->hgt;
    }
  }

  if(sbm==0)
  { // Onscreen effect

    // Copy affected box to dstBm, at the same spot
    qdbg("read; %d,%d %dx%d\n",r.x,r.y,r.wid,r.hgt);
    cv->ReadPixels(bmDst,r.x,r.y,r.wid,r.hgt,r.x,r.y);
    // Equalize box contents before the next swap
    glReadBuffer(GL_FRONT);
    //printf("CopyPixels(%d,%d, %dx%d)\n",r.x,r.y,r.wid,r.hgt);
    glDisable(GL_BLEND);
    cv->CopyPixels(r.x,r.y,r.wid,r.hgt,r.x,r.y);
    glReadBuffer(GL_BACK);

#ifdef OBS
    //$test show
    cv->Disable(QCANVASF_BLEND);
    cv->Blit(bmDst);
    cv->Blit(bmDst,100,100,64,64,1,1);
    app->GetBC()->Swap();
    QNap(100);
    app->GetBC()->Swap();
#endif
  } else
  { // Free FX (in/out)
    /*if(IsOut())
    { // Remember original (inwarded) bitmap
      cvDst->Blit(srcBM,r.x,r.y,srcRect.wid,srcRect.hgt,srcRect.x,srcRect.y);
    }*/
#ifdef OBS
    QRect rb;
    GetBoundingRect(&rb);
    cv->ReadPixels(bmFix,rb.x,rb.y,rb.wid,rb.hgt,rb.x,rb.y);
#endif
#ifdef OBS
    // Equalize box contents before the next swap
    glReadBuffer(GL_FRONT);
    //printf("CopyPixels(%d,%d, %dx%d)\n",r.x,r.y,r.wid,r.hgt);
    glDisable(GL_BLEND);
    cv->CopyPixels(rb.x,rb.y,rb.wid,rb.hgt,rb.x,rb.y);
    glReadBuffer(GL_BACK);
#endif
  }

  // Init FX properties
  Done(FALSE);
  frame=1;			// Frame 0 is initial situation
  //QHexDump(bmDst->GetBuffer(),16);
  return TRUE;			// Let's never fail
}
bool QVideoFX::DefineFlight(QRect *rDst,QBitMap *sbm,QRect *rSrc,int frames, 
    int xParts,int yParts,int delayType,int delayFactor,
    int delay)
{ int x,y,n,i;
  QVParticle *p;
  QDrawable *win=cv->GetDrawable();
  
  Define(QVFXT_FLY_BT,rDst,frames,delay,sbm,rSrc);
  // Particles
  CreateParticles(xParts,yParts);
  CreateParticleDelay(delayType,delayFactor);
  n=PARTICLES;
  for(i=0;i<n;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    // Start/end
    p->x0=r.x+p->sx-srcRect.x;
    p->y0=win->GetHeight()+p->sy-srcRect.y;
    p->xn=p->sx;
    p->yn=r.y+p->sy-srcRect.y;
    p->x=p->x0;
    p->y=p->y0;
    p->frames=frames;
#ifdef OBS
    // Delay
    if(delayType==QVFXDT_LINEAR)
    { p->delay=i*delayFactor;
    } else if(delayType==QVFXDT_SWEEP_LR_TB)
    { p->delay=x*(yParticles*delayFactor/2)+y*delayFactor;
    } else
    { qwarn("QVideoFX: delaytype %d not implemented\n",delayType);
    }
#endif
  }
  return TRUE;
}
bool QVideoFX::DefineExplode(QRect *rDst,QBitMap *sbm,QRect *rSrc,
  int xParts,int yParts,int delayType,int delayFactor,
  int vxMax,int vxBias,int vyMax,int vyBias,
  int gravity,int delay)
// Exploding particles
{ int x,y,n,i;
  QVParticle *p;
  QDrawable *win=cv->GetDrawable();
  
  Define(QVFXT_EXPLODE,rDst,123,delay,sbm,rSrc);
  // Particles
  CreateParticles(xParts,yParts);
  CreateParticleDelay(delayType,delayFactor);
  n=PARTICLES;
  for(i=0;i<n;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    // Start/end at original spot
    p->x=p->sx-srcRect.x+rDst->x;
    p->y=p->sy-srcRect.y+rDst->y;
    p->frames=123;
    // Spurt around +/- the maximum velocity
    if(vxMax==0)p->vx=vxBias;
    else        p->vx=rand()%(vxMax*2)-vxMax+vxBias;
    if(vyMax==0)p->vy=vyBias;
    else        p->vy=rand()%(vyMax*2)-vyMax+vyBias;
    p->ax=0; p->ay=gravity;
    if(!p->ay)
    { if(p->vx==0&&p->vy==0)
      { // Make sure it will ever get away
        p->vx=1;
      }
    }
  }
  return TRUE;
}

#ifdef OBS
void QVideoFX::SetParams(QVideoFXParams *p)
{
  *params=*p;
}
#endif

bool QVideoFX::InitEffect()
// Initialize default settings etc.
{
  /*switch(fxType)
  {
    case QVFXT_FLY_BT:
      params->rows=1; params->cols=1;
      break;
  }*/
  Done(FALSE);
  frame=1;			// Frame 0 is initial situation
  return TRUE;
}
bool QVideoFX::IsFree()
{ if(srcBM)return TRUE;
  return FALSE;
}
bool QVideoFX::IsOut()
{ switch(fxType)
  { case QVFXT_EXPLODE: return TRUE;
  }
  return FALSE;
}

void QVideoFX::GetBoundingRect(QRect *rb)
// Retrieve bounding rectangle of effect (for restores)
{ QDrawable *win=cv->GetDrawable();
  int winHgt,winWid;
  // Start out with destination rect
  rb->x=r.x; rb->y=r.y;
  rb->wid=r.wid; rb->hgt=r.hgt;
  // Static effects don't move around
  if(!IsFree())
    return;
  winWid=win->GetWidth();
  winHgt=win->GetHeight();
  switch(fxType)
  { 
    case QVFXT_FLY_BT:
      // Full to the bottom
      rb->hgt=winHgt-r.y;
      break;
    case QVFXT_EXPLODE:
      // Fullscreen
      rb->x=0; rb->y=0;
      rb->wid=winWid; rb->hgt=winHgt;
      break;
  }
}

/**********
* WIPE LR *
**********/
void QVideoFX::DoWipeLR()
{ int curWid;
  if(frames<=1)curWid=r.wid;
  else         curWid=frame*r.wid/(frames-1);
  if(curWid>0)
    cv->Blit(bmDst,r.x,r.y,curWid,r.hgt,r.x,r.y);
}

/**********
* WIPE RL *
**********/
void QVideoFX::DoWipeRL()
{ int curWid,xOffset;
  if(frames<=1)curWid=r.wid;
  else         curWid=frame*r.wid/(frames-1);
  if(curWid>0)
  { xOffset=r.wid-curWid;
    cv->Blit(bmDst,r.x+xOffset,r.y,curWid,r.hgt,r.x+xOffset,r.y);
  }
}

void QVideoFX::DoWipeTB()
{}
void QVideoFX::DoWipeBT()
{}

/*******
* GROW *
*******/
void QVideoFX::DoGrow()
{ int x,y;
  int curWid,curHgt;
  if(frames<=1)
  { curWid=r.wid; curHgt=r.hgt;
  } else
  { curWid=frame*r.wid/(frames-1);
    curHgt=frame*r.hgt/(frames-1);
  }
  // Deduce starting X/Y offset
  x=(r.wid-curWid)/2;
  y=(r.hgt-curHgt)/2;
  if(curWid>0&&curHgt>0)
    cv->Blit(bmDst,r.x+x,r.y+y,curWid,curHgt,r.x+x,r.y+y);
}

/********
* CROSS *
********/
void QVideoFX::DoCross()
{ int den,nom;
  //proftime_t t;
  den=1;
  nom=(frames-frame);
  if(!nom)nom=1;
  //glFinish(); profStart(&t);
//#define USE_CONSTANT_BLEND
#ifdef USE_CONSTANT_BLEND
  // Not accelerated on Indy
  //printf("> Cross f=%d/%d, alpha=%d/%d\n",frame,frames,den,nom);
  //glClear(GL_COLOR_BUFFER_BIT);
  //glEnable(GL_BLEND);
  //cv->Enable(QCANVASF_BLEND);
#error Does not work; use QCanvas blendColor
  cv->BlendingMode(QCanvas::BLEND_SRC_ALPHA);
  cv->Select();
  glEnable(GL_BLEND);
  glBlendColorEXT(0,0,0,((GLfloat)den)/((GLfloat)nom));
  //glBlendColorEXT(0.1,0.1,0.1,0.1);
  glBlendFunc(GL_CONSTANT_ALPHA_EXT,GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
  //glBlendFunc(GL_CONSTANT_COLOR_EXT,GL_ONE_MINUS_CONSTANT_COLOR_EXT);
  profReport(&t,"glBlendFunc done\n");
  //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  //glBlendEquationEXT(GL_FUNC_ADD_EXT);
  //glBlendFunc(GL_ZERO,GL_ZERO);
#else
  //printf("> Cross f=%d/%d, alpha=%d/%d\n",frame,frames,den,nom);
  //glClear(GL_COLOR_BUFFER_BIT);
  //cv->Enable(QCANVASF_BLEND);
  cv->SetBlending(QCanvas::BLEND_SRC_ALPHA);
  //glEnable(GL_BLEND);
  //glBlendColorEXT(0,0,0,0.9);
  //glBlendFunc(GL_ZERO,GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
  //glBlendFunc(GL_CONSTANT_ALPHA_EXT,GL_CONSTANT_ALPHA_EXT);
  //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  //glBlendEquationEXT(GL_FUNC_ADD_EXT);
  //glBlendFunc(GL_ZERO,GL_ZERO);
  //glBlendFunc(GL_CONSTANT_ALPHA_EXT,GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
  cv->FillAlpha(den*255/nom,r.x,r.y,r.wid,r.hgt,bmDst);
#endif
  cv->Blit(bmDst,r.x,r.y,r.wid,r.hgt,r.x,r.y);
  cv->Blend(FALSE);
  //cv->Disable(QCANVASF_BLEND);
  //glFinish(); profReport(&t,"  cross blit\n");
/*
  GLenum error;
  while((error=glGetError())!=GL_NO_ERROR)
    fprintf(stderr,"GL error: %s\n",gluErrorString(error));
*/
}

/*********
* SCROLL *
*********/
void QVideoFX::DoScrollLR()
{ int curWid;
  if(frames<=1)curWid=r.wid;
  else         curWid=frame*r.wid/(frames-1);
  // Left part
  if(curWid>0)
  { glDisable(GL_BLEND);
    cv->Blit(bmDst,r.x,r.y, curWid,r.hgt, r.x+r.wid-curWid,r.y);
  }
}
void QVideoFX::DoScrollRL()
{ int curWid;
  if(frames<=1)curWid=r.wid;
  else         curWid=frame*r.wid/(frames-1);
  // Left part
  if(curWid>0)
  { glDisable(GL_BLEND);
    cv->Blit(bmDst,r.x+r.wid-curWid,r.y, curWid,r.hgt, r.x,r.y);
  }
}

/**************
* SCROLL PUSH *
**************/
void QVideoFX::DoScrollPushLR()
{ int curWid,prevWid;
  int nPush;
  if(frames<=1)
  { curWid=r.wid;
    prevWid=0;
  } else
  { curWid=frame*r.wid/(frames-1);
    if(frame<=2)prevWid=0;
    else        prevWid=(frame-2)*r.wid/(frames-1);	// Dblbuf!
  }
  // Push entire screen
  glDisable(GL_BLEND);
  nPush=curWid-prevWid;
  if(nPush>0)
  { //cv->CopyPixels(r.x+nPush,r.y,r.wid-nPush,r.hgt,r.x,r.y);
    cv->CopyPixels(r.x,r.y,r.wid-nPush,r.hgt,r.x+nPush,r.y);
    // Push in new piece of the screen
    //cv->Blit(bmDst,r.x,r.y,nPush,r.hgt,r.x+r.wid-curWid-nPush,r.y);
    cv->Blit(bmDst,r.x,r.y,nPush,r.hgt,r.x+r.wid-curWid,r.y);
  }
}
void QVideoFX::DoScrollPushRL()
{ int curWid,prevWid;
  int nPush;
  if(frames<=1)
  { curWid=r.wid;
    prevWid=0;
  } else
  { curWid=frame*r.wid/(frames-1);
    if(frame<=2)prevWid=0;
    else        prevWid=(frame-2)*r.wid/(frames-1);	// Dblbuf!
  }
  // Push entire screen
  glDisable(GL_BLEND);
  nPush=curWid-prevWid;
  if(nPush>0)
  { cv->CopyPixels(r.x+nPush,r.y,r.wid-nPush,r.hgt,r.x,r.y);
  }
  // Push in new piece of the screen
  cv->Blit(bmDst,r.x+r.wid-nPush,r.y,nPush,r.hgt,r.x+curWid-nPush,r.y);
}

void QVideoFX::DoScrollPushBT()
{ int curHgt,prevHgt;
  int nPush;
  if(frames<=1)
  { curHgt=r.hgt;
    prevHgt=0;
  } else
  { curHgt=frame*r.hgt/(frames-1);
    if(frame<=2)prevHgt=0;
    else        prevHgt=(frame-2)*r.hgt/(frames-1);	// Dblbuf!
  }
  // Push entire screen
  //glDisable(GL_BLEND);
  //cv->Disable(QCANVASF_BLEND);
  cv->Blend(FALSE);
  nPush=curHgt-prevHgt-1;
  if(nPush>0)
  { cv->CopyPixels(r.x,r.y+nPush,r.wid,r.hgt-nPush,r.x,r.y);
    // Push in new piece of the screen
    //cv->Blit(bmDst,r.x+r.hgt-nPush,r.y,nPush,r.hgt,r.x+curHgt-nPush,r.y);
    cv->Blit(bmDst,r.x,r.y+r.hgt-nPush,r.wid,nPush,r.x,r.y+prevHgt);
    qdbg("QVideoFX::DoScrollPushBT: nPush=%d, curHgt=%d, prev=%d\n",
      nPush,curHgt,prevHgt);
  }
}
void QVideoFX::DoScrollPushTB()
{ printf("DoScrollPushTB NYI\n");
}

/**********************
* FREE FLIGHT EFFECTS *
**********************/
void QVideoFX::DoFlyBT()
// Simple flight
{ int x,y,sx,sy,swid,shgt,i;
  QDrawable *win=cv->GetDrawable();
  QRect rb;
  QVParticle *p;
  bool done=TRUE;

  //qdbg("DoFlyBT\n");
  x=r.x;
  if(frames<=1)
  { y=r.y;
  } else
  { y=win->GetHeight()+frame*(r.y-win->GetHeight())/(frames-1);
  }
  /*qdbg("DoFlyBT: srcBM=%p, src=%d,%d %dx%d, x/y=%d/%d\n",
    srcBM,srcRect.x,srcRect.y,srcRect.wid,srcRect.hgt,x,y);*/
  //cv->Disable(QCANVASF_BLEND);
  cv->Blend(FALSE);
#ifdef OBS
  if(params->rows==1&&params->cols==1)
  { // Paint new
    cv->Blit(srcBM,x,y,srcRect.wid,srcRect.hgt,srcRect.x,srcRect.y);
  } else
#endif
  // Divide into pieces
  cv->Blend(FALSE);
  for(i=0;i<PARTICLES;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    if(p->flags&QVPF_DONE)
      continue;
    if(p->flags&QVPF_EQUALIZE)
    { p->flags&=~QVPF_EQUALIZE;
      p->flags|=QVPF_DONE;
      // Blit fixed into bmFix so no more paints necessary
      cvFix->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
      goto doit;
    }
    if(p->delay)
    { p->delay--;
      done=FALSE;
    } else
    { if(p->frame<p->frames-1)
      { p->frame++;
        done=FALSE;
      } else p->flags|=QVPF_EQUALIZE;
    }
   doit:
    if(frames<=1)p->y=p->yn;
    else p->y=p->y0+p->frame*(p->yn-p->y0)/(p->frames-1);
    cv->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
  }
  if(done)
    Done(TRUE);
}
void QVideoFX::DoExplode()
// Particles flying around
{ int x,y,sx,sy,swid,shgt,i;
  //QCanvas *cv=app->GetBC()->GetCanvas();
  QDrawable *win=cv->GetDrawable();
  QRect rb;
  QVParticle *p;
  bool done=TRUE;

  //qdbg("DoExplode\n");
  if(frame==1)
  { // Init out effect; optimize
    //qdbg("frame %d\n",frame);
    cv->Blend(FALSE);
    for(i=0;i<PARTICLES;i++)
    { p=&pi[i];
      if(p->delay)
      { // Remember future image for this particle when it's gone
        cvDst->Blit(bmFix,p->x,p->y,p->wid,p->hgt,p->x,p->y);
        //// ^^ was done at Define() time
        // bmFix was already drawn, so blit this part for this frame
        cv->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
        // Fix it for now until it starts moving
        cvFix->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
      }
    }
    //app->GetBC()->Swap(); qdbg("This is Explode first\n"); QNap(500);
    frame=2;
  }
  cv->Blend(FALSE);
  for(i=0;i<PARTICLES;i++)
  { x=PARTICLE_X(i);
    y=PARTICLE_Y(i);
    p=&pi[i];
    if(p->flags&QVPF_DONE)
      continue;
    if(p->flags&QVPF_EQUALIZE)
    { p->flags&=~QVPF_EQUALIZE;
      p->flags|=QVPF_DONE;
      // Blit fixed into bmFix so no more paints necessary
      //cvFix->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
      goto doit;
    }
    if(p->delay)
    { p->delay--;
      done=FALSE;
      if(p->delay==0)
      { // Copy final restore part in fix bitmap
        cvFix->Blit(bmDst,p->x,p->y,p->wid,p->hgt,p->x,p->y);
      }
      // Don't paint it
      continue;
    } else
    { // Keep moving until out of frame
      p->x+=p->vx;
      p->y+=p->vy;
      p->vx+=p->ax;
      p->vy+=p->ay;
      if(p->x+p->wid>0&&p->x<win->GetWidth()&&
         p->y+p->hgt>0&&p->y<win->GetHeight())
      {notdone:
        //qdbg("alive; #%d @%d,%d\n",i,p->x,p->y);
        done=FALSE;
      } else p->flags|=QVPF_EQUALIZE;
    }
   doit:
    //if(frames<=1)p->y=p->win->GetHeight()+1;
    //else p->y=p->y0+p->frame*(p->yn-p->y0)/(p->frames-1);
    cv->Blit(srcBM,p->x,p->y,p->wid,p->hgt,p->sx,p->sy);
  }
  if(done)
    Done(TRUE);
}

void QVideoFX::Equalize()
{
  if(IsFree())
  { // No action
  } else
  { // Copy from displayed buffer to hidden buffer, if possible
    cv->Blit(bmDst,r.x,r.y,r.wid,r.hgt,r.x,r.y);
  }
  //printf("equalized\n");
  //sginap(100);
}

void QVideoFX::Iterate()
{
  if(IsDone())return;

  // Handle equalize (at last iteration)
  if(flags&QFXF_EQUALIZE)
  { Equalize();
    flags&=~QFXF_EQUALIZE;
    Done(TRUE);
    // Free effect must paint itself one more time
    if(!IsFree())
      return;
  }
  // Handle delay
  if(fdelay)
  { fdelay--;
    return;
  }
  switch(fxType)
  { // Static effects
    case QVFXT_WIPE_LR: DoWipeLR(); break;
    case QVFXT_WIPE_RL: DoWipeRL(); break;
    case QVFXT_WIPE_TB: DoWipeTB(); break;
    case QVFXT_WIPE_BT: DoWipeBT(); break;
    case QVFXT_GROW: DoGrow(); break;
    case QVFXT_CROSS: DoCross(); break;
    case QVFXT_SCROLL_LR: DoScrollLR(); break;
    case QVFXT_SCROLL_RL: DoScrollRL(); break;
    case QVFXT_SCROLLPUSH_LR: DoScrollPushLR(); break;
    case QVFXT_SCROLLPUSH_RL: DoScrollPushRL(); break;
    case QVFXT_SCROLLPUSH_TB: DoScrollPushTB(); break;
    case QVFXT_SCROLLPUSH_BT: DoScrollPushBT(); break;

    // Free flight effects
    case QVFXT_FLY_BT: DoFlyBT(); break;
    // Out free fx
    case QVFXT_EXPLODE: DoExplode(); break;
    default: break;		// Unsupported effect; do cut
  }
  // Check to see if effect is done
  if(IsDone())return;		// In case of free effect
  // Free effects don't have a predictable lifespan
  if(IsFree())return;
  if(++frame>=frames)
  { // Equalize at next iteration
    flags|=QFXF_EQUALIZE;
    frame--;
  }
}
