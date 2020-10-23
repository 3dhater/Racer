// qlib/language.h - language support

#ifndef __QLIB_LANGUAGE_H
#define __QLIB_LANGUAGE_H

#include <qlib/types.h>

class QLanguage
{
  enum { MAX_STRING=1000 };
  cstring str[MAX_STRING];
  int     strs;
  qstring fileName;

 public:
  QLanguage(cstring fname);
  ~QLanguage();

  bool Open();

  int     GetStrings(){ return strs; }
  cstring GetString(int n);
  cstring GetString(cstring name);
};

#endif
