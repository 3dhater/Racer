/*
 * DTexFont - texture fonts
 * 18-02-2001: Created! (12:47:36)
 * NOTES:
 * - OGL: Uses a bit of GLU in Create() (scaling an image)
 * - OGL: Lots of OpenGL in Paint()
 * BENEFITS WITH RESPECT TO NORMAL QFONTS:
 * - It anti-aliases the font for free once generated.
 * - It is textured, so even a bit more anti-aliased for free
 * - It is placable in full 3D; perspective text, rotated/scaled etc.
 * - On PC's, it's a LOT faster than glBitMap() (which QFont uses)
 * THE CONS:
 * - It has to be pregenerated; it's not THAT slow, but perhaps a bit too
 * slow to create on the fly at each program start.
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <GL/glu.h>
#include <d3/texfont.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DTexFont"

DTexFont::DTexFont(cstring fname)
// Create a font from a file
// If 'fname'==0, an empty font will be  created
// (create a font using Load() or Create()
{
  minChar=maxChar=0;
  charData=0;
  tex=0;
  scaleX=scaleY=1.0f;
  if(fname)
  {
    Load(fname);
  }
}
DTexFont::~DTexFont()
{
  Destroy();
}
void DTexFont::Destroy()
{
  QFREE(charData);
  QDELETE(tex);
}

/**********
* Attribs *
**********/
void DTexFont::SetScale(dfloat x,dfloat y)
// Set font scaling
{
  scaleX=x; scaleY=y;
}

/**********
* Loading *
**********/
bool DTexFont::Load(cstring fname)
{
  char buf[256];
  int  i;
  
  Destroy();
  
  // Load definition
  sprintf(buf,"%s.dtf",fname);
  QInfo info(buf);
  minChar=info.GetInt("general.minchar");
  maxChar=info.GetInt("general.maxchar");
  charData=(DTCharData*)qcalloc((maxChar-minChar+1)*sizeof(DTCharData));
  for(i=minChar;i<=maxChar;i++)
  {
    sprintf(buf,"chars.char%d.x",i);
    charData[i-minChar].x=info.GetInt(buf);
    sprintf(buf,"chars.char%d.y",i);
    charData[i-minChar].y=info.GetInt(buf);
    sprintf(buf,"chars.char%d.wid",i);
    charData[i-minChar].wid=info.GetInt(buf);
    sprintf(buf,"chars.char%d.hgt",i);
    charData[i-minChar].hgt=info.GetInt(buf);
  }
  
  // Load image
  sprintf(buf,"%s.tga",fname);
  QImage img(buf);
  if(img.IsRead())
  {
    DBitMapTexture *tbm;
    tbm=new DBitMapTexture(&img);
    tex=tbm;
    // Precalculate texture coordinates of each letter
    dfloat iwid,ihgt;
    dfloat fx,fy,fwid,fhgt;
    iwid=img.GetWidth();
    ihgt=img.GetHeight();
    for(i=minChar;i<=maxChar;i++)
    {
      fx=charData[i-minChar].x;
      fy=charData[i-minChar].y;
      fwid=charData[i-minChar].wid;
      fhgt=charData[i-minChar].hgt;
      charData[i-minChar].sx=fx/iwid;
      charData[i-minChar].sy=fy/ihgt;
      charData[i-minChar].ex=(fx+fwid-1.0f)/iwid;
      charData[i-minChar].ey=(fy+fhgt-1.0f)/ihgt;
    }
    // Reverse Y, and reverse start/end (image is flipped)
    for(i=minChar;i<=maxChar;i++)
    {
      float t;
      t=charData[i-minChar].sy;
      charData[i-minChar].sy=1.0f-charData[i-minChar].ey;
      charData[i-minChar].ey=1.0f-t;
    }
  } else return FALSE;
  
  return TRUE;
}

