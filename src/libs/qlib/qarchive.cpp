/*
 * QArchive - archiving compressed files (mgar compatible file format)
 * 16-06-93: Created! (on Amiga)
 * 01-11-94: Now crunches! Intel BigEndian format used.
 * 01-06-99: Could trap because of div0 in CompressFile() report.
 * 26-08-99: C++ QArchive class
 * 30-08-99: Several chunk size tests added; bugs appeared that are
 * hard to track when these checks are not present (overflowing memory).
 * 31-10-99: New archive format; uses directory for quick file access, and
 * tags to extend file attribs (stat data and such). Optionally. Interface
 * has changed a bit (no more SkipFile() actions in user code)!
 * 06-12-01: Extended the directory with the filesize, and decompression
 * into memory is now possible. Files unfortunately incompatible with
 * older QArchive files.
 * NOTES:
 * - Directory is stored at the end of the file; last long of file
 * points to start of directory.
 * FUTURE: 
 * - Nicer removal of src, des, size, pos and Hash member variables
 * BUGS:
 * - QArchive ctor doesn't check for magic ID if in APPEND mode
 * - 'fileSize' may not be updated when appending new files.
 * - DecompressFile() doesn't cancel nicely when the 'cb' says to cancel.
 * (C) MarketGraph/RVG
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <qlib/archive.h>
#include <qlib/debug.h>
DEBUG_ENABLE

//#define CRUNCH_IDS      "AR93"
//#define CRUNCH_IDS      "AR99"
#define CRUNCH_IDS      "AR01"
#define CRUNCH_ID       0x33395241
#define FLAG_COPIED     0x80
#define FLAG_COMPRESS   0x40

#undef  DBG_CLASS
#define DBG_CLASS "QArchive"

/*********
* HELPER *
*********/
uword swap_word(uword w)
// For SGI/Amiga (littleEndian). Also works on Win32/Linux(Intel)
{
  return QPC2Host_S(w);
}

/**************
* COMPRESSING *
**************/
bool QArchive::GetMatch(ubyte *src,uword x,int srcsize)
{ uword HashValue;
  HashValue=(40543*((((src[x]<<4)^src[x+1])<<4)^src[x+2])>>4)&0xfff;
  pos=Hash[HashValue];
  Hash[HashValue]=x;
  if((pos!=-1)&&((x-pos)<4096))
  {  for(size=0;
         ((size<18)&&src[x+size]==src[pos+size])&&((x+size)<srcsize);
         size++);
    return size>=3;
  }
  return 0;
}
int QArchive::Compress(ubyte *src,ubyte *des,uword srcsize)
{ ubyte bit=0;
  uword key,command=0,x=0,y=3,z=1;

  for(key=0;key<4096;key++)Hash[key]=-1;
  des[0]=FLAG_COMPRESS;
  while(x<srcsize)
  { if(bit>15)
    { des[z++]=(command>>8)&0x00ff;
      des[z]=command&0x00ff;
      z=y;
      bit=0;
      y+=2;
    }
    for(size=1;(src[x]==src[x+size])&&(size<0x0fff)&&(x+size<srcsize);size++);
    if(size>=16)
    { des[y++]=0;
      des[y++]=((size-16)>>8)&0x00ff;
      des[y++]=(size-16)&0x00ff;
      des[y++]=src[x];
      x+=size;
      command=(command<<1)+1;
    } else if(GetMatch(src,x,srcsize))
    { key=((x-pos)<<4)+(size-3);
      des[y++]=(key>>8)&0x00ff;
      des[y++]=key&0x00ff;
      x+=size;
      command=(command<<1)+1;
    } else
    { des[y++]=src[x++];
      command=(command<<1);
    }
    bit++;
  }
  command<<=(16-bit);
  des[z++]=(command>>8)&0x00ff;
  des[z]=command&0x00ff;
//qdbg("srcsize=%d, x=%d, y=%d (RET)\n",srcsize,x,y);
  /** if y>srcsize you'd better NOT crunch **/
  if(y>srcsize)
  { des[0]=FLAG_COPIED;
    memcpy(&des[1],src,srcsize);
//qdbg("  copied; "); QHexDump((char*)des,10);
    return srcsize+1;
  }
  return y;
}
bool QArchive::CompressFile(FILE *fc,cstring name,bool verbose)
// Compresses file 'name' into fc (the archive file)
// Returns TRUE for success
{ FILE *fp1;
  uword srcsize,dessize;
  int   totalsrc,totaldest;
  
  totalsrc=totaldest=0;
  fp1=fopen(QExpandFilename(name),"rb");
  if(!fp1)
  { printf("** ERROR: '%s' not found!\n",name);
    return FALSE;
  }
  if(verbose)printf("Compress '%s'",name);
  for(;(srcsize=fread(src,1,16384,fp1))!=0;)
  { dessize=Compress(src,des,srcsize);
    if(dessize>DESSIZE)
    { qerr("QArchive: compressed chunk overflow of size %d (max %d)!",
        dessize,DESSIZE);
      break;
    }
    if(verbose)printf(".");
    // Amiga/SGI
    dessize=swap_word(dessize);
    fwrite(&dessize,sizeof(uword),1,fc);
    dessize=swap_word(dessize);
//qdbg("QAr:Cmpr: srcsize=%d, dessize=%d\n",srcsize,dessize);
    fwrite(des,1,dessize,fc);
    totalsrc+=srcsize; totaldest+=dessize;
  }
  /** Notify EOF **/
  dessize=0; fwrite(&dessize,sizeof(uword),1,fc);
  fclose(fp1);
  if(!totalsrc)totalsrc=1;		// Div0
  if(verbose)printf(" %d%%\n",totaldest*100/totalsrc);
  return TRUE;
}

