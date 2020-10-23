// qlib/cedit.h

#ifndef __QLIB_CEDIT_H
#define __QLIB_CEDIT_H

#include <qlib/edit.h>
#include <sys/stat.h>

class QCEditInfo
// Contains members that need to be remembered when swapping between
// different files.
{
 public:
  qstring fileName;
  int    cx,cy,cxWanted;
  string text;
  int    maxChar;
  string textTop;
  string rangeStart,rangeEnd;
  char   rangeDirection,
         pad;
  string marker;
  int    eFlags;
  struct stat lastStat;		// Last statistics (time file was modified)

 public:
  QCEditInfo();
  ~QCEditInfo();

  void Reset();
};

class QCEdit : public QEdit
// An editor with a lot of extra functionality for editing source code
{
  enum
  { MAX_FILES=10 		// Max remember files
  };

  QCEditInfo *ei[MAX_FILES];
  int         curFile;

  // Find & replace
  char findBuffer[MAX_FIND_LEN];
  char replaceBuffer[MAX_FIND_LEN];

 protected:
  void SubtractSpace(char *start,char *end);
  void AddSpace(char *start,char *end);

 public:
  QCEdit(QWindow *parent,QRect *r);
  ~QCEdit();

  // Information
  cstring GetFileName();
  int     GetCurFile(){ return curFile; }

  // I/O
  bool Load(cstring fname,int slot=-1);
  bool Save(cstring fname=0);

  bool Select(int n,bool refresh=TRUE);

  void DoFind();
  void DoFindNext();
  void DoFindPrev();
  void DoSearchReplace();
  void DoSelectWord();

  void MacroHeader(cstring s);
  void MacroScene();

  bool EvKeyPress(int key,int x,int y);
  bool EvButtonPress(int key,int x,int y);
};

#endif
