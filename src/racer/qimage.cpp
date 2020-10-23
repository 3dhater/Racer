/*
 * QImage - based on QBitMap; loads images
 * 01-10-96: Created!
 * 03-07-98: Targa file format support
 * 05-10-99: IFL support (see: man IFL); lots of new formats
 * 08-12-00: BMP support (for Win32; SGI uses IFL)
 * 15-04-01: Support for Linux Intel (endian fixes)
 * NOTES:
 * - TGA v2.0: no Extension Area or Developer Area is read or used
 * - TGA files are in Intel byte order (always)
 * BUGS:
 * - For most TGA formats, the direction indicators in the header are not listened to.
 * If you see a TGA file that is mirrored horizontally or vertically, please mail me
 * with the picture so I can add support for that.
 * FUTURE:
 * - Compress written TGA files.
 * (C) MarketGraph/RVG
 */

#include <qlib/image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <windows.h>
//#include <wingdi.h>
#endif
#ifdef __sgi
#include <bstring.h>
#include <ifl/iflFile.h>
#include <jpeg/jpeglib.h>
#endif
#include <qlib/debug.h>
DEBUG_ENABLE

#ifdef WIN32
#define bcopy(s,d,len)  memcpy(d,s,len)
#endif

// Define FAST_LOAD to preload all RLE data, and then decompress from memory
#define FAST_LOAD	1

// Max width in pixels of incoming picture
#define MAX_WID         4096

// sgi.c import
typedef unsigned short uword;

/*********
* c/dtor *
*********/
QImage::QImage(cstring _name,int dep,int wid,int hgt,int flags)
  : QBitMap(dep,wid,hgt,flags)
{
  name=qstrdup(_name);
  err=0;

  //proftime_t t;
  //profStart(&t);
  if(!Read(name))
  { qwarn("QImage ctor: can't load '%s'",name);
    err=1;
  } else
  { //profReport(&t,"QImage::Read speed");
    //printf("QImage ctor read(%s)\n",name);
    err=0;
  }
}

QImage::~QImage()
{ qfree(name);
}

/** Intel Byte Ordering **/

static void cvtShorts(ushort *buffer,long n)
// From Paul Haeberli source code
{
#if defined(__sgi)
  short i;
  long nshorts = n>>1;
  unsigned short swrd;

  for(i=0; i<nshorts; i++)
  { swrd = *buffer;
    *buffer++ = (swrd>>8) | (swrd<<8);
  }
#endif
}
static void cvtLongs(long *buffer,long n)
{
#if defined(__sgi)
  short i;
  long nlongs = n>>2;
  unsigned long lwrd;

  for(i=0; i<nlongs; i++)
  { lwrd = buffer[i];
    buffer[i]=((lwrd>>24)          |
              (lwrd>>8 & 0xff00)   |
              (lwrd<<8 & 0xff0000) |
              (lwrd<<24)           );
  }
#endif
}
static void cvtImageHdr(long *buffer)
{ cvtShorts((ushort*)buffer,12);
  cvtLongs(buffer+3,12);
  cvtLongs(buffer+26,4);
}

/**************
* Type detect *
**************/
int QImage::DetectType(cstring name)
// Returns: 0=RGB, 1=TGA, 3=JPG/JPE/JFIF, 4=BMP, 2=other (IFL)
//          -1=unknown
// Currently, -1 is never returned (IFL instead)
{ char s[256],*p;
  // Ignore caps
  strncpy(s,name,256); s[255]=0;
  for(p=s;*p;p++)*p=toupper(*p);
  if(strstr(s,".RGB"))return 0;
  if(strstr(s,".TGA"))return 1;
  if(strstr(s,".JPG"))return 3;
  if(strstr(s,".JPE"))return 3;
#if defined(WIN32) || defined(linux) || defined(__sgi)
  // SGI uses IFL to read BMPs (better loader) and then fills ALPHA=255
  if(strstr(s,".BMP"))return 4;
#endif
  // All others (.png, .ppm, .tif etc) are done using SGI's IFL
  return 2;
}