/****************
* DECOMPRESSING *
****************/
int QArchive::Decompress(ubyte *src,ubyte *des,uword srcsize)
{ uword x=3,y=0,k,command=(src[1]<<8)+src[2];
  ubyte bit=16;
  if(src[0]==FLAG_COPIED)
  { /** Will never happen, viewing crunchy **/
    memcpy(des,&src[1],srcsize-1);
    //for(y=1;y<srcsize;y++)des[y-1]=src[y];
    return srcsize-1;
  }
  for(;x<srcsize;)
  { if(bit==0)
    { command=(src[x++]<<8);
      command+=src[x++];
      bit=16;
    }
    if(command&0x8000)
    { pos=(src[x++]<<4);
      pos+=(src[x]>>4);
      if(pos)
      { size=(src[x++]&0x0f)+3;
        for(k=0;k<size;k++)des[y+k]=des[y-pos+k];
        y+=size;
      } else
      { size=(src[x++]<<8);
        size+=src[x++]+16;
        for(k=0;k<size;des[y+k++]=src[x]);
        x++;
        y+=size;
      }
    } else des[y++]=src[x++];
    command<<=1;
    bit--;
  }
  return y;
}
bool QArchive::DecompressFile(cstring dname,short flags,int *size,void *dst)
// Decompress 'dname'. If flags&1, then don't write out the file
// (used in listing the file).
// If 'size' is not 0, the uncompressed file size will be stored there.
// If 'dst' is not 0, the file is decompressed into that memory chunk. It is
// assumed that there is enough space in 'dst'.
// Returns TRUE for success.
{
  FILE *fp2=0;
  uword srcsize,dessize;
  bool  r=TRUE;
  char  cmd;
  
  DBG_C("DecompressFile")

  if(size)*size=0;

  // Load file tags
 next_tag:
  cmd=fgetc(fc);
  if(cmd!='D')
  { // Skip unknown tga
    int size;
    fread(&size,1,sizeof(size),fc);
    size=QPC2Host_L(size);
    fseek(fc,size,SEEK_CUR);
    goto next_tag;
  }

  // Data chunk  
  if(!(flags&1))
  { fp2=fopen(QExpandFilename(dname),"wb");
    if(!fp2)
    { qerr("Can't open '%s' for output",dname);
      //SkipFile();
      return FALSE;
    }
    //printf("- Decompressing %s",dname);
  }
  while(1)
  {
    fread(&srcsize,sizeof(uword),1,fc);
    srcsize=swap_word(srcsize);
    if(!srcsize)break;
    if(srcsize>SRCSIZE)
    { qerr("QArchive: attempt to read decompress chunk of size %d (max %d)",
        srcsize,SRCSIZE);
      r=FALSE;
      break;
    }

    if(cb!=0&&cb(cbP,ftell(fc),fileSize)==FALSE)
    { r=FALSE;
      break;
    }

    fread(src,1,srcsize,fc);
    //if(!flags)
    {
      if(dst)
        dessize=Decompress(src,(ubyte*)dst,srcsize);
      else
        dessize=Decompress(src,des,srcsize);
//qdbg("dec: dessize=%d, srcsize=%d\n",dessize,srcsize);
      if(dessize>DESSIZE)
      { qerr("QArchive: decompressed chunk overflow of size %d (max %d)",
          dessize,DESSIZE);
        r=FALSE;
        break;
      }
      if(size)(*size)+=dessize;
      if(dst)
      {
        dst=(void*)((char*)dst+dessize);
      }
      //printf(".");
      if(fp2)
      {
        if(fwrite(des,1,dessize,fp2)!=dessize)
        { printf("Can't write to %s\n",dname); fclose(fp2); return FALSE; }
      }
    }
  }
  if(fp2)fclose(fp2);
  //if(!flags)printf("\n");
  return r;
}
bool QArchive::ExtractFile(cstring srcName,void **p,int *len)
// Decompresses a file into memory.
// Returns TRUE if succesful. In that case, *p contains the memory pointer,
// and *len contains the chunk length.
{
  int pos,size;
  qstring fname;
  bool found=FALSE;

  // Find the file
  BeginIteration();
  while(FetchName(fname,&pos,&size))
  {
    if(fname==srcName)
    {
      fseek(fc,pos,SEEK_SET);
      found=TRUE;
      break;
    }
  }
  // Was the file found?
  if(!found)
    return FALSE;

//qdbg("size=%d\n",size);
  if(size<=0)return FALSE;
  *len=size;
  *p=qalloc(size);
  if(!*p)return FALSE;
  // Decompress into memory
  if(!DecompressFile("",1,0,*p))
    return FALSE;
  return TRUE;
}
QFile *QArchive::OpenFile(cstring srcName)
// Open a file that should be inside the archive.
// If it succeeds, it returns a QFile pointer, which you can use as
// a regular file.
// Returns 0 in case of failure (file not found, not enough memory...).
{
  void  *p;
  int    len;
  QFile *f;
  
  if(!ExtractFile(srcName,&p,&len))
  {
    // File not found most often
    return FALSE;
  }
  // Open a file using this memory chunk, and hand over memory managent
  // to the file object (it will free it upon closing).
  f=new QFile(srcName,p,len);
  f->OwnMemory();
  return f;
}

