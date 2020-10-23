// qlib/set.h

#ifndef __QLIB_SET_H
#define __QLIB_SET_H

#include <qlib/types.h>

class QSet
{
#ifdef OBS
 public:
  enum
  { IS_COPY=1			// data[] entry is allocated copy
  };
#endif

 protected:
  int allocated,used;
  void **data;
  //int  *dataFlags;

  bool AddInternal(void *p,int flags=0);
  virtual void Destroy();

 public:
  QSet(int max=100);
  virtual ~QSet();

  // Insert
  bool Add(void *p);
  //bool Add(cstring s);
  bool Add(int n);

  // Remove
  //...

  // Admin
  void Reset();
  bool Resize(int newMax);

  // Information retrieval
  bool Contains(void *p);
  //bool Contains(cstring s);
  bool Contains(int n);
};

class QStringSet : public QSet
// Set which stores strings (copies of strings!)
{
 protected:
  void Destroy();

 public:
  QStringSet(int max=100);
  ~QStringSet();

  // Insert
  bool Add(cstring s);

  // Remove
  //...

  // Information retrieval
  bool Contains(cstring s);
};

#endif
