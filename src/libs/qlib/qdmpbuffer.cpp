/*
 * QDMPBuffer - definition/implementation
 * NOTES:
 * - Generated by mkclass
 * LOG:
 * 08-10-98: Bug removed from CopyTexture(); used to eat memory.
 * (C) ??-??-98 MarketGraph/RVG
 */

#include <qlib/dmpbuffer.h>
#include <qlib/dmbpool.h>
#include <qlib/error.h>
#include <qlib/app.h>
#include <qlib/opengl.h>
#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DMCHK(s,e)\
  if((e)!=DM_SUCCESS)QShowDMErrors(s)
  //if((s)!=DM_SUCCESS)qerr("DM failure: %s",e)

/** HELP **/

#define ARRAY_COUNT(a)    (sizeof(a)/sizeof(a[0]))

#undef  DBG_CLASS
#define DBG_CLASS "QDMPBuffer"

static void DbgPrintFBConfig(GLXFBConfigSGIX config)
{
#ifndef WIN32
    static struct { int attrib; const char* name; } attribs [] = {
        { GLX_BUFFER_SIZE, "GLX_BUFFER_SIZE" },
        { GLX_LEVEL, "GLX_LEVEL" },
        { GLX_DOUBLEBUFFER, "GLX_DOUBLEBUFFER" },
        { GLX_AUX_BUFFERS, "GLX_AUX_BUFFERS" },
        { GLX_RED_SIZE, "GLX_RED_SIZE" },
        { GLX_GREEN_SIZE, "GLX_GREEN_SIZE" },
        { GLX_BLUE_SIZE, "GLX_BLUE_SIZE" },
        { GLX_ALPHA_SIZE, "GLX_ALPHA_SIZE" },
        { GLX_DEPTH_SIZE, "GLX_DEPTH_SIZE" },
        { GLX_DRAWABLE_TYPE_SGIX, "GLX_DRAWABLE_TYPE_SGIX" },
        { GLX_RENDER_TYPE_SGIX, "GLX_RENDER_TYPE_SGIX" },
        { GLX_X_RENDERABLE_SGIX, "GLX_X_RENDERABLE_SGIX" },
        { GLX_MAX_PBUFFER_WIDTH_SGIX, "GLX_MAX_PBUFFER_WIDTH_SGIX" },
        { GLX_MAX_PBUFFER_HEIGHT_SGIX, "GLX_MAX_PBUFFER_HEIGHT_SGIX"},
    };
    int i;

    for ( i = 0;  i < ARRAY_COUNT( attribs );  i++ )
    {
        int value;
        int errorCode = glXGetFBConfigAttribSGIX(
            XDSP, config, attribs[i].attrib, &value );
        //if(errorCode==0,"glXGetFBConfigAttribSGIX failed" );
        printf( "    %24s = %d\n", attribs[i].name, value );
    }
#endif
}

QDMPBuffer::QDMPBuffer(int w,int h)
{
#ifndef WIN32
  // FB attribs
  GLint attrib32[] = {GLX_DOUBLEBUFFER, GL_FALSE,
                      GLX_RED_SIZE, 8,
                      GLX_GREEN_SIZE, 8,
                      GLX_BLUE_SIZE, 8,
                      GLX_ALPHA_SIZE, 8,
                      GLX_DRAWABLE_TYPE_SGIX, GLX_PBUFFER_BIT_SGIX,
                      None};
  // PBuffer attribs
  GLint pbattrib[] = {GLX_DIGITAL_MEDIA_PBUFFER_SGIX, GL_TRUE,
                        GLX_PRESERVED_CONTENTS_SGIX, GL_TRUE,
                        None};

  GLint *attrib,n;
  int redSize;
  Display *dpy=XDSP;

  DBG_C("ctor")

  // Store information
  wid=w; hgt=h;

  // Get FB config
  attrib=attrib32;
  redSize=8;

  fbConfig=glXChooseFBConfigSGIX(dpy,DefaultScreen(dpy),attrib,&n);

  if(!fbConfig)
  { fprintf(stderr, "can't find suitable FBConfig\n");
    exit(EXIT_FAILURE);
  }
  while(n)
  { int bits;
    //DEBUG_F(printf("Test this config:\n"));
    //DEBUG_F(DbgPrintFBConfig(*fbConfig));
    glXGetFBConfigAttribSGIX(dpy, *fbConfig, GLX_RED_SIZE, &bits);
    if (bits == redSize) break;
    fbConfig++; n--;
  }
  if(!n)
  { fprintf(stderr, "can't find suitable FBConfig\n");
    exit(EXIT_FAILURE);
  }
 
  //qdbg("  create ctx with config\n");
  GLXContext pBufCtx;
  pBufCtx=glXCreateContextWithConfigSGIX(dpy, *fbConfig,
    GLX_RGBA_TYPE_SGIX,QCV->GetGLContext()->GetGLXContext(),TRUE);
  if (!pBufCtx)
  { fprintf(stderr, "can't create context\n");
    exit(EXIT_FAILURE);
  }
  glCtx=new QGLContext(pBufCtx);

  //qdbg("  create pbuffer\n");
  pBuffer=glXCreateGLXPbufferSGIX(dpy,*fbConfig,w,h,pbattrib);
  if(!pBuffer)
  { qerr("Can't create pbuffer (glXCreateGLXPbufferSGIX)");
  }

  // Create DMparams to describe pbuffer
  //qdbg("  create pbuffer gfx params\n");
  DMCHK("QDMPBuffer",dmParamsCreate(&gfxParams));
  DMCHK("QDMPBuffer",
    dmSetImageDefaults(gfxParams,wid,hgt,DM_IMAGE_PACKING_RGBA));
  dmParamsSetEnum(gfxParams,DM_IMAGE_LAYOUT,DM_IMAGE_LAYOUT_GRAPHICS);
  //dmParamsSetEnum(gfxParams,DM_IMAGE_ORIENTATION,DM_BOTTOM_TO_TOP);
  dmParamsSetEnum(gfxParams,DM_IMAGE_ORIENTATION,DM_TOP_TO_BOTTOM);
  //DbgPrintParams(gfxParams);
  //DMparams *pm;
  //dmParamsCreate(&pm);
  //dmBufferGetGLPoolParams(gfxParams,pm);
  //qdbg("GetGLPoolParams results in:\n");
  //DbgPrintParams(pm);
//#ifdef ND
  /*
  **  The intial associate must occur before a dmpuffer can
  **  be made current to a context.
  */
  //glXAssociateDMPbufferSGIX(XDSP, pBuffer,gfxParams, *dmbuffer);

  //glXMakeCurrent(dpy, pBuffer, pBufCtx);   
//#endif
#endif
}

