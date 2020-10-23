// qlib/args.h

#ifndef __QLIB_ARGS_H
#define __QLIB_ARGS_H

#include <qlib/types.h>

// Options
string getoption(int argc,char **argv,string *strPtr);
// Old version of getoption()
string getoption(int argc,char **argv,qstring& str);

// Get normal argument
string getarg(int argc,char **argv,qstring& str);
// Old version
string getarg(int argc,char **argv);

#endif
