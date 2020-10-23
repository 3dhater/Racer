/*
 * DAABBTree - 3D tree for hierarchical geometry
 * 06-11-01: Created! (2D)
 * 08-11-01: Extended to 3D since that's coming very quickly.
 * NOTES:
 * - Lightly based on the paper 'Collision detection with Complex
 * Deformable Models using AABBTrees' from Gino van den Bergen ('98).
 * - A binary tree; splits space into about half at every node. Much like
 * an OBBTree, for which information is on the Internet, but simpler, as
 * no oriented boxes are used. May possibly be used for object-object
 * collisions (mainly deformable bodies). Target is to store context
 * information at each leaf (for Racer tracks for example: the surface type,
 * the material and such).
 * - All polygons end up in the leaves. They start out before building
 * in one big list in the root. The tree is created top-down.
 * - As this is all precalculation, a lot of 'new' is used and memory
 * is used freely.
 * USAGE:
 * - Create a DAABBTree instance; add all polygons to it using AddPoly(),
 * then call BuildTree() and Save().
 * (c) Dolphinity/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/aabbtree.h>
DEBUG_ENABLE

// Output status on tree building?
//#define VERBOSE

#ifdef VERBOSE
// Remember depth while building a tree?
#define KEEP_DEPTH
#endif

// Count split counts (into left & right subtree)?
// Always turn is on, since it is used in forced splits when all
// polygons get hung on one side.
#define COUNT_LEFT_RIGHT

// Store vertex coordinates locally? (recommended is to use just
// the pointers to the floating point data; since this is better
// for the memory footprint).
//#define COPY_VERTICES

#ifdef VERBOSE
#define QDBG  qdbg
#else
#define QDBG  1?(void)0:qdbg
#endif

/***********
* PolyInfo *
***********/
DAABBPolyInfo::DAABBPolyInfo(float *v0,float *v1,float *v2)
// Create a polygon info object with 3 vertices (v0[3], v1[3], v2[3])
{
  int i;

  userData=0;
  next=0;
  // Store vertex locations (don't copy, assume vertex data will stay)
#ifdef COPY_VERTICES
  // Copy vertex locations into polygon info
  for(i=0;i<3;i++)
  {
    v[0][i]=v0[i];
    v[1][i]=v1[i];
    v[2][i]=v2[i];
  }
#else
  v[0]=v0;
  v[1]=v1;
  v[2]=v2;
#endif
}
void DAABBPolyInfo::GetCenter(DVector3 *ov)
// Returns geometric center in 'v'
{
  ov->x=(v[0][0]+v[1][0]+v[2][0])/3.0;
  ov->y=(v[0][1]+v[1][1]+v[2][1])/3.0;
  ov->z=(v[0][2]+v[1][2]+v[2][2])/3.0;
}

/********
* Nodes *
********/
DAABBNode::DAABBNode(DAABBNode *_parent)
  : parent(_parent)
{
  aabb.Clear();
  aabbSpace.Clear();
  child[0]=child[1]=0;
  userData=0;
  polyList=0;
}
DAABBNode::~DAABBNode()
{
}

void DAABBNode::CalcBoundingBox()
// Check all polygons and calculate the bounding box (2D) for the node.
// Note that several nodes may overlap, but all triangles are split across
// boxes by looking only at their geometric center (to avoid polygons
// appearing in multiple leafs).
{
QDBG("Node:CalcBoundingBox()\n");
  DAABBPolyInfo *pi;
  int i;

  // Min/max extreme start
  aabb.min.x=aabb.min.y=aabb.min.z=99999.0;
  aabb.max.x=aabb.max.y=aabb.max.z=-99999.0;
  for(pi=polyList;pi;pi=pi->next)
  {
    // Note that the 3D X and Z values are used to map onto X/Y in 2D (!)
    for(i=0;i<3;i++)
    {
      if(pi->v[i][0]<aabb.min.x)aabb.min.x=pi->v[i][0];
      if(pi->v[i][1]<aabb.min.y)aabb.min.y=pi->v[i][1];
      if(pi->v[i][2]<aabb.min.z)aabb.min.z=pi->v[i][2];
      if(pi->v[i][0]>aabb.max.x)aabb.max.x=pi->v[i][0];
      if(pi->v[i][1]>aabb.max.y)aabb.max.y=pi->v[i][1];
      if(pi->v[i][2]>aabb.max.z)aabb.max.z=pi->v[i][2];
    }
  }
//aabb.min.DbgPrint("  bbox min");
//aabb.max.DbgPrint("  bbox max");
}

