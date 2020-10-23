/*
 * QSerial
 * 15-09-97: Created!
 * 29-02-00: No longer old termios, but new functions
 * 20-11-00: Win32 support
 * NOTES:
 * - (NT) CharsWaiting() returns 1 tops.
 * - (NT) CharsWaiting() is implemented crappy; use sproc() and block read
 * (C) MG/RVG
 */

// For O2, use old termio (new termio has c_ispeed member to set baudrate)
//#define _OLD_TERMIOS

// Use hardware handshaking?
//#define USE_RTSCTS

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(__sgi)
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <sys/termio.h>
#include <sys/stermio.h>
#include <stropts.h>
#include <sys/z8530.h>
#endif
#if defined(linux)
#include <termios.h>
#include <sys/ioctl.h>
#endif
#include <string.h>
#include <qlib/serial.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QSerial::QSerial(int iunit,int ibps,int iflags)
{
  flags=iflags;
  bps=ibps;
  unit=iunit;
  fd=0;
  bOpen=FALSE;
#ifdef WIN32
  hComm=0;
#endif
  Open();
}
QSerial::~QSerial()
{
#ifdef WIN32
  if(hComm)
    CloseHandle(hComm);
#else
  // Unix
  if(fd)
    close(fd);
#endif
}

bool QSerial::Open()
{ //struct termio termio;
#ifndef WIN32
  struct termios t;
#endif
  char fname[80];

  //
  // Open serial device file
  //
  bOpen=FALSE;
#ifdef WIN32
  sprintf(fname,"COM%d",unit);
  hComm=CreateFile(fname,GENERIC_READ|GENERIC_WRITE,0,0,
    OPEN_EXISTING,0,0);
  if(hComm==INVALID_HANDLE_VALUE)
  {
    // Failed to open
    qwarn("Failed to open serial port %s",fname);
    return FALSE;
  }
#else
  // Irix
  sprintf(fname,"/dev/ttyd%d",unit);
  fd=open(fname,O_RDWR|O_NOCTTY|O_NONBLOCK);
  if(fd<0)
  { perror("qserial");
    return FALSE;
  }
#endif
  
#ifdef FUTURE_RS422_MODE
  {
    struct strioctl str;
    int arg;
    str.ic_cmd = SIOC_RS422;
    str.ic_timout = 0;
    str.ic_len = 4;
    arg = RS422_ON;
    str.ic_dp = (char *)&arg;
    if (ioctl(fd, I_STR, &str) < 0) 
      qerr("QSerial: setting RS422 mode");
  }
  // RS422 for deck control also needs:
  // - PARENB|PARODD in c_cflag
#endif

#ifndef WIN32
  // Irix
  if(tcgetattr(fd,&t))
    qerr("QSerial: getattr");
#endif

  bOpen=TRUE;

  // Set speed
#ifdef WIN32
  // Settings for Win32
  COMMCONFIG cfg;
  DWORD cfgSize;
  char  buf[80];

  cfgSize=sizeof(cfg);
  GetCommConfig(hComm,&cfg,&cfgSize);
  //CommConfigDialog(fname,0,&cfg);
  // Set baudrate and bits etc.
  // Note that BuildCommDCB() clears XON/XOFF and hardware control by default
  sprintf(buf,"baud=%d parity=N data=8 stop=1",bps);
  if(!BuildCommDCB(buf,&cfg.dcb))
  { qerr("Can't build comm dcb; %s",buf);
  }
  if(!SetCommState(hComm,&cfg.dcb))
  {
	  qerr("Can't set comm state");
  }
  /*sprintf(buf,"bps=%d, xio=%d/%d\n",cfg.dcb.BaudRate,cfg.dcb.fOutX,cfg.dcb.fInX);
  AfxMessageBox(buf);*/

  // Set communication timeouts (NT)
  COMMTIMEOUTS tOut;
  GetCommTimeouts(hComm,&tOut);
  // Make timeout so that:
  // - return immediately with buffered characters
  tOut.ReadIntervalTimeout=MAXDWORD;
  tOut.ReadTotalTimeoutMultiplier=0;
  tOut.ReadTotalTimeoutConstant=0;
  SetCommTimeouts(hComm,&tOut);
#else
  int  n;

  // Irix
  if(bps==1200)
  { if(cfsetispeed(&t,B1200))
      qerr("QSerial: cfsetispeed 1200");
    if(cfsetospeed(&t,B1200))
      qerr("QSerial: cfsetispeed 1200");
  } else if(bps==9600)
  { if(cfsetispeed(&t,B9600))
      qerr("QSerial: cfsetispeed 9600");
    if(cfsetospeed(&t,B9600))
      qerr("QSerial: cfsetispeed 9600");
  } else if(bps==38400)
  { n=B38400;
    if(cfsetispeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
    if(cfsetospeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
  } else if(bps==31250)
  { // MIDI speed
    n=bps;
    if(cfsetispeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
    if(cfsetospeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
  } else
  { // Any other speed; just try it
    n=bps;
    if(cfsetispeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
    if(cfsetospeed(&t,n))qerr("QSerial: cfsetispeed %d",bps);
  } //else qerr("QSerial: baud rates!=1200/9600/38k4 not yet supported");

  // set 'raw' mode as it is set by "stty raw"
  t.c_cflag&=~(CSIZE);
  t.c_cflag|=CS8|CREAD;         // PARENB|PARODD for RS422
#ifdef USE_RTSCTS
  // RTS/CTS
  t.c_cflag|=CNEW_RTSCTS;
#endif
  
  t.c_iflag&=~(-1);
  t.c_iflag|=0;
  //t.c_lflag &= ~(ISIG|ICANON|XCASE); // RS422?
  t.c_lflag&=~(ISIG|ICANON|XCASE);
  t.c_lflag|=0;
  t.c_oflag&=~OPOST;
  t.c_oflag|=0;
  t.c_cc[VMIN]=0;
  t.c_cc[VTIME]=0;
  
  // turn off echo
  t.c_lflag &= ~(ECHO);

  // From old serial code (termio)
  //t.c_lflag=0;
  //t.c_oflag=0;
 
  if(!(flags&WRITEONLY))
    t.c_cflag|=CLOCAL;
  if(flags&STOPBITS2)
    t.c_cflag|=CSTOPB;
  
#ifdef OBS_OLD_TERMIO
  termio.c_iflag=0;
  termio.c_oflag=0;
  if(bps==1200)termio.c_cflag=B1200|CS8|CREAD;
  else if(bps==9600)termio.c_cflag=B9600|CS8|CREAD;
  else qerr("QSerial: baud rates != 1200 or 9600 not yet supported");
  if(!(flags&WRITEONLY))
    termio.c_cflag|=CLOCAL;
  if(flags&STOPBITS2)
    termio.c_cflag|=CSTOPB;
  termio.c_lflag=0;
  termio.c_line=0;
#ifdef OBS
  termio.c_cc[0]=0;
#else
  n=0;
  termio.c_cc[n++]=VTIME; termio.c_cc[n++]=0;
  termio.c_cc[n++]=VMIN; termio.c_cc[n++]=0;
  termio.c_cc[n++]=0;
#endif
  if(ioctl(fd,TCSETA,&termio)==-1)
  {
    qerr("QSerial:Open(); ioctl TCSETA failed (%s)",strerror(oserror());
  }
#endif
#ifdef ND
  // Speed
  if(ioctl(fd,MIN,0)==-1)perror("MIN");
  if(ioctl(fd,TIME,0)==-1)perror("TIME");
#endif
  
  // Set attribs
  if(tcsetattr(fd,TCSANOW,&t))
    qerr("QSerial: setattr");
    
  // Modify buffering time
  // This is normally set to the value in /var/sysgen/master.d/sduart
  // (int duart_rsrv_duration = (HZ/50)
  // Which is a kernel tunable parameter
  // O2's can do this with a ioctl() call
  // Time==0 means lowest buffering possible
#if defined(__sgi)
  ioctl(fd,SIOC_ITIMER,0);
#endif

#endif

  return bOpen;
}

int QSerial::Write(char *s,int len)
// Returns #chars written
{
  
  QASSERT_0(s);
  if(!s)return 0;
  if(len==-1)len=strlen(s);
//QHexDump(s,len);
#ifdef WIN32
  DWORD written;
  if(hComm==0)return 0;
  if(!WriteFile(hComm,s,len,&written,0))
  {
    qwarn("Can't write com");
  }
#else
  int r;

  if(fd==0)return 0;
  r=write(fd,s,len);
  if(r<len)
  { qwarn("Write %d bytes to serial; written only %d\n",len,r);
  }
#endif
  return len;
}

int QSerial::Read(char *s,int len)
// Returns #chars read
{
  int r;
#ifdef WIN32
  DWORD nRead;
#endif

//qdbg("QSerial:Read\n");
  if(len<=0)return -1;
  if(flags&FUTUREC)
  { *s++=futureC;
    len--;
    r=0;		// Adjusted later here (to +1)
  } else
  { r=0;
  }
  if(len>0)
  {
#ifdef WIN32
    if(!ReadFile(hComm,s,len,&nRead,0))
    {
      qwarn("Can't read com");
    } else
    {
      r=nRead;
    }
#else
    // Unix
    r=read(fd,s,len);
#endif
  }
  // Adjust for cached char
  if(flags&FUTUREC)
  { if(r!=-1)r++;
    flags&=~FUTUREC;
  }
  //qdbg("QSerial:Read returns %d\n",r);
  return r;
}

int QSerial::CharsWaiting()
// Check for any activity
{
  int r;
//qdbg("QSerial:CharsWaiting\n");
#ifdef WIN32
  if(flags&FUTUREC)
  { // Cached char
    return 1;
  }
#else
  // Unix does actual #chars
  if(ioctl(fd,FIONREAD,&r)<0)
  {
    // ioctl error
    return 0;
  }
  return r;
#endif

#ifdef WIN32
  r=Read(&futureC,1);
  if(r<1)
  { // Nothing read
    return 0;
  }
  // Something was there
  flags|=FUTUREC;
  return 1;
#endif
}

int QSerial::GetStatusLines()
// TIOCM_xxx
// NYI for Win32
{ int n;
#ifdef WIN32
  n=0;
#else
  ioctl(fd,TIOCMGET,&n);
#endif
  return n;
}

//#define TEST
#ifdef TEST
void main(int argc,char **argv)
{
  QSerial *s;
  s=new QSerial(QSerial::UNIT2,9600);
  s->Write("This is UNIT2\n");
  delete s;
}
#endif

