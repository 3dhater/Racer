/*
 * RButton - Racer GUI button
 * 04-11-01: Created!
 * NOTES:
 * - An attempt to get a 3D polygon-type button, instead of the regular
 * QLib buttons, but in the same framework to avoid coding a 2nd GUI.
 * (c) Dolphinity/RvG
 */

#include <raceru/button.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

RButton::RButton(QWindow *parent,QRect *pos,cstring text,DTexFont *_tfont)
  : QButton(parent,pos,text)
{
  if(!_tfont)
  {
    qerr("RButton: can't pass empty font");
    // Prepare to crash...
  }
  tfont=_tfont;
  tex=0;

  colNormal=new QColor(255,255,255,95);
  colHilite=new QColor(255,155,155,120);
  colEdge=new QColor(155,155,255,0);

  cv->SetMode(QCanvas::DOUBLEBUF);
  // Avoid Y flipping in the canvas
  cv->Enable(QCanvas::NO_FLIP);
  // Cancel offset installed by QWindow; we're drawing in 3D
  cv->SetOffset(0,0);

  aBackdrop=new RAnimTimer(pos->wid);
  aText=new RAnimTimer(strlen(text));
  aHilite=new RAnimTimer(0);
}
RButton::~RButton()
{
//qdbg("RButton dtor\n");
  QDELETE(colNormal);
  QDELETE(colHilite);
  QDELETE(colEdge);
  QDELETE(aBackdrop);
  QDELETE(aText);
  QDELETE(aHilite);
}

/**********
* Attribs *
**********/
void RButton::SetTexture(DTexture *_tex,QRect *_rDisarmed,QRect *_rArmed)
// Define a texture to use instead of the default drawing.
// If 'rDisarmed' is 0, then the whole texture is used.
// Same for 'rArmed'.
{
  QASSERT_V(_tex);

  tex=_tex;
  // Copy rectangles
  if(_rDisarmed==0)
  {
    // Use full texture
    rDisarmed.x=rDisarmed.y=0;
    rDisarmed.wid=tex->GetWidth();
    rDisarmed.hgt=tex->GetHeight();
  } else
  {
    // Explicit area
    rDisarmed.x=_rDisarmed->x;
    rDisarmed.y=_rDisarmed->y;
    rDisarmed.wid=_rDisarmed->wid;
    rDisarmed.hgt=_rDisarmed->hgt;
  }
  if(_rArmed==0)
  {
    // Use full texture
    rArmed.x=rArmed.y=0;
    rArmed.wid=tex->GetWidth();
    rArmed.hgt=tex->GetHeight();
  } else
  {
    // Explicit area
    rArmed.x=_rArmed->x;
    rArmed.y=_rArmed->y;
    rArmed.wid=_rArmed->wid;
    rArmed.hgt=_rArmed->hgt;
  }
}

static void Rect2TC(QRect *r,float *v,DTexture *tex)
// Convert a rectangle to tex coordinates
{
  v[0]=float(r->x)/float(tex->GetWidth());
  v[1]=1.0-float(r->y)/float(tex->GetHeight());
  v[2]=float(r->x+r->wid)/float(tex->GetWidth());
  v[3]=1.0-float(r->y+r->hgt)/float(tex->GetHeight());
//qdbg("Rect2TC: in: %d,%d %dx%d, out %.2f,%.2f %.2f,%.2f\n",
//r->x,r->y,r->wid,r->hgt,v[0],v[1],v[2],v[3]);
}

void RButton::Paint(QRect *rr)
{
  QRect r;
  char buf[256];
  unsigned int  n;
  int tx,ty;                   // Text position
  float tc[4];                 // Texture coordinates
  float w;

  GetPos(&r);
  cv->Map2Dto3D(&r.x,&r.y);

  if(tex)
  {
    // Use explicit texture instead of calculated button
    tex->Select();
    if(state==ARMED)Rect2TC(&rArmed,tc,tex);
    else            Rect2TC(&rDisarmed,tc,tex);
    //Rect2TC(&rArmed,tc,tex);
//if(state==ARMED)qdbg("ARMED\n");
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
      glTexCoord2f(tc[0],tc[1]);
      glVertex2f(r.x,r.y);
      glTexCoord2f(tc[2],tc[1]);
      glVertex2f(r.x+r.wid,r.y);
      glTexCoord2f(tc[2],tc[3]);
      glVertex2f(r.x+r.wid,r.y-r.hgt);
      glTexCoord2f(tc[0],tc[3]);
      glVertex2f(r.x,r.y-r.hgt);
    glEnd();
  } else
  {
    // Hardcoded draw
//qdbg("RButton:Paint() at %d,%d\n",r.x,r.y);
  w=r.wid;
  r.wid=aBackdrop->GetValue();
  cv->Blend(TRUE);
  if(state==ARMED)
    cv->Rectfill(&r,colHilite,colEdge,colEdge,colHilite);
  else
    cv->Rectfill(&r,colNormal,colEdge,colEdge,colNormal);

  if(IsFocus())
    cv->Rectfill(&r,colNormal,colNormal,colNormal,colNormal);

  // Outline
  r.wid=w;
  cv->SetColor(255,255,255);
  cv->Rectangle(r.x,r.y+1,r.wid,1);
  cv->Rectangle(r.x,r.y+0,1,r.hgt);
  cv->Rectangle(r.x,r.y-r.hgt+1,r.wid,1);
  cv->Rectangle(r.x+r.wid-1,r.y+0,1,r.hgt);

  // Text (a bit weird, as texts seem flipped wrt the rectfills)
  tx=r.x+(r.wid-tfont->GetWidth(text))/2;
  ty=r.y-tfont->GetHeight(text)-(r.hgt-tfont->GetHeight(text))/2;
  if(aText->IsFinished())
  { tfont->Paint(text,tx,ty);
  } else
  { // Create left(text,n) string
    n=aText->GetValue();
    if(n>sizeof(buf)-1)
      n=sizeof(buf)-1;
    strncpy(buf,text,n);
    buf[n]=0;
    tfont->Paint(buf,tx,ty);
  }
  }

//qdbg("RButton:Paint() RET\n");
}

/************
* Animation *
************/
void RButton::AnimIn(int t,int delay)
// Make the button appear animated (no move)
{
  QRect r;

  GetPos(&r);
  aBackdrop->Trigger(r.wid,t,delay);
  // Text a little later
  aText->Trigger(strlen(text),t,delay+t/2);
}

bool RButton::EvEnter()
{
qdbg("enter\n");
  return TRUE;
}

bool RButton::EvExit()
{
qdbg("leave\n");
  return TRUE;
}
bool RButton::EvMotionNotify(int x,int y)
{
//qdbg("motion %d,%d\n",x,y);
  return TRUE;
}
