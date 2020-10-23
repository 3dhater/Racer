//
// QDocument.cpp - Document encapsulation
//

#include <qlib/doc.h>

QDocument::QDocument()
{
}

QDocument::~QDocument()
{
}

bool QDocument::Serialize(QFile *f,bool write)
{
  // Does nothing; overrule in your class
  return TRUE;
}

bool QDocument::Load(string fname)
{ QFile *f;
  bool r;
  f=new QFile(fname,QFile::WRITE);
  r=Serialize(f,FALSE);
  return r;
}

bool QDocument::Save(string fname)
{ QFile *f;
  bool r;
  f=new QFile(fname,QFile::WRITE);
  r=Serialize(f,TRUE);
  return r;
}
