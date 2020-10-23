/*
 * QLanguage - language support
 * 08-06-2000: Created! (11:50:39)
 * 14-11-00: Destructor now frees for real.
 * NOTES:
 * - Originally created for Lingo, based on Amiga's lang.c
 * (C) MarketGraph/RvG
 */

#include <qlib/language.h>
#include <qlib/file.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QLanguage::QLanguage(cstring fname)
{
  fileName=fname;
  strs=0;
  Open();
}
QLanguage::~QLanguage()
{
  int i;
  for(i=0;i<strs;i++)
  {
    if(str[i])qfree((void*)str[i]);
  }
}

/******
* I/O *
******/
bool QLanguage::Open()
// Read a language file
// Returns FALSE if file could not be opened
{
  char buf[256];
  QFile *f;
  
  f=new QFile(fileName);
  if(!f->IsOpen())
  {fail:
    delete f;
    return FALSE;
  }
  // Read all strings
  while(1)
  {
    f->GetLine(buf,sizeof(buf));
    if(f->IsEOF())break;
    if(strs==MAX_STRING)
    { qwarn("QLanguage:Open(); too many strings in '%s'",(cstring)fileName);
      // Don't work; let the operator know
      strs=0;
      goto fail;
    }
    str[strs]=qstrdup(buf);
    strs++;
  }
  delete f;
  return TRUE;
}

cstring QLanguage::GetString(int n)
{
  if(n<0||n>=strs)
  { qwarn("QLanguage::GetString(%d) out of range",n);
    return "???";
  }
  return str[n];
}
cstring QLanguage::GetString(cstring name)
// Get string by name
// NYI
{
  qwarn("QLanguage::GetString(name) nyi");
  return "QLanguage::GetString(name) NYI";
}
