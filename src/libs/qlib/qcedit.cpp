/*
 * QCEdit - Edit control for C(++) code; fancy stuff
 * 07-09-1999: Created!
 * NOTES:
 * - Should not be in QLib; is too high for that
 * (C) MG/RVG
 */

#include <qlib/cedit.h>
#include <qlib/keys.h>
#include <qlib/time.h>
#include <qlib/dialog.h>
#include <qlib/menu.h>
#include <sys/stat.h>
#include <time.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Options
// Always save document, even though it is marked unchanged?
//#define ALWAYS_SAVE

// Max chars in a document
#define MAX_DOCUMENT_LENGTH	100000

/***
  QCEditInfo helper class
***/
QCEditInfo::QCEditInfo()
{
  cx=cy=0;
  cxWanted=0;
  maxChar=MAX_DOCUMENT_LENGTH;
  text=(char*)qcalloc(maxChar);
  textTop=text;
  rangeStart=rangeEnd=0;
  rangeDirection=0;
  marker=0;
  eFlags=QEdit::AUTOINDENT;
}
QCEditInfo::~QCEditInfo()
{
}


QCEdit::QCEdit(QWindow *parent,QRect *r)
  : QEdit(parent,r,MAX_DOCUMENT_LENGTH,0,QEdit::MULTILINE|QEdit::SCROLLING)
{
  int i;

  curFile=-1;
  for(i=0;i<MAX_FILES;i++)
    ei[i]=new QCEditInfo();

  findBuffer[0]=0;
  replaceBuffer[0]=0;
}
QCEdit::~QCEdit()
{
  int i;
  for(i=0;i<MAX_FILES;i++)
    delete ei[i];
}

cstring QCEdit::GetFileName()
// Returns filename of current file (or empty string if none exists)
{
  if(curFile==-1)return "";
  if(ei[curFile]->fileName.IsEmpty())
    return "";
  return ei[curFile]->fileName;
}

/***********
* FILE I/O *
***********/
bool QCEdit::Load(cstring fname,int slot)
// Load a text file into a slot
// If 'slot'==-1, the next available slot is used
{
  int i;

  if(slot==-1)
  { for(i=0;i<MAX_FILES;i++)
      if(ei[i]->fileName.IsEmpty())
      { slot=i; goto found; }
    return FALSE;			// Out of slot space
  }
 found:
  if(QFileSize(fname)>MAX_DOCUMENT_LENGTH)
  { qwarn("QCEdit: file '%s' is too large to edit",fname);
    return FALSE;
  }
//qdbg("QCEdit:Load(%s) in slot %d\n",fname,slot);
  QQuickLoad(fname,ei[slot]->text,ei[slot]->maxChar);
  ei[slot]->fileName=fname;
  // Get time of last modification to compare when saving
  stat(QExpandFilename(fname),&ei[slot]->lastStat);
  Select(slot,FALSE);
  return TRUE;
}
bool QCEdit::Save(cstring fname)
// Save the text
// If 'fname'==0, the current filename is used
// Checks modification time of the underlying file before saving,
// to avoid overwrites of old versions in memory (!)
// Returns FALSE if it did not save (no text, no file or file error)
{
  bool r;
  struct stat curStat;

//qdbg("QCEdit::Save()\n");
  if(curFile==-1)return FALSE;

  if(!fname)fname=ei[curFile]->fileName;
#ifdef ALWAYS_SAVE
  // Don't do unnecessary save
  if(!IsChanged())return TRUE;
#endif

  // Check if current modification time of file is other than
  // what we think it should be. If they don't match, someone
  // else has modified (the contents) of the file, so it is worth a good
  // warning before you actually overwrite the file.
  // Only works with seconds, but hey, safety only goes so far.
  stat(QExpandFilename(fname),&curStat);
#ifdef linux
  if(curStat.st_mtime!=ei[curFile]->lastStat.st_mtime)
#else
  // SGI/Win32
  if(curStat.st_mtim.tv_sec!=ei[curFile]->lastStat.st_mtim.tv_sec)
#endif
  { // Warn!
    if(QMessageBox("Warning",
      "The file has been modified\nby some other program.\n"
      "Are you sure you want to overwrite the file?")!=IDOK)
    { return FALSE;
    }
  }

  //if(ei[curFile]->fileName.IsEmpty())return FALSE;
  r=QQuickSave(fname,text,strlen(text));
  // Only set unchanged mode if save worked
  if(r)
  { eFlags&=~CHANGED;
    // Update expected modification time
    stat(QExpandFilename(fname),&ei[curFile]->lastStat);
  }
  return r;
}

