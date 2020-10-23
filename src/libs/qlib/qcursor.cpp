/*
 * QCursor - cursor creation and handling
 * 18-04-97: Created!
 * FUTURE:
 * - Hotspot QCursor
 * (C) MarketGraph/RVG
 */

#include <qlib/cursor.h>
#include <qlib/app.h>
#include <stdio.h>
#include <X11/Xlib.h>

QCursor::QCursor(int image,int from)
{
  xc=0;
  //dsp=_dsp;
  // X uses 32x32 cursor (??)
  wid=hgt=32;
  if(from==QCFROM_Q)
    CreateQCursor(image);
}
QCursor::~QCursor()
{
#ifndef WIN32
  if(xc)
    XFreeCursor(app->GetDisplay()->GetXDisplay(),xc);
#endif
}

// Cursor: arrow 16x16
static byte arrowDef[32]=
{ 0x01,0x00,0x03,0x00, 0x07,0x00,0x0F,0x00, 
  0x1F,0x00,0x3F,0x00, 0x7F,0x00,0xFF,0x00, 
  0xFF,0x01,0x1F,0x00, 0x3B,0x00,0x31,0x00, 
  0x60,0x00,0x60,0x00, 0xc0,0x00,0xc0,0x00 };

void QCursor::CreateQCursor(int image)
// X function to create a Q cursor
{
#ifndef WIN32
  Display *dpy=app->GetDisplay()->GetXDisplay();
  Pixmap pm1,pm2;		// Image, mask
  Window win=DefaultRootWindow(dpy);
  int screen=DefaultScreen(dpy);
  Colormap cmap=DefaultColormap(dpy,screen);
  XColor col1,col2,bidon;
  unsigned long maskvgc;
  XGCValues vgc;
  GC gc;

  if(image==QC_ARROW)
  { //pm1=XCreatePixmapFromBitmapData(dpy,win,1,16,16);
    pm1=XCreateBitmapFromData(dpy,win,(const char*)arrowDef,16,16);
    pm2=pm1;
    // Create cursor from the pixmaps; hotspot?
    XAllocNamedColor(dpy,cmap,"red",&col1,&bidon);
    XAllocNamedColor(dpy,cmap,"yellow",&col2,&bidon);
    xc=XCreatePixmapCursor(dpy,pm1,pm2,&col1,&col2,0,0);
    //printf("pixmap arrow cursor created %p\n",xc);
    XFreePixmap(dpy,pm1);
  } else if(image==QC_EMPTY)
  { // Create pixmaps
    pm1=XCreatePixmap(dpy,win,wid,hgt,1);
    pm2=XCreatePixmap(dpy,win,wid,hgt,1);

    vgc.foreground=1;
    vgc.background=0;
    maskvgc=GCForeground|GCBackground;
    gc=XCreateGC(dpy,pm1,maskvgc,&vgc);

    // Clear pixmaps
    XSetForeground(dpy,gc,0);
    XFillRectangle(dpy,pm1,gc,0,0,wid,hgt);
    XFillRectangle(dpy,pm2,gc,0,0,wid,hgt);
    // Paint cursor here
    //...
    // Create cursor from the pixmaps; hotspot?
    XAllocNamedColor(dpy,cmap,"red",&col1,&bidon);
    XAllocNamedColor(dpy,cmap,"yellow",&col2,&bidon);
    xc=XCreatePixmapCursor(dpy,pm1,pm2,&col1,&col2,0,0);
    //printf("pixmap cursor created %p\n",xc);
    // Kill used resources
    XFreePixmap(dpy,pm1);
    XFreePixmap(dpy,pm2);
    XFreeGC(dpy,gc);
  }
#endif
}

