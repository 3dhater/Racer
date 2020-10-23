/*
 * QInfo.cpp - class QInfo for settings, like QIni, only subtrees are possible
 * 25-03-1997: Created!
 * 23-08-1999: Node inserts are now reversed for the sake of QInfoIterator.
 * 25-10-1999: Support for subtrees to have values
 * NOTES:
 * - Sort of like the Registry from Win95/NT.
 * - A Write() imposes the standard layout (the entire file is written from
 *   scratch).
 * - QInfoNodes() are not really deleted (RemovePath()), but marked deleted,
 * and not written by Write().
 * BUGS:
 * - When reading QInfo, a warning should be generated for level>QINFO_MAX_LEVEL
 * (C) MG/RVG
 */

#include <qlib/info.h>
#include <qlib/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// DebugOut() active?
//#define DBG_OUTPUT

// Define TEST to check alloc/free consistency
#define TEST

#ifdef TEST
int qinCountCtor,qinCountDtor;
#endif

#undef  DBG_CLASS
#define DBG_CLASS "QInfoNode"

/************
* QINFONODE *
************/
QInfoNode::QInfoNode(QInfoNode *iparent,int t,cstring iname,cstring ivalue)
{ type=t;
  value=0;
  name=0;
  next=0;
  child=0;
  parent=iparent;
  if(ivalue)SetValue(ivalue);
  SetName(iname);
#ifdef TEST
  qinCountCtor++;
#endif
}
QInfoNode::~QInfoNode()
{ if(value)qfree(value);
  if(name)qfree(name);
#ifdef TEST
  //printf("QInfoNode dtor\n");
  qinCountDtor++;
#endif
}

void QInfoNode::GetFullName(qstring& newName) const
// Returns full path name of this node in 'newName' (e.g. "root.val.x")
{
  char buf[1024];
  const QInfoNode *lev[QINFO_MAX_LEVEL];
  int i,levels;
  const QInfoNode *node;

  levels=0;
  node=this;
  lev[0]=node;
  for(i=1;i<QINFO_MAX_LEVEL;i++)
  {
    if(node->parent!=0&&node->parent->parent!=0)  // 2nd arg skips root (!)
    { lev[i]=node->parent;
      node=node->parent;
    } else break;
  }
  levels=i;
  // Construct name
  buf[0]=0;
  for(i=levels-1;i>=0;i--)
  {
    if(buf[0])strcat(buf,".");
    sprintf(&buf[strlen(buf)],lev[i]->GetName());
  }
  newName=buf;
}

void QInfoNode::Delete()
{ type|=QINF_DELETED;
}
bool QInfoNode::IsDeleted()
{ if(type&QINF_DELETED)return TRUE;
  return FALSE;
}

void QInfoNode::SetType(int ntype)
{ type=ntype;
}
void QInfoNode::SetName(cstring newName)
{
  if(name)qfree(name);
  name=qstrdup(newName);
}
void QInfoNode::SetValue(cstring newValue)
{
  if(value)qfree(value);
  value=qstrdup(newValue);
}
QInfoNode *QInfoNode::AddChild(int type,cstring name,cstring value)
// Add a node as a child
{ QInfoNode *node,*lastNode;
  node=new QInfoNode(this,type,name,value);

  // Insert AFTER all previous children for nicer iterations
  node->next=0;
  if(!child)
  { child=node;
  } else
  { lastNode=child;
    while(lastNode->next)
      lastNode=lastNode->next;
    lastNode->next=node;
  }
  // Head insert
  //node->next=child;
  //child=node;
  return node;
}

#undef  DBG_CLASS
#define DBG_CLASS "QInfo"

/********
* QINFO *
********/
QInfo::QInfo(cstring fname)
{
  DBG_C("ctor")
  DBG_ARG_S(fname)

  root=0;
  flags=0;
  fileName=qstrdup(fname);
  // Create implicit root of which all trees are subtrees
  root=new QInfoNode(0,QINT_TREE,"root");
  Read();
}
QInfo::~QInfo()
{ // Test for file update
  if(flags&QIF_MODIFIED)
    Write();
  // Free tree
  FreeTree(root);
  if(fileName)qfree(fileName);
}

