// memorize - remember allocated nodes (chunks of mem) in an array
// To help find allocated/freed parts

#ifndef __ALIB_MEMORIZE_H
#define __ALIB_MEMORIZE_H

typedef struct MemorizerEntry
{ char *ptr;
} MemorizerEntry;

typedef struct tagMemorizer
{ int  ptrs;
  MemorizerEntry *p;
  char *name;
} Memorizer;

Memorizer *MemorizeCreate(char *name,int n);
void MemorizeAdd(Memorizer *m,char *p);
void MemorizeRemove(Memorizer *m,char *p);
void MemorizeReport(Memorizer *m);
bool MemorizeContains(Memorizer *m,char *p);

#endif