/***********
* Painting *
***********/
int DTexFont::GetWidth(cstring s) const
// Returns total length of 's'
// Takes into account any scaling
{
  int w=0,cw;
//qdbg("DTF:GetWidth(%s)\n",s);
  for(;*s;s++)
  {
    // Skip chars that are not in the font
    if(*s<minChar||*s>maxChar)continue;
    cw=charData[*s-minChar].wid;
    cw=(int)(((dfloat)cw)*scaleX);
//qdbg("DTF:GetWid: '%c'=> %d (scale %f) orgW=%d\n",*s,cw,scaleX,
//charData[*s-minChar].wid);
    w+=cw;
  }
//qdbg("  total width=%d\n",w);
  return w;
}
int DTexFont::GetHeight(cstring s) const
// Returns height of 's'
{
  return charData[0].hgt;
}

int DTexFont::Paint(cstring s,int x,int y)
// Paint a string on (x,y) (in the current drawing context)
// Note that you can use 2D and 3D here; it will be drawn as a series
// of polygons, so you can rotate/scale whatever just like a regular
// 3D model.
// Returns x-coordinate of character to be drawn next
{
  DBG_C("Paint")
  DBG_ARG_S(s)
  DBG_ARG_I(x)
  DBG_ARG_I(y)
  
  DTCharData *cd;
  int wid,hgt;

  // Check for presence of font
  if(!tex)return x;
  
#ifdef ND_ALWAYS_SET_OGL_STATE
  // Check for any drawing to take place
  if(!*s)return x;
  if(!s)return x;
#endif
  
  tex->Select();
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  // Usual blending
  //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  //glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_ONE_MINUS_SRC_ALPHA);
  // Blend func for fat texts (Nieuwsquiz)
  glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);

//qdbg("DTexFont:Paint(%s); x=%d\n",s,x);
  glBegin(GL_QUADS);
  for(;*s;s++)
  {
    // Skip chars that are not in the font
    if(*s<minChar||*s>maxChar)continue;
    cd=&charData[*s-minChar];
    // Scale the character size
    wid=cd->wid;
    hgt=cd->hgt;
    wid=(int)(((dfloat)wid)*scaleX);
    hgt=(int)(((dfloat)hgt)*scaleY);
    // Draw a textured quads, counterclockwise
    glTexCoord2f(cd->sx,cd->sy);
    glVertex2i(x,y);
    glTexCoord2f(cd->sx,cd->ey);
    glVertex2i(x,y+hgt);
    glTexCoord2f(cd->ex,cd->ey);
    glVertex2i(x+wid,y+hgt);
    glTexCoord2f(cd->ex,cd->sy);
    glVertex2i(x+wid,y);
    x+=wid;
  }
#ifdef OBS_WHOLE_FONT
  glTexCoord2f(0,0);
  glVertex2i(200,200);
  glTexCoord2f(0,1);
  glVertex2i(200,200+128);
  glTexCoord2f(1,1);
  glVertex2i(200+256,200+128);
  glTexCoord2f(1,0);
  glVertex2i(200+256,200);
#endif
  glEnd();
  
  // Restore state
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
//qdbg("  at end x=%d\n",x);
  return x;
}

/***********
* Creation *
***********/
bool DTexFont::Create(QFont *font,int texWid,int texHgt,
  int _minChar,int _maxChar,cstring outName,int aaFactor)
