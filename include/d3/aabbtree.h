// d3/aabbtree.h

#ifndef __D3_AABBTREE_H
#define __D3_AABBTREE_H

#include <d3/aabb.h>
#include <d3/geode.h>

struct DAABBPolyInfo
// Information about polygons, general (more like TriInfo, but well)
{
 public:
  DAABBPolyInfo *next;       // It's a linked list
  //float v[3][3];             // 3 vertices that make the triangle
  float *v[3];
  void *userData;            // Other info of the caller

 public:
  DAABBPolyInfo(float *v0,float *v1,float *v2);
  ~DAABBPolyInfo(){}

  // Attribs
  void *GetUserData(){ return userData; }
  void  SetUserData(void *p){ userData=p; }

  // Calculated info on polygon
  void GetCenter(DVector3 *v);
};

// Callback when querying the tree
typedef bool (*ABtreeCallback)(DAABBPolyInfo *plist);

class DAABBNode
// Node in the tree
{
 public:
  DAABB     aabb;             // Bounding box of the contained geometry
  DAABB     aabbSpace;        // The bounding space
  DAABBNode *parent,          // Parent node
            *child[2];        // Left/right tree
  void      *userData;
  DAABBPolyInfo *polyList;    // All polygons contained in this node

 public:
  DAABBNode(DAABBNode *parent);
  ~DAABBNode();

  // Attribs
  void *GetUserData(){ return userData; }
  void  SetUserData(void *p){ userData=p; }
  bool  IsLeaf(){ return polyList!=0; }

  void CalcBoundingBox();
  void AddPoly(DAABBPolyInfo *pi);

  // Building
  void BuildTree();

  // Querying
  bool Query(DAABB *aabb,ABtreeCallback cbfunc);
  DAABBNode *FindEnclosingNode(DAABB *aabb);

  // Debug
  void DbgPrint(int dep);
};

class DAABBTree
{
  DAABBNode *root;                // The tree top level
  //DAABBPolyInfo *polyList;        // Polygons to build the tree from

 public:
  DAABBTree();
  ~DAABBTree();

  // Attribs
  DAABBNode *GetRoot(){ return root; }

  // Building a tree
  void AddPoly(DAABBPolyInfo *pi);
  void AddGeob(DGeob *geob);
  void AddGeode(DGeode *geode);
  void BuildTree();
  //void BuildChildTree(DAABBNode *node);

  // Killing a tree
  void FreeTree(DAABBNode *tree);

  // I/O
  bool Load(cstring fname);
  bool Save(cstring fname);

  // Querying
  void Query(DAABB *ab,ABtreeCallback cb);

  // Debugging
  void DbgPrint();
};

#endif

