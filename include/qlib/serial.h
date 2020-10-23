// serial.h

#ifndef __QLIB_SERIAL_H
#define __QLIB_SERIAL_H

#include <qlib/object.h>
#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif

class QSerial : public QObject
{
 protected:
  int  unit,
       bps,
       flags;
  bool bOpen;
  int  fd;
  char futureC;			// Next char to read (nasty trick)
#ifdef WIN32
  HANDLE hComm;
#endif

 public:
  // Flags
  enum Flags
  { STOPBITS2=1,	// Use 2 stopbits (otherwise 1)
    WRITEONLY=2,	// No reading
    FUTUREC=4		// 'futureC' is valid
  };
  // Units
  enum Units
  { UNIT1=1,		// Mostly login console
    UNIT2=2		// First usable port
  };
 public:
  QSerial(int unit,int bps,int flags=0);
  ~QSerial();

  bool Open();
  int  Read(char *s,int len);
  int  Write(char *s,int len=-1);
  bool IsOpen(){ return bOpen; }

  int  GetFD(){ return fd; }
  int  CharsWaiting();
  int  GetStatusLines();
};

#endif
