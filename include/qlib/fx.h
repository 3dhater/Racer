// qlib/qfx.h - a general effect (virtual base class)

#ifndef __QLIB_QFX_H
#define __QLIB_QFX_H

#include <qlib/object.h>

#define QFXF_DONE	1	// FX is done processing
#define QFXF_EQUALIZE	2	// For double buffered effects

#define QFXT_FX		0	// QFX class types
#define QFXT_VIDEO	1	// QVideoFX

class QFX : public QObject
{
 public:
  string ClassName(){ return "fx"; }
  virtual int FXType(){ return QFXT_FX; }
    
 protected:
  int flags;
 public:
  QFX *next;

 public:
  QFX(int flags=0);
  ~QFX();

  // Using
  virtual void Iterate();
  virtual bool IsDone(){ if(flags&QFXF_DONE)return TRUE; return FALSE; }

  void Done(bool yn){ if(yn)flags|=QFXF_DONE; else flags&=~QFXF_DONE; }

  int GetFlags(){ return flags; }
};

class QFXList : public QObject
// Use QFXManager instead
{
 public:
  string ClassName(){ return "fxlist"; }

 protected:
  QFX *head;

 public:
  QFXList();
  ~QFXList();

  void Add(QFX *fx);		// Add any type of effect
  bool IsDone();		// All effects done?

  QFX *GetHead(){ return head; }

  void Iterate();		// Do 1 iteration, swap?
  virtual void Run();
};

#endif
