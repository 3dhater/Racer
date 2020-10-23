/*
 * QInstall - based on .ins files, this class installs products
 * 27-08-1999: Created!
 * NOTES:
 * - SGI only
 * - MachTech-related commands exist
 * - SGI IRIX only for installing fonts
 * DOCS:
 * - Commands in .ins:
 *   desc <s>            Gives a description of the product
 *   ar   <archive>      (NYI) Open the archive
 *   set  <var> <val>    Variable set
 *   md   <dir>          Make a directory
 *   cp   <file>         Copy the file from the archive
 *                       (cp -ifnew <file> to copy only if the file
 *                        does not yet exist)
 *   dir  <dir>          Define the install directory
 *   exe  <file>         Makes the file executable
 *   mtl  <key> <val>    Sets a key in MTLauncher's ini (MachTech)
 *   font <pfb> <xname>  Install a font
 *   Uninstall commands:
 *   ~mtl <key>          Kills entire subtree in MT's launcher.ini
 * (C) MarketGraph/RvG
 */

#ifdef __sgi

#include <qlib/install.h>
#include <machtech/system.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef linux
#include <stdlib.h>
#endif
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "QInstall"

// Debug mode; no REAL extracting?
//#define FAKE_EXTRACT

QInstall::QInstall()
// Create installer object
// Open() an .ins file and Install()
{
  int i;

  ar=0;
  fIns=0;
  prg=0;
  msg=0;
  for(i=0;i<MAX_IVAR;i++)
    ivar[i]=new QInstallVar();
//qdbg("QInstall ctor; name.p->s=%p\n",(cstring)name);
}

QInstall::~QInstall()
{
  int i;

qdbg("QInstall dtor; name.p->s=%p\n",(cstring)name);
  delete ar;
  delete fIns;
  for(i=0;i<MAX_IVAR;i++)
  { 
//qdbg("delete ivar[%d]=%p\n",i,ivar[i]);
    delete ivar[i];
  }
}

bool QInstall::Open(cstring _name,cstring iname,cstring arName)
// Opens an .ins file and returns TRUE if succesful.
// Supply 'arName' to open the archive
// Returns FALSE if the .ins file or the archive file could not be opened
{
  // Take name of install script
  name=_name;
  fIns=new QFile(iname);
  if(!fIns->IsOpen())
  { Close();
    return FALSE;
  }
  if(arName)
  {
    delete ar;
    ar=new QArchive(arName);
    if(!ar->IsOpen())return FALSE;
  }
  return TRUE;
}
bool QInstall::Close()
// Closes the .ins install file
{
  delete fIns; fIns=0;
  return TRUE;
}

/************
* VARIABLES *
************/
QInstallVar* QInstall::FindVar(cstring name)
{
  int i;
  for(i=0;i<MAX_IVAR;i++)
  {
//qdbg("ivar[%d]='%s'\n",i,(cstring)ivar[i]->name);
    if(!strcmp(ivar[i]->name,name))
    { return ivar[i];
    }
  }
  return 0;
}
QInstallVar* QInstall::FindFreeVar()
// Return free variable, or 0 if none is available
{
  int i;
  for(i=0;i<MAX_IVAR;i++)
  {
    if(ivar[i]->name.IsEmpty())
    { return ivar[i];
    }
  }
  qwarn("QInstall: not enough variables (%d) to handle script (%s)",
    MAX_IVAR,(cstring)name);
  return 0;
}

void QInstall::ExpandVars(qstring& str)
// Expands string; abc%myvar%def becomes abcFOOdef for example.
// Non-existing variables are expanded to the empty string
// (a log warning is generated in this case)
{
  cstring s;
  string  d;
  char buf[256];

  s=str; d=buf;
  while(*s)
  {
    if(*s=='%')
    { // Get var name (or %% = %)
      s++;
      if(*s=='%')
      { *d++='%';
      } else
      { // Actual var name
        QInstallVar *v;
        char   vname[256];
        string dv;

        dv=vname;
        for(dv=vname;*s!='%'&&*s!=0;s++)
        { *dv++=*s;
        }
        *dv=0;
        if(*s=='%')s++;		// Skip trailing %
//qdbg("find var '%s'\n",vname);
        v=FindVar(vname);
        if(v)
        { // Insert value of variable
          strcpy(d,(cstring)v->val);
          d+=strlen(v->val);
        } else
        { // Generate warning for the log
          qwarn("QInstall: variable '%s' not found in script '%s'",vname,name);
        }
      }
    } else
    { *d++=*s;
      s++;
    }
  }
  *d=0;
  str=buf;
}

