// html.h - HTML Generator

#ifndef __QLIB_HTML_H
#define __QLIB_HTML_H

#include <qlib/types.h>
#include <stdio.h>

class QHTML
{
  FILE *fp;

 public:
  QHTML(string fname);
  ~QHTML();

  FILE *GetFP();
};

#endif