// Create a texture font from a regular font.
// 'aaFactor' is the anti-aliasing magnification factor
// 2 files will be generated;
//   <outname>.tga - the font texture data
//   <outname>.dft - the font definition file
// Assumptions: a Q_BC window is up and ready for use
{
  QBitMap *bmLetter,*bmFontLetter,*bmFont;
  int i;
  int dx,dy;              // Position of char in bitmap
  char buf[256];
  int  cWid,cHgt;         // Char size
  QCanvas *cvFont;        // Canvas of font bitmap
  
  Destroy();
  
  minChar=_minChar;
  maxChar=_maxChar;
  charData=(DTCharData*)qcalloc((maxChar-minChar+1)*sizeof(DTCharData));
  
  // Allocate a bitmap for the entire font
  bmFont=new QBitMap(32,texWid,texHgt);
  cvFont=new QCanvas(bmFont);
  cvFont->UseBM(bmFont);            // BUG in QCanvas so we must set it here!
  
  // Render all chars
  dx=dy=0;
  QCV->SetFont(font);
  for(i=minChar;i<=maxChar;i++)
  {
    buf[0]=i; buf[1]=0;
    
    // Get character size in original font
    cWid=font->GetWidth(buf[0]);
    cHgt=font->GetHeight();
    
    // Can we fit in the character on this line in the texture?
    if(dx+cWid/aaFactor>=texWid)
    {
      // Go to next line
      dx=0;
      dy+=cHgt/aaFactor;
      // Check for Y overflow
      if(dy+cHgt/aaFactor>texHgt)
      {
        qwarn("DTexFont:Create(); texture Y overflow");
        dy-=cHgt/aaFactor;
      }
    }
    
    // Render the letter
    QCV->Clear();
    QCV->Text(buf,0,0);
    bmLetter=new QBitMap(32,cWid,cHgt);
    QCV->ReadPixels(bmLetter /*,0,0,cWid,cHgt*/);
    // Show progress
    Q_BC->Swap();
    
    // Create aa version of the letter
    // Bug; allocation just enough space for the resized letter
    // results in crashes; probably overwritten bitmap pixel buffers.
    // However, this should in theory not be the case, so it's strange.
    //bmFontLetter=new QBitMap(32,cWid/aaFactor+1,cHgt/aaFactor+1);
    bmFontLetter=new QBitMap(32,cWid,cHgt);
    gluScaleImage(GL_RGBA,cWid,cHgt,GL_UNSIGNED_BYTE,bmLetter->GetBuffer(),
      cWid/aaFactor,cHgt/aaFactor,GL_UNSIGNED_BYTE,bmFontLetter->GetBuffer());
    
    // Copy aa-ed letter into font bitmap
    cvFont->Blit(bmFontLetter,dx,dy,cWid/aaFactor,cHgt/aaFactor,
      0,cHgt-cHgt/aaFactor);
    
    // Store char info
    charData[i-minChar].x=dx;
    charData[i-minChar].y=dy;
    charData[i-minChar].wid=cWid/aaFactor;
    charData[i-minChar].hgt=cHgt/aaFactor;
    
    // Find next character location in the font bitmap
    // Wrapping will be done for next character
    dx+=cWid/aaFactor;
    
    // Cleanup for this letter
    QDELETE(bmFontLetter);
    QDELETE(bmLetter);
  }
 
  // Create anti-aliasing alpha based on the grayness of the letter
  bmFont->CopyChannel(bmFont,QBitMap::CHANNEL_RED,QBitMap::CHANNEL_ALPHA);
  
  // Save the end bitmap
  sprintf(buf,"%s.tga",outName);
  bmFont->Write(buf,QBMFORMAT_TGA);
  
  // Save the definition file
  sprintf(buf,"%s.dtf",outName);
  QInfo info(buf);
  info.SetInt("general.minchar",minChar);
  info.SetInt("general.maxchar",maxChar);
  for(i=minChar;i<=maxChar;i++)
  {
    sprintf(buf,"chars.char%d.x",i);
    info.SetInt(buf,charData[i-minChar].x);
    sprintf(buf,"chars.char%d.y",i);
    info.SetInt(buf,charData[i-minChar].y);
    sprintf(buf,"chars.char%d.wid",i);
    info.SetInt(buf,charData[i-minChar].wid);
    sprintf(buf,"chars.char%d.hgt",i);
    info.SetInt(buf,charData[i-minChar].hgt);
  }
  info.Write();
  
  return TRUE;
}
