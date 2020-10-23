// raceru/button.h

#ifndef __RACERU_BUTTON_H
#define __RACERU_BUTTON_H

#include <raceru/window.h>
#include <qlib/button.h>
#include <d3/texfont.h>

class RButton : public QButton
{
 public:
  string ClassName(){ return "rbutton"; }

  // Attribs
  DTexFont *tfont;
  QColor   *colNormal,*colHilite,*colEdge;
  // Texture (if a custom image is used)
  DTexture *tex;
  QRect rDisarmed,rArmed;        // The texture source areas

  // Animation
  RAnimTimer *aBackdrop,         // Backdrop rectangle
             *aText,             // Text walking in
             *aHilite;           // OnMouseOver
 public:
  RButton(QWindow *parent,QRect *pos=0,cstring text=0,DTexFont *font=0);
  ~RButton();

  // Attribs
  DTexture *GetTexture(){ return tex; }
  void      SetTexture(DTexture *tex,QRect *rDisarmed=0,QRect *rArmed=0);

  void    Paint(QRect *r=0);

  bool EvEnter();
  bool EvExit();
  bool EvMotionNotify(int x,int y);

  // Animation
  void AnimIn(int t,int delay=0);
};

#endif