int QArchive::GetFileSize()
// Assumes the file pointer is at the compressed file.
// Returns the uncompressed file size.
{
  int size;
  int pos=ftell(fc);

  DecompressFile("xxx",1,&size);
  fseek(fc,pos,SEEK_SET);
  return size;
}

/*********
* C/Dtor *
*********/
QArchive::QArchive(cstring name,int _mode)
  : QObject(),mode(_mode)
// Create new or open existing archive, depending on 'mode'
// Use IsOpen() to see if the file was succesfully opened
{
  DBG_C("ctor");
  
  string sm;
  int    i;

  // Give the object a name
  Name(name);
  
  cb=0;
  cbP=0;
  fileSize=0;

  directory=0;
  directorySize=0;
  dirEntry=0;
  
  // Compress variables
  size=0; pos=0;
  for(i=0;i<20000;i++)src[i]=0;
  for(i=0;i<16388;i++)des[i]=0;
  for(i=0;i<4096 ;i++)Hash[i]=0;

  if(mode==WRITE)
  { sm="wb+";
  } else if(mode==APPEND)
  { sm="rb+";
  } else
  { sm="rb";
  }
  fc=fopen(QExpandFilename(name),sm);
  if(!fc)
  { qwarn("QArchive: can't open '%s'",name);
    goto skip_open;
  }
  if(mode==READ||mode==APPEND)
  { 
    // Read directory
    ReadDirectory();
    // Detect file size (of data) for progress functionality
    SeekDirectory();
    fileSize=ftell(fc);
#ifdef OBS
    fseek(fc,0,SEEK_END);
    fileSize=ftell(fc);
    fseek(fc,0,SEEK_SET);
#endif
  }
  // Auto create header in case of a new file
  if(mode==WRITE)Create();
  else if(mode==READ)
  { if(!Open())
      Close();			// Don't accept non-archive files
  }
 skip_open:
  ;
}

QArchive::~QArchive()
{
  DBG_C("dtor")

  Close();
}

