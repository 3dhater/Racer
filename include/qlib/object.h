// qlib/qobject.h

#ifndef __QLIB_QOBJECT_H
#define __QLIB_QOBJECT_H

#ifndef __QLIB_TYPES_H
#include <qlib/types.h>
#endif

#ifdef DEBUG
class QObject;
extern QObject *qObjectList;
#endif

class QObject
{
  string   name;
  QObject *prev,*next;		// List of QObjects

 protected:

 public:
  virtual string ClassName(){ return "object"; }

 public:
  QObject();
  virtual ~QObject();

  // Attribs
  QObject *GetPrev(){ return prev; }
  QObject *GetNext(){ return next; }
  cstring Name();
  void    Name(cstring s);
};

// Debugging
void QDebugListObjects();

#endif
