// qlib/qimage.h

#ifndef __QLIB_QIMAGE_H
#define __QLIB_QIMAGE_H

#include <qlib/bitmap.h>

// File formats; SGI's .rgb, TrueVision .tga

// .RGB SGI file structures
#define RGB_IMAGIC		0732
#define RGB_TYPE_RLE		0x0100
#define RGB_TYPE_VERBATIM	0x0000
#define RGB_IS_RLE(type)	(((type)&0xFF00)==RGB_TYPE_RLE)
#define RGB_IS_VERBATIM(type)	(((type)&0xFF00)==RGB_TYPE_VERBATIM)
#define RGB_BPP(type)		((type)&0x00FF)

#ifndef WIN32

// Win32 uses non 32-bit alignment which
// leads to trouble on SGI MIPSpro 7.2.1
//#pragma pack(2)
typedef struct tagBITMAPFILEHEADER
{
  unsigned short bfType;
  long           bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  long           bfOffBits;
} BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER
{
  long  biSize;
  long  biWidth;
  long  biHeight;
  short biPlanes;
  short biBitCount;
  long  biCompression;
  long  biSizeImage;
  long  biXPelsPerMeter;
  long  biYPelsPerMeter;
  long  biClrUsed;
  long  biClrImportant;
} BITMAPINFOHEADER;
typedef struct tagRGBQUAD
{ char rgbBlue;
  char rgbGreen;
  char rgbRed;
  char rgbReserved;		// Alpha?
} RGBQUAD;
// Revert to default packing
//#pragma pack(0)

#endif

typedef struct tagSGIHeader
{ 
  ushort imagic;		// Magic RGB value; Big or Little-Endian
  ushort type;			// RLE/Verbatim and BytesPerPixel
  ushort dim;			// Dimensions of image (mostly 2)
  ushort xsize;
  ushort ysize;
  ushort zsize;
  ulong  min,max;
  ulong  wasteBytes;
  char   name[80];
  ulong  colormap;
  //char   pad[2];
} SGIHeader;                // 512 bytes

// .TGA Targa file structures (shorts use Intel order on disk)
typedef struct tagTGAHeader
{
  char  idLen;			// Length of ID chunk
  char  colorMapType;		// Colormap included?
  char  imageType;		// Type 2/3 most used, I guess
  // Color map specification
  short cmFirstEntry;		// Colormap offset
  short cmLength;		// #entries
  char  cmEntrySize;		// Bits per entry
  // Image specification
  short xOrg;			// X origin (hmm)
  short yOrg;
  short width,height;		// Size of picture
  char  pixDepth;		// Bits per pixel (incl. alpha bits)
  char  imgDesc;		// 76=0, 54=hor/vert dir, 3210=alpha bits
  // Rest is imageID, colorMap, pixel data (and extensions for 2.0)
} TGAHeader;
 
class QImage : public QBitMap
{
  string className() const { return "image"; }
 protected:
  string name;
  bool   doRev;			// Reverse byte order?
  int    err;			// Error

 protected:
  bool Info(cstring name,SGIHeader* hdr);
  bool Info(cstring name,TGAHeader* hdr);
  bool ReadRGB(cstring name);		// SGI .rgb
  bool ReadTGA(cstring name);		// TrueVision Targa .tga
  bool ReadIFL(cstring name);		// IFL supported formats
  bool ReadJPG(cstring name);		// Better than SGI JPEG support
  bool ReadBMP(cstring name);		// Win32 BMP (OS/2?)
  int  DetectType(cstring name);		// RGB/TGA?

 public:
  QImage(cstring name,int dep=0,int wid=0,int hgt=0,int flags=0);
  ~QImage();

  bool Read(cstring name);
  bool IsRead();
  //bool Write(string name);
};

#endif