static void makeKey(qstring& s)
{
  int len,i;
  len=strlen(s);
  for(i=0;i<len;i++)
  { char c;
    c=s[i];
//qdbg("makeKey: c='%c'\n",c);
    if(c>='A'&&c<='Z')continue;
    if(c>='a'&&c<='z')continue;
    if(c>='0'&&c<='9'&&i>0)continue;
    // Use a _ for safety
    s[i]='_';
  }
}

static int MakeRoot(cstring filename)
{
  //ShowID();
  //printf("Changing '%s' to setuid root.\n",filename);
  // Root=user 0, group 0
  QSetRoot(TRUE);
  if(chown(filename,0,0)==-1)qerr("QInstall:MakeRoot: chown failed");
  //if(chmod(filename,04755)==-1)perror("chmod failed");
  if(chmod(filename,04757)==-1)qerr("QInstall:MakeRoot: chmod failed");
  QSetRoot(FALSE);
  return TRUE;
}

/*************
* INSTALLING *
*************/
bool QInstall::Install()
// Run the script
// Returns FALSE in case an error occurred or the install was interrupted
{
  char cmd[256];
  char buf[256],buf2[256];
  int  launcherIndex;         // Number of launcher directory entries
  qstring desc,key,val,val2;
  qstring destDir(".");
  QInfo *log,*mtlInfo;

  if(!fIns)return FALSE;
  log=new QInfo(MTGetLogIni());
  mtlInfo=new QInfo(MTGetLauncherIni());
  launcherIndex=1;
  while(1)
  {
    fIns->GetString(cmd);
    if(fIns->IsEOF())break;
//qdbg(">> cmd='%s'\n",cmd);
    if(cmd[0]==';')
    { // Comment
      fIns->GetLine(buf,sizeof(buf));
    } else if(!strcmp(cmd,"set"))
    { QInstallVar *v;
      fIns->GetString(buf);		// Var
      fIns->GetString(val);		// Value
      if(msg)msg->printf("set %s = '%s'\n",(cstring)buf,(cstring)val);
//qdbg("set %s = '%s'\n",(cstring)buf,(cstring)val);
      v=FindVar(buf);
      if(!v)
      { v=FindFreeVar();
        v->name=buf;
      }
      if(v)
      { v->val=val;
      }
    } else if(!strcmp(cmd,"desc"))
    { // Description; skip
      fIns->GetString(desc);
      sprintf(buf2,"products.%s.desc",(cstring)name);
qdbg("desc->log '%s'='%s'\n",buf2,(cstring)desc);
      log->SetString(buf2,(cstring)desc);
      if(prg)
      { prg->SetText(desc);
        prg->Paint(); //XSync(XDSP,FALSE);
        app->Run1();
      }
    } else if(!strcmp(cmd,"dir"))
    { // Install directory
      fIns->GetString(destDir);
      ExpandVars(destDir);
      sprintf(buf,"%s",(cstring)destDir);
      if(msg)msg->printf("Install directory '%s'...\n",buf);
    } else if(!strcmp(cmd,"md"))
    { // Make directory
      fIns->GetString(val);
      ExpandVars(val);
      //sprintf(buf,"%s/%s",(cstring)destDir,(cstring)val);
      sprintf(buf,"%s",(cstring)val);
      if(msg)msg->printf("Create directory '%s'...\n",buf);
      MakeDir(val);
      // Log creation for uninstall
      qstring s(buf);
      makeKey(s);
      sprintf(buf2,"products.%s.dirs.%s",(cstring)name,(cstring)s);
      log->SetString(buf2,buf);
qdbg("log dir '%s'=%s\n",buf2,buf);

    } else if(!strcmp(cmd,"cp"))
    { // Copy file (options: -ifnew)
      bool pIfNew=FALSE;
     readfname:
      fIns->GetString(val);
      if(val[0]=='-')
      { // Option given
        if(val=="-ifnew")pIfNew=TRUE;
        else qwarn("cp option '%s' unknown",(cstring)val);
        // Read real filename
        goto readfname;
      }
      ExpandVars(val);
      
      sprintf(buf,"%s/%s",(cstring)destDir,(cstring)val);
      if(msg)msg->printf("Copy file '%s'...\n",(cstring)val);
      if(pIfNew)
      { // Check dst file
        if(QFileExists(buf))
        { qdbg("File '%s' already exists; skipping\n",buf);
          goto no_copy;
        }
      }
      CopyFile(val,destDir);
      // Log file copy for uninstall
      { qstring s(buf);
        makeKey(s);
        sprintf(buf2,"products.%s.files.%s",(cstring)name,(cstring)s);
        log->SetString(buf2,buf);
qdbg("log file '%s'=%s\n",buf2,buf);
      }
     no_copy:
      ;
    } else if(!strcmp(cmd,"exe"))
    { // Make executable
      fIns->GetString(val);
      ExpandVars(val);
      sprintf(buf,"%s/%s",(cstring)destDir,(cstring)val);
      if(msg)msg->printf("Exe file '%s'...\n",buf);
qdbg("exe '%s'\n",buf);
      chmod(QExpandFilename(buf),0755);
    } else if(!strcmp(cmd,"makeroot"))
    { // Make executable
      fIns->GetString(val);
      ExpandVars(val);
      sprintf(buf,"%s/%s",(cstring)destDir,(cstring)val);
      if(msg)msg->printf("Makeroot '%s'...\n",buf);
qdbg("makeroot '%s'\n",buf);
      MakeRoot(QExpandFilename(buf));
    } else if(!strcmp(cmd,"font"))
    { // Font; filename & xfontname
      fIns->GetString(val);
      ExpandVars(val);
      fIns->GetString(val2);
      if(msg)msg->printf("Font '%s'...\n",(cstring)val2);
      InstallFont(val,val2);
      // Log font for uninstall (future)
      qstring s(val2);
      makeKey(s);
      sprintf(buf2,"products.%s.fonts.%s",(cstring)name,(cstring)s);
      log->SetString(buf2,val2);
qdbg("log file '%s'=%s\n",buf2,buf);
    } else if(!strcmp(cmd,"mtl"))
    { // MTLaunch entry (the 'registry')
      fIns->GetString(key);
      fIns->GetString(val);
      ExpandVars(val);
qdbg("set '%s' to '%s' in MTL\n",(cstring)key,(cstring)val);
      if(msg)msg->printf("MTL '%s'='%s'\n",(cstring)key,(cstring)val);
      mtlInfo->SetString(key,val);
    } else if(!strcmp(cmd,"~mtl"))
    { // Launcher removal directory
      fIns->GetString(key);
      sprintf(buf2,"products.%s.launcher%d",(cstring)name,launcherIndex);
qdbg("~mtl '%s'='%s'\n",buf2,(cstring)key);
      log->SetString(buf2,(cstring)key);
      launcherIndex++;
    } else if(cmd[0]=='~')
    { // Uninstall commands; skip
      fIns->GetLine(buf,sizeof(buf));
    } else
    { // Unknown
      fIns->GetLine(buf,sizeof(buf));
      if(msg)msg->printf("** ? %s%s\n",cmd,buf);
      qwarn("QInstall: unsupported line '%s%s' in '%s'\n",
        cmd,buf,(cstring)name);
    }
//qdbg("cmd='%s'\n",cmd);
  }
  // Artificial 100% marker
  UpdateProgress(1,1);
  delete mtlInfo;
  delete log;
  return TRUE;
}

