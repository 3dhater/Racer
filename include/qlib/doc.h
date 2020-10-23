// qlib/doc.h - QDocument class

#ifndef __QLIB_DOC_H
#define __QLIB_DOC_H

#include <qlib/file.h>

class QDocument : public QObject
{
 public:
  string ClassName(){ return "document"; }

 public:
  QDocument();
  ~QDocument();

  virtual bool Serialize(QFile *f,bool write=TRUE);
  bool Load(string fname);
  bool Save(string fname);
};

#endif
