/*
 * QFile.cpp - file encapsulation
 * 13-04-97: Created!
 * 07-10-98: Support for assigns (MY_ENV_VAR:file...)
 * 25-12-00: Byte ordering for writing qstring's has been reversed (!).
 * Now I use the PC (Big-endian) ordering, while I still can. This is done
 * for 2 reasons; 1st because DGeode's DOF files are then entirely in
 * big-endian format (not a mix anymore), and 2nd because this keeps me
 * from using ntohl() and such, which needs a link with WSOCK32.lib on Win32.
 * 07-12-01: Added memory file support; both for reading a file into memory
 * when opened, and for external memory chunks that you want to read as a file.
 * NOTES:
 * - Can preload the entire file for faster performance.
 * - Can map a piece of memory which can be read just like a file.
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

/*********
* C/Dtor *
*********/
QFile::QFile(cstring name,int imode)
  : QObject()
// Open a file from disk.
// 'mode' gives the read/write mode (various mixes are possible).
{
  cstring xname;

  // Give this object the name of the file.
  Name(name);
  
  // Init members
  flags=0;
  mem=0;
  memSize=0;
  memReadPtr=0;

  mode=imode&MODE_MASK;
  xname=QExpandFilename(name);

  // Open the file
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

  // Memory support
  if(imode&MEMORY)
  {
    // Check for a read mode
    switch(mode)
    {
      case READ: case READ_TEXT:
      //case READWRITE: case WRITEREAD:        // Not supported
        // Read entire file into memory
        fseek(fp,0,SEEK_END);
        memSize=ftell(fp);
        if(memSize>0)
        {
          mem=(char*)qalloc(memSize);
          if(!mem)
          {
            qwarn("QFile:ctor; not enough memory to preload file '%s'",name);
            memSize=0;
            break;
          }
          fseek(fp,0,SEEK_SET);
          fread(mem,1,memSize,fp);
          // Close the actual file, and read from memory
          fclose(fp); fp=0;
          memReadPtr=mem;
          // Make sure to free it once we close up
          //flags|=MEMORY_OWNER;
        } // else it's a null-file, don't use memory access
        break;
      default:
        qwarn("QFile:ctor MEMORY req'd for '%s' but incompatible mode given",
          name);
        // Ignore flag
        break;
    }
  }
}
QFile::QFile(cstring name,void *p,int len)
  : QObject()
// Open a file using a piece of memory.
// Note that closing the file will NOT free that memory. If you want
// that to happen, call file->OwnMemory() after opening.
// The 'name' is given just to get a name for this file, but isn't
// really used, except for debugging.
{
  // Give this object the name of the file.
  Name(name);
  
  // Init members
  flags=0;
  fp=0;

  // Fixed mode
  mode=READ;

  // Take over memory
  mem=(char*)p;
  memSize=len;
  memReadPtr=mem;
//if(!strcmp(name,"body.dof"))
//{FILE *fw=fopen("test.dmp","wb");fwrite(mem,1,memSize,fw);fclose(fw);}
}
QFile::~QFile()
{
//qdbg("QFile dtor\n");
//qdbg("  fp=%p, flags=%d\n",fp,flags);
  // Free the memory buffer if it is ours.
  if(flags&MEMORY_OWNER)
  {
    QFREE(mem);
  }
  if(fp)fclose(fp);
  // Free the buffer once in a while
  if(stringBuf){ qfree(stringBuf); stringBuf=0; }
}

/**********
* Attribs *
**********/
void QFile::OwnMemory()
// Make this file own the memory it was given.
// This means that upon destroying the object, the memory is released.
{
  flags|=MEMORY_OWNER;
}

/********
* CHARS *
********/
char QFile::GetChar()
{
  if(flags&USE_UGC)
  { flags&=~USE_UGC;
    return ugc;
  }
  if(IsMemoryFile())
  {
    char c;
    if(memReadPtr-mem>=memSize)
    {
      // EOF
      return 0;
    }
    c=*memReadPtr++;
    return c;
  }
  if(fp)
    return fgetc(fp);
  return 0;
}
void QFile::UnGetChar(char c)
// Unget 1 char. Not more than 1 char can be 'ungotten'!
// Better yet is to do without UnGetChar() altogether.
{
  ugc=c;
  flags|=USE_UGC;
}
bool QFile::PutChar(char c)
// Adds a char
{
  if(IsMemoryFile())
    return FALSE;
  fputc(c,fp);
  return TRUE;
}

