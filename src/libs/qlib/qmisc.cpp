/*
 * Q Misc support functions (flat)
 * 07-10-98: Created! (from qmem.cpp)
 * 25-12-00: Endian routines added (QPC2SGI_L, QSGI2PC_L etc).
 * BUGS:
 * - Linux is assumed to be of the same endianness as WIN32 (Intel)
 * (C) MarketGraph/RVG
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef WIN32
// _rmdir()
#include <direct.h>
// Sleep()
#include <windows.h>
#endif
#include <string.h>
#include <unistd.h>
#include <bstring.h>
#include <sys/schedctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <qlib/timer.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/***********
* SLEEPING *
***********/
void QNap(int nMicrosecs)
// Naps in a very system-friendly way (really sleeps)
// Timer is 100Hz on SGI systems, 1kHz on Win32
{
#ifdef WIN32
  Sleep(nMicrosecs*10);
#endif
#ifdef __sgi
  sginap(nMicrosecs);
#endif
#ifdef linux
  //nanosleep()
#endif
}
void QWait(int msecs)
// Wait a number of msecs in a reasonably system-friendly way
// Accuracy: about 10ms
{
#ifndef WIN32
  QTimer t;
  t.WaitForTime(0,msecs);
#endif
}

/*******************
* PROCESS PRIORITY *
*******************/
bool QSetThreadPriority(int n)
// Set this thread's priority; max=NDPHIMAX, upto NDPHIMIN
// Should run from root account, or setuid root (the exe)
// Warns if it fails (qwarn)
// NOTE: always sets root mode to FALSE (!)
// From dmplay.c
{ 
#ifdef OBS_WAS_STRANGE_METHOD
  bool r=TRUE;			// Optimistic, although not really
  //ASSERT_0(n==PRI_HIGH||n==PRI_MEDIUM||n==PRI_LOW);	// Illegal priority
  /*
   * set non-degrading priority
   * use high priority if running as root
   */
  /*
   * swap permissions so that we become root for a moment.
   * setreuid() should let us set real uid to effective uid = root
   * if the program is setuid root
   *
   */
  setreuid(geteuid(), getuid());
  setregid(getegid(), getgid());

  //if (schedctl(NDPRI, 0, NDPHIMIN)< 0)
  //if (schedctl(NDPRI, 0, NDPHIMAX)< 0)
  if (schedctl(NDPRI, 0, n)< 0)
  {
    qdbg("Can't set thread priority to %d; need root permissions\n",n);
    //fprintf(stderr, "** run as root to enable real time scheduling\n");
    r=FALSE;
  }

  /* swap permissions back, now we're just "joe user" */
  setreuid(geteuid(), getuid());
  setregid(getegid(), getgid());
#endif

#ifdef WIN32
  return FALSE;
#endif
#ifdef linux
  return FALSE;
#endif
#if defined(__sgi)
  if(!QSetRoot(TRUE))return FALSE;
  int r=schedctl(NDPRI, 0, n);
  QSetRoot(FALSE);
  if(r<0)
  { qerr("Can't set thread priority to %d",n);
    return FALSE;
  }
  return TRUE;
#endif
}
bool QSetRoot(bool root)
// Try to be superuser or not
// Not that although you can 'makeroot' your application, there
// are subtleties regarding file creation.
// Your file will be created under the name of your login name, NOT root.sys
// To change this, call QSetRoot(TRUE).
{
#ifdef WIN32
  return FALSE;
#else
  int r;
  if(!root)
  { r=setreuid(getuid(),getuid()); // geteuid() is better?
    if(r!=-1)
      r=setregid(getgid(),getgid());
  } else
  { // Become root.sys
    r=setreuid(getuid(),0);
    if(r!=-1)
      r=setregid(getgid(),0);
  }
  // Return succes or failure
  if(r==-1)return FALSE;
  return TRUE;
#endif
}

/**********
* DUMPING *
**********/
void QHexDump(char *p,int len)
{
  int i;
  qdbg("Hex %p:",p);
  for(i=0;i<len;i++)
    qdbg(" %02x",*p++);
  qdbg("\n");
}

/**********
* STRINGS *
**********/
#ifndef WIN32
void strupr(string s)
{
  for(;*s;s++)
  { if(*s>='a'&&*s<='z')*s=*s-'a'+'A';
  }
}
void strlwr(string s)
{
  for(;*s;s++)
  { if(*s>='A'&&*s<='Z')*s=*s-'A'+'a';
  }
}
#endif