/******************
* RGB file format *
******************/
bool QImage::Info(cstring name,SGIHeader* hdr)
// .rgb header
{ FILE *fr;

  memset(hdr,0,sizeof(SGIHeader));
  fr=fopen(QExpandFilename(name),"rb");
  if(!fr)return FALSE;
  // Get header
  fread(hdr,1,sizeof(SGIHeader),fr);
  // Check for Intel (big-endian) (LightWave PC writes Intel RGBs)
  if(((hdr->imagic>>8)|((hdr->imagic&0xFF)<<8))==RGB_IMAGIC)
  { doRev=TRUE; cvtImageHdr((long*)hdr);
    //printf("  Intel Image\n");
  } else doRev=FALSE;

  //IntelHeader(hdr);
  fclose(fr);
/*
  printf("  Image dims=%d, name=%s, sizeof hdr=%d\n",hdr->dim,hdr->name,
    sizeof(*hdr));
  printf("  size: x=%d, y=%d, z=%d\n",hdr->xsize,hdr->ysize,hdr->zsize);
*/
  return TRUE;
}

#define ADVANCE 4		// RGBA
static void Decompress(char *s,char *d)
{ unsigned char pixel,count;
  while(1)
  { pixel=*s++;
    if(!(count=(pixel&0x7F)))
      return;
    if(pixel&0x80)
    { while(count--)
      { *d=*s++; d+=ADVANCE; }
    } else
    { pixel=*s++;
      while(count--)
      { *d=pixel; d+=ADVANCE; }
    }
  }
}

