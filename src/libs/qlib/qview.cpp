//
// QView.cpp
//

#include "qlib/view.h"

QView::QView()
{
}

QView::~QView()
{
}

bool QView::Event(QEvent *e)
// Default handler does nothing
{
  return FALSE;
}

