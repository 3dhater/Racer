/*
 * QScroll - vertical gel (Bobs/Texts) scroller
 * NOTES:
 * - To be enhanced into a usable Q class
 * 23-04-97: Created!
 * (C) MG/RVG
 */

#include <qlib/scroll.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <qlib/filter.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Due to something in 5.3 OpenGL, copying from front to back or vice versa
// is a lot slower in OpenGL (not in GL)
// Define SAMEBUF will use more intelligent double buffered scrolling
// techniques to keep the copies from BACK to BACK or FRONT to FRONT.
//#define SAMEBUF

void BenchCP(char *s);

// Define ONE_STEP to scroll the entire PAL area, instead of per-gel
// If speedy enough, ONE_STEP should work for worst-case scrollers
// where this entire screen is filled with tiny things.
#define ONE_STEP
//#define BENCH

#define MAX_GEL		100	// Initially

#ifdef BENCH
void BenchCP(char *s)
{ proftime_t t;
  int i;
  printf("BenchCP(%s)\n",s);
  glDisable(GL_BLEND);
  glFinish();
  profStart(&t);
  for(i=0;i<100;i++)
  {
    glRasterPos2i(300,300);
    glCopyPixels(200,200,294,38,GL_COLOR);
    //glFinish();
  }
  profReport(&t,"glCopyPixels 294x38");
  //printf("BenchCP ret\n");
}
#endif

QScroller::QScroller(QCanvas *icv,QRect *r)
{
  //QWidget *wg;
  curX=curY=0;
  cv=icv;
  // Scroll area
  if(r==0)
  { //wg=cv->GetWidget();
    area.x=0; area.y=0;
    //area.wid=wg->GetWidth(); area.hgt=wg->GetHeight();
    area.wid=768; area.hgt=576;
  } else
  { area.x=r->x; area.y=r->y;
    area.wid=r->wid; area.hgt=r->hgt;
  }
  maxGel=MAX_GEL;
  gel=(QBob**)qcalloc(sizeof(QBob*)*maxGel);
  gels=0;
}
QScroller::~QScroller()
{
  if(gel)qfree(gel);
}

QBob *QScroller::GetGel(int index)
{
  if(index<0||index>=gels)
    return 0;
  return gel[index];
}

bool QScroller::Add(QBob *ngel)
{
  if(gels>=maxGel)
  { qwarn("QScroller::Add: no more space for new gels\n");
    return FALSE;
  }
  gel[gels]=ngel;
  gels++;
  if(!strcmp(ngel->ClassName(),"text"))
  { // Burn the alpha channel onto black, so we can avoid blending
    QFilterBlendBurn(ngel->GetSourceBitMap());
  }
  return TRUE;
}

void QScroller::ScrollGel(QBob *gel,int vx,int vy)
{ QRect r,rArea;
  proftime_t t;
  char buf[80];
  int sx,sy,swid,shgt,dx,dy;	// CopyPixels

 //BenchCP();
  r.x=gel->x->GetCurrent(); //r.x=100;
  r.y=gel->y->GetCurrent(); //r.y=600;
  //printf("org x/y=%d,%d\n",gel->x->GetCurrent(),gel->y->GetCurrent());
  r.wid=gel->GetWidth();
  r.hgt=gel->GetHeight();

#ifdef ONE_STEP
  goto do_new;
#endif

  //r.y-=curY;
  //printf("gel should now be at virtual %d,%d\n",r.x,r.y);
  sx=r.x;
#ifdef SAMEBUF
  sy=r.y-curY+2*vy;
#else
  sy=r.y-curY+vy;
#endif
  dx=r.x; dy=r.y-curY;
  swid=r.wid;
  shgt=r.hgt;

  // Clip to viewport
#ifdef SAMEBUF
  if(sy<area.y+2*vy)
  { shgt=shgt-(area.y-sy)-2*vy;
    sy=area.y+2*vy;
    dy=area.y;
  }
#else
  if(sy<area.y+vy)
  { shgt=shgt-(area.y-sy)-vy;
    sy=area.y+vy;
    dy=area.y;
  }
#endif
  if(sy+shgt>area.y+area.hgt)
  { shgt=area.y+area.hgt-sy;
  }

  if(swid<=0||shgt<=0)goto skip;
  //printf("  copy %d,%d %dx%d to %d,%d\n",sx,sy,swid,shgt,dx,dy);
//#define OPTIMIZE_COPYPIXELS
#ifdef OPTIMIZE_COPYPIXELS
  // Optimized CopyPixels
  sprintf(buf,"glCopyPixels sized %dx%d",swid,shgt); glFinish(); profStart(&t);
  dy=area.hgt-shgt-dy;
  glRasterPos2i(dx,dy);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,area.wid);
  glPixelStorei(GL_PACK_ROW_LENGTH,area.wid);

///*
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_ROWS,0);
//*/
 
  glDisable(GL_DITHER);
  glDisable(GL_BLEND);
  //glFinish();
  //profStart(&t);
  sy=area.hgt-shgt-sy;			// Reverse Y
  glCopyPixels(sx,sy,swid,shgt,GL_COLOR);
  //glFinish();
  //profReport(&t,buf);
  //BenchCP();		// Slow!
  //printf("blend? %d\n",glIsEnabled(GL_BLEND));
