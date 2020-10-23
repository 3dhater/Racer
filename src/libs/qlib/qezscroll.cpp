/*
 * QEZScroller - Easy scrolling
 * 24-04-97: Created!
 * (C) MG/RVG
 */

#include <qlib/ezscroll.h>

QEZScroller::QEZScroller(QCanvas *cv,QRect *r)
  : QScroller(cv,r)
{
  curFont=0;
  curAlign=CENTRE;
  flags=0;
  curX=0;
  curY=cv->GetDrawable()->GetHeight();
  spacingX=0;
  spacingY=20;
}
QEZScroller::~QEZScroller()
{ int i,n;
  FreeFont();
  n=GetGelCount();
  for(i=0;i<n;i++)
  { delete GetGel(i);
  }
}

void QEZScroller::FreeFont()
{
  if(flags&OWN_FONT)
    delete curFont;
  flags&=~OWN_FONT;
  curFont=0;
}

void QEZScroller::SetAlignment(int align)
{ curAlign=align;
}
void QEZScroller::SetFont(QFont *nfont)
{ FreeFont();
  curFont=nfont;
}
void QEZScroller::SetFont(string fname,int size)
{ QDisplay *dsp;
  FreeFont();
  dsp=new QDisplay(0);
  curFont=new QFont(fname,size,QFONT_SLANT_ROMAN);
  delete dsp;
  flags|=OWN_FONT;
}

//
// Adding stuff
//
void QEZScroller::AddText(string s)
{ QText *t;
  if(!curFont)
  { qwarn("QEZScroller: no font set before first text");
    return;
  }
  t=new QText(cv,curFont,"",QGELF_AA);
  t->SetText(s,QBM_AA_ALG_9POINT);
  switch(curAlign)
  { case LEFT:
      t->x->Set(area.x); break;
    case RIGHT:
      t->x->Set(area.x+area.wid-t->GetWidth()); break;
    case CENTRE:
      t->x->Set(area.x+(area.wid-t->GetWidth())/2); break;
    default:
      t->x->Set(0); break;
  }
  t->y->Set(curY);
  curY+=t->GetHeight()+spacingY;
  if(!Add(t))
    delete t;
}
/*
void QEZScroller::AddImage(string fname)
{ QImage *img;
  QBob *bob;
  img=new QImage(fname);
  bob=new QBob(img);
  if(!Add(bob))
    delete bob;
}
*/

//#define TEST
#ifdef TEST

//#include <qlib/qgelwg.h>
#include <qlib/shell.h>

//extern QGelWidget *gw;
QShell *sh;
extern QCanvas *cv;
extern QFont *font;
//extern QImage *imgBalk;

void ezscrollDemo()
{
  QEZScroller *es;
  int i,vy=1;
  QTextSetMargins(2,3*vy);		// X for anti-aliasing, Y for autoclear

  sh=app->GetBC();

  es=new QEZScroller(cv);
  es->SetFont(font);
  es->AddText("This is a demo yg");
//#ifdef NOTDEF
  es->AddText("of what you can do");
  es->AddText("for your Indy");
  //es->AddImage("/usr/people/rvg/data/bobpics/mglogo.rgb");
  es->SetAlignment(QEZScroller::LEFT);
  es->AddText("This is left");
  es->SetAlignment(QEZScroller::LEFT);
  es->AddText("This is right");
  es->AddText("This is a demo");
  es->AddText("This is a demo");
//#endif
  for(i=0;i<1200;i++)
  { es->Scroll(0,vy);
    sh->Swap();
  }
  //delete es;
}

#endif

