/*
 * QSet - a basic unorganized set of (void*)
 * 27-09-99: Created!
 * NOTES:
 * -
 * (C) MarketGraph/RvG
 */

#include <qlib/set.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QSet::QSet(int max)
{
  data=0;
  used=0;
  allocated=0;
  Resize(max);
}
QSet::~QSet()
{
  Destroy();
}

void QSet::Destroy()
// Destroy 'data' memory
{
  if(data)
  {
    qfree(data);
    data=0;
  }
}

void QSet::Reset()
{
  used=0;
}
bool QSet::Resize(int newMax)
// Allocate space for 'newMax' elements.
// BUGS: doesn't store old objects
{
  if(data)
  { if(used)
      qwarn("QSet:Resize() freed old data without copying first");
    Destroy();
  }
  data=(void**)qcalloc(newMax*sizeof(void*));
  allocated=newMax;
  used=0;
  // Check result
  if(data==0)return FALSE;
  return TRUE;
}

/*********
* INSERT *
*********/
bool QSet::AddInternal(void *p,int flags)
// Store an element
// Returns FALSE if out of space
{
  if(used==allocated)
  { qwarn("QSet::Add(); set is full");
    return FALSE;
  }
  data[used]=p;
  used++;
  return TRUE;
}

bool QSet::Add(void *p)
// Store an element
{
  return AddInternal(p,0);
}

// Often used easy type conversion (no template, sorry)
bool QSet::Add(int n)
{ return Add((void*)n);
}

/*********
* SEARCH *
*********/
bool QSet::Contains(void *p)
{ int i;
  for(i=0;i<used;i++)
  { if(data[i]==p)
      return TRUE;
  }
  return FALSE;
}

// Often used easy type conversion (no template, sorry)
bool QSet::Contains(int n)
{ return Contains((void*)n);
}

/*************
* STRING SET *
*************/
QStringSet::QStringSet(int max)
  : QSet(max)
{
}
QStringSet::~QStringSet()
{
  Destroy();
}

void QStringSet::Destroy()
{ int i;
  // Free all string copies
  for(i=0;i<used;i++)
  {
    { // Free data associated with this element
//qdbg("  QSS:Destory: free data[%d]=%p\n",i,data[i]);
      qfree(data[i]);
    }
  }
  used=0;
  QSet::Destroy();
}

bool QStringSet::Add(cstring s)
{ // Create copy of string (!)
//qdbg("QSS:Add(%p;%s)\n",s,s);
  s=qstrdup(s);
#ifdef OBS
qdbg("  copy @%p (%s)\n",s,s);
cstring newS=qstrdup(s);
//qfree((void*)s);
qdbg("  copy2 @%p (%s)\n",newS,newS);
#endif
  return AddInternal((void*)s);
}

bool QStringSet::Contains(cstring s)
// Searches for the *string* (not the data ptr)
// Returns TRUE if string 's' is contained in the set.
{ int i;
  for(i=0;i<used;i++)
  { if(strcmp((cstring)data[i],s)==0)
      return TRUE;
  }
  return FALSE;
}