bool QImage::ReadRGB(cstring name)
// RGB loader
{
  FILE *fr;
  int  c;
  short i,j;
  char *p;
  SGIHeader hdr;
  char bufLine[MAX_WID*4];        // PAL max line buffer

  // RLE support
  int tableSize;		// Size of RLE line information
  ulong *rowStart;		// Start of each row
  long  *rowSize;		// Size of each (compressed) row
  int   curStart,curSize;
  char *bufComp;		// Compressed data
  int   bufCompSize;
#ifdef FAST_LOAD
  char *rleData;		// Fast loading RLE picture
  int   rleSize;
#endif

  if(!Info(name,&hdr))
    return FALSE;
  // Resize bitmap
  wid=hdr.xsize;
  hgt=hdr.ysize;
  dep=32;		// SGI default
  // Realloc
  Alloc(dep,wid,hgt,flags);

  fr=fopen(QExpandFilename(name),"rb");
  if(!fr)return FALSE;
  // Header was already read
  // Check bitmap
  if(!mem){ fclose(fr); return FALSE; }
  // Check for mismatching hgt's (bitmap vs. picture)!!!!!!
  //...
  if(hdr.xsize>MAX_WID){ fclose(fr); return FALSE; }  // lineBuf[] size

  /** Read picture data **/
  if(RGB_IS_RLE(hdr.type))
  { //printf("RGB compressed not yet supported\n");
    // Read RLE line info
    tableSize=hdr.ysize*hdr.zsize*sizeof(long);
    rowStart=(ulong*)qcalloc(tableSize);
    rowSize=(long*)qcalloc(tableSize);
    //printf("rle info sized %d\n",tableSize);
    if(rowStart==0||rowSize==0)
    { qwarn("QImage::Read: No mem for row info\n");
      fclose(fr);
      return FALSE;
    }
    fseek(fr,512,0);
    fread(rowStart,1,tableSize,fr);
    fread(rowSize ,1,tableSize,fr);
    if(doRev)
    { cvtLongs((long*)rowStart,tableSize);
      cvtLongs(rowSize ,tableSize);
    }
    // Get max. compressed line size
    bufCompSize=0;
    for(c=0;c<hdr.zsize;c++)
      for(i=0;i<hdr.ysize;i++)
      { if(rowSize[i+c*hdr.ysize]>bufCompSize)
          bufCompSize=rowSize[i+c*hdr.ysize];
      }
    bufComp=(char*)qcalloc(bufCompSize);	// Max ever needed
#ifdef FAST_LOAD
    // Get the length of the RLE data
    int curPos,endPos;
    curPos=ftell(fr);
    fseek(fr,0,SEEK_END);
    endPos=ftell(fr);
    rleSize=endPos-curPos;
    //printf("rle Data sized %d\n",rleSize);
    rleData=(char*)qcalloc(rleSize);
    if(!rleData)
    { qwarn("QImage::Read: can't read rle chunk\n");
      fclose(fr); return FALSE;
    }
    fseek(fr,curPos,SEEK_SET);
    fread(rleData,1,rleSize,fr);
#endif

    for(c=0;c<4;c++)			// Always do RGBA
    { for(i=0;i<hdr.ysize;i++)
      { p=mem+i*wid*(dep/8);
        p+=c;
        if(c<hdr.zsize)
        { curStart=rowStart[i+c*hdr.ysize];
          curSize =rowSize[i+c*hdr.ysize];
          //printf("line %d: start=%d, size=%d\n",i,curStart,curSize);
          if(curSize>bufCompSize)
          { // Should never get here
            printf("** QImage::Read: Line size bigger than buffer!\n");
            curSize=bufCompSize;
          }
#ifdef FAST_LOAD
          curStart-=curPos;		// Cut off start of RLE file pos
          //printf("line %d: curStart=%d\n",i,curStart);
          Decompress(rleData+curStart,p);
#else
          fseek(fr,curStart,SEEK_SET);
          fread(bufComp,1,curSize,fr);
#endif
          Decompress(bufComp,p);
        } else
        { // Fill in default
          for(j=0;j<hdr.xsize;j++)
          { *p=255;
            p+=4;
          }
        }
      }
    }

    // Cleanup allocated resources
#ifdef FAST_LOAD
    if(rleData)qfree(rleData);
#endif
    qfree(bufComp);
    qfree(rowStart);
    qfree(rowSize);

  } else
  { // Verbatim
    // Hardcoded for dim=3, zsize=4 (RGBA)
    fseek(fr,512,SEEK_SET);
    p=mem;
    for(c=0;c<4;c++)          // RGBA
    { for(i=0;i<hdr.ysize;i++)
      { p=mem+i*wid*(dep/8);
        p+=c;                 // Offset into RGBA
        fread(bufLine,1,hdr.xsize,fr);
        for(j=0;j<hdr.xsize;j++)
        { *p=bufLine[j];
          p+=4;               // Skip 1 RGBA elt
        }
      }
      /*fread(p+i*bm->wid*(bm->bits/8),
        1,hdr.wid*(bm->bits/8),fr);*/
    }
  /*    fread(bm->buffer+(bm->hgt-i-1)*bm->wid*(bm->bits/8),
        1,si.wid*(si.bits/8),fr);*/
    //fread(bm->buffer,1,si.wid*si.hgt*(si.bits/8),fr);
  }

  fclose(fr);
  /*fr=fopen("test.dmp","wb");
  if(fr)
  { fwrite(mem,1,2000,fr);
    fclose(fr);
  }*/
  return TRUE;

  /** Extract end **/
}

/******************
* TGA file format *
******************/
bool QImage::Info(cstring name,TGAHeader* hdr)
// .tga header (Intel orderings!)
{ FILE *fr;

  memset(hdr,0,sizeof(*hdr));
  fr=fopen(QExpandFilename(name),"rb");
  if(!fr)return FALSE;
  // Structure is refitted on SGI, so just bluntly reading the entire header
  // does NOT work (char[3], short[2] -> padding occurs)
  //fread(hdr,1,sizeof(*hdr),fr);
  fread(hdr,1,3,fr);
  fread(&hdr->cmFirstEntry,1,5,fr);
  fread(&hdr->xOrg,1,10,fr);
  // Reverse shorts for little-Endian platform (SGI=LE)
  cvtShorts((ushort*)&hdr->cmFirstEntry,4);
  cvtShorts((ushort*)&hdr->xOrg,8);

  fclose(fr);
  return TRUE;
}

