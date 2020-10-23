// lex.h - Lexical analyser

#ifndef __QLIB_LEX_H
#define __QLIB_LEX_H

#include <qlib/types.h>

// Default tokens (based on C tokenizing)
enum Tokens
{ LEX_EOF=1000,				// End of script
  LEX_BEGIN,LEX_END,			// {, }
  LEX_BRKOPEN,LEX_BRKCLOSE,		// [, ]
  LEX_EOC,				// ;
  LEX_PAROPEN,LEX_PARCLOSE,		// (, )
  LEX_COMMA,
  LEX_NOT,LEX_ASSIGN,			// !, =
  LEX_EXPR,				// A full expression
  LEX_STRING,				// A literal string
  LEX_UNKNOWN,				// Variable or unrecognized keyword
  LEX_OR,				// | (mathematical 5|1)
  LEX_AND,				// & (math)
  LEX_LOGICAL_OR,			// ||
  LEX_LOGICAL_AND,			// &&
  LEX_PLUS,				// +
  LEX_MINUS,				// - (unary/binary)
  LEX_XOR,				// ^
  LEX_INT,				// Number (const)
  LEX_TILDE,				// ~
  LEX_ASTERISK,				// *
  LEX_SLASH,				// /
  LEX_MOD,				// %
  LEX_DOT,				// .
  LEX_EQUALS,				// ==
  LEX_NEQUAL,				// !=
  LEX_LT,				// <
  LEX_GT,				// >
  LEX_LTE,				// <=
  LEX_GTE 				// >=
};

struct QLexState
// Structure to store location info (for loops, proc's etc)
{
  char *curS;
  int   curLine;
  char *lastLine;
  // Stored lookahead
  int   lookahead;
  char  lookaheadStr[256];
};

class QLex
{
  enum Max
  { MAX_TOKEN=128,
    MAX_LITERAL_LEN=256
  };

 protected:
  // Token
  int  lookahead;
  char lookaheadStr[256];
  char  *token[MAX_TOKEN];
  int    tokenNr[MAX_TOKEN];
  int    tokens;

  // Script
  char *mem;
  // Script statistics
  char *curS;
  int   curLine;
  char *lastLine;

 protected:
  string skipWhiteSpace(string s);
  string skipComments(string s);
  void ShowLine();

 public:
  QLex();
  ~QLex();

  // Tokens
  int    GetLookahead(){ return lookahead; }
  string GetLookaheadStr(){ return lookaheadStr; }
  int    GetLookaheadInt();
  bool   AddToken(string name,int tokenNr=-1);
  cstring GetTokenName(int t);
  // Morphing tokens
  void   SetLookahead(int n);
  void   SetLookaheadStr(string s);

  // Scripting
  bool Load(cstring fname);		// Read script from file
  void Start();
  void Match();				// Match current token and move on
  int  FetchToken();

  void Free();

  // Jumping support
  void StoreState(QLexState *state);
  void RestoreState(QLexState *state);

  // High level aid (handy)
  void SkipCommand();			// After errors for example
  void SkipBody();			// Skip full compound commands
  void Warn(string s);
  void Error(string s);
};

#endif
