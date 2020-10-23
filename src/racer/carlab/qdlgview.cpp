/*
 * QDlgViewFile - quickviewing an ASCII file
 * 20-06-01: Created!
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <qlib/dialog.h>
#include <qlib/debug.h>
DEBUG_ENABLE

bool QDlgViewFile(cstring fname,int maxLine,cstring title,cstring text,
  QRect *r)
// Load a file and view it
// Useful for reports for example
// For relatively small files only (<64K)
{
  int   len;
  QRect pos(900,40,270,490);
  char  buf[65536];
  FILE *fr;
  
  // Default title?
  if(!title)
    title=fname;
  if(!text)text="";
  
  // Get file contents
  fr=fopen(fname,"rb");
  if(!fr)
  { // No BC
    buf[0]=0;
  } else
  { // Load the text
    fseek(fr,0,SEEK_END);
    len=ftell(fr);
    if(len>sizeof(buf))len=sizeof(buf)-1;
    fseek(fr,0,SEEK_SET);
    fread(buf,1,len,fr);
    buf[len]=0;
    fclose(fr);
  }
  
  // Location
  if(r==0)
  {
    // Default location & size
    r=&pos;
    r->SetXY(100,150);
    r->wid=300;
    r->hgt=160+maxLine*app->GetSystemFontNP()->GetHeight();
  }
  QStringDialog dlg(QSHELL,r,title,text,buf,maxLine);
  // Use non-proportional font
  dlg.eString->SetFont(app->GetSystemFontNP());
  dlg.eString->EnableReadOnly();
  dlg.Execute();
  return TRUE;
}
