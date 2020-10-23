/*
 * QNServer - network server
 * 31-07-01: Created!
 * (c) MarketGraph/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNServer::QNServer()
{
  id=0;
  stage=0;
}
QNServer::~QNServer()
{
  Close();
}

/*******
* Open *
*******/
bool QNServer::Open(int port,int type)
// Opens a server on port #'port'
{
  return TRUE;
}

void QNServer::Close()
{
}