/*******************
* Pattern matching *
*******************/
bool QMatch(cstring pat,cstring str)
/** From the Amiga, src:c.alib/match.c. Case-insensitive. **/
{ switch(*pat)
  { case   0: return !*str;
    case '?': return QMatch(pat+1,str+1);
    case '*': if(*(pat+1)==0)return 1;
              /* If there is no new deciding char behind *, it's TRUE. This
                 doesn't work 100%: look at *(!*c*) ... It just returns too
                 soon, but else we would have to check all possibilities. */
              while(*str){ if(QMatch(pat+1,str))return 1; else str++; }
              return 0;
    case '!': return !QMatch(pat+1,str);
    case '(': return QMatch(pat+1,str);
    case ')': return QMatch(pat+1,str);
    default : return (tolower(*pat)==tolower(*str))?QMatch(pat+1,str+1):0;
  }
}

/******
* DOS *
******/
bool QQuickSave(cstring fname,void *p,int len)
{ FILE *fw;
  fw=fopen(QExpandFilename(fname),"wb");
  if(fw)
  { fwrite(p,1,len,fw);
    fclose(fw);
    return TRUE;
  }
  return FALSE;
}
bool QQuickLoad(cstring fname,void *p,int len)
{ FILE *fr;
  fr=fopen(QExpandFilename(fname),"rb");
  if(fr)
  { fread(p,1,len,fr);
    fclose(fr);
    return TRUE;
  }
  return FALSE;
}
bool QFileExists(cstring fname)
{ FILE *fr;
  fr=fopen(QExpandFilename(fname),"rb");
  if(fr)
  { fclose(fr);
    return TRUE;
  }
  return FALSE;
}
int QFileSize(cstring fname)
// Returns -1 for 'file not found', length otherwise
{ FILE *fr;
  int len=-1;
  fr=fopen(QExpandFilename(fname),"rb");
  if(fr)
  { fseek(fr,0,SEEK_END);
    len=ftell(fr);
    fclose(fr);
  }
  return len;
}
bool QCopyFile(cstring src,cstring dst)
// File copy; returns FALSE in case of failure (can't open/out of disk space)
{ FILE *fr,*fw;
  char buf[1024];
  fr=fopen(QExpandFilename(src),"rb");
  if(!fr)
  { return FALSE;
  }
  fw=fopen(QExpandFilename(dst),"wb");
  if(!fw)
  { fclose(fr); return FALSE;
  }
  while(1)
  { int n,nw;
    n=fread(buf,1,sizeof(buf),fr);
    if(n<=0)break;
    nw=fwrite(buf,1,n,fw);
    // Check for write to fail (out of disk space?)
    if(nw!=n)
    { fclose(fw); fclose(fr);
      return FALSE;
    }
  }
  fclose(fw);
  fclose(fr);
  return TRUE;
}

bool QRemoveFile(cstring name)
{
  if(unlink(QExpandFilename(name))==0)
    return TRUE;
  return FALSE;
}
bool QRemoveDirectory(cstring name)
{
  if(rmdir(QExpandFilename(name))==0)
    return TRUE;
  return FALSE;
}

cstring QExpandFilename(cstring s)
// Translates assign-like parts into a real directory
// MACHTECH:myfile => /usr/people/rvg/machtech/myfile
// Returns static buffer (don't modify)
{
#ifdef WIN32
  // Leave things like C:\ intact
  return s;
#else
  // SGI
  static char buf[256];
  char *pDC;
  if(strlen(s)>sizeof(buf))
  { qerr("QExpandFilename() called on string with length>256");
    return s;
  }
  pDC=strchr(s,':');
  if(pDC)
  { strncpy(buf,s,pDC-s);
    buf[pDC-s]=0;
//qdbg("getenv(%s)\n",buf);
    sprintf(buf,"%s/%s",getenv(buf),pDC+1);
    return buf;
  } else return s;
#endif
}

cstring QFileBase(cstring s)
// Return base file or directory of full path ('s')
// Example: /usr/people/rvg/mydir => mydir
// String returned is inside the original (no static buf)
{
  int i;
  if(!*s)return s;
  for(i=strlen(s)-1;i>=0;i--)
  { if(s[i]=='/'||s[i]=='\\')
    { return &s[i+1];
    }
  }
  return s;
}
cstring QFileExtension(cstring s)
// Return file extension, or 0 if none is found
// Example: /usr/people/rvg/myfile.txt => .txt
// String returned is inside the original (no static buf)
{
  int i;
  if(!*s)return s;
  for(i=strlen(s)-1;i>=0;i--)
  { if(s[i]=='.')
    { return &s[i];
    }
  }
  return 0;
}

