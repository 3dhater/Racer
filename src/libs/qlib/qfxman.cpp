/*
 * QFXManager - manages all kinds of QFX derivatives to run at the same time
 * 18-12-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/fxman.h>
#include <stdio.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QFXManager::QFXManager()
{
  win=0;
  head=0;
  gl=0;
}
QFXManager::~QFXManager()
{
}

// FrameFX support
void QFXManager::SetWindow(QWindow *nwin)
{ win=nwin;
}
#ifdef OBSOLETE
void QFXManager::SetWidget(QGLXWidget *_wg)
{ wg=_wg;
}
#endif
void QFXManager::SetGelList(QGelList *_gl)
{ gl=_gl;
}

void QFXManager::Add(QFX *fx)
{
  //printf("QFXMan::Add(%p)\n",fx);
  fx->next=head;
  head=fx;
}
void QFXManager::Remove(QFX *fx)
{ QFX *f;
  //qdbg("QFXMgr:Remove %p\n",fx);
  if(fx==head)
  { head=fx->next;
    return;
  }
  for(f=head;f;f=f->next)
  { if(f->next==fx)
    { f->next=fx->next;
      return;
    }
  }
  qwarn("QFXManager:Remove: couldn't find FX");
}
bool QFXManager::IsDone()
{ QFX *fx;
  for(fx=head;fx;fx=fx->next)
    if(!fx->IsDone())
      return FALSE;
  return TRUE;
}

void QFXManager::Iterate(int flags)
{ QFX *fx;
  //qdbg("QFXMan::Iterate\n");
  //QNap(1);		// Yield videoout
  // Remember current locations/sizes
  if(gl)gl->UpdatePipeline();
  // Make modifications to various controls (if any)
  for(fx=head;fx;fx=fx->next)
    fx->Iterate();
  // Update any changed gels in an intelligent way
  if(gl)gl->Update();
  if(!(flags&NO_SWAP))
  { if(win)win->GetQXWindow()->Swap();
  }
}

void QFXManager::Run()
{ // Any effects?
  //qdbg("QFXManager::Run\n");
  if(!head)return;
  while(!IsDone())
  { Iterate();
  }
}

