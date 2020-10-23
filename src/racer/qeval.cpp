/*
 * Expression evaluator (infix->postfix->value)
 * Using the following grammar (from Compilers principles, techniques....)

 Priorities:  |		(lowest)
              &
              +,-
              *,/	(highest)

 start       -> expr
              | ø
 expr        -> orop moreorops
 moreorops   -> | orop moreorops
              | ø
 orop        -> andop moreandops
 moreandops  -> & andop moreandops
              | ^ andop moreandops ; priority of ^ right?
              | ø
 andop       -> term moreterms
 moreterms   -> + term moreterms
              | - term moreterms
              | ø
 term        -> factor morefactors
 morefactors -> * factor morefactors (SGI: x too)
              | / factor morefactors
              | % factor morefactors
              | ø
 factor      -> (expr)                     (brackets are primary operators)
              | id
              | num
 id          -> ø                          (no default id's defined yet)
 num         -> $ hexnum
              | # binnum
              | 0 octnum
              | 0x hexnum
              | unaryop num
              | 1-9                        (decimal number)
              | ø                          (actually an error)
 unaryop     -> ~ num                      (one's complement)
              | - num                      (unary negation)
              | + num                      (unary positive)

 * 29-10-89: Created! Handles decimal, hex, binary (#), +,-,/,*. Never gives an
 * error, so it works on all expressions, but faulty on faulty expressions.
 * Protected against division by zero...
 * 27-02-90: Now numbers may be prefixed by unary operators. Version before
 * this said that -5==0! Also &,| built in with priorities like in C. Slight
 * change in handling lookahead.
 * 06-02-91: Expanded (because of Arp which uses $) to handle '$' as well as
 * '0x' and octal reps (033 etc.). Fixed bug when dealing with large numbers
 * (e.g. 300000/12 went wrong).
 * 26-01-96: ASSERTed, cut back on static data (40 instead of 256 tokens).
 * Added %, ^ (not too sure about the ^ priority, though!)
 * 09-10-98: SGI.
 * 22-09-99: Finally! Unary minus implemented.
 * BUGS:
 * - ^ priority may not match C.
 * (C) MG/Dave Mentos/RVG
 */

#include <qlib/debug.h>
DEBUG_ENABLE

// Additional checks to check stack push/pops?
//#define SELF_CHECK

#define DONE		256		/* End of Expression */
#define T_CONST		257		/* Token types */
#define T_UNARYMINUS	258		// -(4+3) for example
#define T_XOR		259		// ~10 (bitwise invert)

#define MAX_TOKEN	40		// Max. expression complexity

struct Token { short token; long val; };

/*****************************
* INFIX TO POSTFIX CONVERTER *
*****************************/
static struct Token dest[MAX_TOKEN];
//static char  *src,srcindex,destindex;
static cstring src;
static char  srcindex,destindex;
static short lookahead;
static long  tokenval;

