// qlib/string.h - string abstraction

#ifndef __QLIB_STRING_H
#define __QLIB_STRING_H

#include <iostream.h>
#include <string.h>
#include <qlib/types.h>

struct srep		// Internal string representation
{ char *s;		// C string
  int   refs;		// Reference count of 's' buffer
  int   alen;		// Allocated chars
  srep(){ refs=1; alen=0; s=0; }
  // Storage
  void Resize(int n);
  void Free();
};

class qstring
{
  srep *p;

 public:
  qstring(const char *);	// qstring x = "abc"
  qstring();			// qstring x;
  qstring(const qstring &);	// qstring x = qstring ...
  qstring& operator=(const char *);
  qstring& operator=(const qstring &);
  ~qstring();

  // Comparison
  bool operator==(const char *);

  // Indexing
  char& operator[](int i);
  const char& operator[](int i) const;

  // Type conversion
  operator const char* () const;
  cstring cstr(){ return p->s; }
  //operator char* ();

/*
  friend ostream& operator<<(
*/
  friend int operator==(const qstring &x,const char *s)
  { return strcmp(x.p->s,s)==0; }
  friend int operator==(const qstring &x,const qstring &y)
  { return strcmp(x.p->s,y.p->s)==0; }

  friend int operator!=(const qstring &x,const char *s)
  { return strcmp(x.p->s,s)!=0; }
  friend int operator!=(const qstring &x,const qstring &y)
  { return strcmp(x.p->s,y.p->s)!=0; }

  bool IsEmpty();
};

typedef const qstring cqstring;

// Flat string operations (qlib/string.cpp)
cstring strext(cstring s);
char    QCharCutAccent(char c);
char    QToUpper(char c);
void    QStrUpr(string s);
void    QStripLF(string s);

#endif
