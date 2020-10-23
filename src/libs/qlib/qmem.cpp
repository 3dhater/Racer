/*
 * Q Memory allocation (also debugging)
 * 28-04-97: Created! (from zmem.cpp)
 * 21-07-97: MemWatch-type of checking started for qcalloc's.
 * NOTES:
 * - For possibly tracking memory allocations etc.
 * - Makes it possible to get the total amount of allocated memory
 * - Don't use alloc/free preferrably
 * - Some misc functions provided
 * BUGS:
 * - QSetThreadPriority() should assume it is 'Joe User', not 'root'.
 * (C) MarketGraph/RVG
 */

#define DEBUG

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bstring.h>
#include <sys/schedctl.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Define the next to check memory at allocation and free time
//#define CHECK_ALWAYS

#define F_INACTIVE	1	// Actively recording stuff?
#define F_CHECKALLOC	2	// Check mem at every allocation?
#define F_KEEPFREE	4	// Don't free qfree()-d stuff directly
#define F_REPORTED	8	// Mqreport(1) has printed this

#define M_HEADERLEN	4
#define M_TRAILERLEN	4

#define M_HEADSTR	"HEAD"
#define M_TRAILSTR	"TRAL"

#define DIRT_ALLOC	0xBA	// Dirt upon allocation
#define DIRT_FREED	0xFA	// Freed dirt

struct Malc			// Allocation
{
  Malc  *next;
  int    size;			// Size in bytes
  int    flags;			// 1=calloc
  string file;			// What file was the alloc done in
  int    line;
  char   header[M_HEADERLEN];	// Header tag to check
  char   memory[M_TRAILERLEN];	// Reserves space for trailer tag; start of mem
};

struct ModuleInfo
{
  int flags;			// F_xxx
  Malc *first;			// Head of allocation list
  Malc *freed;			// Freed allocations
  int   alcs;			// Number of allocations
  int   totalMem;		// Total number of allocated bytes
  int   alcsFreed;		// Number of frees held
  int   totalFreed;		// Total number of freed bytes held
};

#ifdef qcalloc
#undef qcalloc
#endif
#ifdef qalloc
#undef qalloc
#endif
#ifdef qfree
#undef qfree
#endif
#ifdef qstrdup
#undef qstrdup
#endif

static ModuleInfo g;

void *qcalloc(int size)
{
  return calloc(1,size);
}
void *qalloc(int size)
{
  return malloc(size);
}

void qfree(void *p)
{
  // Check ptr to exist
  //...
  free(p);
}

string qstrdup(cstring s)
{
  string newS;
  newS=(string)qcalloc(strlen(s)+1);
  if(newS)
    strcpy(newS,s);
  return newS;
}

/********************
* DEBUG ALLOCATIONS *
********************/
int Mqtotalmem()
{ return g.totalMem;
}
int Mqtotalallocs()
{ return g.alcs;
}