// Install() helper functions
static bool updateProgress(void *p,int current,int total)
{ QInstall *install;
  install=(QInstall*)p;
//qdbg("updateProgress(%d,%d)\n",current,total);
  install->UpdateProgress(current,total);
  return TRUE;
}
bool QInstall::CopyFile(cstring fname,cstring destDir)
// Copy 'fname' from the archive into the destDir
{
  char buf[1024];
  DBG_C("CopyFile");
  
qdbg("Copy '%s' into '%s'\n",fname,destDir);
qdbg("  ar=%p\n",ar);
  if(!ar)return FALSE;
  // Start timer
  timing=app->GetVTraceCount();
  if(ar->FindFile(fname))
  {
    ar->SetCallback(updateProgress,(void*)this);
#ifdef FAKE_EXTRACT
    ar->SkipFile();
#else
    sprintf(buf,"%s/%s",destDir,fname);
    ar->ExtractFile(fname,buf,TRUE);
#endif
  } else qdbg("Can't find '%s'\n",fname);
  // FindFile() also calls the cb
  ar->SetCallback(0);
  return TRUE;
}

bool QInstall::MakeDir(cstring dirName)
// Create directory 'dirName'
// FUTURE: Report errors during directory create
{
  char buf[1024];
qdbg("Make dir '%s'\n",dirName);
  strcpy(buf,QExpandFilename(dirName));
  // Use default rwxr-xr-x options
  mkdir(buf,0755);
  return TRUE;
}

