/*
 * QDir - directory support (system-independent mostly)
 * 30-05-97: Created!
 * 22-06-97: Added to QLib
 * 13-11-00: Support added for Win32
 * (C) MarketGraph/RvG
 */

#include <qlib/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef WIN32
// _findfirst() etc
#include <io.h>
#else
#include <bstring.h>
#endif
#include <errno.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDirectory::QDirectory(cstring name)
{
  dirp=0;
  dirName=0;
#ifdef WIN32
  firstReported=FALSE;
#endif
  Begin(name);
}

QDirectory::~QDirectory()
{
  //printf("qdir ctor\n");
  if(dirName)qfree(dirName);
#ifdef WIN32
  if(dirp)
    _findclose(dirp);
#else
  if(dirp)
    closedir(dirp);
#endif
  //printf("qdir ctor RET\n");
}

bool QDirectory::Begin(cstring name)
{
#ifdef WIN32
  char buf[1024];
  if(dirp)_findclose(dirp);
  memset(&findData,0,sizeof(findData));
  // Windows needs pattern
  sprintf(buf,"%s/*",name);
  dirp=_findfirst(buf,&findData);
  firstReported=FALSE;
#else
  if(dirp)closedir(dirp);
  dirp=opendir(name);
#endif
  if(dirName)qfree(dirName);
  dirName=qstrdup(name);
#ifdef WIN32
  if(dirp!=-1&&dirp!=0)return TRUE;
#else
  if(dirp)return TRUE;
#endif
  return FALSE;
}

bool QDirectory::IsFound()
{
  if(dirp)return TRUE;
  return FALSE;
}

bool QDirectory::ReadNext(string name,QFileInfo *fi)
{
#ifdef WIN32
  // Directory ok?
  if(dirp==0||dirp==-1)return FALSE;
#else
  struct dirent *direntp;
  if(!dirp)return FALSE;
#endif
#ifdef WIN32
  if(!firstReported)
  {
    // We already obtained the first file through findfirst()
    // (it's a bit unhandy that the interface works
    //  this way in Win32)
    firstReported=TRUE;
  } else
  {
    // Find next file
    int r;
    r=_findnext(dirp,&findData);
    if(r==-1)
    {
      // End of dir
      return FALSE;
    }
  }
  // Name
  strcpy(name,findData.name);
#else
  // SGI
  direntp=readdir(dirp);
  if(!direntp)return FALSE;
  // Name
  strcpy(name,direntp->d_name);
#endif
  if(fi)
  { char buf[300];
    sprintf(buf,"%s/%s",dirName,name);
    GetFileInfo(buf,fi);
  }
  return TRUE;
}

bool QDirectory::GetFileInfo(cstring name,QFileInfo *fi)
{ struct stat st;
  //qdbg("QDir:GetFileInfo(%s)\n",name);
  int r=stat(name,&st);
  if(r)
  { //printf("  stat: r=%d, errno=%d\n",r,errno);
    //bzero(fi,sizeof(*fi));
	memset(fi,0,sizeof(*fi));
    return FALSE;
  }
  fi->size=(int)st.st_size;
  if(st.st_mode&S_IFDIR)
  { fi->isDir=TRUE;
    //qdbg(" is a dir\n");
  } else fi->isDir=FALSE;
/*
  else printf(" ** not a dir\n");
  printf("QDir:GFI: mode=%x, st_dev=%x\n",st.st_mode,st.st_dev);
  printf("  ino=%d, rdev=%d, nlink=%d\n",(int)st.st_ino,(int)st.st_rdev,(int)st.st_nlink);
  printf("  blksize=%d,blocks=%d\n",(int)st.st_blksize,(int)st.st_blocks);
  */
  return TRUE;
}