void DAABBNode::AddPoly(DAABBPolyInfo *pi)
// Add a polygon to the node list.
// The leaves will end up with 1 polygon.
{
  //DAABBPolyInfo *next;
//QDBG("DAABBPoly::AddPoly(%p); this=%p\n",pi,this);
  // Hang the polygon in the root node.
  // Things will be subdivided later in BuildTree() once all polygons
  // are collected.
  // Do a head insert
  pi->next=polyList;
  polyList=pi;
//QDBG("AddPoly RET\n");
}

void DAABBNode::BuildTree()
// Build the node and its subtrees recursively.
// Splits up the polygon list if more than 1 polygon is present
// until only leaves with 1 polygon are left.
{
  dfloat d;
  int    i,axis;
  DAABBPolyInfo *pi,*nextPI;
  DVector3 center;
#ifdef KEEP_DEPTH
  static int dep;
#endif
#ifdef COUNT_LEFT_RIGHT
  int    countLeft,countRight;
#endif

#ifdef KEEP_DEPTH
  dep++;
QDBG("BuildChildTree(); dep=%d\n",dep);
//if(dep>5)return;
#else
QDBG("BuildChildTree()\n");
#endif
//if(dep>8)return;

  // Calculate the tighest bounding box around the polygon list
  CalcBoundingBox();

  // Is it a leaf? (just 1 polygon)
  if(polyList==0)
  {
QDBG("Empty leaf; no polygons\n");
#ifdef KEEP_DEPTH
    dep--;
#endif
    return;
  } else if(polyList->next==0)
  {
QDBG("Leaf with 1 polygon\n");
    // We're done
#ifdef KEEP_DEPTH
    dep--;
#endif
    return;
  }

  // Calculate 2 childs to split the polygons into
  child[0]=new DAABBNode(this);
  child[1]=new DAABBNode(this);

  // Child spaces are almost exactly like the current space
  child[0]->aabbSpace=aabbSpace;
  child[1]->aabbSpace=aabbSpace;

  // Calculate a coordinate to split the polygon list in 2
  // First decide on which axis to split; take the side (X or Y) that is
  // the longest to create square-like subnodes.
  //if(fabs(aabbSpace.max.x-aabbSpace.min.x)>fabs(maxSpace.y-minSpace.y))
  if(aabbSpace.GetWidth()>aabbSpace.GetHeight())
  {
    //if(fabs(maxSpace.x-minSpace.x)>fabs(maxSpace.z-minSpace.z))
    if(aabbSpace.GetWidth()>aabbSpace.GetDepth())
    {
      // X range is biggest
      axis=0;
      d=(aabbSpace.max.x+aabbSpace.min.x)/2;
      // Set the halfspace sizes of the children
      child[0]->aabbSpace.max.x=d;
      child[1]->aabbSpace.min.x=d;
    } else
    {
     do_z:
      // Z range is biggest
      axis=2;
      d=(aabbSpace.max.z+aabbSpace.min.z)/2;
      // Set the halfspace sizes of the children
      child[0]->aabbSpace.max.z=d;
      child[1]->aabbSpace.min.z=d;
    }
  //} else if(fabs(maxSpace.y-minSpace.y)>fabs(maxSpace.z-minSpace.z))
  } else if(aabbSpace.GetHeight()>aabbSpace.GetDepth())
  {
    // Y range is bigger
    axis=1;
    d=(aabbSpace.max.y+aabbSpace.min.y)/2;
    // Set the halfspace sizes of the children
    child[0]->aabbSpace.max.y=d;
    child[1]->aabbSpace.min.y=d;
  } else goto do_z;

QDBG("axis=%d, d=%f\n",axis,d);

  // Split polygons into 2 childs with the appropriate halfspace
#ifdef COUNT_LEFT_RIGHT
  countLeft=countRight=0;
#endif
  for(pi=polyList;pi;pi=nextPI)
  {
//QDBG("Split poly %p\n",pi);
    // Remember next polygon before it is hung under a different child
    nextPI=pi->next;
//QDBG("  nextPI=%p\n",nextPI);
    pi->GetCenter(&center);
    if(axis==0)
    {
      if(center.x<d)
      { child[0]->AddPoly(pi);
#ifdef COUNT_LEFT_RIGHT
        countLeft++;
#endif
      } else
      { child[1]->AddPoly(pi);
#ifdef COUNT_LEFT_RIGHT
        countRight++;
#endif
      }
    } else
    { // Y axis split
      if(center.y<d)
      { child[0]->AddPoly(pi);
#ifdef COUNT_LEFT_RIGHT
        countLeft++;
#endif
      } else
      { child[1]->AddPoly(pi);
#ifdef COUNT_LEFT_RIGHT
        countRight++;
#endif
      }
    }
  }
#ifdef COUNT_LEFT_RIGHT
QDBG("  into %d left, %d right.\n",countLeft,countRight);
#endif

  if(child[0]->polyList==0||child[1]->polyList==0)
  {
    int ci,i,n;

QDBG("Dead end; split forced\n");
    // Didn't split anything; force a split.
    // Note that the polygons have already been dealt to the fat child.
    // Find out how many polygons there were, and which child to add to
    if(countLeft){ n=countLeft; ci=1; }
    else         { n=countRight; ci=0; }
    // Deal out polygons left & right
QDBG("  %d polygons found, %d to be hung in child %d\n",n,n/2,ci);
    for(i=0,pi=child[ci^1]->polyList;i<(n-1)/2;pi=pi->next,i++);
    // Split poly list into two
//qdbg("  child[ci]=%p\n",child[ci]);
    child[ci]->polyList=pi->next;
//qdbg("  pi=%p\n",pi);
    pi->next=0;

#ifdef OBS
qdbg("child0:\n");
for(pi=child[0]->polyList;pi;pi=pi->next)qdbg("  pi %p\n",pi);
qdbg("child1:\n");
for(pi=child[1]->polyList;pi;pi=pi->next)qdbg("  pi %p\n",pi);
#endif

    // Keep the subtree space the same however as the current node (!)
    // May be optimized later to really be the bounding box of the split
    // geometry.
    // Perhaps even better would be to split along a different 'd'.
    child[0]->aabbSpace=aabbSpace;
    child[1]->aabbSpace=aabbSpace;
  }

#ifdef VERBOSE
  child[0]->aabbSpace.DbgPrint("child0 Space");
  child[1]->aabbSpace.DbgPrint("child1 Space");
#endif

  // Check for recursively building the subtrees (if the child has members)
  for(i=0;i<2;i++)
  {
    if(!child[i]->polyList)
    {
      // No members there
QDBG("Child%d empty\n",i);
      QDELETE(child[i]);
    }
  }
  // All polys divided up
  polyList=0;

  // Build children (hmm, tail recursion can be iterated probably)
  if(child[0])child[0]->BuildTree();
  if(child[1])child[1]->BuildTree();

#ifdef KEEP_DEPTH
  dep--;
#endif
QDBG("BuildChildTree() RET\n");
}

