/*
 * RListBox - Racer GUI listbox
 * 14-11-01: Created!
 * NOTES:
 * - Listbox, based on QLib's QListBox
 * (c) Dolphinity/RvG
 */

#include <raceru/listbox.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/flat.h>
DEBUG_ENABLE

// Cloned from qlib/qlistbox.cpp
#define BORDER_WID 4

RListBox::RListBox(QWindow *parent,QRect *pos,int flags,DTexFont *_tfont)
  : QListBox(parent,pos,flags)
{
  if(!_tfont)
  {
    qerr("RListBox: can't pass empty font");
    // Prepare to crash...
  }
  tfont=_tfont;

  // Rethink #items visible
  int w,h;
  GetItemSize(&w,&h);
  if(!h)itemsVisible=0;
  else  itemsVisible=(pos->hgt-2*BORDER_WID)/h;

#ifdef ND_WHITE
  colNormal=new QColor(255,255,255,95);
  colHilite=new QColor(155,155,255,120);
  colEdge=new QColor(155,155,255,0);
#else
  colNormal=new QColor(0,0,0,95);
  colHilite=new QColor(155,155,255,120);
  colEdge=new QColor(155,155,255,0);
#endif

  cv->SetMode(QCanvas::DOUBLEBUF);
  // Avoid Y flipping in the canvas
  cv->Enable(QCanvas::NO_FLIP);
  // Cancel offset installed by QWindow; we're drawing in 3D
  cv->SetOffset(0,0);

  aList=new RAnimTimer(itemsVisible);
  aRest=new RAnimTimer(10);
}
RListBox::~RListBox()
{
qdbg("RListBox dtor\n");
#ifdef OBS
  QDELETE(colNormal);
  QDELETE(colHilite);
  QDELETE(colEdge);
#endif
  QDELETE(aList);
  QDELETE(aRest);
}

/**********
* Attribs *
**********/
#ifdef OBS
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
#endif

void RListBox::GetItemSize(int *wid,int *hgt)
// Returns size of an item in 'wid' and 'hgt'
{
  QRect r;
  GetPos(&r);
  // Mind the scrollbars
  if(barVert)r.wid-=barVert->GetWidth();
  *wid=r.wid-2*BORDER_WID;
  *hgt=tfont->GetHeight(" ");
}

#ifdef ND
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
#endif

void RListBox::PaintItem(int n)
// Paint only item 'n'
// Override this function if you do your specialized listbox
// Default behavior is to paint the associated string
{
  QRect r;
  QLBItemInfo *ii;

  if(n<0||n>=items)return;

  ii=&itemInfo[n];
  //GetItemRect(n,&r);
  int relItem;
  GetPos(&r);
  GetItemSize(&r.wid,&r.hgt);
  relItem=n-firstItem;
  r.x=r.x+BORDER_WID;
  r.y=r.y+BORDER_WID+relItem*r.hgt;
  RScaleRect(&r,&r);

  cv->Map2Dto3D(&r.x,&r.y);
//qdbg("item '%d'='%s'\n",n,itemInfo[n].text);
//qdbg("  @%d,%d %dx%d\n",r.x,r.y,r.wid,r.hgt);
  if(ii->flags&QLBItemInfo::SELECTED)
  { //cv->SetColor(colHilite);
    cv->Rectfill(&r,colHilite,colEdge,colEdge,colHilite);
  } else
  {// cv->SetColor(colNormal);
    //cv->Rectfill(&r);
  }
  //cv->SetFont(font);
#ifdef OBS
  if(ii->flags&QLBItemInfo::SELECTED)
    cv->SetColor(QApp::PX_WHITE);
  else
    cv->SetColor(QApp::PX_BLACK);
  //cv->Text(itemInfo[n].text,r.x+itemInfo[n].indent,r.y);
#endif
  tfont->Paint(itemInfo[n].text,r.x+itemInfo[n].indent,
    r.y-tfont->GetHeight("."));
}            

void RListBox::Paint(QRect *r)
{
  int i,w,h;
  int bvWid,bhHgt;
//qdbg("qlb:Paint\n");
  if(!IsVisible())return;

  QRect pos;
  GetPos(&pos);
  cv->Map2Dto3D(&pos.x,&pos.y);

#ifdef OBS
  cv->Blend(TRUE);
  cv->SetColor(255,0,0,80);
  //cv->SetColor(QApp::PX_WHITE);
  pos.wid=150; pos.hgt=200;
  cv->Rectfill(&pos);
//return;
#endif

  cv->Blend(TRUE);

  //if(barVert)pos.wid-=barVert->GetWidth()-2;
  // Take into account any scrollbars
  if(barVert)bvWid=barVert->GetWidth();
  else       bvWid=0;
  bhHgt=0;
  //cv->Inline(pos.x,pos.y,pos.wid,pos.hgt);
  cv->SetColor(colNormal);
  if(items<firstItem+itemsVisible)
  { // Clear more than normal because items are missing
    cv->Rectfill(pos.x+2,pos.y+2,pos.wid-barVert->GetWidth()-2*2,pos.hgt-2*2);
  }
  // Take care when drawing outline to minimize flashing
  cv->Rectfill(pos.x+2,pos.y+2,pos.wid-bvWid-2*2,2);
  cv->Rectfill(pos.x+2,pos.y+2,2,pos.hgt-2*2);
  cv->Rectfill(pos.x+pos.wid-bvWid-2*2,pos.y+2,2,pos.hgt-bhHgt-2*2);
  GetItemSize(&w,&h);
  cv->Rectfill(pos.x+2,pos.y+pos.hgt-bhHgt-2-h,pos.wid-bvWid-2*2,h);
  for(i=firstItem;i<firstItem+itemsVisible&&i<items;i++)
  {
    PaintItem(i);
  }

  //if(IsFocus())
    //cv->Rectfill(&r,colNormal,colNormal,colNormal,colNormal);

#ifdef ND_FUTURE
  // Outline
  r.wid=w;
  cv->SetColor(255,255,255);
  cv->Rectangle(r.x,r.y+1,r.wid,1);
  cv->Rectangle(r.x,r.y+0,1,r.hgt);
  cv->Rectangle(r.x,r.y-r.hgt+1,r.wid,1);
  cv->Rectangle(r.x+r.wid-1,r.y+0,1,r.hgt);
#endif

//qdbg("RListBox:Paint() RET\n");
}

/************
* Animation *
************/
void RListBox::AnimIn(int t,int delay)
// Make the button appear animated (no move)
{
#ifdef ND
  QRect r;

  GetPos(&r);
  aBackdrop->Trigger(r.wid,t,delay);
  // Text a little later
  aText->Trigger(strlen(text),t,delay+t/2);
#endif
}

#ifdef FUTURE
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
#endif
