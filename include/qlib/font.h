// qlib/qfont.h - font

#ifndef __QLIB_QFONT_H
#define __QLIB_QFONT_H

#include <qlib/app.h>
#include <qlib/display.h>
#include <X11/Xlib.h>

#define QFONT_WEIGHT_DONTCARE	0
#define QFONT_WEIGHT_MEDIUM	1
#define QFONT_WEIGHT_BOLD	2
#define QFONT_WEIGHT_DEMIBOLD	3		// DEMI
#define QFONT_WEIGHT_DEMI	3		// DEMI
#define QFONT_WEIGHT_REGULAR	4

#define QFONT_SLANT_DONTCARE	0
#define QFONT_SLANT_ROMAN	1
#define QFONT_SLANT_ITALIC	2
#define QFONT_SLANT_OBLIQUE	3

#define QFONT_WIDTH_DONTCARE	0
#define QFONT_WIDTH_NORMAL	1
#define QFONT_WIDTH_NARROW	2
#define QFONT_WIDTH_CONDENSED	3

struct QFontInternal;

class QFont : public QObject
{
 private:
  QFontInternal *internal;
#ifdef WIN32
  HFONT  hFont;
  TEXTMETRIC tm;		// Info about font
#endif
  // Creation info
  string fname;			// Family name
  int    size,slant,weight,width,resX,resY;
  int    listBase;		// OpenGL lists

 public:
  QFont(cstring fname,int size,int slant=QFONT_SLANT_ROMAN,
    int weight=QFONT_WEIGHT_DONTCARE,int width=QFONT_WIDTH_DONTCARE);
  ~QFont();

  // Pre-creation
  void SetResX(int n);
  void SetResY(int n);
  int  GetListBase();

  bool Create();

  // Font information
#ifdef WIN32
  HFONT GetHFONT(){ return hFont; }
#endif
  Font GetXFont();
  int  GetAscent();
  int  GetDescent();
  int  GetHeight(cstring s=0);

  int  GetWidth(ubyte c);
  int  GetWidth(cstring s,int len=-1);
  int  GetLeftBearing(ubyte c);
  int  GetRightBearing(ubyte c);
};

class QFontList : public QObject
{
 protected:
  char **xlist;			// X11
  int    count;			// X font count
  char **family;		// Collected family names
  int    familyCount;
  char **foundry;		// Collected foundry names
  int    foundryCount;
 public:
  QFontList();
  ~QFontList();

  int     GetFamilyCount(){ return familyCount; }
  cstring GetFamily(int n);
  int     GetFoundryCount(){ return foundryCount; }
  cstring GetFoundry(int n);
};

#endif
