/*
 * CSV - Comma Separated File support
 * 11-03-97: Created!
 * 04-06-98: Safety when file could not be opened
 * 09-04-99: Boolean ops changed for IRIX6.3 downcopies.
 * 04-08-99: Bug removed from ctor; recs was never reliably returned (!)
 * BUGS:
 * - GetField() doesn't look at maxLen
 * - NoofRecs() doesn't restore file ptr
 * - NoofFields() doesn't restore file ptr
 * (C) MG/RVG
 */

#include <alib/standard.h>
#include <qlib/csv.h>
#include <stdio.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DEFAULT_RECSIZE 10240

QCSVFile::QCSVFile(cstring name,int mode)
{ fp=0;
  recs=0;

  fName=qstrdup(name);
  fp=fopen(QExpandFilename(fName),"rb+");
  if(!fp)
  { qwarn("QCSVFile ctor: can't open '%s'\n",fName);
  }
  recSize=DEFAULT_RECSIZE;
  rec=(char*)calloc(1,recSize);
  if(!rec){ fclose(fp); fp=0; }
  recs=NoofRecs();
  Reset();
}

QCSVFile::~QCSVFile()
{ if(fp)fclose(fp);
  if(fName)qfree(fName);
}

int QCSVFile::NoofFields()
// Try a record and see if you can make out the number of fields
// NOTE: This function resets the record pointer to the first record
{ char *s;
  int  count;
  bool inSide;
  ReadRecord();
  inSide=FALSE; count=1;            // #fields=#commas+1
  for(s=rec;*s;s++)
  { if(*s==34)inSide=!inSide;            // Works for double quotes
    if(*s==','&&inSide==FALSE)
      count++;
  }
  Reset();
  return count;
}
int QCSVFile::NoofRecs()
// Returns #records; leaves pointer at first record
{ int n;
  if(!fp)return 0;
  if(recs)return recs;          // Return cached value
  Reset();
  for(n=0;;n++)
  { ReadRecord();
    if(IsEOF())break;
  }
  Reset();
  recs=n;                       // Cache #recs
  qdbg("QCSV: NoofRecs()=%d\n",recs);
  return n;
}

bool  QCSVFile::IsOK()
// Returns TRUE if CSV file is in ok condition.
// It is NOT if it couldn't be opened.
{ if(!fp)return FALSE;
  return TRUE;
}
bool  QCSVFile::IsEOF()
{ if(!fp)return TRUE;
  if(feof(fp))
    return TRUE;
  return FALSE;
}
bool  QCSVFile::ReadRecord()
// Returns FALSE if either the record could not be read (at eof)
// or the record is mangled/invalid.
{ if(!fp)
  { *rec=0;
    return FALSE;
  }
 do_next:
  if(fgets(rec,recSize,fp)==0)
    return FALSE;
  //StripLF(rec);
  if(!rec[0])goto do_next;
  //fgetc(fp);
  return TRUE;
}

void QCSVFile::Reset()
{ if(fp)
    fseek(fp,0,SEEK_SET);
}

bool  QCSVFile::GetField(int n,char *d,int maxLen)
// Non-existent fields return empty string
{ char *s;
  int  curField;
  bool inSide;

  inSide=FALSE;
  curField=0;
  for(s=rec;*s;s++)
  { if(*s==34)
    { // Check for dbl quote
      if(*(s+1)==34&&inSide==TRUE)s++;
      else
      { inSide=!inSide;
        continue;
      }
    }
    if(*s==','&&inSide==FALSE)
    { curField++;
    }
    // Copy chars that are inside quotes (works for double quotes!)
    if(curField==n)
    { if(inSide)
        *d++=*s;
    }
  }
  *d=0;
  return TRUE;
}
int   QCSVFile::GetInt(int n)
{ int e;
  char buf[256];
  GetField(n,buf);
  return atoi(buf);
}

bool QCSVFile::GotoRecord(int n)
{
  if(!fp)return FALSE;
  Reset();
  for(int i=0;i<=n;i++)
    ReadRecord();
  return TRUE;
}

