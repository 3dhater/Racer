/*
 * QState - state handling (current & managing the VCR)
 * 10-04-97: Created!
 * 23-05-97: Loading/saving vcr files. (^L/^S)
 * NOTES:
 * - I still wonder whether it was a good idea to make the StateManager
 *   a subclass of QWindow. Now, it is always the visual VCR that you see.
 * (C) MarketGraph/RVG
 */

#include <qlib/state.h>
#include <qlib/app.h>
#include <qlib/button.h>
#include <qlib/keys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qlib/file.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define QSM_MAX_STATE	100		// Max #states in system
#define QSM_MAX_NAME_LEN 32		// State names

#define VCR_TEMP_FILE	"temptape.vcr"	// Working tape

/*********
* QSTATE *
*********/
QState::QState(string iname,QSEventFunc ievFunc,QSPaintFunc ipaintFunc,
  QSEnterFunc ienterFunc,QSLeaveFunc ileaveFunc)
{
  name=iname;
  evFunc=ievFunc;
  paintFunc=ipaintFunc;
  enterFunc=ienterFunc;
  leaveFunc=ileaveFunc;
  app->GetStateManager()->Add(this);
}
QState::~QState()
{
  app->GetStateManager()->Remove(this);
}

void QState::SetPaintFunc(QSPaintFunc npaintFunc)
{
  paintFunc=npaintFunc;
}

bool QState::ProcessEvent(QEvent *e)
// Dispatch event to the event handler function
{ if(evFunc)
    return evFunc(e);
  return FALSE;
}

void QState::Paint()
{ if(paintFunc)
    paintFunc();
}
void QState::Leave()
{ if(leaveFunc)
    leaveFunc();
}
void QState::Enter()
{ if(enterFunc)
    enterFunc();
}

/*****************
* QSTATE MANAGER *
*****************/
QStateManager::QStateManager(QWindow *parent,QRect *ipos)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{ QRect pb;
  curState=0;
  globalState=0;
  stateListSize=QSM_MAX_STATE;
  stateList=(QState**)qcalloc(stateListSize*sizeof(QState*));
  stateCount=0;
  bRewind=0;
  bPlay=0;
  // etc. zeroing
#ifdef NOTDEF
  qdbg("pos=%p\n",pos);
  pb.x=pos->x; pb.y=pos->y;
  pb.wid=pos->wid; pb.hgt=pos->hgt;
  pb.wid=pos->wid/4;
  pb.wid=80; pb.hgt=80;
  qdbg("new bRewind\n");
#endif
  QRect r(0,0,100,10);		// Rebuilt during Paint()
  bRewind=new QButton(app->GetShell(),&r,QB_MAGIC_REWIND);
  bPlay=new QButton(app->GetShell(),&r,QB_MAGIC_PLAY);
#ifndef WIN32
  bPlay->ShortCut(QK_CTRL|QK_RIGHT);
#endif
  bForward=new QButton(app->GetShell(),&r,QB_MAGIC_FORWARD);
  bReverse=new QButton(app->GetShell(),&r,QB_MAGIC_REVERSE);
#ifndef WIN32
  bReverse->ShortCut(QK_CTRL|QK_LEFT);
#endif
  // VCR
  fVCR=new QFile(VCR_TEMP_FILE,QFile::WRITEREAD);
  if(!fVCR->IsOpen())
  { qerr("Can't open VCR file\n");
  }
  vcrFrames=vcrFrame=0;
  readFunc=0; writeFunc=0;

  //qdbg("QStateMgr::ctor ret\n");
}
QStateManager::~QStateManager()
{
  QDELETE(bPlay);
  QDELETE(bRewind);
  QDELETE(bForward);
  QDELETE(bReverse);
#ifdef OBS
  if(stateList)
    qfree((void*)stateList);
#endif
  QDELETE(fVCR);
}

void QStateManager::Add(QState *s)
// Add state to the list of known states
{ int i;
  //qdbg("QStateMgr::Add(%p)\n",s);
  // Check for an existing name
  if(Search(s->GetName()))
  { printf("Multiply defined state named '%s'\n",s->GetName());
    exit(0);
  }
  stateList[stateCount]=s;
  stateCount++;
}
void QStateManager::Remove(QState *s)
// Remove state
{
  int i;
  if(stateCount<1)
  { qerr("QStateManager:Remove without states");
    return;
  }
  if(stateList[stateCount-1]!=s)
  { qerr("QStateManager:Remove: state must be the last added");
    return;
  }
  stateCount--;
}

QState *QStateManager::Search(string name)
{ int i;
  for(i=0;i<stateCount;i++)
  { if(stateList[i])
    { if(!strcmp(name,stateList[i]->GetName()))
      { return stateList[i];
      }
    }
  }
  return 0;
}

void QStateManager::SetGlobalState(QState *gstate)
{ globalState=gstate;
}
void QStateManager::SetGlobalRW(QSReadFunc rf,QSWriteFunc wf)
{ readFunc=rf;
  writeFunc=wf;
}