static void Decompress(FILE *fp,char *d,int dep,int wid)
// Decompress a line of data into 'd'
// Direct from file (hmm)
{
  char buf[4];
  int  n,x,n0;
  //qdbg("---- line\n");
  if(dep==32)
  { // RGBA quadruples (in order ABGR probably)
    for(x=0;x<wid;)
    { n=fgetc(fp);
      // Take care not to loop infinitely on bad files
      if(feof(fp))break;
      //qdbg("n=%d\n",n);
      if(n&0x80)
      { // Run
        //qdbg("run %d\n",n&0x7f);
        n^=0x80;
        n++; x+=n;
        fread(buf,1,4,fp);
        for(;n>0;n--)
        { *d++=buf[2];
          *d++=buf[1];
          *d++=buf[0];
          *d++=buf[3];
        }
      } else
      { // Copy
        //qdbg("copy %d\n",n+1);
        n0=n;
        for(n++;n>0;n--)
        { fread(buf,1,4,fp);
          *d++=buf[2];
          *d++=buf[1];
          *d++=buf[0];
          *d++=buf[3];
        }
        //fread(d,1,(n+1)*4,fp);
        //QHexDump(d,20);
        x+=n0+1;
        //d+=(n+1)*4;
      }
    }
  } else if(dep==24)
  { // RGB tuples
    for(x=0;x<wid;)
    { n=fgetc(fp);
      //printf("n=%d\n",n);
      if(n&0x80)
      { // Run
        //printf("run %d\n",n&0x7f);
        n^=0x80;
        n++; x+=n;
        fread(buf,1,3,fp);
        for(;n>0;n--)
        { *d++=buf[2];
          *d++=buf[1];
          *d++=buf[0];
          //*d++=buf[3];
          *d++=255;
        }
      } else
      { // Copy
        //printf("copy %d\n",n+1);
        n0=n;
        for(n++;n>0;n--)
        { fread(buf,1,3,fp);
          *d++=buf[2];
          *d++=buf[1];
          *d++=buf[0];
          *d++=255;
          //*d++=buf[3];
        }
        //fread(d,1,(n+1)*4,fp);
        //QHexDump(d,20);
        x+=n0+1;
        //d+=(n+1)*4;
      }
    }
  }

}
bool QImage::ReadTGA(cstring name)
// .tga loader
{
  FILE *fp;
  TGAHeader hdr;
  char *d,*s,c;
  char bufLine[MAX_WID*4];        // max line buffer (RGBA)
  int x,y;
 
qdbg("QImage:ReadTGA()\n");

  // Read header
  if(!Info(name,&hdr))
    return FALSE;
  fp=fopen(QExpandFilename(name),"rb");
  if(!fp)return FALSE;
  fseek(fp,3+10+5 /*sizeof(hdr)*/,SEEK_SET);	// Hdr is padded!
  // Allocate bitmap (before checking, BTW; this way we will crash less)
  wid=hdr.width;
  hgt=hdr.height;
  dep=32;
  if(!Alloc(dep,wid,hgt,flags))
  { qwarn("QImage:ReadTGA(); out of memory");
    goto fail;
  }
  // Check some things
  if(hdr.idLen!=0)
  { qwarn("QImage:ReadTGA(); ID included in file is not supported");
   fail:
    fclose(fp);
    return FALSE;
  }
  if(hdr.width>MAX_WID)
  { qwarn("QImage:ReadTGA(); too wide"); goto fail; }  // lineBuf[] size
  if(hdr.colorMapType!=0)
  { qwarn("QImage:ReadTGA(); colormap is not supported"); goto fail;
  }
qdbg("ReadTGA: %dx%dx%d\n",hdr.width,hdr.height,hdr.pixDepth);
qdbg("  imgDesc=%d, hdr.imageType=%d\n",hdr.imgDesc,hdr.imageType);
  //QHexDump((char*)&hdr,sizeof(hdr));
  // Read pixel data
  if(hdr.imageType==3&&hdr.pixDepth==8)
  { // Expand from 8-bit Y to RGBA
    //qdbg("  ReadTGA: grayscale 8-bit\n");
    for(y=0;y<hgt;y++)
    { fread(bufLine,1,wid,fp);
      // Put in line
      d=mem+y*wid*(dep/8);
      s=bufLine;
      for(x=0;x<wid;x++)
      { c=*s++;
        *d++=c;
        *d++=c;
        *d++=c;
        *d++=c;       // Alpha
      }
    }
  } else if(hdr.imageType==2&&hdr.pixDepth==24)
  { // BGR (PhotoShop 4.0 PC)
    //qdbg("  ReadTGA: 24-bit\n");
    for(y=0;y<hgt;y++)
    {
      fread(bufLine,1,wid*3,fp);
      // Put in line
      d=mem+y*wid*(dep/8);
      s=bufLine;
      for(x=0;x<wid;x++)
      { *d++=s[2];
        *d++=s[1];
        *d++=*s;
        s+=3;
        *d++=255;       // Alpha
      }
    }
  } else if(hdr.imageType==2&&hdr.pixDepth==32)
  { // BGRA (PhotoShop 4.0 PC; 32-bits per pixel)
    //qdbg("  ReadTGA: 32-bit\n");
    for(y=0;y<hgt;y++)
    {
      fread(bufLine,1,wid*4,fp);
      // Put in line
      d=mem+y*wid*(dep/8);
      s=bufLine;
      for(x=0;x<wid;x++)
      { *d++=s[2];
        *d++=s[1];
        *d++=s[0];
        *d++=s[3];
        s+=4;
      }
    }
  } else if(hdr.imageType==10&&hdr.pixDepth==32)
  {
    // BGRA compressed (TVPaint 3.60 for Win32, Gimp)
//qdbg("  ReadTGA: 32-bit compressed\n");
    for(y=0;y<hgt;y++)
    {
      Decompress(fp,bufLine,hdr.pixDepth,wid);
      // Put in line (mind picture storing direction)
      if(hdr.imgDesc&0x20)d=mem+(hgt-y-1)*wid*(dep/8);
      else                d=mem+y*wid*(dep/8);
      //bcopy(bufLine,d,wid*4);
      memcpy(d,bufLine,wid*4);
    }
  } else if(hdr.imageType==10&&hdr.pixDepth==24)
  { // ??? compressed
    qdbg("  ReadTGA: 24-bit compressed\n");
    for(y=0;y<hgt;y++)
    {
      Decompress(fp,bufLine,hdr.pixDepth,wid);
      //fread(bufLine,1,wid*4,fp);
      // Put in line
      d=mem+y*wid*(dep/8);
      bcopy(bufLine,d,wid*4);
#ifdef OBS
      s=bufLine;
      for(x=0;x<wid;x++)
      { *d++=s[2];
        *d++=s[1];
        *d++=s[0];
        //*d++=s[3];
        *d++=255;
        s+=3;
      }
#endif
    }
  } else
  { qwarn("QImage:ReadTGA(); no image loader for type %d, pixDepth %d",
      hdr.imageType,hdr.pixDepth);
  }
  fclose(fp);
  return TRUE;
}