/*******
* INFO *
*******/
bool QFile::IsEOF()
{
  if(IsMemoryFile())
  {
    if(flags&USE_UGC)return FALSE;      // Still last ungetc'ed char left
    if(memReadPtr>=mem+memSize)return TRUE;
    return FALSE;
  }
  // File pointer
  if(fp==0)return TRUE;
  if(flags&USE_UGC)return FALSE;        // Still last ungetc'ed char left
  if(feof(fp))return TRUE;
  return FALSE;
}
bool QFile::IsOpen()
{
  // In case of a memory file, the file is always open.
  if(IsMemoryFile())
    return TRUE;
  if(fp==0)return FALSE;
  return TRUE;
}

/**********
* SEEKING *
**********/
bool QFile::Seek(int pos,int mode)
// Seek to a position based on 'mode', which can be S_SET, S_END
// or S_CUR.
// Returns FALSE if seek couldn't be done.
// Caveats: for memory files, seeks cannot be done further than
// the end of file. In this case, the seek will be restricted to
// only the range of the file that actually exist.
{
  if(IsMemoryFile())
  {
    switch(mode)
    { case S_SET:
        if(pos>memSize)pos=memSize;
        memReadPtr=mem+pos;
        break;
      case S_END:
        memReadPtr=mem+memSize-pos;
        // Underflow
        if(memReadPtr<mem)
          memReadPtr=mem;
        break;
      case S_CUR:
        memReadPtr+=pos;
        // Overflow
        if(memReadPtr>mem+memSize)
          memReadPtr=mem+memSize;
        break;
      default:
        return FALSE;
    }
  } else
  {
    // File pointer
    switch(mode)
    { case S_SET:
        fseek(fp,pos,SEEK_SET); break;
      case S_END:
        fseek(fp,pos,SEEK_END); break;
      case S_CUR:
        fseek(fp,pos,SEEK_CUR); break;
      default:
        return FALSE;
    }
  }
  return TRUE;
}
int QFile::Tell()
{
  if(IsMemoryFile())
  {
    return memReadPtr-mem;
  } else
  {
    // File pointer
    if(fp)
      return ftell(fp);
  }
  return 0;
}

int QFile::Size()
// Friendly size calculation; leaves file ptr intact.
{
  int pos,len;

  if(IsMemoryFile())
  {
    return memSize;
  }

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
// Read a block of memory.
// Returns the number of bytes read, and 0 if nothing was read.
{
//qdbg("QFile:Read(len=%d)\n",len);
  if(len<=0)return 0;

  // UnGetChar() available? (works for both memory and real files)
  if(flags&USE_UGC)
  { flags&=~USE_UGC;
    *(char*)p=ugc;
    p=((char*)p)+1;
    len--;
    
    // Got anything left to read?
    if(len==0)return 1;
  }

  if(IsMemoryFile())
  {
    // Read past end-of-file?
    if(memReadPtr+len>mem+memSize)
    {
//qdbg("read past eof\n");
      len=mem+memSize-memReadPtr;
      if(len<=0)return 0;
    }
//qdbg("memcpy len=%d\n",len);
    memcpy(p,memReadPtr,len);
    memReadPtr+=len;
    return len;
  } else
  {
    if(fp)
    {
      return fread(p,1,len,fp);
    }
  }
  return 0;
}
int QFile::Read(qstring& s)
// Read a Q string
// Returns the length of the string.
{
  short len;

  // Check for open file
  if(!IsMemoryFile())
    if(!fp)return 0;

  Read(&len,sizeof(len));
  //fread(&len,1,sizeof(len),fp);
  len=QPC2Host_S(len);
//qdbg("QFile:Read(qstring); len=%d\n",len);

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
  Read(stringBuf,len);
  //fread(stringBuf,1,len,fp);

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
{
  if(IsMemoryFile())
    return 0;
  if(fp)
    return fwrite(p,1,len,fp);
  return 0;
}
int QFile::Write(qstring& s)
// Writes the contents of the string to the file
// Returns the total #bytes written, including the string length variable
{
  short len,n;

  if(IsMemoryFile())
    return 0;

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
{
  char buf[1024];
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
