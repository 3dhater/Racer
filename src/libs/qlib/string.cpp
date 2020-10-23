/*
 * Qlib/string.cpp - string operations
 * 03-08-99: Created!
 * (C) MarketGraph/RvG
 */

#include <qlib/string.h>
#include <ctype.h>
#include <qlib/debug.h>
DEBUG_ENABLE

cstring strext(cstring s)
// Returns extension or 0 if no extension (.) was found
// FUTURE: scan string only once; find last dot
{ int i,len;
  QASSERT_0(s);			// strext arg is null
  len=strlen(s)-1;
  for(i=len-1;i>=0;i--)
    if(s[i]=='.')return &s[i+1];
  return 0;
}

/**********
* ACCENTS *
**********/
char QCharCutAccent(char c)
// Given char 'c', cut off any accent and return the normal character
// For unsupported chars, returns just 'c' again.
{
  switch(c)
  { case '�': case '�': case '�': case '�': case '�': case '�':
      return 'A';
    case '�': return 'c';
    case '�': return c;		// Hmm?
    case '�': return 'C';
    case '�': case '�': case '�': case '�': return 'E';
    case '�': case '�': case '�': case '�': return 'I';
    case '�': return 'D';
    case '�': return 'N';
    case '�': case '�': case '�': case '�': case '�': case '�': return 'O';
    case '�': case '�': case '�': case '�': return 'U';
    case '�': return 'Y';
    case '�': case '�': case '�': case '�': case '�': case '�': return 'a';
    case '�': return c;		// Hmm?
    case '�': return 'c';
    case '�': case '�': case '�': case '�': return 'e';
    case '�': case '�': case '�': case '�': return 'i';
    case '�': return 'n';
    case '�': case '�': case '�': case '�': case '�': case '�': case '�':
      return 'o';
    case '�': case '�': case '�': case '�': return 'u';
    case '�': case '�': return 'y';
  }
  // No change
  return c;
}

char QToUpper(char c)
// toupper() that works for accents too
// For unsupported chars, returns just 'c' again.
{
  switch(c)
  {
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    //case '�': return c;         // Hmm?
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '?';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    case '�': return '�';
    //case '�': return c;
  }
  // No special char
  return toupper(c);
}
void QStrUpr(string s)
// strupr() only handles accents as well (so normally better than strupr())
{
  QASSERT_V(s!=0);	// QStrUpr argument string == NULL
  for(;*s;s++)
    *s=QToUpper(*s);
}

void QStripLF(string s)
// Strips any trailing LF's
{
  int len;
 retry:
  len=strlen(s);
  if(len>0)
  { if(s[len-1]==10)
    { s[len-1]=0;
      goto retry;
    }
  }
}

#ifdef ND_NOT_PRETTY
string strbase(string s)
{
  string sExt;
  static char buf[256];
  sExt=strext(s);
  if(*sExt)
  { strcpy(buf,s);
    buf[sExt-s-1]=0;
  } else return s;
  return buf;
}
#endif
