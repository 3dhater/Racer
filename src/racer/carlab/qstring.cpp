/*
 * QString - string class for easier string handling
 * 15-07-1999: Created! (based on Bjarne Stroustrup's example; C++ pg 248)
 * 27-08-1999: Flood checking; cost a lot of time in MT's Installer.
 * Now checks for memory overwrites beyond the buffer.
 * FUTURE:
 * - Don't allocate new buffer if new assigned string fits into current
 * buffer. Only free it when large strings are used (>1K).
 * - Let this class slowly sip through the entire QLib (and others)
 * (C) MarketGraph/RvG
 */

#include <qlib/string.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Check for bad usages of strings?
// This will set a marker at the end of the string, checking if it is
// still there when freeing up the space and notifying if it was malformed
// This is handy because qstring's, when typecast to 'string' may be presumed
// to contain more space than they actually do, thus overwriting memory
// beyond the character space.
#define CHECK_FLOOD
#define FLOOD_CHAR	0xE3		// Unlikely char

#undef  DBG_CLASS
#define DBG_CLASS "qstring"

//
// Constructors
//
qstring::qstring()
{
  DBG_C("ctor")
//qdbg("qstring ctor (this=%p)\n",this);
  p=new srep;
  // Empty string
  p->Resize(1);
  //p->s=new char[1];
  *p->s=0;
//qdbg("qstring ctor; p->s=%p\n",p->s);
}
qstring::qstring(const qstring& x)
// Construct based on other qstring
// Copies only srep pointer, NOT the string itself (for efficiency)
{
  DBG_C("ctor-copy")
//qdbg("qstring ctor(const qstring&)\n");
  x.p->refs++;
  p=x.p;
}
qstring::qstring(const char *s)
// Construct based on typical C string
{
  DBG_C("ctor-char*")
//qdbg("qstring ctor(cchar *)\n");
  p=new srep;
  p->Resize(strlen(s)+1);
  //p->s=new char[strlen(s)+1];
  strcpy(p->s,s);
}

qstring::~qstring()
{
  DBG_C("dtor")
//qdbg("qstring dtor\n");
//qdbg("p=%p, p->s=%p\n",p,p->s);
//QHexDump(p->s,10);
//#ifdef ND_TEST
  if(--p->refs==0)
  { // Delete string buffer
    p->Free();
    //delete[] p->s;
    delete p;
  }
//#endif
  else printf("qstring dtor; p->refs not 0\n");
}

/**********
* STORAGE *
**********/
void srep::Resize(int n)
// Allocate 'n' bytes for the string
{
  if(s)
  {
    Free();
  }
#ifdef CHECK_FLOOD
  s=new char[n+1];
  s[n]=FLOOD_CHAR;
  alen=n;		// Don't calculate the flood extension
#else
  s=new char[n];
  alen=n;
#endif
}
void srep::Free()
{
  if(s)
  {
#ifdef CHECK_FLOOD
    if(s[alen]!=((char)FLOOD_CHAR))
    { qerr("qstring: memory modified beyond buffer end! see qstring.cpp! (%s/%c!=%c)",
        s,s[alen],FLOOD_CHAR);
    }
#endif
    delete[] s;
    s=0;
  }
}

//
// Operators
//
qstring& qstring::operator=(const char* s)
{
  QASSERT_VALID()
  DBG_C("operator=")
  DBG_ARG_S(s)

//qdbg("qstring op=const\n");
//qdbg("  s='%s', p=%p, refs=%d\n",s,p,p?p->refs:0);
  if(p->refs>1)
  { // Detach our buffer; contents are being modified
    p->refs--;
    p=new srep;
  } else
  { // Delete old string buffer
//qdbg("  delete[] old p->s=%p (p=%p)\n",p->s,p);
    p->Free();
    //delete[] p->s;
  }
  // Copy in new string
  p->Resize(strlen(s)+1);
  //p->s=new char[strlen(s)+1];
//qdbg("  p->s=%p, len=%d (%s)\n",p->s,strlen(s)+1,p->s);
  strcpy(p->s,s);
  return *this;
}
qstring& qstring::operator=(const qstring& x)
{
  x.p->refs++;			// Protect against 's==s'
  if(--p->refs==0)
  { // Remove our own rep
    //delete[] p->s;
    p->Free();
    delete p;
  }
  p=x.p;			// Take over new string's rep
  return *this;
}

char& qstring::operator[](int i)
{
  if(i<0||i>=strlen(p->s))
  { qwarn("qstring(%s): ([]) index %d out of range (0..%d)",
      p->s,i,strlen(p->s));
    i=strlen(p->s);		// Return the 0 (reference)
  }
  if(p->refs>1)
  { // Detach to avoid 2 string rep's mixing
    srep* np=new srep;
    //np->s=new char[strlen(p->s)+1];
    np->Resize(strlen(p->s)+1);
    strcpy(np->s,p->s);
    p->refs--;
    p=np;
  }
  return p->s[i];
}

const char& qstring::operator[](int i) const
// Subscripting const strings
{
  if(i<0||i>=strlen(p->s))
  { qwarn("qstring(%s): (const[]) index %d out of range (0..%d)",
      p->s,i,strlen(p->s));
    i=strlen(p->s);		// Return the 0 (reference)
  }
  return p->s[i];
}

//
// Type conversions
//
qstring::operator const char *() const
{
  return p->s;
}
#ifdef ND_NO_MODIFIABLE_STRING_PLEASE
// This typecast cut out on 27-8-99, because it delivered too many
// late-captured problems
qstring::operator char *() // const
{
  // Check refs
  if(p->refs>1)
  { // Detach; the returned pointer may be used to modify the buffer!
qdbg("qstring op char* detach\n");
    srep* np=new srep;
    //np->s=new char[strlen(p->s)+1];
    np->Resize(strlen(p->s)+1);
    strcpy(np->s,p->s);
    p->refs--;
    p=np;
  }
  return p->s;
}
#endif

/************
* COMPARIONS *
*************/
bool qstring::operator==(const char *s)
{
  return (strcmp(p->s,s)==0);
}

// Information
bool qstring::IsEmpty()
{
  if(*p->s)return FALSE;
  return TRUE;
}
