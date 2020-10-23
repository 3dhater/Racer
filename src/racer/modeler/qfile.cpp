/*
 * QFile.cpp - file encapsulation
 * 13-04-97: Created!
 * 07-10-98: Support for assigns (MY_ENV_VAR:file...)
 * 25-12-00: Byte ordering for writing qstring's has been reversed (!).
 * Now I use the PC (Big-endian) ordering, while I still can. This is done
 * for 2 reasons; 1st because DGeode's DOF files are then entirely in
 * big-endian format (not a mix anymore), and 2nd because this keeps me
 * from using ntohl() and such, which needs a link with WSOCK32.lib on Win32.
 * NOTES:
 * - Read() handling of USE_UGC (UnGetChar) has NOT been tested yet.
 * - Read/Write() of qstring is done in network byte order (no change on Irix,
 * Win32 reverses bytes)
 * (C) MG/RVG
 */

#include <qlib/file.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif
#include <qlib/debug.h>
DEBUG_ENABLE

// String reading buffer for all QFile instances
static char *stringBuf;
static int   stringBufLen;

QFile::QFile(cstring name,int imode)
{ cstring xname;
  mode=imode;
  flags=0;
  xname=QExpandFilename(name);
  if(mode==READ)
  { fp=fopen(xname,"rb");
  } else if(mode==WRITE)
  { fp=fopen(xname,"wb");
  } else if(mode==READWRITE)
  { fp=fopen(xname,"rb+");
  } else if(mode==WRITEREAD)
  { fp=fopen(xname,"wb+");
  } else if(mode==READ_TEXT)
  { fp=fopen(xname,"r");
  } else fp=0;
}
QFile::~QFile()
{
  if(fp)fclose(fp);
  // Free the buffer once in a while
  if(stringBuf){ qfree(stringBuf); stringBuf=0; }
}

/********
* CHARS *
********/
char QFile::GetChar()
{ if(flags&USE_UGC)
  { flags&=~USE_UGC;
    return ugc;
  }
  if(fp)
    return fgetc(fp);
  return 0;
}
void QFile::UnGetChar(char c)
{ ugc=c;
  flags|=USE_UGC;
}
bool QFile::PutChar(char c)
{
  fputc(c,fp);
  return TRUE;
}

/*******
* INFO *
*******/
bool QFile::IsEOF()
{ if(fp==0)return TRUE;
  if(flags&USE_UGC)return FALSE;        // Still last ungetc'ed char left
  if(feof(fp))return TRUE;
  return FALSE;
}
bool QFile::IsOpen()
{ if(fp==0)return FALSE;
  return TRUE;
}

/**********
* SEEKING *
**********/
bool QFile::Seek(int pos,int mode)
{ switch(mode)
  { case S_SET:
      fseek(fp,pos,SEEK_SET); break;
    case S_END:
      fseek(fp,pos,SEEK_END); break;
    case S_CUR:
      fseek(fp,pos,SEEK_CUR); break;
    default:
      return FALSE;
  }
  return TRUE;
}
int QFile::Tell()
{ if(fp)
    return ftell(fp);
  return 0;
}

int QFile::Size()
// Friendly size calculation; leaves file ptr intact
{ int pos,len;
  if(!fp)return 0;
  pos=Tell();
  Seek(0,S_END);
  len=Tell();
  Seek(pos);
  return len;
}

/**********
* READING *
**********/
int QFile::Read(void *p,int len)
// Read a block of memory
// Returns the number of bytes read
{
  if(len<=0)return 0;
  if(fp)
  { // If UnGetChar() was used, insert this char first
    if(flags&USE_UGC)
    { flags&=~USE_UGC;
      *(char*)p=ugc;
      p=((char*)p)+1;
      len--;
    }
    if(len<=0)return 0;
    return fread(p,1,len,fp);
  }
  return 0;
}
int QFile::Read(qstring& s)
// Read a Q string
// Returns the length of the string.
{
  short len;

  if(!fp)return 0;
  fread(&len,1,sizeof(len),fp);
  len=QPC2Host_S(len);
//qdbg("QFile:Read; len=%d\n",len);

  // Empty string?
  if(!len)
  { s="";
    return 0;
  }

  // Grow string buffer
  if(!stringBuf)
  {
   allocate_it:
    stringBufLen=len+1;
    stringBuf=(char*)qcalloc(stringBufLen);
    if(!stringBuf)
    { qwarn("QFile:Read(s); out of memory");
      return 0;
    }
  } else if(stringBufLen<len+1)
  {
    // Resize to fit
    qfree(stringBuf);
    goto allocate_it;
  }
  // Read string chars
  fread(stringBuf,1,len,fp);

  // Close down and assign
  stringBuf[len]=0;
  s=stringBuf;

  // Return length of string
  return len;
}

/**********
* WRITING *
**********/
int QFile::Write(const void *p,int len)
{ if(fp)
    return fwrite(p,1,len,fp);
  return 0;
}
int QFile::Write(qstring& s)
// Writes the contents of the string to the file
// Returns the total #bytes written, including the string length variable
{
  short len,n;

  // Write string length
  len=strlen(s);
  n=QHost2PC_S(len); fwrite(&n,1,sizeof(n),fp);
  // Write string chars
  fwrite((cstring)s,1,len,fp);
  return len+sizeof(len);
}

/**********
* STRINGS *
**********/
void QFile::GetString(string s,int maxLen)
// Retrieve string; first skip whitespace, then pick up the arg,
// and return.
// If 'maxLen'==-1, then the size is unlimited (!)
{
  char c;
  int  len;

  if(maxLen==-1)maxLen=2000000000;		// Assume strings <2Gb
  //maxLen--;					// To keep maxLen-1 out loop

  *s=0;
  len=0;
  SkipWhiteSpace();
  c=GetChar();
  if(IsEOF())return;
  if(c=='"')
  { // Retrieve until ending " or EOF
    while(1)
    { c=GetChar();
      if(IsEOF()==TRUE||c=='"')break;
      if(++len>=maxLen)break;
      *s++=c;
    }
    *s=0;
  } else
  { // Normal string (sep'd by whitespace)
    while(1)
    {
      if(++len>=maxLen)break;
      *s++=c;
      c=GetChar();
      if(IsEOF())break;
      if(c==' '||c==9||c==13||c==10)
      { UnGetChar(c);
        break;
      }
    }
    *s=0;
  }
}
void QFile::GetString(qstring& s)
// Like GetString(string s,int maxLen), only for qstring, and it doesn't
// accept strings>1024 bytes
{ char buf[1024];
  GetString(buf,sizeof(buf));
  s=buf;
}

void QFile::GetLine(char *s,int maxLen)
// Get an entire line, leaving out the line feed at the end
// 0-terminated
{
  int n;
  char c;

  for(n=0;n<maxLen-1;n++)
  { c=GetChar();
    if(IsEOF())break;
    if(c==10)break;
    *s++=c;
  }
  *s=0;
}
bool QFile::FindLine(cstring s)
// Search a line that matches 's' or return FALSE
// File pointer is one the line just beyond the line that matched
{ char buf[256];
  while(1)
  { if(IsEOF())return FALSE;
    GetLine(buf,256);
    if(strcmp(buf,s)==0)return TRUE;
  }
}

/*************
* HIGH LEVEL *
*************/
void QFile::SkipWhiteSpace()
// Skip spaces, TABs, CRs, LFs
{
  char c;

  while(1)
  { c=GetChar();
    if(IsEOF())return;
    if(c!=' '&&c!=9&&c!=13&&c!=10)
    { UnGetChar(c); break;
    }
  }
}

