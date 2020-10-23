/*
 * QPolygon - a 3D polygon - possibly textured
 * 02-07-97: Created!
 * 11-08-99: Although obsolete, for Lingo's sake I'm implementing texture
 * objects, since this module used to generate textures from an RGB source
 * EVERY time the polygon was drawn. No wonder it was darn slow!
 * Future versions: use D3 instead. MUCH better texture usage, easier etc.
 * NOTES:
 * - Indy is very slow with textured polygons. O2 is fast.
 * BBUGS:
 * - Texture image resizing should resize to nearest 2^n, instead of
 * minimizing (as is done as per 21-7-97)
 */

#include <qlib/polygon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qlib/app.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Use RGB textures? (RGBA nyi)
#define TEXTURE_RGB

QPolygon::QPolygon(int ivertices)
{ int i;
  QASSERT_V(ivertices>=3&&ivertices<=4);

  texID=0;
  vertices=ivertices;
  for(i=0;i<vertices;i++)
  { x[i]=0; y[i]=0; z[i]=0;
    tx[i]=0; ty[i]=0;
  }
  flags=0;
  // Standard texture coordinates
  ty[1]=1;
  tx[2]=1; ty[2]=1;
  tx[3]=1;

  texture=0;
  textureSize.x=textureSize.y=0;
}

QPolygon::~QPolygon()
{ qdbg("QPolygon dtor\n");
  if(texID)
  { // Make sure we're deleting the texture from the right context
    app->GetBC()->GetCanvas()->Select();
    glDeleteTextures(1,(GLuint*)&texID);
  }
  if(texture)
    qfree(texture);
}

void QPolygon::SetVertex(int n,qfloat px,qfloat py,qfloat pz)
{
  QASSERT_V(n>=0&&n<vertices);
  x[n]=px;
  y[n]=py;
  z[n]=pz;
}

static int HiBit(int n)
{
  int i;
  for(i=31;i>=0;i--)
  { if(n&(1<<i))
      return i;
  }
  return 0;
}

// Small RGB scaler; no filtering or interpolation
// (mailed this once to somebody; slightly modified here)
void RGBScale(char *srcRGB,int swid,int shgt,char *dstRGB,int dwid,int dhgt)
// 24-bit RGB scaling; no Alpha: RGBRGBRGB
// No boxfiltering, no interpolation, just raw speed.
// Can zoom in and out, arbitrarily
{
  int advWid,advHgt;
  int x,y;
  char *srcRGB0;			// Holds the line

  advHgt=0;
  srcRGB0=srcRGB;
  for(y=0;y<dhgt;y++)
  { advWid=0;
    srcRGB=srcRGB0;                     // Source hor. line
    for(x=0;x<dwid;x++)
    { //*dstRGB++=*srcRGB;                // Write a pixel
      *dstRGB++=srcRGB[0];
      *dstRGB++=srcRGB[1];
      *dstRGB++=srcRGB[2];
      advWid+=swid;
      while(advWid>dwid)
      { srcRGB+=3;                      // Next source pixel
        advWid-=dwid;
      }
    }
    advHgt+=shgt;
    while(advHgt>dhgt)
    { srcRGB0+=swid*3;                  // Move to next source line
      advHgt-=dhgt;
    }
  }
}
void RGBAScale(char *srcRGB,int swid,int shgt,char *dstRGB,int dwid,int dhgt)
// 32-bit RGBA scaling; RGBARGBARGBA
// No boxfiltering, no interpolation, just raw speed.
// Can zoom in and out, arbitrarily
{
  int advWid,advHgt;
  int x,y;
  char *srcRGB0;                        // Holds the line

  advHgt=0;
  srcRGB0=srcRGB;
  for(y=0;y<dhgt;y++)
  { advWid=0;
    srcRGB=srcRGB0;                     // Source hor. line
    for(x=0;x<dwid;x++)
    { //*dstRGB++=*srcRGB;                // Write a pixel
      *dstRGB++=srcRGB[0];
      *dstRGB++=srcRGB[1];
      *dstRGB++=srcRGB[2];
      *dstRGB++=srcRGB[3];
      advWid+=swid;
      while(advWid>dwid)
      { srcRGB+=4;                      // Next source pixel
        advWid-=dwid;
      }
    }
    advHgt+=shgt;
    while(advHgt>dhgt)
    { srcRGB0+=swid*4;                  // Move to next source line
      advHgt-=dhgt;
    }
  }
}

