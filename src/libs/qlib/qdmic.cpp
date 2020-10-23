/*
 * QDMIC - dmIC encapsulation
 * 08-09-98: Created!
 * (C) MG/RVG
 */

#include <qlib/dmic.h>
#include <qlib/dmbpool.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QDMIC"

QDMIC::QDMIC(int w,int h)
{
  src=0;
  dst=0;
  conv=0;

  fd = -1;
  speed = DM_IC_SPEED_REALTIME;
  direction =  DM_IC_CODE_DIRECTION_UNDEFINED;
  srcPacking = DM_IMAGE_PACKING_CbYCrY;
  dstPacking = DM_IMAGE_PACKING_CbYCrY;
  //srcPacking = DM_IMAGE_PACKING_RGBX;
  //dstPacking = DM_IMAGE_PACKING_RGBX;
  srcCompression = DM_IMAGE_UNCOMPRESSED;
  dstCompression = DM_IMAGE_UNCOMPRESSED;
  quality=0.7;
  wid=w;
  hgt=h;
  ctx=0;
}
QDMIC::~QDMIC()
{
  delete src;
  delete dst;
  delete conv;
  if(ctx)
   dmICDestroy(ctx);
}

int QDMIC::GetFrameSize()
{
  return bufSize;
}

bool QDMIC::Create()
{
  // Source parameters
  src=new QDMParams();
  src->SetImageDefaults(wid,hgt,srcPacking);
  src->SetString(DM_IMAGE_COMPRESSION,srcCompression);
  src->SetEnum(DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  //src->SetEnum(DM_IMAGE_LAYOUT,DM_IMAGE_LAYOUT_LINEAR);

  dst=new QDMParams();
  dst->SetImageDefaults(wid,hgt,dstPacking);
  dst->SetString(DM_IMAGE_COMPRESSION,dstCompression);
  dst->SetEnum(DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
  //dst->SetEnum(DM_IMAGE_LAYOUT,DM_IMAGE_LAYOUT_GRAPHICS);

  conv=new QDMParams();
  conv->SetEnum(DM_IC_SPEED,speed);
  conv->SetFloat(DM_IMAGE_QUALITY_SPATIAL,quality);
  //conv->SetEnum(DM_IMAGE_LAYOUT,DM_IMAGE_LAYOUT_GRAPHICS);

//src->SetInt(DM_IMAGE_WIDTH,512);
//src->SetInt(DM_IMAGE_HEIGHT,256);
//dst->SetInt(DM_IMAGE_WIDTH,256);
//dst->SetInt(DM_IMAGE_HEIGHT,256);
//conv->SetInt(DM_IMAGE_WIDTH,720);
//conv->SetInt(DM_IMAGE_HEIGHT,288);

/*DEBUG_F(qdbg("QDMIC:Create parameters:\n"));
  DEBUG_F(qdbg("src:\n")); DEBUG_F(src->DbgPrint());
  DEBUG_F(qdbg("dst:\n")); DEBUG_F(dst->DbgPrint());
  DEBUG_F(qdbg("conv:\n")); DEBUG_F(conv->DbgPrint());
*/

  int id;
  id=dmICChooseConverter(src->GetDMparams(),dst->GetDMparams(),
    conv->GetDMparams());
  if(id<0)
  { qerr("QDMIC::Create() failed to choose converter");
    char detail[DM_MAX_ERROR_DETAIL];
    const char *s;
    int    err;
    s = dmGetError(&err, detail);
    qdbg("DM error %s (%d): %s\n", s,err,detail);
    return FALSE;
  }
  //qdbg("QDMIC::Create(); n=%d\n",id);
  if(dmICCreate(id,&ctx)!=DM_SUCCESS)
  {fail_create:
    qerr("QDMIC::Create() failed to create converter");
    return FALSE;
  }
  if(ctx==0)goto fail_create;
  
  // Get information
  if(direction==DM_IC_CODE_DIRECTION_ENCODE)
  { bufSize=dmImageFrameSize(src->GetDMparams());
  } else
  {
    bufSize=dmImageFrameSize(dst->GetDMparams());
  }
  //qdbg("QDMIC::Create(); bufSize=%d\n",bufSize);

  // Set parameters
  dmICSetSrcParams(ctx,src->GetDMparams());
  dmICSetDstParams(ctx,dst->GetDMparams());
  dmICSetConvParams(ctx,conv->GetDMparams());

  // Cleanup
  return TRUE;
}

/**************************************
* INTERACTION WITH OTHER SYSTEM TYPES *
**************************************/
void QDMIC::AddProducerParams(QDMBPool *p)
{
  //DEBUG_F(qdbg("QDMIC::AddProducerParams\n"));
  //DEBUG_F(p->GetCreateParams()->DbgPrint());
  dmICGetDstPoolParams(ctx,p->GetCreateParams()->GetDMparams());
  //DEBUG_F(qdbg("  after:\n"));
  //DEBUG_F(p->GetCreateParams()->DbgPrint());
  //DEBUG_F(qdbg("-----\n"));
}
void QDMIC::AddConsumerParams(QDMBPool *p)
{
  //DEBUG_F(qdbg("QDMIC::AddConsumerParams\n"));
  //DEBUG_F(p->GetCreateParams()->DbgPrint());
  dmICGetSrcPoolParams(ctx,p->GetCreateParams()->GetDMparams());
  //DEBUG_F(qdbg("  after:\n"));
  //DEBUG_F(p->GetCreateParams()->DbgPrint());
  //DEBUG_F(qdbg("-----\n"));
}

/**************
* INFORMATION *
**************/
int QDMIC::GetFD()
{
  if(fd==-1)
  { fd=dmICGetDstQueueFD(ctx);
  }
  return fd;
}

int QDMIC::GetSrcFilled()
// Returns the #buffers that are still waiting to be processed.
// Note that GetSrcFilled()+GetDstFilled() is NOT necessarily the number
// of total buffers that are in the pipeline. Buffers CURRENTLY being
// processed are not counted by GetSrcFilled() or GetDstFilled()
{
  return dmICGetSrcQueueFilled(ctx);
}
int QDMIC::GetDstFilled()
// Returns the #buffers that are ready to be picked up.
// Note that GetSrcFilled()+GetDstFilled() is NOT necessarily the number
// of total buffers that are in the pipeline. Buffers CURRENTLY being
// processed are not counted by GetSrcFilled() or GetDstFilled()
{
  return dmICGetDstQueueFilled(ctx);
}

/*********
* PARAMS *
*********/
void QDMIC::SetSourceSize(int wid,int hgt)
// Define source image size
// May be called after creation.
{
  src->SetInt(DM_IMAGE_WIDTH,wid);
  src->SetInt(DM_IMAGE_HEIGHT,hgt);
  dmICSetSrcParams(ctx,src->GetDMparams());
}
void QDMIC::SetDestinationSize(int wid,int hgt)
// Define destination image size.
// Note that if this doesn't match with the source size, resizing may and
// probably WILL take place in software, thus slowing things down.
// (non-realtime zooming will occur)
{
  dst->SetInt(DM_IMAGE_WIDTH,wid);
  dst->SetInt(DM_IMAGE_HEIGHT,hgt);
  dmICSetDstParams(ctx,dst->GetDMparams());
}
void QDMIC::SetSize(int wid,int hgt)
// Set size of both src and dst
{
  SetSourceSize(wid,hgt);
  SetDestinationSize(wid,hgt);
}

/***********
* CONVERT! *
***********/
void QDMIC::Convert(DMbuffer buf)
// Send a buffer through the image converter
{
  DBG_C("Convert")
  DBG_ARG_P(buf);
  
  if(dmICSend(ctx,buf,0,0)!=DM_SUCCESS)
  { int e;
    const char *s;
    char e_long[DM_MAX_ERROR_DETAIL];
    s=dmGetError(&e,e_long);
    qerr("QDMIC::Convert() failed; %s (%s)",e_long,s);
  }
}