void QInstall::DefineProgress(QProgress *_prg)
// To get feedback on the installation, supply your progress indicator
// and/or messagebox (DefineMsg)
{
  prg=_prg;
}
void QInstall::DefineMsg(QMsg *_msg)
// See DefineProgress()
{
  msg=_msg;
}

void QInstall::UpdateProgress(int current,int total)
{
  //static time t;
  int t;
//qdbg("QInstall:UpdateProgress(%d,%d)\n",current,total);
  t=app->GetVTraceCount();
  if(t>timing+0)
  { timing=t;
    prg->SetProgress(current,total,TRUE);
    XSync(XDSP,FALSE);
  }
}

/********************
* FONT INSTALLATION *
********************/
bool FileContains(cstring fname,cstring line)
// Returns true if file 'fname' contains a line 'line'
{
  QFile f(fname);
  char buf[256];
qdbg("Find '%s'\n",line);

  while(!f.IsEOF())
  {
    f.GetLine(buf,sizeof(buf));
//qdbg("line '%s'\n",buf);
    if(!strcmp(buf,line))
    { return TRUE;
    }
  }
  return FALSE;
}
bool FileAppend(cstring fname,cstring line)
// Append a line
{ FILE *fp;
  fp=fopen(fname,"ab");
  if(!fp)return FALSE;
  fprintf(fp,"%s\n",line);
  fclose(fp);
  return TRUE;
}

bool QInstall::InstallFont(qstring& fileName,qstring& xFontName)
// Install font (.PFB) with the given xfontname
// Does a bit what ~/bin/AddFont does.
// As we are root, we should be careful with system(); use full paths
{
  char buf[256],buf2[256];
  qstring t1name;
  
  if(!QSetRoot(TRUE))
  { qerr("Can't install font without having root permissions.\n");
    return FALSE;
  }
  
  // Extract .PFB file
  CopyFile(fileName,"/tmp");
  
  // Convert to PFA
  unlink("/tmp/newfont.pfa");
  sprintf(buf,"/usr/sbin/pfb2pfa /tmp/%s /tmp/newfont.pfa",(cstring)fileName);
  system(buf);
  if(!QFileExists("/tmp/newfont.pfa"))
  { qerr("Couldn't create newfont.pfa from '%s'",(cstring)fileName);
    return FALSE;
  }
  
  // Find Type1 name
  system(
    "/sbin/grep /FontName /tmp/newfont.pfa | tr -d '/' | awk '{print $2}'"
    " >/tmp/newfont.txt");
  { QFile f("/tmp/newfont.txt");
    f.GetString(t1name);
  }
  unlink("/tmp/newfont.txt");
  
  // Copy .pfa file into Type1 directory
qdbg("Type1 name='%s'\n",(cstring)t1name);
  sprintf(buf,"/usr/lib/DPS/outline/base/%s",(cstring)t1name);
qdbg("Copy .pfa to '%s'\n",buf);
  if(!QCopyFile("/tmp/newfont.pfa",buf))
  { qerr("Can't copy font .pfa (%s) to Type1 directory",(cstring)fileName);
    return FALSE;
  }
  
  // Enter font line in local fonts file, IF it doesn't already exist
  sprintf(buf,"%s %s",(cstring)t1name,(cstring)xFontName);
  if(!FileContains("/usr/lib/X11/fonts/ps2xlfd_map.local",buf))
  {
qdbg("Append font name\n");
    if(!FileAppend("/usr/lib/X11/fonts/ps2xlfd_map.local",buf))
    {
      qerr("Can't append font name (%s) to ps2xlfd_map.local",
        (cstring)fileName);
    }
  }
  
  // Remake PostScript resource database
  system("/usr/bin/X11/makepsres -o /usr/lib/DPS/DPSFonts.upr "
    "/usr/lib/DPS/outline/base /usr/lib/DPS/AFM");
  // Build X style font directory for Type1 fonts
  system("/usr/bin/X11/type1xfonts");
  // Rebuild font table
  system("/usr/bin/X11/xset fp rehash");
  
  QSetRoot(FALSE);
  return TRUE;
}

#endif
// __sgi

