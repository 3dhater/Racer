// qlib/state.h - states in which an application may reside

#ifndef __QLIB_STATE_H
#define __QLIB_STATE_H

#include <qlib/event.h>
#include <qlib/window.h>

class QFile;

typedef bool (*QSEventFunc)(QEvent *);
typedef void (*QSPaintFunc)();
typedef void (*QSEnterFunc)();
typedef void (*QSLeaveFunc)();
typedef void (*QSReadFunc)(QFile *);
typedef void (*QSWriteFunc)(QFile *);

class QState : public QObject
// An application always has at most 1 current state
{
 protected:
  QSEventFunc evFunc;
  QSPaintFunc paintFunc;
  QSEnterFunc enterFunc;
  QSLeaveFunc leaveFunc;
  string      name;

 public:
  QState(string name,QSEventFunc evFunc=0,QSPaintFunc paintFunc=0,
    QSEnterFunc enterFunc=0,QSLeaveFunc leaveFunc=0);
  ~QState();

  string GetName(){ return name; }
  void   SetName(string nname){ name=nname; }

  void SetPaintFunc(QSPaintFunc paintFunc);
  bool ProcessEvent(QEvent *e);
  void Paint();
  void Leave();
  void Enter();
};

class QButton;
class QFile;

class QStateManager : public QWindow
// A statemanger is window because it looks (literally) like a VCR
{
 protected:
  QState *curState;
  QState *globalState;		// Global that has priority
  QState **stateList;		// QState list
  int    stateListSize;
  int    stateCount;		// #states in list
  QButton *bPlay,*bRewind,	// Buttons in VCR
          *bForward,*bReverse;
  QSReadFunc readFunc;		// Global reading/writing states
  QSWriteFunc writeFunc;

  // VCR
  QFile  *fVCR;			// Recording frames (stills)
  int     vcrFrames,		// #frames in VCR
          vcrFrame;		// Current active frame

 public:
  // VCR
  bool VCR_Record();
  bool VCR_SkipBack();
  bool VCR_SkipForward();
  bool VCR_FrameRead(bool fRewind=FALSE);
  bool VCR_FrameBack();
  bool VCR_FrameForward();
  bool VCR_UseFile(string fname);
  bool VCR_SaveAs(string fname);
  void VCR_Reset();

 public:
  QStateManager(QWindow *parent,QRect *pos=0);
  ~QStateManager();

  void SetGlobalState(QState *gstate);
  void SetGlobalRW(QSReadFunc rf,QSWriteFunc wf);

  // The list
  void Add(QState *state);
  void Remove(QState *state);
  QState *Search(string name);

  void Paint(QRect *r=0);
  bool ProcessEvent(QEvent *e);
  void GotoState(QState *state,bool silent=FALSE);

  QState *GetCurrentState(){ return curState; }
};

#endif
