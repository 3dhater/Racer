/*
 * QDMBuffer - definition/implementation
 * NOTES:
 * - Tiny wrapper around DMbuffer concept
 * (C) 28-02-99 MarketGraph/RVG
 */

#include <qlib/dmbuffer.h>
//#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DMCHK(s,e)\
  if((e)!=DM_SUCCESS)QShowDMErrors(s)
  //if((s)!=DM_SUCCESS)qerr("DM failure: %s",e)

/** HELP **/

QDMBuffer::QDMBuffer(DMbuffer buf)
{
  dmbuffer=buf;
}
QDMBuffer::~QDMBuffer()
{
}

bool QDMBuffer::Attach()
{
  QASSERT_F(dmbuffer);
#ifndef WIN32
  dmBufferAttach(dmbuffer);
#endif
  return TRUE;
}

bool QDMBuffer::Free()
{
  QASSERT_F(dmbuffer);
#ifndef WIN32
  dmBufferFree(dmbuffer);
#endif
  return TRUE;
}

int QDMBuffer::GetSize()
{
  QASSERT_0(dmbuffer);
#ifdef WIN32
  return 0;
#else
  return dmBufferGetSize(dmbuffer);
#endif
}