QDMPBuffer::~QDMPBuffer()
{
}

int  QDMPBuffer::GetFD()
{
  return 0;
}

void QDMPBuffer::AddProducerParams(QDMBPool *p)
{
#ifndef WIN32
  DMCHK("QDMPB:AddPP",
    dmBufferGetGLPoolParams(gfxParams,p->GetCreateParams()->GetDMparams()));
#endif
}
void QDMPBuffer::AddConsumerParams(QDMBPool *p)
{
#ifndef WIN32
  DMCHK("QDMPB:AddCP",
    dmBufferGetGLPoolParams(gfxParams,p->GetCreateParams()->GetDMparams()));
#endif
}

void QDMPBuffer::RegisterPool(QDMBPool *pool)
{
}

void QDMPBuffer::Start()
{
}

//void QDMPBuffer::Associate(DMbuffer dmbuffer)
void QDMPBuffer::Associate(QDMBuffer *dmb)
// May use 0 to indicate that you want to free the association with ANY dmb
{
#ifndef WIN32
  DMbuffer dmbuf;

  if(dmb)dmbuf=dmb->GetDMbuffer();
  else   dmbuf=0;
//qdbg("QDMPBuffer::Associate %p\n",dmbuf);
  if(!glXAssociateDMPbufferSGIX(XDSP,pBuffer,gfxParams,dmbuf))
    qerr("QDMPBuffer::Associate failed");
//qdbg("QDMPBuffer::Associate RET\n");
#endif
}

void QDMPBuffer::CopyTexture(QGLContext *dstGL)
//GLXContext dstCtx)
// Copy from our pbuffer to destination GL context texture
// Normally this will copy by reference on the O2
{
#ifndef WIN32
  // This next one loses memory on IRIX 6.3/O2!?
  //glXMakeCurrent(XDSP,pBuffer,dstGL->GetGLXContext());
  glXMakeCurrentReadSGI(XDSP,
    dstGL->GetDrawable()->GetDrawable(),
    pBuffer,
    dstGL->GetGLXContext());
  // Hack to get cached GLXContext right
  SetCurrentQGLContext(glCtx);

QShowGLErrors("QDMPBuf: glXMakeCurrentRead()");
  //qdbg("glCopyTexSubImage2DEXT\n");
#ifndef ND_TEST
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,
    0,0,		// offset
    0,0,		// x,y
    wid,hgt);
//qdbg("QDDMPBuf: %dx%d\n",wid,hgt);
QShowGLErrors("QDMPBuf: glCopyTexSubImage2D()");
#else
  glCopyPixels(0,0,wid,hgt,GL_COLOR);
QShowGLErrors("QDMPBuf: glCopyPixels()");
#endif
  //qdbg("glCopyTexSubImage2DEXT\n");
#endif
}

void QDMPBuffer::Select()
{
#ifndef WIN32
  //qwarn("QDMPBuffer::Select() called; should do this with QGLContext()");
  glXMakeCurrent(XDSP,pBuffer,glCtx->GetGLXContext());
  // Hack to get cached GLXContext right
  SetCurrentQGLContext(glCtx);
#endif
}