/*******************
* IFL file formats *
*******************/
bool QImage::ReadIFL(cstring name)
// Generic IFL reader; reads .BMP, .JPG etc
{
#ifdef WIN32
  // No IFL
  return FALSE;
#endif
#ifdef linux
  // No IFL
  return FALSE;
#endif
#ifdef __sgi
  iflStatus sts;
  iflFile *file;
  int lwid,lhgt;             // Load size

//qdbg("QImage::ReadIFL(%s)\n",name);
  file=iflFile::open(name,O_RDONLY,&sts);
  if(sts!=iflOKAY)
  { qwarn("Qimage:ReadIFL(): can't open '%s'\n",name);
    return FALSE;
  }

  iflSize dims;
  iflConfig cfg(iflUChar,iflInterleaved,4);
  file->getDimensions(dims);
//qdbg("IFL: dims x%d y=%d, z=%d, c=%d bm=%dx%d\n",
//dims.x,dims.y,dims.z,dims.c,wid,hgt);

  // Allocate bitmap
  if(!Alloc(32,dims.x,dims.y /*,flags*/))
  { qwarn("QImage:ReadIFL(); (%s) out of memory",name);
    goto fail;
  }

  // Read pixels
  if(dims.x>wid)lwid=wid; else lwid=dims.x;
  if(dims.y>hgt)lhgt=hgt; else lhgt=dims.y;
  sts=file->getTile(0,0,0,lwid,lhgt,1,mem,&cfg);
  if(sts!=iflOKAY)
  { qwarn("QImage:ReadIFL(): (%s) can't getTile\n",name);
    goto fail;
  }
  file->close();
  return TRUE;
 fail:
  file->close();
  return FALSE;
#endif
}

