/*
 * qlib/qinfo.h - class QInfo for settings; subtrees are possible
 * 25-03-97: Created!
 * NOTES:
 * - Sort of like the Registry from Win95/NT.
 * - A Write() imposes the standard layout (the entire file is written from
 *   scratch).
 * BUGS:
 * - When reading QInfo, a warning should be generated for level>MAX_LEVEL
 * (C) MG/RVG
 */

#ifndef __QLIB_QINFO_H
#define __QLIB_QINFO_H

#include <qlib/object.h>
#include <qlib/file.h>

#define QINT_COMMENT    0	// QInfoNode types
#define QINT_SYMBOL	1
#define QINT_TREE	2	// Tree or subtree
#define QINF_DELETED	256	// Flag to indicate is has been deleted

#define QINFO_MAX_LEVEL	10	// Max nesting of value names

#define QIF_MODIFIED    1	// QInfo flags; at least 1 setting was changed
#define QIF_EXISTS	2	// File could be opened?

//class QInfoIterator;

class QInfoNode
{ 
  friend class QInfoIterator;
  friend class QInfo;

  int type;
  string value;
  string name;

  QInfoNode *next;      // List of current depth
  QInfoNode *child;     // List of the children
  QInfoNode *reference; // Reference tree, if any
  QInfoNode *parent;        // 0 for root

 public:
  QInfoNode(QInfoNode *parent,int type,cstring name,cstring value=0);
  ~QInfoNode();

  QInfoNode *AddChild(int type,cstring name,cstring value=0);
  
  QInfoNode *GetChild(){ return child; }
  QInfoNode *GetNext(){ return next; }

  int  GetType(){ return type; }
  void SetType(int newType);
  void Delete();
  bool IsDeleted();

  cstring GetName() const { return name; }
  cstring GetValue() const { return value; }
  void   GetFullName(qstring& s) const;
  void   SetName(cstring name);
  void   SetValue(cstring v);
};

class QInfo : public QObject
{
  friend class QInfoIterator;		// TrackPath()

 protected:
  string fileName;
  QInfoNode *root;
  int    flags;         // Modified?
  QInfo *infoFallback;                  // In case values are not found

  QInfoNode *TrackPath(cstring path);
  void       FreeTree(QInfoNode *t);

  void WriteNode(QFile *f,QInfoNode *node,int level);

 public:
  QInfo(cstring fname);
  ~QInfo();

  bool FileExists();			// File was found?
  bool Read();
  bool Write(cstring fName=0);       // Write revised version
  bool IsModified();                    // Info file is dirty?
  void DebugOut();
  void   SetFallback(QInfo *info);
  QInfo *GetFallback(){ return infoFallback; }
  
  bool PathExists(cstring path);
  bool IsTree(cstring path);		// Path has subvariables?
  bool IsLeaf(cstring path);		// Path is a leaf?

  bool RemovePath(cstring path);	// Remove subtree or leaf

  bool    GetString(cstring vName,string dst,cstring def=0);
  bool    GetString(cstring vName,qstring& dst,cstring def=0);
  cstring GetStringDirect(cstring vName,cstring def=0);
  int     GetInt(cstring vName,int def=0);
  bool    GetInts(cstring vName,int n,int *p);
  float   GetFloat(cstring vName,float def=0.0);
  bool    GetFloats(cstring vName,int n,float *p);
  bool    GetRect(cstring vName,QRect *r);
  bool    GetPoint(cstring vName,QPoint *p);
  bool    GetPoint3(cstring vName,QPoint3 *p);
  bool    GetSize(cstring vName,QSize *p);

  bool SetString(cstring vName,cstring v);
  bool SetInt(cstring vName,int n);
  bool SetFloat(cstring vName,float n);
};

class QInfoIterator
{
 private:
  QInfo *info;
  QInfoNode *curNode;
  QInfoNode *nestNode[QINFO_MAX_LEVEL];
  int        nestLevel;

  bool GetNextAny(qstring& vName,bool recursive);

 public:
  QInfoIterator(QInfo *info,cstring path=0);
  ~QInfoIterator();

  bool GetNext(qstring& vName,bool recursive=FALSE);
};

#endif