/**********
* Attribs *
**********/
bool QArchive::IsOpen()
// Returns TRUE if file is open and ready
{ if(fc)return TRUE;
  return FALSE;
}

/*******
* Open *
*******/
bool QArchive::Open()
// Read the header from the just-open file
// Returns FALSE if the file is NOT an archive file.
{ char mybuf[5];
  
  DBG_C("Open")

  if(!fc)return FALSE;

  /** Read crunch ID **/
  fseek(fc,0,SEEK_SET);
  fread(mybuf,4,1,fc); mybuf[4]=0;
  if(strcmp(mybuf,CRUNCH_IDS))return FALSE;
  return TRUE;
}

/*********
* Create *
*********/
bool QArchive::Create()
// Create new archive
{ char mybuf[5];
  int  pos;

  DBG_C("Create")
  
  // Safety
  if(!fc)return FALSE;

  /** Write crunch ID **/
  strcpy(mybuf,CRUNCH_IDS);
  fwrite(mybuf,4,1,fc);
  
  // Write empty directory
  directorySize=0;
  pos=ftell(fc);
  pos=QHost2PC_L(pos);
  fwrite(&pos,1,sizeof(pos),fc);
  pos=QPC2Host_L(pos);
  
  return TRUE;
}

bool QArchive::AddFile(cstring srcName,cstring dstName)
// Adds a file to the archive.
// If 'dstName'==0, the destination name is the same as the source name
{
  bool r;
  int  startPos;            // Start of compressed file data
  int  fileSize;

  if(!fc)return FALSE;
  if(!dstName)dstName=srcName;
 
  fileSize=QFileSize(srcName);

  // Overwrite, starting at directory
  SeekDirectory();
  startPos=ftell(fc);
//qdbg("AddFile @%d/0x%x\n",startPos,startPos);
#ifdef OBS
  fprintf(fc,"%s ",dstName);
#endif
  // Write actual file contents ("data")
  fprintf(fc,"D");
  r=CompressFile(fc,srcName,FALSE);
  
  // Update directory
  WriteDirectory(dstName,startPos,fileSize);
  ReadDirectory();
  return r;
}

/********
* Close *
********/
bool QArchive::Close()
// Close archive file, if open, and free resources
{
  DBG_C("Close")

  if(directory)qfree(directory);
  if(fc){ fclose(fc); fc=0; }
  return TRUE;
}

/************
* DIRECTORY *
************/
void QArchive::SeekDirectory()
{
  int pos,fsize;
  
  DBG_C("SeekDirectory")

  fseek(fc,-(int)(sizeof(int)),SEEK_END);
  fsize=ftell(fc);
#ifdef OBS
qdbg("fsize=%d\n",fsize); if(fsize>1000)fsize=1000;
perror("ftell");
#endif
  fread(&pos,1,sizeof(int),fc);
  pos=QPC2Host_L(pos);
#ifdef OBS
qdbg("pos=%d\n",pos);
perror("fread");
#endif

  fseek(fc,pos,SEEK_SET);
  // Calculate directory size
  directorySize=fsize-pos;
//qdbg("QAr: dirpos=%d, size=%d, dirsize=%d\n",pos,fsize,directorySize);
}

bool QArchive::ReadDirectory()
{
  SeekDirectory();
  if(directory)qfree(directory);
  directory=(char*)qalloc(directorySize);
  if(directory)
  {
    fread(directory,1,directorySize,fc);
  }
  return TRUE;
}
bool QArchive::WriteDirectory(cstring newFile,int filePos,int fileSize)
// Write directory plus new file at current position
{
  int startPos;
  
  DBG_C("WriteDirectory")

  // Remember dir start
  startPos=ftell(fc);
  
  // Write old directory
  if(directorySize>0)
  {
    fwrite(directory,1,directorySize,fc);
  }
  
  // Write new entry; name+trailing 0, size
  fprintf(fc,"%s",newFile);
  fputc(0,fc);
  filePos=QHost2PC_L(filePos);
  fwrite(&filePos,1,sizeof(int),fc);
  filePos=QPC2Host_L(filePos);
  fileSize=QHost2PC_L(fileSize);
  fwrite(&fileSize,1,sizeof(int),fc);
  fileSize=QPC2Host_L(fileSize);
  
  // Write directory start
  startPos=QHost2PC_L(startPos);
  fwrite(&startPos,1,sizeof(startPos),fc);
  startPos=QPC2Host_L(startPos);
  return TRUE;
}

