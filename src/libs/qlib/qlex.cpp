/*
 * QLex - flexible lexical analyser
 * 24-02-99: Created!
 * 04-03-99: Jump support (so loops, proc's etc are now possible)
 * NOTES:
 * - Tokenizes an input file
 * - Reasonably simple, but handy for a lot of things.
 * (C) MG/RVG
 */

#include <qlib/lex.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QLex::QLex()
{
  mem=0;
  tokens=0;
}
QLex::~QLex()
{
  int i;
  for(i=0;i<tokens;i++)
    if(token[i])qfree(token[i]);
}

bool QLex::AddToken(string name,int tnr)
{
  if(tokens==MAX_TOKEN)
  { //DEBUG_DO(qwarn("QLex::AddToken(); no space to add token"));
    return FALSE;
  }
  token[tokens]=qstrdup(name);
  tokenNr[tokens]=tnr;
  strupr(token[tokens]);
  tokens++;
  return TRUE;
}
int QLex::GetLookaheadInt()
{
  return Eval(lookaheadStr);
}
cstring QLex::GetTokenName(int t)
{
  if(t<0||t>=tokens)return 0;
  return token[t];
}

void QLex::SetLookahead(int n)
{ lookahead=n;
}
void QLex::SetLookaheadStr(string s)
{
  strcpy(lookaheadStr,s);
}

/*********
* SCRIPT *
*********/
void QLex::Free()
{
  if(mem)
  { qfree(mem);
    mem=0;
  }
}

void QLex::Start()
// Start tokenizing at the begin
{
  curS=mem;
  curLine=0;
  lastLine=curS;
  // Update lookahead
  FetchToken();
}

bool QLex::Load(cstring fname)
{
  int   size;
  size=QFileSize(fname);
  if(size==-1)return FALSE;
  Free();
  mem=(char*)qcalloc(size+1);		// Extra 0-byte at end to mark EOF
  if(!mem)return FALSE;
  QQuickLoad(fname,mem,size);
  Start();
  return TRUE;
}

/*************
* TOKENIZING *
*************/
string QLex::skipWhiteSpace(string s)
{ while(1)
  { switch(*s)
    {
      // SPACE, CR, TAB
      case 32: case 13: case 9: s++; break;
      // LF remember where we are
      case 10:
        lastLine=s+1;
        curLine++;
        s++; break;
      default: return s;
    }
  }
}
string QLex::skipComments(string s)
/** Skips these types of comments:
      '#' upto end of line
      The / *  * / C-style of comments.
      And the // C++ style comment.
**/
{redo:
  if(*s!='#')goto check_c;

 skip_eol:
  // Skip until eoline
  for(;*s!=10;s++)
  { if(*s==0)return s;
  }
  /*s_lastline=s+1;
  s_curline++;*/
  // Process next line
  s=skipWhiteSpace(s);
  goto redo;

  /** Check C style comments **/
 check_c:
  if(*s!='/'||*(s+1)!='*')goto check_cpp;
  s+=2;
 false_alarm:
  for(;*s!='*';s++)
  { if(*s==10){ lastLine=s+1; curLine++; }
    else if(*s==0)return s;
  }
  if(*(s+1)!='/'){ s++; goto false_alarm; }
  s=skipWhiteSpace(s+2);
  goto redo;

 check_cpp:
  if(*s!='/'||*(s+1)!='/')return s;
  goto skip_eol;
}

