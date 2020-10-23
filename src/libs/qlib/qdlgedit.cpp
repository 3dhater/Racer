/*
 * QDlgEditFile - quickly edit a file
 * 18-03-2000: Created! (01:13:08)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <qlib/dialog.h>
#include <qlib/debug.h>
DEBUG_ENABLE

bool QDlgEditFile(cstring fname,int maxLine,cstring title,int flags)
// Load a file, edit it and then save it if wished
// Returns TRUE if file was indeed saved, FALSE otherwise
// Taken from Lingo 1.43
// For small files only
{
  int len;
  int r;
  QRect pos(900,40,270,490);
  char buf[65536];
  FILE *fr;
  
  if(!title)
    title=fname;

  if(flags&1)
  {
    // Wide dialog
    pos.x=20;
    pos.wid=QSHELL->GetWidth()-2*20;
  }
  // Calc hgt
  pos.hgt=160+maxLine*app->GetSystemFontNP()->GetHeight();
  
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
 retry:
  r=QDlgString((string)title,"Edit the file contents",
    buf,sizeof(buf),maxLine,&pos);
  if(r==TRUE)
  { // Write back results
    // Write back
    fr=fopen(fname,"wb");
    if(fr)
    { fwrite(buf,1,strlen(buf),fr);
      fclose(fr);
    }
  }
  if(r==TRUE)return TRUE;
  return FALSE;
}