/************
* ITERATING *
************/
bool QArchive::BeginIteration()
// Restart searching the archive
{
  DBG_C("BeginIteration")

  if(!fc)return FALSE;
#ifdef OBS
  (void)fseek(fc,4,SEEK_SET);
#endif
  dirEntry=directory;
  return TRUE;
}
bool QArchive::FetchName(qstring& name,int *pos,int *size)
// Returns TRUE and next name in archive, or FALSE for end-of-archive.
{
  DBG_C("FetchName")

#ifdef OBS
  static char mybuf[130];
#endif
  
  if(!fc)return FALSE;
  if(!directory)return FALSE;

  // Automatic begin?
  if(!dirEntry)
    BeginIteration();
  
#ifdef OBS
  fscanf(fc,"%s",mybuf);
  if(feof(fc))return FALSE;

  /** Skip the space **/
  fgetc(fc);
  name=mybuf;
#endif

  // End of directory?
//qdbg("dir=%p, dirSize=%d, dirEntry=%p, dir+size=%p\n",
  //directory,directorySize,dirEntry,directory+directorySize);
  if(dirEntry>=directory+directorySize)
    return FALSE;
//qdbg("  some left\n");

  // Copy filename
  name=dirEntry;
  dirEntry+=strlen(dirEntry)+1;
  // Copy file position
  if(pos)
  { // Watch out for odd addresses
    char buf[4];
    int  i;
    for(i=0;i<4;i++)buf[i]=dirEntry[i];
    *pos=QPC2Host_L(*(int*)buf);
  }
  dirEntry+=sizeof(int);
  // Copy file size
  if(size)
  { // Watch out for odd addresses
    char buf[4];
    int  i;
    for(i=0;i<4;i++)buf[i]=dirEntry[i];
    *size=QPC2Host_L(*(int*)buf);
  }
  // Skip file position and size
  dirEntry+=sizeof(int);

  return TRUE;
}
bool QArchive::FindFile(cstring name,bool fSeek)
// Tries to find 'name' in the archive.
// If 'fSeek'=TRUE, the file position will be directed to the start of
// the file.
{ qstring fname;
  int pos;
  DBG_C("FindFile")

  BeginIteration();
  while(FetchName(fname,&pos))
  {
qdbg("QAr:FF: check '%s' to match '%s'\n",name,(cstring)fname);
    if(fname==name)  // strcmp((cstring)fname,name)==0)
    {
      if(fSeek)
      {
        // Goto the file
        fseek(fc,pos,SEEK_SET);
      }
      return TRUE;
    }
#ifdef OBS
    SkipFile();
#endif
  }
  // Not found
  return FALSE;
}

/***************
* Extract file *
***************/
bool QArchive::ExtractFile(cstring srcName,cstring dstName,bool overwrite)
// Extracts file to 'dstName'
// If 'dstName'==0, it is assumed the same as 'srcName'
{ char   *oldDirEntry;
  qstring s;
  int     pos;
  bool    r=FALSE;

  DBG_C("ExtractFile")
  DBG_ARG_S(srcName);
  DBG_ARG_S(dstName);
  DBG_ARG_B(overwrite);
  
  if(!dstName)dstName=srcName;
qdbg("ExtractFile(dst=%s)\n",dstName);
  // Find file position without disturbing any current directory read
  oldDirEntry=dirEntry;
  BeginIteration();
  while(FetchName(s,&pos))
  { if(s==srcName)
    { fseek(fc,pos,SEEK_SET);
      r=TRUE;
      break;
    }
  }
  dirEntry=oldDirEntry;
  
  if(!r)return FALSE;
//qdbg("  pos=%d\n",pos);
  DecompressFile(dstName,0);
  return TRUE;
}

/************
* Callbacks *
************/
ARCALLBACK QArchive::SetCallback(ARCALLBACK newCB,void *p)
// Sets the callback function, which is called at some points
// to indicate the progress.
// If the callback function returns FALSE, the decompression is cancelled.
// Returns the old callback function (may very well be 0).
{
  ARCALLBACK cbOld;

  cbOld=cb;
  cb=newCB;
  cbP=p;
  return cbOld;
}