bool QCEdit::Select(int n,bool refresh)
// Selects a file to use
// Returns FALSE in case of any error
{
  QCEditInfo *p;

  if(n<0||n>MAX_FILES)return FALSE;
  if(ei[n]==0)return FALSE;

  // Any change?
  if(curFile==n)return TRUE;

  // Store current state
  if(curFile!=-1)
  {
    p=ei[curFile];
    p->cx=cx; p->cy=cy;
    p->cxWanted=cxWanted;
    p->marker=marker;
    p->text=text;
    p->textTop=textTop;
    p->maxChar=maxChar;
    p->rangeStart=rangeStart;
    p->rangeEnd=rangeEnd;
    p->rangeDirection=rangeDirection;
    p->eFlags=eFlags;
  }

  // Copy in variables
  curFile=n;
  p=ei[curFile];
  cx=p->cx; cy=p->cy;
  cxWanted=p->cxWanted;
  marker=p->marker;
  text=p->text;
  textTop=p->textTop;
  maxChar=p->maxChar;
  rangeStart=p->rangeStart;
  rangeEnd=p->rangeEnd;
  rangeDirection=p->rangeDirection;
  eFlags=p->eFlags;

//qdbg("QCEdit:Select() '%s'\n",text);
  if(refresh)Paint();
  return TRUE;
}

void QCEdit::DoFind()
// Put up a dialog and find text
{
  char buf[MAX_FIND_LEN];
  QRect r(40,40,400,150);

//qdbg("QCEdit::DoFind\n");

  sprintf(buf,findBuffer);
  if(QDlgString("Find","Enter text to find:",buf,sizeof(buf),1,&r)==IDOK)
  {
    strcpy(findBuffer,buf);
    if(!findBuffer[0])return;
    if(!Find(findBuffer,0))
      QMessageBox("Find","String not found.",0,&r);
  }
}
void QCEdit::DoFindNext()
// Find next occurrence of 'findBuffer'
{
//qdbg("QCEdit::DoFindNext\n");
  QRect r(40,40,400,150);
  if(!findBuffer[0])return;
  if(!Find(findBuffer,0))
    QMessageBox("Find next","String not found.",0,&r);
}
void QCEdit::DoFindPrev()
// Find previous occurrence of 'findBuffer'
{
//qdbg("QCEdit::DoFindPrev\n");
  QRect r(40,40,400,150);
  if(!findBuffer[0])return;
  if(!Find(findBuffer,FIND_BACKWARDS))
    QMessageBox("Find previous","String not found.",0,&r);
}
void QCEdit::DoSearchReplace()
// Put up a dialog and find/replace text
{
  char  find[MAX_FIND_LEN],replace[MAX_FIND_LEN];
  QRect r(40,40,400,150);
  cstring sFNR="Find & replace";
  int   findLen,replaceLen,i;
  bool  first=TRUE;

//qdbg("QCEdit::DoFind\n");

 retry:
  sprintf(find,findBuffer);
  if(QDlgString(sFNR,"Enter text to find:",find,sizeof(find),1,&r)==IDOK)
  {
    strcpy(findBuffer,find);
    if(!findBuffer[0])return;
  }
  // Request replacement string
  sprintf(replace,replaceBuffer);
  if(QDlgString(sFNR,"Enter replacement text:",
    replace,sizeof(replace),1,&r)==IDOK)
  {
    strcpy(replaceBuffer,replace);
  } else return;

  findLen=strlen(findBuffer);
  replaceLen=strlen(replaceBuffer);

 find_next:
  // Find occurence
  if(!Find(findBuffer,0))
  {
    if(first)
    { QMessageBox(sFNR,"String not found.",0,&r);
      goto retry;
    } else
    {
      // No more occurences
      QMessageBox(sFNR,"No more occurences.",0,&r);
      return;
    }
  }

  // Show found part
  RangeSetStart(CurrentCharPtr()); 
  RangeSetEnd(rangeStart+findLen);
  FormatView(); RefreshView(); RefreshSmart();

  // Replace?
  if(QMessageBox(sFNR,"Replace?")==IDOK)
  {
    // Replace string
    // Replace text
    for(i=0;i<findLen;i++)
      DeleteChar();
    for(i=0;i<replaceLen;i++)
      InsertChar(replaceBuffer[replaceLen-i-1]);
    FormatView(); RefreshView(); RefreshSmart();
  } else
  {
    // Stop this run
    return;
  }
  first=FALSE;
  goto find_next;
}