/***************
* Node queries *
***************/
bool DAABBNode::Query(DAABB *otherAABB,ABtreeCallback cbfunc)
// Checks which nodes overlap with 'aabb' and calls cbfunc() for
// any potentially intersecting polygons.
// Returns FALSE if no more querying is desired (probably when
// the callback function says its found a good match).
// Returns TRUE when all nodes have been processed (this does not
// say ANYTHING though whether intersecting nodes were found).
{
//qdbg("DAABBNode:Query(AABB,cb); this=%p\n",this);
//aabb.DbgPrint("this.aabb");
//otherAABB->DbgPrint("otherAABB");
  // Check if bounding boxes overlap. If not, no collisions
  if(!aabb.Overlaps(otherAABB))
  {
//qdbg("  No overlap\n");
    return TRUE;
  }

  // If this is a leaf, we have a potential hit/intersection
  if(IsLeaf())
  {
//qdbg("Leaf hit!\n");
    // Call callback function; if it returns FALSE, stop searching
    if(cbfunc)
      if(!cbfunc(polyList))
        return FALSE;
  } else
  {
    // Continue searching the children
    if(child[0])
      if(!child[0]->Query(otherAABB,cbfunc))
        return FALSE;
    if(child[1])
      if(!child[1]->Query(otherAABB,cbfunc))
        return FALSE;
  }
  return TRUE;
}
DAABBNode *DAABBNode::FindEnclosingNode(DAABB *extAABB)
// Starting from the current node, finds the tightest enclosing
// node that still fully contains 'aabb'.
// This is useful for example for temporal coherence in Racer's track tree
// collisions. The car is always on a small part of the track. This can
// be represented by starting collision searches somewhere deep into the
// tree already (to avoid walking mostly the same path at each step).
// Starting out from the last step's enclosing node, we can walk up or down
// the tree just a little (is expected to be at most 2 steps or so with
// small steps), and come up with the new enclosing node very efficiently.
// The enclosing node will then be used as a starting point for queries,
// instead of starting at the root.
// This function never returns 0, but in case of the 'aabb' not being contained
// in the root, it will still return the root to simplify handling in
// the caller.
{
  return this;

#ifdef FUTURE
  DAABBNode *node=this;

 check_node:

qdbg("FindEnclosingNode(); node=%p\n",node);
node->aabb.DbgPrint("node AABB");
  // Check if our 'aabb' encloses the external AABB
  if(node->aabb.Contains(extAABB))
  {
   do_node:
//qdbg("Fits; try kids; node=%p\n",node);
//qdbg("  node=%p, child=%p/%p\n",node,node->child[0],node->child[1]);
    // Check if we can't move on to one of the children
    if(node->child[0]!=0&&node->child[0]->aabb.Contains(extAABB))
    {
      node=node->child[0];
      goto do_node;
    }
    if(node->child[1]!=0&&node->child[1]->aabb.Contains(extAABB))
    {
      node=node->child[1];
      goto do_node;
    }
//qdbg("no child\n");
  } else
  {
//qdbg("No fit anymore; try parent\n");
    // Move up the tree; the current node doesn't fit anymore
    if(node->parent)
    {
      node=node->parent;
      // Check 
      goto check_node;
    } else
    {
      // Can't go higher; just return the highest node
    }
  }
  return node;
#endif
}

