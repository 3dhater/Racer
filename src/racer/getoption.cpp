/*
 * getoption - arg parsing help
 * 12-04-99: Created!
 * FUTURE:
 * - getoption() and getarg() should return bool's, not strings
 * - remove old getoption()/getarg() variants (non-qstring)
 * (C) MG/RVG
 */

#include <unistd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

static int curArg=1;

string getoption(int argc,char **argv,qstring& str)
// Like getoption() below, only with qstring (preferred since 17-9-99)
// Use like:
// { qstring s; while(getoption(argc,argv,s))... }
{
  char *s;

  str="";

  if(curArg>=argc)return 0;
  s=argv[curArg];
  if(*s==0)return 0;
  // Check for option (-)
  if(*s!='-')return 0;
  str=s+1;
  curArg++;
  return s+1;
}

string getoption(int argc,char **argv,string *strPtr)
// Retrieve next option, or 0 if no more exist
// Returns the option name, and sets the string that 'strPtr' points to
// to the same as the return value.
{
  char *s;

  *strPtr=0;

  if(curArg>=argc)return 0;
  s=argv[curArg];
  if(*s==0)return 0;
  // Check for option (-)
  if(*s!='-')return 0;
  *strPtr=s+1;
  curArg++;
  return *strPtr;
}

string getarg(int argc,char **argv)
{
  char *s;

  if(curArg>=argc)return 0;
  s=argv[curArg];
  if(*s==0)return 0;
  curArg++;
  return s;
}
string getarg(int argc,char **argv,qstring& str)
// Puts argument in 'str'. If no argument exists anymore,
// this function returns 0 and 'str' is untouched.
// (unlike getoption(), which sets 'str' to "")
// Otherwise, the argument is returned as a string (consider it cstring),
// and 'str' contains the argument
{
  char *s;

  if(curArg>=argc)return 0;
  s=argv[curArg];
  if(*s==0)return 0;
  curArg++;
  str=s;
  return s;
}

