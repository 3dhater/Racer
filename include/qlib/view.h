// qview.h
// A view encapsulation

#ifndef __QLIB_QVIEW_H
#define __QLIB_QVIEW_H

#include <qlib/event.h>

class QView : public QObject
{
 public:
  QView();
  ~QView();
 public:
  virtual bool Event(QEvent *e);
};

#endif