void QPolygon::DefineTexture(QBitMap *srcBM,QRect *r)
// Filter out the rectangle of the bitmap, and use
// it as a texture for this polygon
{ int area;
  qdbg("QPolygon::DefineTexture\n");
  if(texture)qfree(texture);
  area=r->wid*r->hgt;
  if(!area)
  {noshow:
    flags&=~TEXTURE;
    return;
  }
  texture=(char*)qcalloc(area*4);	// RGBA
  if(!texture)
  { qwarn("No memory for polygon texture"); goto noshow; }
//qdbg("QPolygon::DefineTexture RET early\n");
//return;

  int   x,y;
  char *p=texture,*buf=srcBM->GetBuffer(),*s;
  int ihgt=srcBM->GetHeight();
  int bit,expWid,expHgt;

  for(y=r->y;y<r->y+r->hgt;y++)
  { s=buf+(ihgt-y-1)*srcBM->GetWidth()*4+r->x*4;   // RGBA 4-byte
    //s=buf+y*srcBM->GetWidth()*4+r->x*4;		// RGBA
    for(x=0;x<r->wid;x++)
    { *p++=*s++;
      *p++=*s++;
      *p++=*s;
      //*p++=255;
      //*p++=x;
      s+=2;
    }
  }
  //QQuickSave("texture2.dmp",texture,1000);
  // Resize if necessary to 2^n
  bit=HiBit(r->wid);
  expWid=(1<<bit);
  if(r->wid!=expWid)
  { //qdbg("QPoly:DefTxtr; must expand X %d (!=%d)\n",r->wid,expWid);
    //expWid=1<<(bit+1);
    expWid=1<<(bit+0);
    //qdbg("  to %d\n",expWid);
  }
  bit=HiBit(r->hgt);
  expHgt=(1<<bit);
  if(r->hgt!=expHgt)
  { //qdbg("QPoly:DefTxtr; must expand X %d (!=%d)\n",r->wid,expWid);
    //expWid=1<<(bit+1);
    expHgt=1<<(bit+0);
    //qdbg("  to %d\n",expWid);
  }
  if(r->wid!=expWid||r->hgt!=expHgt)
  { qdbg("QPoly:DefTxtr; must resize %dx%d to %dx%d\n",r->wid,r->hgt,
      expWid,expHgt);
    //qdbg("  alloc expWid=%d, r->hgt=%d\n",expWid,r->hgt);
    char *newTexture=(char*)qcalloc(expWid*expHgt*5);
    if(!newTexture)
    { qwarn("No texture memory for resize");
      goto noshow;
    }
#ifdef DONTKNOWABOUTTHISONE
    int e;
    e=gluScaleImage(GL_RGB,r->wid,r->hgt,GL_UNSIGNED_BYTE,texture,
      expWid,expHgt,GL_UNSIGNED_BYTE,newTexture);
    qdbg("  gluScaleImage: %s\n",gluErrorString(e));
#else
    // Scale by just plucking out the right lines
    qdbg("  RGBScale\n");
    RGBScale(texture,r->wid,r->hgt,newTexture,expWid,expHgt);
#endif
    //QQuickSave("texture3.dmp",newTexture,1000);
    qdbg("  free texture\n");
    qfree(texture);
    texture=newTexture;
  }
  //qdbg("  set flags\n");
  flags|=TEXTURE;
  textureSize.x=expWid;
  textureSize.y=expHgt;
  qdbg("QPolygon::DefineTexture RET\n");
}

void QPolygon::Paint()
// Paints polygon in the current state
{ int i;
  QCanvas *cv=app->GetBC()->GetCanvas();
  cv->Select();
  if(flags&TEXTURE)
  { //qdbg("QPoly: 2D texture %dx%d\n",textureSize.x,textureSize.y);
    if(texID)
    {
      // Select texture instead of redefining it
      glBindTexture(GL_TEXTURE_2D,texID);
    } else
    {
      // Create texture object
      glGenTextures(1,(GLuint*)&texID);
      glBindTexture(GL_TEXTURE_2D,texID);

      glPixelStorei(GL_UNPACK_ALIGNMENT,1);
      glTexImage2D(GL_TEXTURE_2D,0,4,textureSize.x,textureSize.y,0,GL_RGB,
        GL_UNSIGNED_BYTE,texture);
      /*glTexImage2D(GL_TEXTURE_2D,0,4,TWID,THGT,0,GL_RGBA,
        GL_UNSIGNED_BYTE,imgTexture->GetBuffer());*/
      //glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
      //glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
      glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    
      //glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      // Linear interpolation; accelerated on O2
      glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
      //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  
    }
    glEnable(GL_TEXTURE_2D);
  }

  if(vertices==4)
  { glBegin(GL_QUADS);
    //qdbg("QPoly paint\n");
    for(i=0;i<4;i++)
    { if(flags&TEXTURE)glTexCoord2f(tx[i],ty[i]);
      else glColor3f(1,0,1);
      glVertex3f(x[i],y[i],z[i]);
    }
    glEnd();
  }
}

