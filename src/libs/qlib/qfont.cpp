/*
 * QFont - font selection
 * 10-10-96: Created!
 * BUGS:
 * - Win32: hDC's are plucked from QSHELL, instead of from the current QGLContext
 * - Win32: GetWidth() should perhaps use GetABCCharWidths() instead of GetCharWidth32()
 * (no leading/ending spacing added in the other cases)
 * - Left/Right bearing and such nyi
 * FUTURE:
 * - All parameters editable; QFont ctor, QFont::Create() order!
 * - resX/resY should be editable (hor/vert scaling)
 * NOTES:
 * - Depending on ? (the monitor?) fonts are selected differently,
 * so we much try to be as explicit as possible
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <qlib/font.h>
#include <qlib/app.h>
#include <qlib/display.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#include <X11/Xlib.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glx.h>

struct QFontInternal
{ XFontStruct *fi;
};

#undef  DBG_CLASS
#define DBG_CLASS "QFont"

// c/dtor

QFont::QFont(cstring _fname,int _size,int _slant,int _weight,int _width)
{ 
  fname=qstrdup(_fname);
  size=_size;
  slant=_slant;
  weight=_weight;
  width=_width;
  resX=100;
  resY=100;
  listBase=0;
#ifdef WIN32
  hFont=0;
#endif

  internal=(QFontInternal*)qcalloc(sizeof(*internal));
  internal->fi=0;
}
QFont::~QFont()
{
  if(fname)qfree(fname);
#ifdef WIN32
  if(hFont)DeleteObject(hFont);
#else
  // Free used display lists (important!)
  if(listBase)
  { int first,last;
    first=internal->fi->min_char_or_byte2;
    last =internal->fi->max_char_or_byte2;
    glDeleteLists(listBase,last-first+1);
  }
  //XUnloadFont(dsp->GetX11Display(),fi->fid);
  if(internal)qfree(internal);
#endif
}

#ifdef WIN32
static void perr(cstring s)
{
  LPVOID lpMsgBuf;
 
  FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
  );

  // Display the string.
  qdbg("Error (%s): %s\n",s,lpMsgBuf);

  // Free the buffer.
  LocalFree( lpMsgBuf );
}
#endif

bool QFont::Create()
// Actually create the font
{
  int first,last;

  QASSERT_F(slant>=0&&slant<=QFONT_SLANT_OBLIQUE);
  QASSERT_F(width>=0&&width<=QFONT_WIDTH_CONDENSED);
  QASSERT_F(weight>=0&&weight<=QFONT_WEIGHT_REGULAR);

#ifdef WIN32
  int weightVal[]={ 0,FW_MEDIUM,FW_BOLD,FW_NORMAL,FW_REGULAR };

  // Create OS font
  hFont=CreateFont(size,0,0,0,weightVal[weight],0,0,0,0,0,0,0,0,"arial");
  if(!hFont)return FALSE;

  // Generate display lists for the glyphs
  first=32;
  last =127;
  listBase=glGenLists((GLuint)last+1);
  if(!listBase)
  { qerr("QFont::Create: out of display lists (%d)\n",last+1);
    return FALSE;
  } else
  { // Generate lists (with glBitmap() calls)
    HDC hDC=QSHELL->GetQXWindow()->GetHDC();
    HFONT oldFont;
    QSHELL->GetCanvas()->Select();
    oldFont=(HFONT)SelectObject(hDC,hFont);	// VC6 quirk
//qdbg("hdc=%p, listbase=%d, oldfont=%p",hDC,listBase,oldFont);
    if(!wglUseFontBitmaps(hDC,first,last-first+1,listBase+first))
    { qerr("QFont: can't create wgl display lists for %s/%d",fname,size);
      perr("wglUseFontBitmaps");
      return FALSE;
    }
#ifdef ND_TEST
    GLYPHMETRICS gm;
  //MAT2 m2;
  //m2.eM11=(FIXED)1; m2.eM12=(FIXED)0; m2.eM21=(FIXED)0; m2.eM22=(FIXED)1;
  if(GetGlyphOutline(hDC,'3',GGO_METRICS,&gm,sizeof(gm),0,0)==GDI_ERROR)
  {
    qwarn("QFont:Create(); error in GetGlyphOutline()");
  } else qlog(QLOG_INFO,"width of '3'=%d",gm.gmBlackBoxX);
#endif

    // Get font info
    GetTextMetrics(hDC,&tm);
    SelectObject(hDC,oldFont);
  }
  return TRUE;
#else
  char buf[100];
  char *slantName[] ={ "*","r","i","o" };
  char *weightName[]={ "*","medium","bold","demi","regular" };
  char *widthName[] ={ "*","normal","narrow","condensed" };

  //listBase=0;

  //internal=(QFontInternal*)qcalloc(sizeof(*internal));
  //internal->fi=0;

  sprintf(buf,"-*-%s-%s-%s-%s-*-%d-*-%d-%d-*-*-*-*",fname,weightName[weight],
    slantName[slant],widthName[width],size,
    resX,resY);
  //DEBUG_F(qdbg("QFont '%s'\n",buf));
  internal->fi=XLoadQueryFont(app->GetDisplay()->GetXDisplay(),buf);
  if(!internal->fi)
  { qerr("QFont::Create: can't create font '%s'\n",buf);
    return FALSE;
  }
  // Generate display lists for the glyphs
  first=internal->fi->min_char_or_byte2;
  last =internal->fi->max_char_or_byte2;
  listBase=glGenLists((GLuint)last+1);
  if(!listBase)
  { qerr("QFont::Create: out of display lists (%d)\n",last+1);
    return FALSE;
  } else
  { // Generate lists (with glBitmap() calls)
    glXUseXFont(internal->fi->fid,first,last-first+1,listBase+first);
  }
  return TRUE;
#endif
}

// Win32 needs font support still

// Set attributes
void QFont::SetResX(int n)
{
  if(internal->fi)
  { qerr("QFont::SetResX(); can't modify after Create()");
    return;
  }
  resX=n;
}
void QFont::SetResY(int n)
{
  if(internal->fi)
  { qerr("QFont::SetResY(); can't modify after Create()");
    return;
  }
  resY=n;
}

int QFont::GetListBase()
{ return listBase;
}

// Information
Font QFont::GetXFont()
{
#ifdef WIN32
  QASSERT_0(hFont);
  // No X Font under Win32
  //qerr("QFont:GetXFont() nyi/win32");
  return 0;
#else
  QASSERT_0(internal->fi);		// Create() first
  if(internal->fi)
    return internal->fi->fid;
  return 0;
#endif
}
int QFont::GetAscent()
{
#ifdef WIN32
  return tm.tmAscent;
#else
  QASSERT_VALID()
  DBG_C("GetAscent")
  QASSERT_0(internal->fi);		// Create() first
  return internal->fi->ascent;
#endif
}
int QFont::GetDescent()
{
#ifdef WIN32
  return tm.tmDescent;
#else
  QASSERT_0(internal->fi);		// Create() first
  return internal->fi->descent;
#endif
}
int QFont::GetHeight(cstring s)
{
#ifdef WIN32
  return tm.tmHeight;
#else
  QASSERT_0(internal->fi);		// Create() first
  /*printf("QFont::GetHgt: log=%d+%d, max=%d+%d\n",
    fi->ascent,fi->descent,fi->max_bounds.ascent,fi->max_bounds.descent);*/
  return internal->fi->max_bounds.ascent+internal->fi->max_bounds.descent;