void DAABBNode::DbgPrint(int dep)
{
  int count;
  DAABBPolyInfo *pi;

  count=0;
  for(pi=polyList;pi;pi=pi->next,count++);

  qdbg("%*s",dep," ");
  qdbg("Node (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f); %d polys\n",
    aabbSpace.min.x,aabbSpace.min.y,aabbSpace.min.z,
    aabbSpace.max.x,aabbSpace.max.y,aabbSpace.max.z,count);

  if(child[0])child[0]->DbgPrint(dep+1);
  if(child[1])child[1]->DbgPrint(dep+1);
}

/********
* Trees *
********/
DAABBTree::DAABBTree()
{
  root=new DAABBNode(0);
}
DAABBTree::~DAABBTree()
{
qdbg("DAABBTree:dtor, this=%p\n",this);
  if(root)
    FreeTree(root);
}
 
void DAABBTree::FreeTree(DAABBNode *node)
{
  // Well, eh, free it in the future
}

/***********
* Building *
***********/
void DAABBTree::AddPoly(DAABBPolyInfo *pi)
// Add a polygon to the build list
{
  //DAABBPolyInfo *next;
//QDBG("DAABBTree:AddPoly(%p)\n",pi);
  // Hang the polygon in the root node.
  // Things will be subdivided later in BuildTree() once all polygons
  // are collected.
  // Do a head insert
//QDBG("root=%p, root->polyList=%p\n",root,root->polyList);
  pi->next=root->polyList;
  root->polyList=pi;
#ifdef OBS
QDBG("pi->next=%p\n",pi->next);
QDBG("pi=%p, root->polyList=%p\n",pi,root->polyList);

QDBG("List of polys:\n");
  for(pi=root->polyList;pi;pi=pi->next)
    QDBG("PolyInfo %p\n",pi);
#endif
}
void DAABBTree::AddGeob(DGeob *geob)
// Add all tris in the geob
{
  int i,j,k,n;
#ifdef COPY_VERTICES
  float v[3][3];
#else
  float *v[3];
#endif
  DAABBPolyInfo *pi;

  for(i=0;i<geob->indices/3;i++)
  {
#ifdef COPY_VERTICES
    for(k=0;k<3;k++)
    {
      n=geob->index[i*3+k];
      for(j=0;j<3;j++)
        v[k][j]=geob->vertex[n*3+j];
    }
    pi=new DAABBPolyInfo(v[0],v[1],v[2]);
    AddPoly(pi);
#else
    for(k=0;k<3;k++)
    {
      n=geob->index[i*3+k];
      v[k]=&geob->vertex[n*3];
    }
    pi=new DAABBPolyInfo(v[0],v[1],v[2]);
    AddPoly(pi);
#endif
  }
}
void DAABBTree::AddGeode(DGeode *geode)
{
  int i;
  for(i=0;i<geode->geobs;i++)
    AddGeob(geode->geob[i]);
}