bool QInfo::IsModified()
// Returns TRUE if any values were modified (needs writing)
{
  if(flags&QIF_MODIFIED)return TRUE;
  return FALSE;
}
bool QInfo::FileExists()
{
  if(flags&QIF_EXISTS)return TRUE;
  return FALSE;
}

void QInfo::FreeTree(QInfoNode *t)
// Delete tree starting with node 't' (recursive)
{ if(!t)return;
  FreeTree(t->child);
  FreeTree(t->next);
  delete t;
}

/**********
* READING *
**********/
static void SkipSpaces(QFile *f)
{
  char c;
  while(1)
  { if(f->IsEOF())break;
    c=f->GetChar();
    if(c!=' '&&c!=10&&c!=13)
    { f->UnGetChar(c);
      break;
    }
  }
}
static void ReadToken(QFile *f,char *s,int maxLen)
{
  DBG_C_FLAT("ReadToken")
  DBG_ARG_P(f)
  DBG_ARG_S(s)
  DBG_ARG_I(maxLen)

  char c;
  int len;

  SkipSpaces(f);
  if(f->IsEOF())
  { *s=0; return; }
  c=f->GetChar();
  *s++=c;
  // Check single-char commands
  if(c==';'||c=='{'||c=='}'||c=='=')
  { *s=0; return;
  }
  len=1;
  while(1)
  {
    if(f->IsEOF())break;
    c=f->GetChar();
    if(c==' '||c==10||c==13||c=='=')
    { f->UnGetChar(c); break; }
    *s++=c;
//qdbg("  c=%d, IsEOF()=%d\n",c,f->IsEOF());
    if(++len>maxLen-2)break;
  }
  *s=0;
}
static void ReadUptoEOL(QFile *f,char *s,int maxLen)
{ char c;
  int len=0;
  while(1)
  { if(f->IsEOF())break;
    c=f->GetChar();
    if(c==10||c==13)break;
    *s++=c;
    if(++len>maxLen-2)break;
  }
  *s=0;
}

bool QInfo::Read()
{
  DBG_C("Read")

  QFile *f;
  char cmd[256];
  int i,curLevel;
  QInfoNode *in[QINFO_MAX_LEVEL];     // Stack of nodes for every depth
  QInfoNode *lastChild;         // Last generated child
  
//printf("QInfo::Read\n");
  for(i=0;i<QINFO_MAX_LEVEL;i++)
    in[i]=0;
  f=new QFile(fileName,QFile::READ_TEXT);
  if(!f->IsOpen())
  { // Mark file as non-existant
    qwarn("QInfo: can't open '%s'\n",fileName);
  } else
  { flags|=QIF_EXISTS;		// Remember file existed
  }
  in[0]=root; curLevel=0;
  while(1)
  {
    ReadToken(f,cmd,sizeof(cmd));
//qdbg("ReadToken(cmd=%s)\n",cmd);
    if(f->IsEOF())break;
    if(cmd[0]==';')
    { ReadUptoEOL(f,cmd,sizeof(cmd));
      // Add comment as node
      lastChild=in[curLevel]->AddChild(QINT_COMMENT,";",cmd);
    } else if(cmd[0]=='{')
    { // Kiddies coming up
      if(!lastChild){ qwarn("QInfo: { without groupname"); goto skip; }
      // Make the last symbol the root of its subtree
      if(++curLevel>=QINFO_MAX_LEVEL)
      { qwarn("QInfo: '%s' nests too deep (max %d levels)",fileName);
        curLevel--;      // Recovery (go on)
      }
      in[curLevel]=lastChild;
      // Make 'lastChild' a tree
      lastChild->SetType(QINT_TREE);
     skip:;
    } else if(cmd[0]=='}')
    { // Step down a level
      if(curLevel>0)
        curLevel--;
    } else if(cmd[0]=='=')
    { // Assign value to node
      if(!lastChild)
      { printf("QInfo: assign without symbol\n"); goto skip_let; }
      ReadUptoEOL(f,cmd,sizeof(cmd));
//qdbg("Assign '%s' to '%s'\n",cmd,lastChild->GetName());
      lastChild->SetValue(cmd);
     skip_let:;
    } else
    {
//qdbg("Add node '%s' as child of '%s'\n",cmd,in[curLevel]->GetName());
      lastChild=in[curLevel]->AddChild(QINT_SYMBOL,cmd);
    }
    //else printf("cmd '%s'\n",cmd);
  }
  delete f;
  return TRUE;
}