#else
  cv->CopyPixels(sx,sy,swid,shgt,dx,dy);
#endif

 do_new:
  // Add new part
#ifdef SAMEBUF
  sx=0;
  sy=area.y+area.hgt-r.y+curY-2*vy;
  swid=r.wid; shgt=vy*2;
  dx=r.x;
  dy=area.y+area.hgt-2*vy;
#else
  sx=0;
  sy=area.y+area.hgt-r.y+curY-vy;
  swid=r.wid; shgt=vy;
  dx=r.x;
  dy=area.y+area.hgt-vy;
#endif

  // Clip to stay inside bitmap
  if(shgt+sy>r.hgt)
  { shgt=r.hgt-sy;
  }
  // Clip to scroll area
  if(sy>0&&sy<r.hgt&&shgt>0)
  { 
    //cv->Enable(QCANVASF_BLEND);
    //printf("  blit new %d,%d %dx%d, dst %d,%d\n",sx,sy,swid,shgt,dx,dy);
    cv->Blit(gel->GetSourceBitMap(),dx,dy,swid,shgt,sx,sy);
  }
  //sginap(10);
 skip:
  //lastY=r.y;
  ;
}
void QScroller::Scroll(int vx,int vy)
{ proftime_t t;
  int i;
  curX+=vx;
  curY+=vy;
#ifdef OBSOLETE
 //BenchCP();
  BenchCP("QScroller::Scroll1");
  glReadBuffer(GL_FRONT);		// This make it slow?!
  BenchCP("QScroller::Scroll2");
  glDrawBuffer(GL_BACK);
  BenchCP("QScroller::Scroll3");
  glDisable(GL_BLEND);
  BenchCP("QScroller::Scroll4");
  glDisable(GL_DITHER);
  BenchCP("QScroller::Scroll5");
#endif

  glFinish(); profStart(&t);
//#define DO_CLEAR
#ifdef DO_CLEAR
  // You might not clear, but then the bobs must clear after them
  //glClearColor(.2,.5,.8,0);
  //glClearColor(0,0,0,0);
  glClearColor(0,0,1,0);
  glClear(GL_COLOR_BUFFER_BIT);		// So blindingly fast
#endif
 //BenchCP();
#define USE_FRONT2BACK
#ifdef USE_FRONT2BACK
  // Copying from front to back is slow on Indy/OpenGL (fast with GL!)
  glReadBuffer(GL_FRONT);
#else
  glReadBuffer(GL_BACK);
#endif
#ifdef ONE_STEP
  // One big step; far less overhead! (?)
  glReadBuffer(GL_FRONT);
  //glReadBuffer(GL_BACK);		// faster on Indy
  //glReadBuffer(GL_BACK);
  glDisable(GL_BLEND);
  glDisable(GL_DITHER);
  cv->CopyPixels(area.x,area.y+vy,area.wid,area.hgt-vy,area.x,area.y);
  //cv->CopyPixels(area.x,area.y+vy*2,area.wid,area.hgt-vy*2,area.x,area.y);
  //glFinish(); profReport(&t,"PAL scrolled");
#endif
  //return;
  //// Copy Pixels
  //cv->Disable(QCANVASF_BLEND);
  cv->Blend(FALSE);
  //glFinish();
  //BenchCP("QScroller::Scroll9");
  for(i=0;i<gels;i++)
    ScrollGel(gel[i],vx,vy);
  glFinish(); profReport(&t,"gels scrolled");
  glReadBuffer(GL_BACK);
  //glFinish();
 //sginap(70);
}

//#define TEST
#ifdef TEST

//#include <qlib/qgelwg.h>
#include <qlib/shell.h>

// From qtest
QShell *sh;
//extern QGelWidget *gw;
extern QCanvas *cv;
extern QFont *font;
extern QImage *img;

void scrollDemo()
// For qtest
{ QScroller *scroller;
  QText *text[10];
  QBob  *bob[10];
  int i;
  QRect r;
  QImage *imgEye=img;

  sh=app->GetBC();
  printf("scrollDemo\n");
  //gw->Select();
  r.x=0; r.y=0; r.wid=768; r.hgt=576;
  scroller=new QScroller(cv,&r);
  printf("  1\n");
  text[0]=new QText(cv,font,"Scroller doodle doo",QGELF_AA);
  text[0]->SetText("Scrollery doodle doo",QBM_AA_ALG_9POINT);
  //text[0]=txt2;
  printf("  2\n");
  text[0]->x->Set(100);
  text[0]->y->Set(600);
  printf("  3\n");
  scroller->Add(text[0]);
  text[1]=new QText(cv,font,"",QGELF_AA);
  text[1]->SetText("A MarketGraph Product",QBM_AA_ALG_9POINT);
  scroller->Add(text[1]);
  text[1]->x->Set(200);
  text[1]->y->Set(710);
  bob[0]=new QBob(imgEye,0,0,400,100);
  scroller->Add(bob[0]);
  bob[0]->x->Set(130);
  bob[0]->y->Set(760);

  printf("  4\n");
  for(i=0;i<1200;i++)
  { scroller->Scroll(0,1);
    sh->Swap();
  }
  printf("  5\n");
  //delete text;
  //delete scroller;
}

#endif

