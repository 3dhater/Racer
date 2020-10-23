// tbrowse.h - table browser (CSV)

#ifndef __QLIB_TBROWSE_H
#define __QLIB_TBROWSE_H

#include <qlib/dbfield.h>
#include <qlib/csv.h>

#define QTB_MAXFIELD	20

class QTableBrowser
{
 protected:
  QDBField *field[QTB_MAXFIELD];
  QCSVFile *csv;
  int fields;
  // From which field is the contents
  int fieldIndex[QTB_MAXFIELD];
  // Browse info
  int curRec;			// -1=no rec

 public:
  QTableBrowser();
  ~QTableBrowser();

  QDBField *GetField(int n);
  QDBField *AddEdit(int x,int y,int wid,int hgt,int len,string fldName,
    int fieldIndex=-1,int lines=1);

  bool AttachCSV(QCSVFile *csv);
  void Refresh();
  void GoNext();
  void GoPrev();
};

#endif