bool QStateManager::ProcessEvent(QEvent *e)
{
  //qdbg("QStateMgr::ProcEvent %d\n",e->type);
  // First, handle the VCR buttons
  if(e->type==QEVENT_CLICK)
  { if(e->win==(void*)bReverse)
    { qdbg("Reverse 1 frame\n");
      VCR_FrameBack();
    } else if(e->win==(void*)bPlay)
    { VCR_FrameForward();
    }
  } else if(e->type==QEVENT_KEYPRESS)
  { 
#ifndef WIN32
    if(e->n==(QK_CTRL|QK_S))
    { VCR_SaveAs("bc.vcr");
      return TRUE;
    } else if(e->n==(QK_CTRL|QK_L))
    { if(VCR_UseFile("bc.vcr"))
      { // Don't mess on in the new file
        VCR_SaveAs(VCR_TEMP_FILE);
        VCR_UseFile(VCR_TEMP_FILE);
      }
      return TRUE;
    }
#endif
  }

  // Give the global state first chance to handle event
  if(globalState)
    if(globalState->ProcessEvent(e))
      return TRUE;
  // Otherwise send the event through to the current state
  if(curState)
    return curState->ProcessEvent(e);
  return FALSE;
}

void QStateManager::Paint(QRect *r)
// VCR look
{ QRect pb;
  QButton *b;
  QColor col(200,0,50);
  if(!bRewind)return;
  //qdbg("QSM:Paint; bRewi\n");
  // Reposition buttons
  QRect pos;
  GetPos(&pos); //pos.x=pos.y=0;

  pb.x=pos.x; pb.y=pos.y;
  pb.wid=pos.wid/4; pb.hgt=pos.hgt;
  b=bRewind; b->Move(pb.x,pb.y); b->Size(pb.wid,pb.hgt); b->Paint();
  pb.x+=pb.wid;
  b=bReverse; b->Move(pb.x,pb.y); b->Size(pb.wid,pb.hgt); b->Paint();
  pb.x+=pb.wid;
  b=bPlay; b->Move(pb.x,pb.y); b->Size(pb.wid,pb.hgt); b->Paint();
  pb.x+=pb.wid;
  b=bForward; b->Move(pb.x,pb.y); b->Size(pb.wid,pb.hgt); b->Paint();
  /*canvas->SetColor(&col);
  canvas->Rectfill(pos);*/
}

void QStateManager::GotoState(QState *state,bool silent)
{
  //qdbg("Goto state\n");
  if(curState)curState->Leave();
  curState=state;
  if(curState)state->Enter();
  // Save progress if wanted
  if(silent==FALSE)
    VCR_Record();
  //qdbg("  QSM:GotoState ret\n");
}

/**********
* VCR OPS *
**********/
bool QStateManager::VCR_Record()
// Record a frame (curState)
{ int pos;
  char buf[QSM_MAX_NAME_LEN];
  if(!curState)return TRUE;
  // Check curState
  //if(curState->GetWriteFunc())curState->Write(fVCR);

  // Write state header
  pos=fVCR->Tell();
  //qdbg("VCR_Record: pos=%d\n",pos);
  for(int i=0;i<QSM_MAX_NAME_LEN;i++)buf[i]=0;
  strncpy(buf,curState->GetName(),QSM_MAX_NAME_LEN-1);
  fVCR->Write(buf,QSM_MAX_NAME_LEN);

  // Use global writer
  if(writeFunc)writeFunc(fVCR);

  // Write location of previous state
  fVCR->Write(&pos,sizeof(pos));
  vcrFrame++;
  // Set #frames directly; otherwise vcrFrames may be inconsistent
  // when you just rewinded several frames.
  vcrFrames=vcrFrame;
  //qdbg("  frames: %d, prev=%d, current=%d\n",vcrFrames,pos,fVCR->Tell());
  return TRUE;
}

//
// VCR file seeking frames (file location)
//
bool QStateManager::VCR_SkipBack()
// Push the file pointer back 1 state; don't read or do anything else.
// Returns TRUE is succesful (FALSE for example when no previous frames
// exist)
{
  int pos,posPrev;
  // Any frames before this one?
  pos=fVCR->Tell();
  //qdbg("SkipBack: pos=%d\n",pos);
  if(pos<=0)return FALSE;
  // Find location of previous frame
  pos-=4;
  fVCR->Seek(pos);
  fVCR->Read(&posPrev,sizeof(posPrev));
  fVCR->Seek(posPrev);
  vcrFrame--;
  //qdbg("Skipped back to %d\n",posPrev);
  return TRUE;
}
bool QStateManager::VCR_SkipForward()
// Returns TRUE if there is in fact a next frame
{
  int oldPos,pos;
  //qdbg("zsm::vcrskipfwd nyi");
  // Any frames left?
  pos=fVCR->Tell();
  //printf("  vcr_skipfw: pos=%d, size=%d\n",pos,fVCR->Size());
  if(pos==fVCR->Size())return FALSE;
  return TRUE;
}

