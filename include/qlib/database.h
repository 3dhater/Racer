// qlib/database.h

#ifndef __QLIB_DATABASE_H
#define __QLIB_DATABASE_H

#include <qlib/object.h>

class QTable : public QObject
{
 public:
  string ClassName(){ return "table"; }

 protected:

 public:
  QTable(string fname);
  ~QTable();
};

class QField : public QObject
{
 protected:
  QTable *table;
  string  fieldName;
 public:
  QField(QTable *table,string name);
  ~QField();
};

#endif
