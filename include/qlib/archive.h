// qlib/archive.h - MGAr archive encapsulation

#ifndef __QLIB_ARCHIVE_H
#define __QLIB_ARCHIVE_H

#include <stdio.h>
#include <qlib/types.h>
#include <qlib/file.h>

// Status callback; return FALSE to cancel operation
typedef bool (*ARCALLBACK)(void *p,int current,int total);

class QArchive : public QObject
// An archive that contains (compressed) files
// For installation, a script (MT's Installer) is used for options
{
 public:
  string ClassName(){ return "archive"; }

 protected:
  enum				// Buffer sizes (overflow is lethal!)
  { SRCSIZE=20000,
    DESSIZE=20000
  };

 protected:
  int mode;			// READ/WRITE
  FILE *fc;
  int   fileSize;		// Size of archive
  ARCALLBACK cb;
  void *cbP;

  // Directory
  char *directory;		// Directory as read
  int   directorySize;		// #bytes in directory
  char *dirEntry;		// For listing

  // Legacy variables
  short    Hash[4096];
  unsigned short size;
  short    pos;
  ubyte    src[SRCSIZE],des[DESSIZE];

  bool GetMatch(ubyte *src,uword x,int srcsize);
  int  Compress(ubyte *src,ubyte *des,uword srcsize);
  bool CompressFile(FILE *fc,cstring name,bool verbose=FALSE);
  int  Decompress(ubyte *src,ubyte *des,uword srcsize);
  bool DecompressFile(cstring dname,short flags,int *size=0,void *dst=0);

  void SeekDirectory();
  bool ReadDirectory();
  bool WriteDirectory(cstring newFile,int filePos,int fileSize);

 public:
  enum
  { READ=0,
    WRITE=1,
    APPEND=2
  };

 public:
  QArchive(cstring name,int mode=READ);
  ~QArchive();

  bool IsOpen();

  bool Open();
  bool Create();
  bool AddFile(cstring srcName,cstring dstName);
  bool Close();

  // Iterating/extracting
  bool BeginIteration();
  bool FetchName(qstring& name,int *pos=0,int *size=0);
  bool FindFile(cstring name,bool fSeek=FALSE);
  int  GetFileSize();

  // Stepping over contained files
  //bool SkipFile();
  bool ExtractFile(cstring srcName,cstring dstName=0,bool overwrite=FALSE);
  bool ExtractFile(cstring srcName,void **p,int *len);
  // Transparent file access
  QFile *OpenFile(cstring srcName);

  // Status feedback
  ARCALLBACK SetCallback(ARCALLBACK cb,void *p=0);
};

#endif