/*********
* Macros *
*********/
void QCEdit::MacroHeader(cstring buf)
// Insert header with nice surrounding asterisks
{
  int   indent,i;
  char *ptr;

  indent=strlen(buf);
  InsertChar('/'); CursorRight();
  for(i=0;i<indent+3;i++){ InsertChar('*'); CursorRight(); }
  InsertString("\n* ");
  InsertString(buf);
  InsertString(" *\n");
  for(i=0;i<indent+3;i++){ InsertChar('*'); CursorRight(); }
  InsertString("/\n");
  //Goto(ptr);
}

/***************
* KEY HANDLING *
***************/
static bool IsWordChar(char c)
{
  if(c>='0'&&c<='9')return TRUE;
  if(c>='A'&&c<='Z')return TRUE;
  if(c>='a'&&c<='z')return TRUE;
  if(c>=128)return TRUE;
  return FALSE;
}
void QCEdit::DoSelectWord()
{
  RangeSetStart(CurrentCharPtr());
  while(IsWordChar(CurrentChar()))
  {
    if(!CursorRight())
      break;
  }
  RangeSetEnd(CurrentCharPtr());
  FormatView(); RefreshLine(cy); RefreshSmart();
}
void QCEdit::SubtractSpace(char *start,char *end)
// For each line, subtract the prefix space
{
 next_line:
//qdbg("SubSpce: start=%p\n",start);
  if(*start==' ')
  {
    // Remove space
//qdbg("  remove space at %p\n",start);
    Goto(start);
    DeleteChar();
    // Shrink range
    FormatView();
    rangeEnd--;
    end--;
  }
  // Find next line
  for(;*start&&start<end;start++)
  {
    if(*start==10)
    { start++;
      break;
    }
  }
  if(*start==0||start>=end)
  { // End of range
    Goto(end);
    return;
  }
  goto next_line;
}
void QCEdit::AddSpace(char *start,char *end)
// For each line, add a prefix space
{
 next_line:
//qdbg("AddSpace: start=%p\n",start);
  // Insert space
  Goto(start);
  if(!InsertChar(' '))
    return;
  // Grow range
  FormatView();
  end++;
  rangeEnd++;
  // Find next line
  for(;*start&&start<end;start++)
  {
    if(*start==10)
    { start++;
      break;
    }
  }
  if(*start==0||start>=end)
  { // End of range
    Goto(end);
    return;
  }
  goto next_line;
}
bool QCEdit::EvKeyPress(int key,int x,int y)
// Supersedes QEdit's EvKeyPress() functionality
// Specialized behavior
{ bool  refresh=FALSE;
  char  c;
  int   i,len;
  int   nBuf;
  char  buf[256],buf2[256];
  QRect r(40,40,400,150);


//qdbg("QCEdit:EvKeyPress\n");
  if(state==INACTIVE)return FALSE;

  if(key==(QK_ALT|QK_1))
  { nBuf=0;
   do_buf:
//qdbg("select buffer %d\n",nBuf);
    Select(nBuf);
    RefreshView();
    RefreshSmart();
    return TRUE;
  } else if(key==(QK_ALT|QK_2)){ nBuf=1; goto do_buf; }
  else if(key==(QK_ALT|QK_3)){ nBuf=2; goto do_buf; }
  else if(key==(QK_ALT|QK_4)){ nBuf=3; goto do_buf; }
  else if(key==(QK_ALT|QK_5)){ nBuf=4; goto do_buf; }
  else if(key==(QK_ALT|QK_6)){ nBuf=5; goto do_buf; }
  else if(key==(QK_ALT|QK_7)){ nBuf=6; goto do_buf; }
  else if(key==(QK_ALT|QK_8)){ nBuf=7; goto do_buf; }
  else if(key==(QK_ALT|QK_9)){ nBuf=8; goto do_buf; }
  else if(key==(QK_ALT|QK_0)){ nBuf=9; goto do_buf; }
  // Macros
  else if(key==(QK_ALT|QK_E))
  { // Function entry
    strcpy(buf,"");
    if(QDlgString("Function entry debugging","Enter name of function",
       buf,sizeof(buf),1,&r)==IDOK)
    {
      // Indent
      if(cx==0)InsertString("  ");
      else if(cx==1)InsertString(" ");
      InsertString("DBG_C(\"");
      InsertString(buf);
      InsertString("\")\n");
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_C))
  { // Class debug
    strcpy(buf,"");
    if(QDlgString("Class debugging","Enter name of class",
      buf,sizeof(buf),1,&r)==IDOK)
    {
      InsertString("#undef  DBG_CLASS\n");
      InsertString("#define DBG_CLASS \"");
      InsertString(buf);
      InsertString("\"\n");
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_O))
  { // Macro OBSoletize
    // BUGS: works correctly only on fully selected lines
    // BUGS: doesn't work at bottom of view
    if(RangeActive())
    { string s,e;
      s=rangeStart; e=rangeEnd;
      Goto(s); InsertString("#ifdef OBS\n");
      Goto(e+11); FormatView(); RefreshView(); InsertString("#endif\n");
      RangeClear();
      FormatView(); RefreshView();
      RefreshSmart();
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_F))
  { // Macro FOR loop; relatively intelligent
    strcpy(buf,"");
    if(QDlgString("FOR loop","Enter FOR expression (omit ; for simple loop)",
      buf,sizeof(buf),1,&r)==IDOK)
    { int indent;
      char *ptr,var;
      bool fExpandExpr;

      indent=IndentPoint(lineStart[cy]);
      if(!strchr(buf,';'))
      { // Expand expression into default: i<MAX => i=0;i<MAX;i++
        var=buf[0];
        strcpy(buf2,buf);
        sprintf(buf,"%c=0;%s;%c++",var,buf2,var);
      }
      InsertString("for(");
      InsertString(buf);
      InsertString(")\n");
      for(i=0;i<indent;i++){ InsertChar(' '); CursorRight(); }
      InsertString("{\n");
      for(i=0;i<indent+2;i++){ InsertChar(' '); CursorRight(); }
      ptr=CurrentCharPtr();
      InsertString("\n");
      for(i=0;i<indent;i++){ InsertChar(' '); CursorRight(); }
      InsertString("}");
      Goto(ptr);
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_W))
  { // Macro WHILE loop
    strcpy(buf,"");
    if(QDlgString("WHILE loop","Enter WHILE expression",
      buf,sizeof(buf),1,&r)==IDOK)
    { int indent;
      char *ptr;

      indent=IndentPoint(lineStart[cy]);
      InsertString("while(");
      InsertString(buf);
      InsertString(")\n");
      for(i=0;i<indent;i++){ InsertChar(' '); CursorRight(); }
      InsertString("{\n");
      for(i=0;i<indent+2;i++){ InsertChar(' '); CursorRight(); }
      ptr=CurrentCharPtr();
      InsertString("\n");
      for(i=0;i<indent;i++){ InsertChar(' '); CursorRight(); }
      InsertString("}");
      Goto(ptr);
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_H))
  { // Macro HEADER generation
    strcpy(buf,"");
    if(QDlgString("Header generation","Enter header text",
      buf,sizeof(buf),1,&r)==IDOK)
    { MacroHeader(buf);
    }
    return TRUE;
  } else if(key==(QK_ALT|QK_I))
  { // Macro INTRO generation
    strcpy(buf,"");
    if(QDlgString("Intro generation","Enter intro module header",
      buf,sizeof(buf),1,&r)==IDOK)
    {
      string sDate,sTime;

      InsertString("/*\n");
      InsertString(" * "); InsertString(buf); InsertString("\n");
      sDate=QCurrentDate(); sTime=QCurrentTime();
      InsertString(" * "); InsertString(sDate);
      InsertString(": Created! ("); InsertString(sTime);
      InsertString(")\n");
      InsertString(" * NOTES:\n");
      InsertString(" * (C) MarketGraph/RvG\n");
      InsertString(" */\n");
      InsertString("\n");
      InsertString("#include <qlib/debug.h>\n");
      InsertString("DEBUG_ENABLE\n");
      InsertString("\n");
    }
    return TRUE;
  } else if(key==(QK_CTRL|QK_F))
  { // Find
    DoFind();
    return TRUE;
  } else if(key==(QK_CTRL|QK_G))
  { // Find next
    DoFindNext();
    return TRUE;
  } else if(key==(QK_CTRL|QK_D))
  { // Find previous
    DoFindPrev();
    return TRUE;
  } else if(key==(QK_CTRL|QK_H))
  { // Search/replace
    DoSearchReplace();
    return TRUE;
  } else if(key==(QK_CTRL|QK_M))
  { // Mark place
    marker=CurrentCharPtr();
    return TRUE;
  } else if(key==(QK_CTRL|QK_W))
  { // Select word
    DoSelectWord();
    return TRUE;
  } else if(key==QK_F4)
  { // Goto mark (toggle)
    cstring oldMarker=marker;
    marker=CurrentCharPtr();
    Goto(oldMarker);
    FormatView(); RefreshView(); RefreshSmart();
    return TRUE;
  } else if(key==(QK_CTRL|QK_COMMA))
  {
    // For all lines in range, subtract a space
    if(RangeActive())
    {
      SubtractSpace(rangeStart,rangeEnd);
      FormatView(); RefreshView(); RefreshSmart();
      return TRUE;
    }
  } else if(key==(QK_CTRL|QK_PERIOD))
  {
    // For all lines in range, subtract a space
    if(RangeActive())
    {
      AddSpace(rangeStart,rangeEnd);
      FormatView(); RefreshView(); RefreshSmart();
      return TRUE;
    }
  } else if(key==(QK_SHIFT|QK_BRACKETLEFT))
  {
    // '{'
    InsertChar('}');
    InsertChar(' ');
    InsertChar('{');
    CursorRight();
    CursorRight();
    RefreshLine(cy); RefreshSmart();
    return TRUE;
#ifdef ND_TOO_MUCH
  } else if(key==QK_SEMICOLON)
  {
    // ';'
    InsertChar(' ');
    InsertChar(';');
    CursorRight();
    CursorRight();
    RefreshLine(cy); RefreshSmart();
    return TRUE;
#endif
  }
#define ND_FOR_APP
#ifdef ND_FOR_APP
  else if(key==(QK_CTRL|QK_S))
  { if(curFile!=-1&&ei[curFile]->fileName.IsEmpty()==FALSE)
    { if(!Save(ei[curFile]->fileName))
      { // Error
        qwarn("Can't save file.");
      }
    }
    return TRUE;
  }
#endif
  else
  { // Dispatch to QEdit
    return QEdit::EvKeyPress(key,x,y);
  }
  return FALSE;
}