bool QIsDir(cstring path)
// Returns TRUE if 'path' is a directory
{
#ifdef WIN32
  qerr("QIsDir(); nyi/win32");
  return FALSE;
#else
  struct stat sbuf;
  stat(path,&sbuf);
  if(S_ISDIR(sbuf.st_mode))return TRUE;
  return FALSE;
#endif
}
void QFollowDir(cstring oldPath,cstring newDir,string buf)
// Follows path 'oldPath' into 'newDir'
// Result is stored in 'buf'
// 'newDir' may be '.' or '..' and path segments will be correctly
// adjusted
{
  int len;
  strcpy(buf,oldPath);
  if(strcmp(newDir,"..")==0)
  { // Parent; cutoff dir
    if(*buf==0)goto skip;
    for(len=strlen(buf)-1;len>0;len--)
    { if(buf[len]=='/')
      { buf[len]=0;
        goto found;
      }
    }
    // Special root case
    if(buf[0]=='/')buf[1]=0;
    else           buf[0]=0;
   skip:
   found:;
  } else
  { // Append dir
    if(strcmp(oldPath,"/"))
      strcat(buf,"/");
    strcat(buf,newDir);
  }
}

/*******
* MATH *
*******/
int QNearestPowerOf2(int n,bool round)
// Returns nearest power of 2. If round=TRUE, then both upper and lower
// possibilites are checked (NYI).
// Useful for for example texture dims
{
  int p2=1;
  if(round==TRUE)
  { qerr("QNearestPowerOf2(); rounding not yet implemented");
    // Do round=FALSE routine...
  }
  while(p2<n)p2*=2;
  return p2;
}

/*************
* ENDIANNESS *
*************/
unsigned short _QPC2Host_S(unsigned short v)
// Convert (big-endian) PC to host format.
// Note that these functions are the reverse of the ntohs() family of
// functions, which go to/from a NETWORK ordering, which uses little-endian.
// Use the macros, in this case, QPC2Host_S(), to make optimal use when
// compiling on PC's.
// These routines are used in DGeode for example, to get a DOF file format
// that is more targeted at PC's than SGI's (because of Racer).
// Note that the Q family of endian conversion functions include a FLOAT
// version, since floats are reversed as well on PC's (wrt SGI MIPS).
// Note also that floats may not be ENTIRELY compatible between MIPS
// and Intel (although I haven't bumped into problems thusfar).
{
#if defined(WIN32) || defined(linux)
  return v;
#else
  // SGI; swap
  return (v>>8)|(v<<8);
#endif
}
void _QPC2Host_SA(unsigned short *p,int n)
// Convenience function for an array of 'n' elements.
{
#if defined(WIN32) || defined(linux)
#else
  int i;
  unsigned short v;
  for(i=0;i<n;i++)
  { v=*p;
    *p++=(v>>8)|(v<<8);
  }
#endif
}
unsigned long _QPC2Host_L(unsigned long v)
// Convert (big-endian) PC to host format (long).
// Use the macro QPC2Host_L() instead of calling this function directly.
{
#if defined(WIN32) || defined(linux)
  return v;
#else
  return ((v>>24)|
          (v>>8 & 0xFF00)|
          (v<<8 & 0xFF0000)|
          (v<<24));
#endif
}
void _QPC2Host_LA(unsigned long *p,int n)
// Convenience function for an array of 'n' elements.
{
#if defined(WIN32) || defined(linux)
#else
  int i;
  unsigned long v;
  for(i=0;i<n;i++)
  { v=*p;
    *p++=((v>>24)|
          (v>>8 & 0xFF00)|
          (v<<8 & 0xFF0000)|
          (v<<24));
  }
#endif
}
void _QPC2Host_IA(int *p,int n)
// Convenience function for an array of 'n' elements.
// Kept in a function like this so passing an array will be typechecked
// at compile-time (instead of passing the macro directly to the _LA version).
{
  _QPC2Host_LA((unsigned long *)p,n);
}
float _QPC2Host_F(float vf)
// Convert (big-endian) PC to host format.
// Use the macro QPC2Host_F() instead of calling this function directly.
// NOTE: the float returned may not have any meaning and should NOT
// be used as a float any other than merely writing to disk and such.
{
#if defined(WIN32) || defined(linux)
  return vf;
#else
  unsigned long v;
  /*union F2L
  { float         vf;
    unsigned long vl;
  } f2l;
  f2l.vf=v;*/
  // Convert to 32-bits long
  v=*(unsigned long*)&vf;
  v=((v>>24)|
     (v>>8 & 0xFF00)|
     (v<<8 & 0xFF0000)|
     (v<<24));
  return *(float*)&v;
#endif
}
void _QPC2Host_FA(float *pf,int n)
// Convenience function for an array of 'n' elements.
{
#if defined(WIN32) || defined(linux)
#else
  int i;
  unsigned long v,*p=(unsigned long*)pf;
  for(i=0;i<n;i++)
  { v=*p;
    *p++=((v>>24)|
          (v>>8 & 0xFF00)|
          (v<<8 & 0xFF0000)|
          (v<<24));
  }
#endif
}
