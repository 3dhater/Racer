// docgen.h - Documentation Generator

#ifndef __QLIB_DOCGEN_H
#define __QLIB_DOCGEN_H

#include <qlib/types.h>
#include <stdio.h>

class QDocGen
{
  const int MAX_INDIR=1000;		// Directories to process

  enum Flags
  { INCLUDE_ALL=1			// Don't check for missing .o files
  };

  string inDir[MAX_INDIR],outDir;
  int    inDirs;
  int    errors;
  int    flags;

  // File reading
  FILE  *fp;
  FILE  *fpSrc;			// Clone of source (with anchors)
  char   curLine[256];
  bool   nextLineBold;		// Generate bold source line?

  string ConvertToHTML(string s);

 public:
  QDocGen();
  ~QDocGen();

  void IncludeAllSourceFiles(bool yn);

  bool AddInputDir(string dir);

  bool ReadLine(bool autoClone=TRUE);
  bool AddLine();
  bool CloneLine();
  void CopyUntilStartOfFunction(FILE *fph);

  bool GenerateFile(string inDir,string fname);
  bool GenerateDir(string inDir);
  bool GenerateAll(string outDir);
};

#endif