/*******************
* JPG file formats *
*******************/
static void PutLine(char *s,int y,QBitMap *bm,int comps)
// 'comps' is the number of components, 3=RGB, 1=Grayscale
{
//qdbg("PutLine(%p,y=%d, w=%d\n",s,y,bm->GetWidth());
  int w;
  char *d;
  d=bm->GetBuffer()+y*bm->GetWidth()*4;
  if(comps==3)
  { for(w=bm->GetWidth();w>0;w--)
    {
      *d++=*s++;
      *d++=*s++;
      *d++=*s++;
      //*d++=*s++;
      *d++=255;
    }
  } else if(comps==1)
  {
    for(w=bm->GetWidth();w>0;w--)
    {
      *d++=*s;
      *d++=*s;
      *d++=*s++;
      *d++=255;
    }
  } else qwarn("QImage:ReadJPG; unsupported number of components (%d)",comps);
}
bool QImage::ReadJPG(cstring name)
{
#ifdef WIN32
  // No JPEG for Win32
  return FALSE;
#endif
#ifdef linux
  // No JPEG for Linux
  return FALSE;
#endif
#ifdef __sgi
  // IRIX
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  char   lineBuf[2048*3];
  //JSAMPARRAY buffer;
  char  *buffer[10];
  //short lineBuf[2048*3];
  //int  num_scanlines;

//qdbg("QImage::ReadJPG(%s)\n",name);

  FILE *fr;
  fr=fopen(name,"rb");
  if(!fr)
  { printf("Can't open %s\n",name);
   fail:
    if(fr)fclose(fr);
    return FALSE;
  }

  buffer[0]=lineBuf;

  /* Initialize the JPEG decompression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  /* Specify data source for decompression */
  jpeg_stdio_src(&cinfo, fr);
  /* Read file header, set default decompression parameters */
  (void) jpeg_read_header(&cinfo, TRUE);

  // Allocate bitmap
//qdbg("image %dx%d, comps=%d\n",cinfo.image_width,cinfo.image_height,
//cinfo.num_components);
  if(!Alloc(32,cinfo.image_width,cinfo.image_height))
  { qwarn("QImage:ReadJPG(); (%s) out of memory",name);
    goto fail;
  }

  /* Start decompressor */
  (void) jpeg_start_decompress(&cinfo);
  /* Process data */
  while (cinfo.output_scanline < cinfo.output_height) {
    //num_scanlines = jpeg_read_scanlines(&cinfo,(JSAMPARRAY)lineBuf, 1);
    /*num_scanlines=*/ jpeg_read_scanlines(&cinfo,(JSAMPARRAY)buffer,1);
    //(*dest_mgr->put_pixel_rows) (&cinfo, dest_mgr, num_scanlines);
    PutLine((char*)lineBuf,hgt-cinfo.output_scanline,this,
      cinfo.num_components);
  }
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(fr);
  return TRUE;
#endif
}