/************
* DEBUGGING *
************/
#ifdef DBG_OUTPUT
static void qinOutNode(QInfoNode *node)
{ string n,v;
 printit:
  n=node->GetName();
  v=node->GetValue();
  switch(node->GetType())
  { case QINT_TREE:
      printf("tree '%s'\n",n); break;
    case QINT_SYMBOL:
      if(!v)v="<no value>";
      printf("node %s=%s\n",n,v); break;
    default:
      printf("(%s)\n",v);
  }
  if(node->child)
    qinOutNode(node->child);
  if(node->next)
  { node=node->next;
    goto printit;               // Avoid recursion
  }
}
#endif
void QInfo::DebugOut()
// Write to stdout
{
#ifdef DBG_OUTPUT
  printf("-----\n");
  qinOutNode(root);
  printf("-----\n");
#endif
}

/**********
* WRITING *
**********/
static void indent(FILE *fp,int level)
{ if(level)
    fprintf(fp,"%*s",level*2," ");
}
void QInfo::WriteNode(QFile *f,QInfoNode *node,int level)
{ cstring n,v;
  FILE *fp=f->GetFP();

  //if(!out)goto skip_root;
 //printit:

  if(node->IsDeleted())
    goto do_next;
    
 skip_root:
  n=node->GetName();
  v=node->GetValue();
  switch(node->GetType())
  { case QINT_TREE:
      if(strcmp(n,"root"))
      { indent(fp,level);
        if(v)fprintf(fp,"%s=%s\n",n,v);
        else fprintf(fp,"%s\n",n);
        indent(fp,level); fprintf(fp,"{\n");
      }
      break;
    case QINT_SYMBOL:
      if(!v)v="";
      indent(fp,level);
      fprintf(fp,"%s=%s\n",n,v);
      break;
    case QINT_COMMENT:
      indent(fp,level);
      fprintf(fp,";%s\n",v); break;
    default:
      qerr("QInfo:Write() internal error; unknown node type");
      break;
  }

  // Check for closing bracket
  if(node->GetType()==QINT_TREE)
  { if(node->child)
      WriteNode(f,node->child,level+1);
    if(strcmp(n,"root"))
    { indent(fp,level); fprintf(fp,"}\n");
    }
  }

 do_next:
  // Post-dfs; order of writing changed on 26-8-1999
  if(node->next)
  { //node=node->next;
    WriteNode(f,node->next,level);
    //goto printit;               // Avoid recursion
  }
}

bool QInfo::Write(cstring fname)
{ QFile *f;
  if(fname==0&&IsModified()==FALSE)
  {
    // No need to write
    return TRUE;
  }
  if(!fname)fname=fileName;
  //printf("QInfo::Write\n");
  f=new QFile(fname,QFile::WRITE);
  if(f)
  { WriteNode(f,root,-1);
    flags&=~QIF_MODIFIED;
    delete f;
  }
  return TRUE;
}

/*****************
* GETTING VALUES *
*****************/
QInfoNode *QInfo::TrackPath(cstring ipath)
// Track path to a node
// If path doesn't exist, then 0 is returned
// If 'ipath'==0 or an empty string, the root is returned
{ char *s,*path;
  QInfoNode *tree,*node;
 
  DBG_C("TrackPath");
  if(ipath==0||ipath[0]==0)
  { // No path; return root
    return root;
  }

  //printf("GetString(%s)\n",vName);
  path=qstrdup(ipath);
  tree=root;
  for(s=strtok(path,".");s;s=strtok(0,"."))
  { // Find node as a child of 'tree'
    for(node=tree->child;node;node=node->next)
    { if(!strcmp(s,node->GetName()))
      { //printf("found!\n");
        if(node->GetType()==QINT_TREE)
        { // Dive into tree
          tree=node;
          goto do_subtree;
        } else
        { // Symbol
          // Check for remaining tokens; error if so
          if(strtok(0,"."))
          { // Path that was given was too deep!
            qfree(path);
            return 0;
          }
          qfree(path);
          return node;
        }
      }
    }
    // Path was not found
    qfree(path);
    return 0;
   do_subtree:
    ;
  }
  return node;
}

