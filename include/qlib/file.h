// qlib/qfile.h - File encapsulation

#ifndef __QLIB_QFILE_H
#define __QLIB_QFILE_H

#include <qlib/object.h>
#include <stdio.h>

class QFile : public QObject
{
 public:
  string ClassName(){ return "file"; }

  // mode
  enum Mode
  { READ=1,
    WRITE=2,
    READWRITE=3,		// Open existing file
    WRITEREAD=4,		// Open empty file
    READ_TEXT=5,		// Read non-binary (used for Windows files)
    MODE_MASK=0xFF              // Mask off modes
  };
  enum ModeFlags
  // Add these to the mode for extra options
  {
    MEMORY=0x100                // Preread entire file into memory
  };
  
  enum Seek
  { S_SET=0,
    S_END=1,
    S_CUR=2
  };

  // Flags
  enum Flags
  {
    USE_UGC=1,                  // UnGetChar is valid
    MEMORY_OWNER=2		// This object owns the file memory
  };

 protected:
  FILE *fp;
  int   mode;
  char  ugc;			// UnGetChar
  int   flags;
  // Memory support
  char *mem;			// Memory buffer
  int   memSize;		// Size of 'mem' buffer
  char *memReadPtr;		// Where it's reading

 public:
  QFile(cstring name,int mode=READ);
  QFile(cstring name,void *p,int len);
  virtual ~QFile();

  // Attribs
  FILE *GetFP(){ return fp; }
  bool IsEOF();
  bool IsOpen();
  bool IsMemoryFile(){ if(mem)return TRUE; return FALSE; }
  void OwnMemory();
  void *GetMemory(){ return mem; }
  int   GetMemorySize(){ return memSize; }

  // Seeking
  int  Size();
  int  Tell();
  bool Seek(int pos,int mode=SEEK_SET);

  // Chars - read/write
  char GetChar();
  void UnGetChar(char c);
  bool PutChar(char c);

  // Strings; notices '"'
  void GetString(string s,int maxLen=-1);
  void GetString(qstring& s);

  // Chunks - read/write
  int  Read(void *p,int len);
  int  Read(qstring& s);
  int  Write(const void *p,int len);
  int  Write(qstring& s);

  // Lines; high level
  void GetLine(char *s,int maxLen);
  bool FindLine(cstring s);

  // High level
  void SkipWhiteSpace();
};

#endif
