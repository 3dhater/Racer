// qlib/qbitmap.h
// 28-02-99: Now derived from QDrawable

#ifndef __QLIB_QBITMAP_H
#define __QLIB_QBITMAP_H

#include <qlib/color.h>
#include <qlib/drawable.h>

#define QBM_MAXDEPTH	32	// Max #planes

#define QBMF_FAST	1	// Store in fast memory
#define QBMF_USERALLOC	2	// Memory allocated by user, not the class

// Anti-alias algorithm
#define QBM_AA_ALG_LINEAR	0	// 3x3: n/9th alpha (alpha=0 or 255)
#define QBM_AA_ALG_AVG_ALPHA	1	// 3x3: new_alpha=sum(alpha)/9
#define QBM_AA_ALG_4POINT	2	// 1/4th pixel shift and avg box 4x4
#define QBM_AA_ALG_9POINT	3
#define QBM_AA_ALG_16POINT	4

// File formats for writing (reading is done in class QImage)
#define QBMFORMAT_RGB		0
#define QBMFORMAT_TGA		1	// 32-bit Targa with alpha
#define QBMFORMAT_TGAGRAY	2	// 8-bit Targa grayscale

class QBitMap : public QDrawable
{
 public:
  string className() const { return "bitmap"; }

  enum Channels
  { CHANNEL_RED=0,
    CHANNEL_GREEN,
    CHANNEL_BLUE,
    CHANNEL_ALPHA
  };

 protected:
  int flags;
  int dep;
  int wid,hgt;
  char *mem;		// Actual buffer
  int   memSize;	// Size of buffer

 public:
  QBitMap(int dep=0,int wid=0,int hgt=0,int flags=0);
  QBitMap(QBitMap *bm);
  ~QBitMap();

  // Attributes
  int GetWidth();
  int GetHeight();
  int GetDepth(){ return dep; }
  int GetFlags(){ return flags; }
  char *GetBuffer(){ return mem; }
  int   GetBufferSize(){ return memSize; }
  char *GetAddr(int x,int y);
  int   GetModulo(){ return wid*4; }      // Bitmap width in bytes

  // Memory
  bool Alloc(int dep,int wid,int hgt,int flags=0,void *mem=0);
  void Free();
  void Mask(int plane=0);
  void Clear(int plane=-1);
  QBitMap *CreateClone(bool copyImage);

  // Bit-blitting operations
  void CopyBitsFrom(QBitMap *bm);	// Direct copy (full bitmap)
  void CopyPixelsFrom(QBitMap *sbm,int sx,int sy,int wid,int hgt,
    int dx=0,int dy=0);

  // Channel operations
  void CopyChannel(QBitMap *sbm,int srcChannel,int dstChannel=-1,
    int sx=0,int sy=0,int wid=-1,int hgt=-1,int dx=0,int dy=0);
  void InvertChannel(int channel,
    int sx=0,int sy=0,int wid=-1,int hgt=-1);

  // Pixel operations
  void GetColorAt(int x,int y,QColor *c);
  void SetColorAt(int x,int y,QColor *c);

  // Format conversions
  void ConvertToABGR();			// For Cosmo and others (DM_IMAGE_RGBX)

  // Rotations
  void Rotate90(QRect *s,QBitMap *dbm,QRect *d);
  void Rotate180(QRect *s,QBitMap *dbm,QRect *d);
  void Rotate270(QRect *s,QBitMap *dbm,QRect *d);
  void Rotate(int angle,QRect *s,QBitMap *dbm,QRect *d);

  // Nice operations (highlevel)
  void CreateTransparency(QColor *transpCol);
  void CreateAA(int alg=QBM_AA_ALG_9POINT);
  void SetAlpha(int v);			// Set entire alpha channel
  int  KeyPurple();			// Often used on BMP images
  void Lut(int minV=20,bool doAlpha=FALSE);

  // File operations
  bool WriteRGB(cstring name,int format);
  bool WriteTGA(cstring name,int format);
  bool Write(cstring name,int format=QBMFORMAT_RGB);
};

// Operations on bitmaps that are NOT part of the class
// (to reduce linkage size)
void QBM_BlendBurn(QBitMap *bm);

#endif