bool QInfo::PathExists(cstring path)
{
  if(!TrackPath(path))return FALSE;
  return TRUE;
}
bool QInfo::IsTree(cstring path)
{
  QInfoNode *node;
  node=TrackPath(path);
  if(!node)return FALSE;
  if(node->GetType()==QINT_TREE)return TRUE;
  return FALSE;
}
bool QInfo::IsLeaf(cstring path)
{
  QInfoNode *node;
  node=TrackPath(path);
  if(!node)return FALSE;
  if(node->GetType()==QINT_SYMBOL)return TRUE;
  return FALSE;
}

bool QInfo::GetString(cstring vName,string dst,cstring def)
// Returns TRUE if the value was found
// vName is a tree path, broken by '.' (windows.main.x)
// FUTURE: Use TrackPath() to find the node.
{ char *s,*path;
  QInfoNode *tree,*node;
  
  DBG_C("GetString")
  DBG_ARG_S(vName);

  //printf("GetString(%s)\n",vName);
  path=qstrdup(vName);
  tree=root;
  for(s=strtok(path,".");s;s=strtok(0,"."))
  { // Find node as a child of 'tree'
    //printf("s='%s', vName=%s, path=%s\n",s,vName,path);
    for(node=tree->child;node;node=node->next)
    { if(!strcmp(s,node->GetName()))
      {
//qdbg("found!\n");
        if(node->GetType()==QINT_TREE)
        { // Dive into tree
          tree=node;
          goto do_subtree;
        } else
        { // Symbol
          // Check for remaining tokens; error if so
//qdbg("TrackPath: node '%s' value='%s'\n",node->GetName(),node->GetValue());
          if(strtok(0,"."))
          { // Path that was given was too deep!
            goto do_def;
          }
         found_node:
          if(node->IsDeleted())goto do_def;
          strcpy(dst,node->GetValue());
          qfree(path);
          return TRUE;
        }
      }
    }
    // Path was not found; use default (also done for invalid paths)
   do_def:
    if(def)strcpy(dst,def);
    else *dst=0;
    qfree(path);
    return FALSE;
   do_subtree:
    ;
  }
  // Path incomplete to get into a symbol
  if(tree->GetValue())
  { // Subtree has a value (although not a leaf); return it
    node=tree;
    goto found_node;
  }
//qdbg("Path incomplete; use def\n");
  goto do_def;
}
bool QInfo::GetString(cstring vName,qstring& s,cstring def)
// Returns TRUE if the value was found, otherwise FALSE
// and s==def
{ char buf[1024];
  bool r;
  r=GetString(vName,buf,def);
  s=buf;
  return r;
}
cstring QInfo::GetStringDirect(cstring vName,cstring def)
{
  static char buf[1024];
  GetString(vName,buf,def);
  return buf;
}

