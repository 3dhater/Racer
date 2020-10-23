// qlib/sema.h

#ifndef __QLIB_SEMA_H
#define __QLIB_SEMA_H

#if defined(__sgi)
#include <ulocks.h>
#endif
#if defined(linux)
typedef void *usema_t;
typedef void *usptr_t;
#endif
#include <qlib/types.h>

class QSema
{
 public:
  enum Types			// Type of semaphore
  { NORMAL,			// usnewsema() type
    POLLABLE			// usnewpollsema() type
  };

 protected:
  // 1 arena is used for multiple semaphores
  //static usptr_t *arena;
  usema_t *sema;
  int type;

 public:
  QSema(int val=0,int type=NORMAL);
  ~QSema();

  int  p();
  int  v();

  int  GetValue();

  bool CreateArena();
};

#endif
