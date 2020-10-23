// d3/textmap.h - Text as a texturemap

#ifndef __D3_TEXTMAP_H
#define __D3_TEXTMAP_H

#include <d3/texture.h>
#include <qlib/color.h>
#include <qlib/font.h>

class DTextMap
// Texture map object that creates text texture maps
{
 public:
  enum Flags
  { WRAP=1,			// Wrap text lines
  };
  enum AlignFlags
  { LEFT=0,
    RIGHT=1,
    CENTER=2,
    TOP=0,
    BOTTOM=1
  };

 protected:
  // Style
  QFont      *font;
  QTextStyle *textStyle;
  int      alignH,alignV;	// enum AlignFlags
  int      lineSpacing;
  int      maxLines;		// Max #lines in text

  QBitMap *bm;                  // Resulting bitmap
  int      wid,hgt;             // Size of wanted bitmap
  int      curWid,curHgt;	// Size of current bitmap
  int      flags;		// enum Flags

  bool PaintFunc(string start,string end);
  void TextPaint(string s);
  void TextSplit(string s);

 public:
  DTextMap(QFont *font,int wid,int hgt);
  ~DTextMap();

  // Style
  QTextStyle *GetTextStyle(){ return textStyle; }
  void SetTextStyle(QTextStyle *style);
  void SetWrap(bool yn);
  void SetMaxLines(int lines);
  void SetAlign(int hAlign,int vAlign);
  int  GetLineSpacing(){ return lineSpacing; }
  void SetLineSpacing(int space);
  void SetSize(int wid,int hgt);

  // Attribs
  int      GetDesiredWidth(){ return wid; }
  int      GetDesiredHeight(){ return hgt; }
  int      GetActualWidth(){ return curWid; }
  int      GetActualHeight(){ return curHgt; }
  int      GetMaxLines(){ return maxLines; }
  QBitMap *GetBitMap(){ return bm; }
  void     GetRect(QRect *r);
  void     GetSize(QSize *size);	// Actual size

  void SetText(cstring s);
  void SetFont(QFont *font);
};

#endif

