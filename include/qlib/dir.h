// dir.h

#ifndef __QLIB_DIR_H
#define __QLIB_DIR_H

#include <qlib/object.h>
#ifdef WIN32
#include <io.h>
#else
// SGI System V (4.3BSD =sys/dir.h)
#include <time.h>
#include <dirent.h>
#endif

struct QFileInfo
{
  int size;
  bool isDir;
  time_t atime;			// Last access time (reads!)
  time_t mtime,			// Last modified
         ctime;			// Last changed
};

class QDirectory : public QObject
{
 protected:
#ifdef WIN32
  long dirp;
  _finddata_t findData;
  bool firstReported;
#else
  // SGI
  DIR *dirp;
#endif
  string dirName;

 public:
  QDirectory(cstring dirName);
  ~QDirectory();

  bool Begin(cstring name);
  bool IsFound();
  bool ReadNext(string name,QFileInfo *fi=0);
  // Generic
  bool GetFileInfo(cstring name,QFileInfo *fi);
};

#endif