void Mprintf(char *fmt,...)
// Memory debugging printf
{ va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

#ifdef OBS
static void OLOF(string fname,int line)
/** From the Amiga's debug.c (MemWatch); OutputLineOfFile **/
{ FILE *fr;
  char buf[128];
  long i;
  fr=fopen(fname,"rb"); if(fr){ Mprintf("  "); goto do_fr; }
  sprintf(buf,"~/csource/qlib/%s",fname);
  fr=fopen(buf,"rb"); if(fr){ Mprintf("Q "); goto do_fr; }
  return;                               // No source file found
 do_fr:
  for(i=0;i<line;i++)
  { fgets(buf,127,fr);
    if(feof(fr)){ fclose(fr); return; }
  }
  Mprintf("> %s",buf);
  fclose(fr);
}
#endif
static void Dump(char *a,int size)
{ int i;
  if(size>60)size=60;
  qdbg("  : ");
  for(i=0;i<size;i++,a++)
  { if(*a>=32&&*a<127)
      qdbg("%c",*a);
    else qdbg(".");
  }
  qdbg("\n");
}
static void MprintAlc(Malc *a)
{ Mprintf("# qcalloc(%d)",a->size);
  Mprintf(" from '%s', line %d at 0x%p:\n",a->file,a->line,a->memory);
  QOLOF(a->file,a->line);
  Dump(a->memory,a->size);
}

void *Malloc(int size,string file,int line,int flags)
// Allocate a chunk of mem, and record it
// flags&1: clear memory before using (calloc)
{ void *p;
#ifdef CHECK_ALWAYS
  Mqcheckmem(0);
#endif
#ifdef FUTURE
  if(g.flags&F_CHECKALLOC)
    ;
#endif
  // Check args
  if(size<=0)
  { Mprintf("# Bad allocation length (%d) in '%s', line %d\n",
      size,file,line);
    QOLOF(file,line);
    return 0;
  }
  // Check mem limit (to test low-mem situations)
  //...
  p=malloc(size+sizeof(struct Malc));
  if(!p)
  { // If freed memory exists, free this first, and try again
    // (this method may not work in a virtual memory environment)
    Mprintf("# Allocation failed; %d bytes in '%s', line %d\n",
      size,line,file);
    QOLOF(file,line);
    return 0;
  }
  // Fill in header info
  Malc *hd=(Malc*)p;
  hd->size=size;
  hd->file=file;
  hd->line=line;
  hd->flags=flags;
  memcpy(hd->header,M_HEADSTR,M_HEADERLEN);
  memcpy(hd->memory+size,M_TRAILSTR,M_TRAILERLEN);
  // Link in
  hd->next=g.first;
  g.first=hd;
  if(flags&1)
  { // Clear memory
    //memset(
    bzero(hd->memory,size);
  } else
  { // Fill with know dirt
    memset(hd->memory,DIRT_ALLOC,size);
  }
  g.alcs++;
  g.totalMem+=size;
  //qdbg("> qalloc's: %d allocations, %d total memory\n",g.alcs,g.totalMem);
  return hd->memory;
}

void *Mqcalloc(int size,string file,int line)
{ //void *p;
  return Malloc(size,file,line,1);
  //return p;
}

int McheckAlc(Malc *a)
// Check allocation
// Returns: 0=ok, 1=err found
{ int header,trailer;
  header=memcmp((char*)&a->header,M_HEADSTR,M_HEADERLEN);
  trailer=memcmp(a->memory+a->size,M_TRAILSTR,M_TRAILERLEN);
  if(header||trailer)
  {
    MprintAlc(a);
    if(header)
      Mprintf("  ^^^ Bytes trashed in front of allocation!\n");
    if(trailer)
      Mprintf("  ^^^ Bytes trashed behind end of allocation!\n");
    QOLOF(a->file,a->line);
    QHexDump((char*)&a->header,16);
    return 1;
  }
  return 0;
}

void Mqfree(void *p,string file,int line)
// Free memory
{ Malc *prev,*a;
  int   error=0;

#ifdef CHECK_ALWAYS
  Mqcheckmem(0);
#endif

  // Find memory chunk
  for(prev=0,a=g.first;
      a&&a->memory!=p;
      prev=a,a=a->next);
  if(!a)
  { // Try to find the memory chunk in the already freed list
    for(a=g.freed;a&&a->memory!=p;a=a->next);
    if(a)
    { MprintAlc(a);
      Mprintf("  ^^^ Memory freed twice\n");
    }
    Mprintf("# Never allocated qfree() in '%s', line %d:\n",file,line);
    QOLOF(file,line);
    error=1;
  }
  // Size isn't given on Unix free(), so no check
  else if(McheckAlc(a))error=2;
  if(error)
  { // Pause
    //...
    return;
  }
  // Admin
  g.alcs--;
  g.totalMem-=a->size;
  //qdbg("> (qfree) qalloc's: %d allocations, %d total memory\n",g.alcs,g.totalMem);
  // Remove alc from list
  if(prev)prev->next=a->next;
  else    g.first=a->next;
  // Trash the memory for easier-to-spot bugs
  memset(a->memory,DIRT_FREED,a->size);
  // Might choose to hold on to this piece of memory
  // for later checks (add to g.freed)
//#ifdef FUTURE
  if(g.flags&F_KEEPFREE)
  { 
    a->next=g.freed;
    g.freed=a;
    g.alcsFreed++;
    g.totalFreed+=a->size;
    qdbg("> qfree: keep freed (%d)\n",g.totalFreed);
  } else
//#endif
  { // Really free
    free(a);
  }
}

string Mqstrdup(cstring s,string file,int line)
{
  string newS;
  //printf("Mqstrdup(%s:%p)\n",s,s);
  newS=(string)Malloc(strlen(s)+1,file,line,0);
  //printf("  strlen=%d\n",strlen(s));
  if(newS)
    strcpy(newS,s);
  return newS;
}

/*******************
* CLIENT DEBUG AID *
*******************/
void Mqcheckmem(int flags)
// Check all allocations and report any anomalities
{ Malc *a;
  char *tmpChar;
  int i,error=0;

  // Check active allocations
  for(a=g.first;a;a=a->next)
    error|=McheckAlc(a);
  // Check freed memory for still being trashed correctly
  for(a=g.freed;a;a=a->next)
  { McheckAlc(a);
    // Check memory contents to be DIRT_FREED (expected dirt)
    for(i=0,tmpChar=a->memory;
        i<a->size;
        i++,tmpChar++)
    { if(*((unsigned char*)tmpChar)!=DIRT_FREED)
      { error=1;
        MprintAlc(a);
        Mprintf("  ^^^ Memory modified after having been freed!\n");
      }
    }
  }
  //if(error)MWHold();
}
void Mqreport(int flags)
// flags&1: print every allocation (also does a check)
{ Malc *a;
  if(g.alcs==0&&g.alcsFreed==0)
  { // No allocations to report; keep silent
    return;
  }
  Mprintf("### Q Memory Report ###\n");
  Mprintf("Number of allocations: %d, total memory involved: %d\n",
    g.alcs,g.totalMem);
  Mprintf("Number of freed allocations: %d, total memory involved: %d\n",
    g.alcsFreed,g.totalFreed);
  if(flags&1)
  { 
    Mprintf("(New) allocations:\n");
    for(a=g.first;a;a=a->next)
    { if(!(a->flags&F_REPORTED))
      { MprintAlc(a);
        a->flags|=F_REPORTED;
      }
      McheckAlc(a);
    }
    Mprintf("Freed allocations:\n");
    for(a=g.freed;a;a=a->next)
    { MprintAlc(a);
      McheckAlc(a);
    }
  }
  Mprintf("#######################\n");
  Mqcheckmem(0);
}