void DAABBTree::BuildTree()
// Create a tree, top-down, subdiving the polygon list at every level
{
qdbg("DAABBTree:BuildTree()\n");
QDBG("DAABBTree:BuildTree()\n");
#ifdef OBS
QDBG("List of polys:\n");
  DAABBPolyInfo *pi,*nextPI;
  for(pi=root->polyList;pi;pi=pi->next)
    QDBG("PolyInfo %p\n",pi);
#endif
    
  // Create initial space box
  root->CalcBoundingBox();
  root->aabbSpace=root->aabb;
  root->BuildTree();
qdbg("DAABBTree:BuildTree() DONE\n");

#ifdef VERBOSE
  DbgPrint();
#endif
}

/******
* I/O *
******/
bool DAABBTree::Save(cstring fname)
{
QDBG("DAABBTree:Save(%s)\n",fname);
  return TRUE;
}

void DAABBTree::DbgPrint()
{
  if(root)
    root->DbgPrint(0);
}

/********
* Query *
********/
void DAABBTree::Query(DAABB *aabb,ABtreeCallback cbfunc)
// Runs the AABB through the tree, and calls 'cbfunc' with any polygon
// lists (but the list should be only 1 polygon!) that intersect the AABB.
// 'cbfunc's prototype is: bool cbfunc(DAABBPolyInfo *plist);
// If 'cbfunc' returns FALSE, no further tests are done.
{
  if(root)
    root->Query(aabb,cbfunc);
}

#ifdef FUTURE
void DAABBTree::FindIntersection(DVector3 *ray)
// Shoots a 2D ray into the AABB tree.
{
}
#endif
