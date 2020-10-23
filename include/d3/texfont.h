// d3/texfont.h - fonts in a texture

#ifndef __D3_TEXFONT_H
#define __D3_TEXFONT_H

#include <d3/texture.h>
#include <qlib/color.h>
#include <qlib/font.h>

struct DTCharData
{
  int   x,y,wid,hgt;            // Integer source location
  float sx,sy,ex,ey;		// Texture coordinates
};

class DTexFont
// Texture map object that creates text texture maps
{
 protected:
  int minChar,maxChar;		// Characters available
  DTCharData *charData;
  DTexture   *tex;		// Texture storing the font
  dfloat      scaleX,scaleY;

 public:
  DTexFont(cstring fname=0);
  ~DTexFont();

  // Attribs
  DTexture   *GetTexture(){ return tex; }
  DTCharData *GetCharData(){ return charData; }
  int         GetMinChar(){ return minChar; }
  int         GetMaxChar(){ return maxChar; }

  void Destroy();

  bool Load(cstring fname);
  int  GetWidth(cstring s) const;
  int  GetHeight(cstring s) const;
  void SetScale(dfloat x,dfloat y);
  int  Paint(cstring s,int x,int y);

  // Font creation
  bool Create(QFont *font,int texWid,int texHgt,
    int minChar,int maxChar,cstring outName,int aaFactor=1);
};

#endif

