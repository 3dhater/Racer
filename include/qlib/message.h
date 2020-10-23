// qlib/qmessage.h

#ifndef __QLIB_QMESSAGE_H
#define __QLIB_QMESSAGE_H

#include <qlib/types.h>

void qmsg(const char *fmt,...);
void qerr(const char *fmt,...);
void qwarn(const char *fmt,...);
void qdbg(const char *fmt,...);

typedef void (*QMessageHandler)(string s);

QMessageHandler QSetMessageHandler(QMessageHandler h);
QMessageHandler QSetErrorHandler(QMessageHandler h);
QMessageHandler QSetWarningHandler(QMessageHandler h);
QMessageHandler QSetDebugHandler(QMessageHandler h);

// System-wide logging

#ifdef WIN32
#define QLOG_FILE		"QLOG.txt"
#define QLOG_OLDFILE		"QLOG_OLD.txt"
#else
#define QLOG_FILE		"HOME:QLOG"
#define QLOG_OLDFILE		"HOME:QLOG_OLD"
#endif
#define QLOG_MAXSIZE		1048576		// Size in bytes before rotate

// Autolog mask
#define QAUTOLOG_MSG		1		// qmsg()
#define QAUTOLOG_ERR		2		// Errors (default: on)
#define QAUTOLOG_WARN		4		// Warnings (qwarn)
#define QAUTOLOG_DEFAULT 	(QAUTOLOG_ERR|QAUTOLOG_WARN) // Default logging

// Log types
#define QLOG_ERR		1		// Error
#define QLOG_WARN		2		// Warnings
#define QLOG_CRIT		4		// Critical (system funct.)
#define QLOG_NOTICE		8		// Smart remarks
#define QLOG_NOTE		8		// alias
#define QLOG_INFO		16		// Informational (flow)
#define QLOG_DEBUG		32		// Debug information
#define QLOG_NOCONSOLE		64		// Don't write to console
#define QLOG_NOSTDERR		128		// Don't write to stderr
#define QLOG_NOLOG		256		// Don't write to QLOG

#define QLOG_TYPE_MASK		0x3F		// QLOG_ERR..QLOG_DEBUG bits

// Potential name conflict with math.h
#undef qlog
void qlog(int msgType,const char *fmt,...);
void qautolog(int mask);

#endif
