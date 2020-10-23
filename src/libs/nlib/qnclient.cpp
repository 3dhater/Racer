/*
 * QNClient - network server
 * 31-07-01: Created!
 * (c) MarketGraph/RvG
 */

#include <nlib/common.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

QNClient::QNClient()
{
  id=0;
  stage=0;
}
QNClient::~QNClient()
{
  Close();
}

/*******
* Open *
*******/
bool QNClient::Open(int port,int type)
// Opens a server on port #'port'
{
  return TRUE;
}

void QNClient::Close()
{
}

