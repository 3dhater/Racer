/*
 * QCompr - compressing/decompressing
 * 02-04-97: Created!
 * (C) MG/RVG
 */

#include <qlib/compr.h>
#include <unistd.h>
#include <stdio.h>

// Base class

QCompDecomp::QCompDecomp()
{
  cprHdl=0;
  dataBuf=frameBuf=0;
}
QCompDecomp::~QCompDecomp()
{
  printf("qcd dtor\n");
}

/****************
* QDECOMPRESSOR *
****************/
QDecompressor::QDecompressor(int cprType)
  : QCompDecomp()
{
  int r;
  r=clOpenDecompressor(cprType,&cprHdl);
}
QDecompressor::~QDecompressor()
{
  printf("QDecompressor dtor\n");
  if(dataBuf)clDestroyBuf(dataBuf);
  if(cprHdl)clCloseDecompressor(cprHdl);
}

bool QDecompressor::CreateDataBuffer(int size)
{
  // CL_DATA buffers always have size <size,1> (the 1)
  // No ptr is used; use clQueryFree (QueryFree)
  dataBuf=clCreateBuf(cprHdl,CL_DATA,size,1,NULL);
  if(dataBuf)return TRUE;
  return FALSE;
}

bool QDecompressor::SetImageSize(int wid,int hgt)
{ int p[4],n=0;
  p[n++]=CL_IMAGE_WIDTH;  p[n++]=wid;
  p[n++]=CL_IMAGE_HEIGHT; p[n++]=hgt;
  clSetParams(cprHdl,p,n);
  return TRUE;
}

bool QDecompressor::SetInternalImageSize(int wid,int hgt)
{ int p[4],n=0;
  p[n++]=CL_INTERNAL_IMAGE_WIDTH;  p[n++]=wid;
  p[n++]=CL_INTERNAL_IMAGE_HEIGHT; p[n++]=hgt;
  clSetParams(cprHdl,p,n);
  return TRUE;
}

bool QDecompressor::SetTransferMode(int transferMode)
{ int p[4],n=0;
  p[n++]=CL_COSMO_VIDEO_TRANSFER_MODE;
  p[n++]=transferMode;
  p[n++]=CL_ENABLE_IMAGEINFO;
  p[n++]=TRUE;
  clSetParams(cprHdl,p,n);
  return TRUE;
}

bool QDecompressor::Start()
{
  if(!dataBuf)return FALSE;
  clDecompress(cprHdl,CL_CONTINUOUS_NONBLOCK,0,0,CL_EXTERNAL_DEVICE);
  return TRUE;
}
bool QDecompressor::End()
{
  // Kill threads
  if(dataBuf)clDoneUpdatingHead(dataBuf);
  //if(frameBuf)clDoneUpdatingHead(frameBuf);
  return TRUE;
}

int QDecompressor::QueryFree(void **pFree,int *wrap,int space)
// Returns free space upto end of buffer
// If free space exist from the start of the buffer, *wrap will be >0
// pFree receives the location to put your data
// NOTE: Changed behavior if space>0:
// Normally, clQueryFree would block if 'space' would never be reached
// at the end of the buffer, not counting free space at the start of the
// buffer. Here, I wait until the TOTAL of free space (at the end PLUS
// at the START of the buffer) is at least 'space'.
// I think this makes more sense. Note that still, the number of free
// space at the END of the buffer is returned, not the TOTAL space.
// Also, the argument order has been changed to allow for a default 'space=0'
{
  int free;
  if(!cprHdl)return 0;
  if(!wrap)return 0;
  if(!pFree)return 0;
  // Try until at least 'space' is free
  /*
  while(1)
  { free=clQueryFree(dataBuf,0,pFree,wrap);
    if(space==0||free+*wrap>=space)break;
    sginap(1);
  }
  */
  free=clQueryFree(dataBuf,0,pFree,wrap);
  return free;
}

void QDecompressor::UpdateHead(int amount)
{
  clUpdateHead(dataBuf,amount);
}