/****************************
* BMP formats (Win32, OS/2) *
****************************/
bool QImage::ReadBMP(cstring name)
{
#ifdef __sgi
  // Use IFL, which has a better loader
  if(!ReadIFL(name))
    return FALSE;
  // Still, fill alpha channel, since this is trash with IFL
  SetAlpha(255);
  return TRUE;
#else
  // Win32/Linux
  BITMAPFILEHEADER bmfh;
  BITMAPINFOHEADER bmih;
  RGBQUAD colors[256];
  FILE *fr;
  
  fr=fopen(name,"rb");
  if(!fr)
  { //qwarn("Can't open %s\n",name);
   fail:
    if(fr)fclose(fr);
    return FALSE;
  }
  
  // Read file header
  //fread(&bmfh,1,14,fr);            // Structure packing problems
  fread(&bmfh.bfType,1,sizeof(short),fr);
  fread(&bmfh.bfSize,1,sizeof(long),fr);
  fread(&bmfh.bfReserved1,2,sizeof(short),fr);
  fread(&bmfh.bfOffBits,1,sizeof(long),fr);
  cvtShorts((ushort*)&bmfh.bfType,1*sizeof(short));
  cvtLongs((long*)&bmfh.bfSize,1*sizeof(long));
  cvtShorts((ushort*)&bmfh.bfReserved1,2*sizeof(short));
  cvtLongs((long*)&bmfh.bfOffBits,1*sizeof(long));
//qdbg("&bfOffBits=%p (bmfh=%p)\n",&bmfh.bfOffBits,&bmfh);
  
  // Read info header
  fread(&bmih,1,sizeof(bmih),fr);
  cvtLongs((long*)&bmih.biSize,3*sizeof(int));
  cvtShorts((ushort*)&bmih.biPlanes,2*sizeof(short));
  cvtLongs((long*)&bmih.biCompression,6*sizeof(int));

//qdbg("sizeof bmfh=%d, bmih=%d\n",sizeof(bmfh),sizeof(bmih));
//qdbg("bfSize=%d\n",bmfh.bfSize);
//qdbg("biSize=%d\n",bmih.biSize);
//qdbg("bitCount: $%x\n",bmih.biBitCount);
//qdbg("compression: %d\n",bmih.biCompression);
//qdbg("bits: %d\n",bmfh.bfOffBits);
//qdbg("size: %dx%d\n",bmih.biWidth,bmih.biHeight);
  // Allocate bitmap
  if(!Alloc(32,bmih.biWidth,bmih.biHeight))
  { qwarn("QImage:ReadBMP(); (%s) out of memory",name);
    goto fail;
  }
  
  // Read pixels
  if(bmih.biBitCount==24&&bmih.biCompression==0)
  {
    int i,n;
    char *s,*d,t;
    // 24-bit, no compression
    // May be faster to read all in, then expand from back to front
    // (bitmap buffer is larger than 3-byte tuples)
    fseek(fr,bmfh.bfOffBits,SEEK_SET);
    n=bmih.biWidth*bmih.biHeight;
    // Read all tuples
    fread(mem,n*3,1,fr);
    // Reverse BGR -> RGBA in situ
    s=mem+n*3-3; d=mem+n*4-4;
    for(i=0;i<n-2;i++)
    {
      d[0]=s[2];
      d[1]=s[1];
      d[2]=s[0];
      d[3]=255;
      s-=3; d-=4;
    }
    if(n>1)
    { // Last pixel but one
      d[3]=255; d[2]=s[0]; t=s[1]; d[0]=s[2]; d[1]=t; s-=3; d-=4;
    }
    if(n>0)
    { // Last pixel
      d[3]=255; t=s[0]; d[0]=s[2]; d[2]=t; // d[1] is already correct
    }
  } else if(bmih.biBitCount==8&&bmih.biCompression==0)
  {
    // Grayscale, 8-bit
    int  i,n;
    char c,*d;
    fseek(fr,bmfh.bfOffBits,SEEK_SET);
    n=bmih.biWidth*bmih.biHeight;
    d=mem;
//qdbg("Grayscale @%d, n=%d\n",bmfh.bfOffBits,n);
    for(i=0;i<n;i++)
    {
      c=fgetc(fr);
      *d++=c;
      *d++=c;
      *d++=c;
      *d++=255;
    }
  } else
  {
    qwarn("QImage:ReadBMP(); unsupported #bits (%d) or compression (%d)",
      bmih.biBitCount,bmih.biCompression);
  }
  fclose(fr);
  return TRUE;
#endif
}

/**********
* Reading *
**********/
bool QImage::Read(cstring name)
// Load a picture
{ int t;
  t=DetectType(name);
qdbg("QImage: read %s, type=%d\n",name,t);
  switch(t)
  { case 0: return ReadRGB(name);
    case 1: return ReadTGA(name);
    case 2: return ReadIFL(name);
    case 3: return ReadJPG(name);
    case 4: return ReadBMP(name);
    default:
#ifdef DEBUG
      qwarn("QImage:Read(): image '%s' has unknown file format",name);
#endif
      return FALSE;
  }
}

/*******
* Info *
*******/
bool QImage::IsRead()
{
  if(err)return FALSE;
  return TRUE;
}
