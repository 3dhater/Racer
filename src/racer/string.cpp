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
  { case 'À': case 'Á': case 'Â': case 'Ã': case 'Ä': case 'Å':
      return 'A';
    case '¢': return 'c';
    case 'Æ': return c;		// Hmm?
    case 'Ç': return 'C';
    case 'È': case 'É': case 'Ê': case 'Ë': return 'E';
    case 'Ì': case 'Í': case 'Î': case 'Ï': return 'I';
    case 'Ð': return 'D';
    case 'Ñ': return 'N';
    case 'Ò': case 'Ó': case 'Ô': case 'Õ': case 'Ö': case 'Ø': return 'O';
    case 'Ù': case 'Ú': case 'Û': case 'Ü': return 'U';
    case 'Ý': return 'Y';
    case 'à': case 'á': case 'â': case 'ã': case 'ä': case 'å': return 'a';
    case 'æ': return c;		// Hmm?
    case 'ç': return 'c';
    case 'è': case 'é': case 'ê': case 'ë': return 'e';
    case 'ì': case 'í': case 'î': case 'ï': return 'i';
    case 'ñ': return 'n';
    case 'ð': case 'ò': case 'ó': case 'ô': case 'õ': case 'ö': case 'ø':
      return 'o';
    case 'ù': case 'ú': case 'û': case 'ü': return 'u';
    case 'ý': case 'ÿ': return 'y';
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
    case 'à': return 'À';
    case 'á': return 'Á';
    case 'â': return 'Â';
    case 'ã': return 'Ã';
    case 'ä': return 'Ä';
    case 'å': return 'Å';
    //case 'æ': return c;         // Hmm?
    case 'ç': return 'Ç';
    case 'è': return 'È';
    case 'é': return 'É';
    case 'ê': return 'Ê';
    case 'ë': return 'Ë';
    case 'ì': return 'Ì';
    case 'í': return 'Í';
    case 'î': return 'Î';
    case 'ï': return 'Ï';
    case 'ñ': return 'Ñ';
    case 'ð': return '?';
    case 'ò': return 'Ò';
    case 'ó': return 'Ó';
    case 'ô': return 'Ô';
    case 'õ': return 'Õ';
    case 'ö': return 'Ö';
    case 'ø': return 'Ø';
    case 'ù': return 'Ù';
    case 'ú': return 'Ú';
    case 'û': return 'Û';
    case 'ü': return 'Ü';
    case 'ý': return 'Ý';
    //case 'ÿ': return c;
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
