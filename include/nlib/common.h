// nlib/common.h

#ifndef __NLIB_COMMON_H
#define __NLIB_COMMON_H

// Include commonly used files
#include <nlib/client.h>
#include <nlib/server.h>
#include <nlib/socket.h>
#include <nlib/channel.h>
#include <nlib/global.h>

// Prototypes for different systems
#if defined(__sgi) || defined(linux)
// gethostbyname()
#include <netdb.h>
#endif

#endif
