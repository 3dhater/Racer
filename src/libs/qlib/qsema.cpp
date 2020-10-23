/*
 * QSema - semaphores
 * 29-03-2000: Created! (13:22:47)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#if defined(__sgi)

#include <qlib/sema.h>
#include <errno.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QSema"

// Static class members
static usptr_t *arena;

QSema::QSema(int val,int _type)
// Create a semaphore
{
  sema=0;
  type=_type;
  
  if(!arena)
    CreateArena();
  // Error?
  if(!arena)return;
  if(type==NORMAL)
  {
    sema=usnewsema(arena,val);
    if(!sema)
    {fail:
      qwarn("QSema: can't create (%s)",strerror(oserror()));
    }
//qdbg("usnewsema: %d\n",ustestsema(sema));
  } else if(type==POLLABLE)
  {
    sema=usnewpollsema(arena,val);
    if(!sema)goto fail;
  }
}
QSema::~QSema()
{
  if(sema)
  { switch(type)
    { case NORMAL  : usfreesema(sema,arena); break;
      case POLLABLE: usfreepollsema(sema,arena); break;
    }
  }
}

/*******************
* HISTORIC P and V *
*******************/
int QSema::p()
// Decrements the count of the semaphore
// Returns 0 or 1 for pollable semaphores
// For pollable semaphore, if 0 is returned, you should poll or select() on
// the semaphore's file descriptor.
{
  int r;
  r=uspsema(sema);
  if(r==-1)qwarn("QSema: p: %s",strerror(oserror()));
  return r;
}
int QSema::v()
{
  int r;
  r=usvsema(sema);
  if(r==-1)qwarn("QSema: v: %s",strerror(oserror()));
  return r;
}

/********
* VALUE *
********/
int QSema::GetValue()
// Returns the count in the semaphore
// This is a snapshot only; the value may have changed when it returns
{
  return ustestsema(sema);
}

/*********
* GLOBAL *
*********/
bool QSema::CreateArena()
// Create a global semaphore arena to create semaphores in
{
  // Only use arena from this process group (?)
  usconfig(CONF_ARENATYPE,US_SHAREDONLY);
  arena=usinit(tmpnam(0));
  if(!arena)
  { qerr("QSema: can't create arena; use _utrace=1 for more info!");
    return FALSE;
  }
  return TRUE;
}

#endif
