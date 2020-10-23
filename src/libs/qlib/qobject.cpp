/*
 * QObject - base class of all other Q classes
 * 14-11-96: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/object.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Globally mainted object list for debugging
QObject *qObjectList;

QObject::QObject()
{
  name=0;
#ifdef DEBUG
  // Add object to list of all objects
  next=qObjectList;
  if(qObjectList)
    qObjectList->prev=this;
  prev=0;
  qObjectList=this;
#else
  next=prev=0;
#endif
}
QObject::~QObject()
{
//qdbg("QObject:dtor\n");
  QFREE(name);
#ifdef DEBUG
  // Remove from object list (mind the root)
  if(qObjectList==this)
    qObjectList=next;
  if(prev)
    prev->next=next;
  if(next)
    next->prev=prev;
#endif
}

cstring QObject::Name()
// Returns object name
{
  if(!name)return "<unnamed>";
  return name;
}

void QObject::Name(cstring s)
{
  QFREE(name);
  name=qstrdup(s);
}

/************
* Debugging *
************/
void QDebugListObjects()
// Output all objects that are known to us.
{
  QObject *o;
  qdbg("### Q Object Report ###\n");
  for(o=qObjectList;o;o=o->GetNext())
  {
    qdbg("Object '%s', class %s (@%p)\n",o->Name(),o->ClassName(),o);
  }
  qdbg("#######################\n");
}

