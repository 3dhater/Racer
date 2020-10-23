// qlib/qbc.h - Broadcasted window
// Future:
// - Hold GelList

#ifndef __QLIB_BC_H
#define __QLIB_BC_H

#include <qlib/draw.h>
#include <qlib/group.h>
#include <qlib/alphabuf.h>
#include <qlib/gel.h>

class QWindow;
class QGroup;

class QBC : public QDraw
// A broadcast window; gels live in here.
// There should be only 1 active QBC window per application (app->GetBC()).
// Also has transparent alpha buffer option if you need alpha
// (QUITE different from normal operation)
{
 public:
  string ClassName(){ return "bc"; }
 public:
  enum Format
  { PAL=1,
    NTSC=2,
    PAL_DIGITAL=3,
    FULLSCREEN=4,
    CUSTOM=5
  };

#ifdef OBS
  const KEEP_OUT=1;		// Flags; don't move pointer inside window
  const EMULATE_16_9=2;		// Emulate HDTV/PALplus
  const int SAFETY=4;		// Draw safety frame
#endif
  enum Flags
  { KEEP_OUT=1,			// Flags; don't move pointer inside window
    EMULATE_16_9=2,		// Emulate HDTV/PALplus
    SAFETY=4,			// Draw safety frame
    PREVIEW_ALPHA=8		// Show alpha output onscreen
  };

 protected:
  int flags;
 public:
  QBitMap *bmEmul[2];		// Emulation storage
  int      buffer;		// Double buffered backing store
 protected:
  QAlphaBuf *alphaBuf;          // If alpha needs to be output
  QCanvas   *cvScreen;          // Canvas of the screen window
  QGroup    *group;		// Outline
  
 public:
  QBC(QWindow *parent,int typ=PAL,int x=48,int y=48,int wid=320,int hgt=256);
  ~QBC();

  //void Create();

  void Paint(QRect *r);
  void PaintSafety();
  void KeepOut(bool yn);
  bool EvMotionNotify(int x,int y);

  void Swap();

  QGelList *GetGelList();

  // Alpha support
  bool IsAlpha();
  QAlphaBuf *GetAlphaBuf(){ return alphaBuf; }
  bool SwitchToAlpha(bool genlock);
  QCanvas  *GetScreenCanvas(){ return cvScreen; }
  void EnableAlphaPreview();
  void DisableAlphaPreview();
  
  // High-level
  void Emulate16x9(bool yn);
  void Safety(bool yn);
};

#endif

