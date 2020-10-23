// qlib/qfxman.h - FX manager

#ifndef __QLIB_QFXMAN_H
#define __QLIB_QFXMAN_H

#include <qlib/gel.h>
//#include <qlib/qvideofx.h>
#include <qlib/fx.h>
//#include <qlib/qwidget.h>

class QFXManager;
typedef bool (*FXMGRCB)(QFXManager *fxm);

class QFXManager : public QObject
// Manages a list of effects, running them etc.
{
  string ClassName(){ return "fxmanager"; }
 public:
	enum IterateFlags
	{ NO_SWAP=1
	};
 protected:
  QFX *head;
  QWindow *win;			// XWindow that holds any gels
  QGelList   *gl;		// A gel list
  // Callback functions
  FXMGRCB     cbPreSwap;

 public:
  QFXManager();
  ~QFXManager();

  // Attribs
  QFX *GetHead(){ return head; }
  FXMGRCB GetPreSwapCB(){ return cbPreSwap; }
  void    SetPreSwapCB(FXMGRCB func){ cbPreSwap=func; }

  void Add(QFX *fx);            // Add any type of effect
  void Remove(QFX *fx);		// Remove from the list
  bool IsDone();                // All effects done?

  // Support for different types of effects
  QWindow *GetWindow(){ return win; }
  void SetWindow(QWindow *win);		// Hook window
  void SetGelList(QGelList *gl);	// Hook gellist

  void Iterate(int flags=0);
  void Run();
};

#endif