#endif
}

int QFont::GetWidth(ubyte c)
{
#ifdef WIN32
  HDC hDC=QSHELL->GetQXWindow()->GetHDC();
  HFONT oldFont;
  int   w;

  oldFont=(HFONT)SelectObject(hDC,hFont);
  // Get width of a single char
  // Note that GetCharWidth32() is only available on WinNT (!)
  if(!GetCharWidth(hDC,c,c,&w))
  {
    qwarn("QFont:GetWidth(); error in GetCharWidth32()");
  }
  SelectObject(hDC,oldFont);
//qlog(QLOG_INFO,"Metrics width of '%c' = %d",c,w);
  return w;
#else
  QASSERT_0(internal->fi);		// Create() first
  // Check if in font
  if(c<internal->fi->min_char_or_byte2||c>internal->fi->max_char_or_byte2)
    return 0;
  return internal->fi->per_char[c-internal->fi->min_char_or_byte2].width;
#endif
}
int QFont::GetWidth(cstring s,int len)
// Returns width of string (upto 'len' chars)
// This takes into account any overhanging of the LAST char
// BUG: doesn't take into account overhanging of FIRST char
// If 'len'==-1, the entire string is measured (upto chr(0))
{
  int wid=0;
  char lastC;
  int  cwid,rb;

#ifdef WIN32
  QASSERT_0(hFont);
#else
  QASSERT_0(internal->fi);		// Create() first
#endif

  if(len==-1)
  {
    // Take care of right hanging of last char
    if(s!=0&&*s!=0)
    { lastC=s[strlen(s)-1];
      cwid=GetWidth(lastC);
      rb=GetRightBearing(lastC);
      if(rb>cwid)wid=rb-cwid;
    }
    for(;*s;s++)
    { wid+=GetWidth(*s);
    }
  } else
  {
    // Take care of right hanging of last char
    if(s!=0&&len>0)
    { lastC=s[len-1];
      cwid=GetWidth(lastC);
      rb=GetRightBearing(lastC);
      if(rb>cwid)wid=rb-cwid;
    }
    for(;len>0;len--)
    { wid+=GetWidth(*s);
      s++;
    }
  }
  return wid;
}
int QFont::GetLeftBearing(ubyte c)
{
#ifdef WIN32
  QASSERT_0(hFont);
  //qerr("QFont:GetLeftBearing() nyi/win32");
  return 0;
#else
  QASSERT_0(internal->fi);		// Create() first
  // Check if in font
  if(c<internal->fi->min_char_or_byte2||c>internal->fi->max_char_or_byte2)
    return 0;
  return internal->fi->per_char[c-internal->fi->min_char_or_byte2].lbearing;
#endif
}
int QFont::GetRightBearing(ubyte c)
{
#ifdef WIN32
  //qerr("QFont:GetRightBearing() nyi/win32");
  return 0;
#else
  QASSERT_0(internal->fi);		// Create() first
  // Check if in font
  if(c<internal->fi->min_char_or_byte2||c>internal->fi->max_char_or_byte2)
    return 0;
  return internal->fi->per_char[c-internal->fi->min_char_or_byte2].rbearing;
#endif
}