/**********
* COMMONS *
**********/
static short isdigit(char c)
{ if(c>='0'&&c<='9')return 1; return 0; }
static short isoctdigit(char c)
{ if(c>='0'&&c<='7')return 1; return 0; }
static short ishexdigit(char c)
{ if((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'))return 1; return 0; }
static short isbindigit(char c)
{ if(c=='0'||c=='1')return 1; return 0; }
static short hextoint(char c)		/* From ASCII to int */
{ if(c>='0'&&c<='9')return (short)(c-'0');
  if(c>='A'&&c<='F')return (short)(c-'A'+10);
  if(c>='a'&&c<='f')return (short)(c-'a'+10);
  return 0;
}
static long calchex(void)	/* htol */
{ long val=0;
  while(ishexdigit(src[srcindex]))
  { val=val*16+hextoint(src[srcindex]); srcindex++; }
  return val;
}
static long calcbin(void)		/* binary */
{ long val=0;
  while(isbindigit(src[srcindex])){ val=val*2+src[srcindex]-'0'; srcindex++; }
  return val;
}
static long calcdec(void)	/* Calc decimal */
{ long val=0;
  while(isdigit(src[srcindex])){ val=val*10+src[srcindex]-'0'; srcindex++; }
  return val;
}
static long calcoct(void)	/* Calc octal */
{ long val=0;
  while(isoctdigit(src[srcindex]))
  { val=val*8+src[srcindex]-'0'; srcindex++; }
  return val;
}

/*******************
* LEXICAL ANALYSER *
*******************/
static short lexan(void)
/* Lexical analyser; removes white spaces, tokenizes */
{ char c;
 fetch:
  srcindex++;
  if(srcindex==strlen(src))return DONE;
  c=src[srcindex];
  if(c==' ')goto fetch;
  return (short)c;
}

/**********
* EMITTER *
**********/
static void emit(short t)	/* Push result on postfix stack */
{
//qdbg("emit(%d, %c) tokenval=%d\n",t,t,tokenval);
  dest[destindex].token=t;
  if(t==T_CONST)dest[destindex].val=tokenval;
  else          dest[destindex].val=0;
  destindex++;
}
#ifdef NOTDEF
static emittxt(s1,s2,s3,s4,s5)	/* Emit tekst */
{ printf(s1,s2,s3,s4,s5); printf("\n"); }
#endif

/*********
* PARSER *
*********/
static void expr(void);
static void unaryop(void);

static void match(short t)
{ if(lookahead==t)lookahead=lexan();
  else lookahead=DONE;		/* Syntax error */
}

static void num(void)
/*
   Evaluates constant
*/
{
//qdbg("num\n");
  switch(lookahead)
  { case '$':
      match('$'); tokenval=calchex(); emit(T_CONST); return;
    case '0':
      match('0');
      switch(lookahead)
      { case 'x':
        case 'X':
          match(lookahead); tokenval=calchex(); emit(T_CONST); return;
        default:
          tokenval=calcoct(); emit(T_CONST); return;
      }
    case '#':
      match('#'); tokenval=calcbin(); emit(T_CONST); return;
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      tokenval=calcdec(); emit(T_CONST); return;
    default : unaryop(); return;
  }
}
static void unaryop(void)
{
//qdbg("unaryop: lookahead=%c\n",lookahead);
  switch(lookahead)
  { case '~': match(lookahead); expr(); emit(T_XOR); break;
    case '-': match(lookahead); expr(); emit(T_UNARYMINUS); break;
    case '+': match(lookahead); expr(); break;
    default : break;
  }
}
static void factor(void)
{
//qdbg("factor\n");
  switch(lookahead)
  { case '(': match('('); expr(); match(')'); break;
    default : num(); if(srcindex>0)srcindex--;
//qdbg("Factor: lookahead=%d\n",lookahead);
              match(lookahead); break;
  }
}
static void term(void)
{ short t;
//qdbg("term\n");
  factor();
  while(1)
    switch(lookahead)
    { case '*':
      case 'x': t=lookahead; match(t); factor(); emit('*'); break;
      case '/': t=lookahead; match(t); factor(); emit(t); break;
      case '%': t=lookahead; match(t); factor(); emit(t); break;
      default : return;
    }
}
static void andop(void)
{ short t;
//qdbg("andop\n");
  term();
  while(1)
    switch(lookahead)
    { case '+': t=lookahead; match(t); term(); emit(t); break;
      case '-': t=lookahead; match(t); term(); emit(t); break;
      default : return;
    }
}
static void orop(void)
{ short t;
  andop();
  while(1)
    switch(lookahead)
    { case '&': t=lookahead; match(t); andop(); emit(t); break;
      case '^': t=lookahead; match(t); andop(); emit(t); break;
      default : return;
    }
//qdbg("andop RET\n");
}
static void expr(void)
{ short t;
  orop();
  while(1)
    switch(lookahead)
    { case '|': t=lookahead; match(t); orop(); emit(t); break;
      default : return;
    }
//qdbg("orop RET\n");
}
static void parse(void)
{
//qdbg("parse\n");
  lookahead=lexan();
  if(lookahead!=DONE)expr();
//qdbg("parse RET\n");
}

/********************
* POSTFIX EVALUATOR *
********************/
static long   pfstack[MAX_TOKEN];
static short  pfindex;

static void push(long val)
{ pfstack[pfindex]=val;
  if(pfindex<MAX_TOKEN-1)pfindex++;
  // else expr too complex error!
}
static long pop(void)
{ if(pfindex>0)pfindex--;	// Check first because expr might be too complex
//qdbg("pop() returns %d\n",pfstack[pfindex]);
  return pfstack[pfindex];
}
static long top(void)
{ if(pfindex>0)return pfstack[pfindex-1];
  else         return 0;
}

static long eval(void)
/* Evaluate created postfix array */
{ short i;
  long v1,v2;
  pfindex=0;
  for(i=0;i<destindex;i++)
  { switch(dest[i].token)
    { case T_CONST     : push(dest[i].val); break;
      case T_UNARYMINUS: push(-pop()); break;
      case T_XOR       : push(~pop()); break;
      case '&':     push(pop()&pop()); break;
      case '^':     push(pop()^pop()); break;
      case '|':     push(pop()|pop()); break;
      case '+':     push(pop()+pop()); break;
      //case '-':     push(-pop()+pop()); break;
      //case '-':     v2=pop(); v1=pop(); push(v1-v2); break;
      case '-':
//printf("eval: -, pfindex=%d\n",pfindex);
        v2=pop(); v1=pop();
//printf("eval: -, v1=%d, v2=%d\n",v1,v2);
        push(v1-v2); break;
      case '*':     push(pop()*pop()); break;
      case '/':     v2=pop(); v1=pop(); if(v2)push(v1/v2); else push(0); break;
      case '%':     v2=pop(); v1=pop(); if(v2)push(v1%v2); else push(0); break;
      default:      break;
    }
  }
#ifdef SELF_CHECK
  if(pfindex!=1)qdbg("eval: pfindex!=1 at end\n");
#endif
  return top();
}
int Eval(cstring s)		/* Expression evaluator */
{
  QASSERT(s);
  // Empty string?
  if(!*s)return 0;
//qdbg("Eval(%s)\n",s);
  src=s;
  srcindex=-1;
  destindex=0;
  parse();
//qdbg("sizeof(long)=%d\n",sizeof(long));
//qdbg("sizeof(int)=%d\n",sizeof(int));
  return (int)eval();
}