//
// Reading frames and moving between them
//
bool QStateManager::VCR_FrameRead(bool fRewind)
// Reads frame data and setup the application for that frame (ShowFrame)
// If fRewind==TRUE, then the file ptr will be rewound
// to the start of the frame data (used with FrameBack?)
{
  char buf[QSM_MAX_NAME_LEN];
  QState *newState;
  int oldPos;

  oldPos=fVCR->Tell();
  fVCR->Read(buf,QSM_MAX_NAME_LEN);
  //qdbg("QSM:FrameRead: %s\n",buf);
  newState=Search(buf);
  if(!newState)
  { qerr("State '%s' was not found in this application.",buf);
    return FALSE;
  }
  
  // Use global reader
  if(readFunc)readFunc(fVCR);
  if(fRewind)
  { // Go back to start of state
    fVCR->Seek(oldPos);
  } else
  { // Skip the 'previous position' indicator
    fVCR->Seek(sizeof(oldPos),QFile::S_CUR);
    vcrFrame++;
  }
  // Paint the frame
  curState=newState;
  curState->Enter();
  curState->Paint();
  // Synchronize gels and such (don't allow too fast scrubbing)
  app->Sync();
  qdbg("new pos=%d\n",fVCR->Tell());
  return TRUE;
}

bool QStateManager::VCR_FrameBack()
// Go back 1 state
{ 
#ifdef OBSOLETE
  if(!VCR_SkipBack())return FALSE;
  if(!VCR_SkipBack())return FALSE;
#else
  // Don't cancel on failed backskipping to avoid the problem
  // that when forwarding again, the first frame is used twice
  VCR_SkipBack();
  VCR_SkipBack();
#endif
  return VCR_FrameRead();
}

bool QStateManager::VCR_FrameForward()
// Advance 1 frame
{ if(!VCR_SkipForward())return FALSE;
  return VCR_FrameRead();
}

//
// Loading and saving VCR files
//
bool QStateManager::VCR_UseFile(string fname)
// Use a (probably prerecorded) state file
// Keeps recording in the previous file if the new one could not be opened.
{ /*FILE *fNew;
  fNew=fopen(fname,"rb+");
  if(!fNew)return FALSE;
  if(fVCR)fclose(fVCR);
  fVCR=fNew;*/

  QFile *fNew;
  fNew=new QFile(fname,QFile::READWRITE);
  if(!fNew->IsOpen())
  { qwarn("VCR file not found"); return FALSE; }
  if(fVCR)delete fVCR;
  fVCR=fNew;
#ifdef NOTDEF
  fVCR=new QFile("zsmgr.vcr",QFile::WRITEREAD);
  if(!fVCR->IsOpen())
  { qerr("Can't open VCR file\n");
  }
#endif

  // Find #states in file
  //fseek(fVCR,0,SEEK_END);
  fVCR->Seek(0,QFile::S_END);
  for(vcrFrames=0;VCR_SkipBack();vcrFrames++);
  qdbg("VCR_UseFile: %d states in '%s'\n",vcrFrames,fname);

  // Use last recorded state in file (if any)
  fVCR->Seek(0,QFile::S_END);
  //fseek(fVCR,0,SEEK_END);
  vcrFrame=vcrFrames;
  if(VCR_SkipBack())
    VCR_FrameRead(FALSE);
  return TRUE;
}
void QStateManager::VCR_Reset()
// Clear out the states and start all over again
{
  // Reopen
  delete fVCR;
  fVCR=new QFile(VCR_TEMP_FILE,QFile::WRITEREAD);
  if(!fVCR->IsOpen())
  { qerr("Can't open VCR file\n");
  }
  vcrFrames=vcrFrame=0;
}

bool QStateManager::VCR_SaveAs(string fname)
// Save current recording as 'fname'
// DON'T switch to 'fname'
// Makes an inline copy, taking care not to disturb the current still
// position, which might not be the end of the file.
{ int i;
  FILE *fw;
  long oldPos;          // Old file position (==length of copy)
  char buf[256];
  char c;

  //oldPos=ftell(fVCR);
  oldPos=fVCR->Tell();
  qdbg("> Saving VCR recording as '%s'",fname);
  fw=fopen(fname,"wb");
  if(!fw){ qwarn("** VCR Save failed"); return FALSE; }

  //fseek(fVCR,0,SEEK_SET);
  fVCR->Seek(0);
  for(i=0;i<oldPos;i++)
  { //c=(char)fgetc(fVCR);
    c=fVCR->GetChar();
    fputc(c,fw);
  }
  fclose(fw);
  fVCR->Seek(oldPos);
  //fseek(fVCR,oldPos,SEEK_SET);  // Should be there already
  qdbg("\n");
  return TRUE;
}