int QInfo::GetInt(cstring vName,int def)
{ char buf[80];
  if(!GetString(vName,buf))
    return def;
  return Eval(buf);
}
// Some useful variations
bool QInfo::GetInts(cstring vName,int n,int *p)
// Shortcut to get multiple values out of 1 string
{ char buf[256],*s;
  int i;
  if(!GetString(vName,buf))
    return FALSE;
  // Get values
  i=0;
  for(s=strtok(buf," ,");s!=0&&i<n;s=strtok(0," ,"),i++)
  { p[i]=Eval(s);
  }
  for(;i<n;i++)
    p[i]=0;
  return TRUE;
}
float QInfo::GetFloat(cstring vName,float def)
{ char buf[80];
  if(!GetString(vName,buf))
    return def;
  return atof(buf);
}
bool QInfo::GetRect(cstring vName,QRect *r)
{
  int n[4];
  if(!GetInts(vName,4,n))
  { // Return FALSE for failure? Rather fill in default rectangle,
    // cause probably no-one will ever check.
    // Log warning though
    //qwarn("QInfo::GetRect(); '%s' not found; using default 0,0 1x1",vName);
    r->x=r->y=0;
    r->wid=r->hgt=0;
    return TRUE;
  }
  // Distribute over rectangle
  r->x=n[0];
  r->y=n[1];
  r->wid=n[2];
  r->hgt=n[3];
  return TRUE;
}
bool QInfo::GetPoint(cstring vName,QPoint *p)
// Retrieve 2D (int) point
{
  int n[2];
  if(!GetInts(vName,2,n))
  { // Return FALSE for failure? Rather fill in default point,
    // cause probably no-one will ever check.
    // Log warning though
    //qwarn("QInfo::GetPoint(); '%s' not found; using default 0,0,0",vName);
    p->x=p->y=0;
    return TRUE;
  }
  p->x=n[0];
  p->y=n[1];
  return TRUE;
}
bool QInfo::GetPoint3(cstring vName,QPoint3 *p)
// Retrieve 3D (int) point
{
  int n[3];
  if(!GetInts(vName,3,n))
  { // Return FALSE for failure? Rather fill in default point,
    // cause probably no-one will ever check.
    // Log warning though
    //qwarn("QInfo::GetPoint3(); '%s' not found; using default 0,0,0",vName);
    p->x=p->y=p->z=0;
    return TRUE;
  }
  p->x=n[0];
  p->y=n[1];
  p->z=n[2];
  return TRUE;
}
bool QInfo::GetSize(cstring vName,QSize *p)
// Retrieve 2D size
{
  int n[2];
  if(!GetInts(vName,2,n))
  { // Return FALSE for failure? Rather fill in default point,
    // cause probably no-one will ever check.
    // Log warning though
    //qwarn("QInfo::GetPoint(); '%s' not found; using default 0,0,0",vName);
    p->wid=p->hgt=0;
    return TRUE;
  }
  p->wid=n[0];
  p->hgt=n[1];
  return TRUE;
}

/*****************
* SETTING VALUES *
*****************/
bool QInfo::SetString(cstring vName,cstring v)
// Set a variable; creates the path if needed
// Returns TRUE if the set was succesful. It may fail because
// vName references a tree, for example.
{ 
  DBG_C("SetString")
  char *s,*path;
  QInfoNode *tree,*node;
  
  path=qstrdup(vName);
  tree=root;
  
  for(s=strtok(path,".");s;)
  { // Find node as a child of 'tree'
    //printf("s='%s', vName=%s, path=%s\n",s,vName,path);
    for(node=tree->child;node;node=node->next)
    { if(!strcmp(s,node->GetName()))
      { //printf("found!\n");
        if(node->GetType()==QINT_TREE)
        { // Dive into tree
          tree=node;
          goto do_subtree;
        } else
        { // Symbol; check for remaining tokens; error if so
          if(strtok(0,"."))
          { // Path that was given was too deep!
            goto do_def;
          }
         setsym:
          node->SetValue(v);
          flags|=QIF_MODIFIED;          // Mark as modified
          qfree(path);
          return TRUE;
        }
      }
    }
    // Path node was not found; create it
    tree=tree->AddChild(QINT_TREE,s);
    s=strtok(0,".");
    if(!s)
    { // This was the last path item; set the symbol value
      node=tree;
      node->SetType(QINT_SYMBOL);
      goto setsym;
    }
    goto do_next;
   do_def:
    qfree(path);
    return FALSE;
   do_subtree:
    s=strtok(0,".");
   do_next:;
  }
  // Path incomplete to get into a symbol
  goto do_def;
}

bool QInfo::SetInt(cstring vName,int n)
{ char buf[30];
  sprintf(buf,"%d",n);
  return SetString(vName,buf);
}
bool QInfo::SetFloat(cstring vName,float n)
{ char buf[30];
  sprintf(buf,"%f",n);
  return SetString(vName,buf);
}