int QLex::FetchToken()
{ char c;
  short i;
  char *s=curS;
  static char *mp;

  lookaheadStr[0]=0;
  mp=lookaheadStr;
  s=skipWhiteSpace(s);
  s=skipComments(s);
  c=*s;
  if(!c){ lookahead=LEX_EOF; goto ret; }
  /* Single char commands? */
  s++;
  *mp=c; *(mp+1)=0;		// Just incase it is 1 letter
  switch(c)
  { case '{': lookahead=LEX_BEGIN; goto ret;
    case '}': lookahead=LEX_END; goto ret;
    case '[': lookahead=LEX_BRKOPEN; goto ret;
    case ']': lookahead=LEX_BRKCLOSE; goto ret;
    case ';': lookahead=LEX_EOC; goto ret;
    case '(': lookahead=LEX_PAROPEN; goto ret;
    case ')': lookahead=LEX_PARCLOSE; goto ret;
    case ',': lookahead=LEX_COMMA; goto ret;
    case '!':
      // ! or !=
      if(*s=='='){ s++; strcpy(mp,"!="); lookahead=LEX_NEQUAL; goto ret; }
      lookahead=LEX_NOT; goto ret;
    case '=':
      // = or ==
      if(*s=='='){ s++; strcpy(mp,"=="); lookahead=LEX_EQUALS; goto ret; }
      lookahead=LEX_ASSIGN; goto ret;     // ASSIGN from DQL
    case '<':
      // < or <=
      if(*s=='='){ s++; strcpy(mp,"<="); lookahead=LEX_LTE; goto ret; }
      lookahead=LEX_LT; goto ret;
    case '>':
      // > or >=
      if(*s=='='){ s++; strcpy(mp,">="); lookahead=LEX_GTE; goto ret; }
      lookahead=LEX_GT; goto ret;
    case '|': lookahead=LEX_OR; goto ret;
    case '&': lookahead=LEX_AND; goto ret;
    case '+': lookahead=LEX_PLUS; goto ret;
    case '-': lookahead=LEX_MINUS; goto ret;
    case '^': lookahead=LEX_XOR; goto ret;
    case '~': lookahead=LEX_TILDE; goto ret;
    case '*': lookahead=LEX_ASTERISK; goto ret;
    case '/': lookahead=LEX_SLASH; goto ret;
    case '%': lookahead=LEX_MOD; goto ret;
    case '.': lookahead=LEX_DOT; goto ret;
  }
  // Literal strings?
  if(c==34)
  { //s++;
    // Read literal until LF, EOF or " (source: DQL)
    for(i=0;i<MAX_LITERAL_LEN;i++)
    { if(*s==0||*s==10)break;
      if(*s==34){ s++; break; }
      if(*s=='\\')
      { s++;
        switch(*s)
        { case 'n': *mp++=10; break;
          case 't': *mp++=9; break;
          default: *mp++=*s; break;
        }
        s++;
      } else
      { *mp++=*s; s++;
      }
    }
    *mp=0;
    lookahead=LEX_STRING;
    goto ret;
  }
  if((c>='0'&&c<='9')||c=='-')
  { // Numeric (expr?)
    *mp++=c;
    while(1)
    { if(*s>='0'&&*s<='9')goto ok_expr;
      if(*s>='a'&&*s<='f')goto ok_expr;
      if(*s>='A'&&*s<='F')goto ok_expr;
      //if(*s=='-'||*s=='+'||*s=='*'||*s=='/')goto ok_expr;
      if(*s=='x'||*s=='X')goto ok_expr;
      if(*s>='a'&&*s<='z')goto ok_expr;
      if(*s>='A'&&*s<='Z')goto ok_expr;
      //if(*s=='('||*s==')'||*s=='_')goto ok_expr;
      break;
     ok_expr:
      *mp++=*s++;
    }
    *mp=0;
    lookahead=LEX_INT;
  } else
  { // Alpha string
    *mp++=c;
    while(1)
    { if(*s>='a'&&*s<='z')goto ok;
      if(*s>='A'&&*s<='Z')goto ok;
      if(*s>='0'&&*s<='9')goto ok;
      //if(*s=='_'||*s=='/'||*s=='.')goto ok;
      if(*s=='_')goto ok;
      // Allow some expressional stuff
      //if(*s=='['||*s==']')goto ok;
      //if(*s=='-'||*s=='+'||*s=='*'||*s=='/')goto ok;
      break;
     ok:
      *mp++=*s++;
    }
    *mp=0;
    // Lookup token
    char cmp[256];
    lookahead=LEX_STRING;
    strcpy(cmp,lookaheadStr); strupr(cmp);
    for(i=0;i<tokens;i++)
    { if(!strcmp(token[i],cmp))
      { lookahead=tokenNr[i];
        goto ret;
      }
    }
    lookahead=LEX_UNKNOWN;
  }
 ret:
  curS=s;
  return lookahead;
}

void QLex::Match()
// Legacy function for lexical analysers (see DQL etc.)
{
  FetchToken();
}

/**********
* JUMPING *
**********/
void QLex::StoreState(QLexState *state)
{
  state->curS=curS;
  state->curLine=curLine;
  state->lastLine=lastLine;

  state->lookahead=lookahead;
  strcpy(state->lookaheadStr,lookaheadStr);
}
void QLex::RestoreState(QLexState *state)
{
  curS=state->curS;
  curLine=state->curLine;
  lastLine=state->lastLine;
  // Restore lookahead/lookaheadStr at that point (curS is beyond that!)
  lookahead=state->lookahead;
  strcpy(lookaheadStr,state->lookaheadStr);
#ifdef OBS
  FetchToken();
  // Reset the FetchToken() changes
  curS=state->curS;
  curLine=state->curLine;
  lastLine=state->lastLine;
#endif
}

/*************
* HIGH LEVEL *
*************/
void QLex::SkipCommand()
// Skip the current command (for error skipping)
{ while(1)
  {
    if(lookahead==LEX_EOC||lookahead==LEX_EOF)break;
    FetchToken();
  }
}
void QLex::SkipBody()
// Skip command or body (compound statement) if one is found.
{ int nest=0;
//qdbg("QLex:SkipBody (LA=%d)\n",lookahead);
  while(1)
  {
//qdbg("QLex:SkipBody: '%s'\n",lookaheadStr);
    if(lookahead==LEX_EOF)break;
    if(lookahead==LEX_EOC&&nest==0)
    { FetchToken(); break; }
    if(lookahead==LEX_BEGIN)
    { nest++;
    } else if(lookahead==LEX_END)
    { nest--;
      if(nest<=0)
      { FetchToken(); break; }
    }
    FetchToken();
  }
}
void QLex::ShowLine()
{ int i;
  string s=lastLine;
  for(i=0;i<180;i++)
  { if(s[i]==10)
    { s[i]=0;
      //if(p_script)fprintf(flog,"=> %s\n",s);
      //else
      printf("=> %s\n",s);
      s[i]=10;
      return;
    }
  }
}
void QLex::Warn(string s)
{ 
  qdbg("** Warning in line %d: %s\n",curLine+1,s);
  ShowLine();
  // Skip until EOC
  SkipCommand();
}
void QLex::Error(string s)
{ //errors++;
  printf("** Error in line %d: %s\n",curLine+1,s);
  ShowLine();
  // Skip until EOC
  SkipCommand();
}

