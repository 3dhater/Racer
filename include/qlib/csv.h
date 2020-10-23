// qlib/csv.h

#ifndef __QLIB_CSV_H
#define __QLIB_CSV_H

#include <qlib/types.h>
#include <stdio.h>

const int QCSVMODE_READ=1;
const int QCSVMODE_WRITE=2;             // NYI

class QCSVFile
{protected:
  string fName;
  FILE *fp;
  char *rec;                    // 1 Record
  int   recSize;
  int   recs;			// Cached #records

 public:
  QCSVFile(cstring name,int mode=QCSVMODE_READ);
  ~QCSVFile();
  
  int   NoofFields();
  int   NoofRecs();

  bool  IsOK();
  bool  IsEOF();
  bool  ReadRecord();
  bool  GotoRecord(int n);
  void  Reset();
  bool  GetField(int n,char *d,int maxLen=256);
  int   GetInt(int n);
};

#endif