bool QInfo::RemovePath(cstring path)
// Removes an entire tree of nodes from the info file
// Returns FALSE if path could not be found
{
  QInfoNode *node;
  node=TrackPath(path);
  if(!node)return FALSE;
  // Data is being changed
  flags|=QIF_MODIFIED;
  // Mark tree as deleted
  node->Delete();
  return TRUE;
}

/****************
* QINFOITERATOR *
****************/

#undef  DBG_CLASS
#define DBG_CLASS "QInfoIterator"

QInfoIterator::QInfoIterator(QInfo *_info,cstring path)
{
  int i;
  info=_info;
  for(i=0;i<QINFO_MAX_LEVEL;i++)
  { nestNode[i]=0;
  }
  nestLevel=0;
  curNode=info->TrackPath(path);
  if(curNode!=0&&curNode->GetType()==QINT_TREE)
  { // Begin with the first child
    curNode=curNode->child;
  }
//qdbg("qinfoit: curNode=%p\n",curNode);
}
QInfoIterator::~QInfoIterator()
{
}

bool QInfoIterator::GetNextAny(qstring& vName,bool recursive)
// Puts the name of the next node in 'vName'
// Returns TRUE if an item was available, FALSE if at the end of the list
// Users normally use GetNext(), which skips comments and extraneous stuff
// 'vName' is currently ignored
// Notice that GetNext() advances the curNode, but it keeps it out of sync
// for the best report order.
{
  if(recursive)
  {
    // Try depth first
    if(curNode->child)
    { nestNode[nestLevel]=curNode;
      nestLevel++;
      if(nestLevel>QINFO_MAX_LEVEL)
#ifdef DEBUG
        // Should not get here because QInfo should guard level depths
        qerr("nestLevel>%d (QINFO_MAX_LEVEL) in QInfoIterator",
          QINFO_MAX_LEVEL);
#else
        nestLevel--;
#endif
      curNode=curNode->child;
      goto report_node;
    }
  }
  // Try siblings
  curNode=curNode->next;
  if(!curNode)
  { // Fall back from nesting?
    if(nestLevel==0)
      return FALSE;
    nestLevel--;
    curNode=nestNode[nestLevel];
    curNode=curNode->next;
  }

 report_node:
  if(!curNode)return FALSE;
  // Store name
  //*vName=curNode->GetName();
#ifdef OBS
  curNode->GetFullName(vName);
#endif
  return TRUE;
}

bool QInfoIterator::GetNext(qstring& vName,bool recursive)
// Puts the name of the next item in 'vName'
// Returns TRUE if an item was available, FALSE if at the end of the list
// This function skips comments and such.
// Note this one is off-by-1 with GetNextAny() (which is why you should
// only call this function).
{
  QInfoNode *lastNode;

 retry:
  lastNode=curNode;
  if(!lastNode)
    return FALSE;
  GetNextAny(vName,recursive);
  if(lastNode->GetType()==QINT_COMMENT)
    goto retry;
  lastNode->GetFullName(vName);
#ifdef OBS
  if(!GetNextAny(vName,recursive))
    return FALSE;
  //if(!curNode)return FALSE;
  if(curNode->GetType()==QINT_COMMENT)
    goto retry;
#endif
  return TRUE;
}

//#define TEST
#ifdef NOTDEF

void main()
{ char buf[80],*v;
  QInfo *info;
  //printf("mem: %ld\n",coreleft());
  info=new QInfo("tree.txt");
  //info->DebugOut();
  v="app.prefs.finale.rate";
  info->GetString(v,buf,"pierre");
  printf("%s=%s\n",v,buf);
  v="app.mainwindow.x";
  printf("%s=%d\n",v,info->GetInt(v));
  v="newpath.newsub.newsym";
  info->SetInt(v,info->GetInt(v)+1);
  v="newpath.newsub2.x";
  info->SetInt(v,info->GetInt(v)+1);
  //info->DebugOut();
  info->Write("tree.tx2");
  //printf("mem before dtor: %ld\n",coreleft());
  delete info;
  //printf("mem after  dtor: %ld\n",coreleft());
#ifdef TEST
  printf("QInfoNode ctor: %d, dtors: %d\n",qinCountCtor,qinCountDtor);
#endif
}

#endif