/***************
* FONT LISTING *
***************/
static bool AddString(char **list,string s)
// Adds string to list 'list'; makes sure 's' is unique
{
  int i;
  for(i=0;;i++)
  { if(list[i]==0)
    { list[i]=qstrdup(s);
      qdbg("AddString(%s); list[%d] set (%p)\n",s,i,list[i]);
      return TRUE;
    } else
    { 
      //qdbg("  cmp(%s,%s)\n",list[i],s);
      if(strcmp(list[i],s)==0)return FALSE;
    }
  }
  //return FALSE;
}

QFontList::QFontList()
{
#ifdef WIN32
  qerr("QFontList ctor nyi/win32");
#else
  char *s;
  int   i,n;
  qdbg("QFontList ctor\n");
  xlist=XListFonts(app->GetDisplay()->GetXDisplay(),"*",9999,&count);
  qdbg("  count=%d\n",count);
  for(i=0;i<count;i++)
  { //qdbg("  '%s'\n",xlist[i]);
  }
  // Collect family names
  familyCount=count/2;
  family=(char**)qcalloc(sizeof(char*)*familyCount);
  char buf[150];
  for(i=n=0;i<count;i++)
  { if(xlist[i][0]!='-')continue;
    strcpy(buf,xlist[i]);
    // Find 2nd field awk { print $2; }
    s=strtok(buf,"-");
    if(s)s=strtok(0,"-");
    if(s)
    { if(AddString(family,s))n++;
    }
  }
  familyCount=n;

  // Foundry names
  foundryCount=0;
  foundry=0;
#endif
}

QFontList::~QFontList()
{
#ifndef WIN32
  if(xlist)
    XFreeFontNames(xlist);
#endif
}

cstring QFontList::GetFamily(int n)
{
  if(n<0||n>=familyCount)return 0;
  return family[n];
}

cstring QFontList::GetFoundry(int n)
{
  if(n<0||n>=foundryCount)return 0;
  return foundry[n];
}