void QCEdit::MacroScene()
// Add a complete scene piece of source
{
  char buf[256],sceneName[256],sceneComment[256];
  QRect r(40,40,400,150);

  strcpy(buf,"");
  if(QDlgString("Scene generation","Enter scene name",
    buf,sizeof(buf),1,&r)==IDOK)
  {
    strcpy(sceneName,buf);
    strcpy(buf,"");
    if(QDlgString("Scene generation","Enter scene comment",
      buf,sizeof(buf),1,&r)==IDOK)
    {
      char *pInsert;

      strcpy(sceneComment,buf);
      // Generate source
      sprintf(buf,"Scene: %s",sceneName);
      MacroHeader(buf);
      sprintf(buf,"void Scene%s()\n",sceneName);
      InsertString(buf);
      sprintf(buf,"// %s\n",sceneComment);
      InsertString(buf);
      InsertString("{\n");
      InsertString("  while(1)\n");
      InsertString("  {\n");
      InsertString("    INKEY();\n");
      InsertString("    ");
      // Remember insert (cursor) place
      pInsert=CurrentCharPtr();
      InsertString("\n");
      InsertString("  }\n");
      InsertString(" abort:;\n");
      InsertString("}\n");
      Goto(pInsert);
    }
  }
}
bool QCEdit::EvButtonPress(int button,int x,int y)
{
  int r;

  if(button==3)
  {
    // Menu
    string sOpts[]={ "Add scene",0 };
    //r=QMenuPopup(x,y,sOpts);
//#ifdef OBS
    QMenu *menu;
    menu=new QMenu(this,x,y,200);
    menu->AddLine("Add scene");
    r=menu->Execute();
    QDELETE(menu);
//#endif
    switch(r)
    {
      case 0: MacroScene(); break;
    }
    return TRUE;
  }
  // Parent
  return QEdit::EvButtonPress(button,x,y);
}

