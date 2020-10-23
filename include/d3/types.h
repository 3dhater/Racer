// d3/types.h - common D3 types

#ifndef __D3_TYPES_H
#define __D3_TYPES_H

#include <qlib/types.h>
#include <iostream.h>
#include <math.h>
#include <stdio.h>

typedef float dfloat;

// General epsilon for noticing values close to 0
#define D3_EPSILON   (0.00001f)

// Generic constants
#ifndef PI
#define PI 3.14159265358979
#endif

//
// Basic mathematical classes for use in 3D applications
//

/***********
* DVECTOR2 *
***********/
class DVector2
// 2D vector
{
 public:
  // Public direct access to members
  dfloat x,y;

 public:
  // Constructors
  DVector2()
  { // Members are NOT initialized to 0 (!)
  }
  DVector2(dfloat _x,dfloat _y)
    : x(_x), y(_y)
  { }
  DVector2(const dfloat *v) : x(v[0]), y(v[1])
  // Construct from an array of floats
  { }
  DVector2(const DVector2& v)
  // Copy constructor
  {
    x=v.GetX();
    y=v.GetY();
  }

  void Set(dfloat _x,dfloat _y)
  { x=_x; y=_y; }
  void Set(DVector2 *v)
  { x=v->x; y=v->y; }

  // Attributes
  dfloat GetX() const { return x; }
  dfloat GetY() const { return y; }
  void   SetX(dfloat _x){ x=_x; }
  void   SetY(dfloat _y){ y=_y; }
#ifdef ND_FUTURE
  // Access like an array
  dfloat& operator[](int i) const { return ((dfloat*)this)[i]; }
  operator dfloat* () { return (dfloat*)this; }
#endif

  // Functions
  dfloat Length() const
  {
    return sqrt(x*x+y*y);
  }
  dfloat LengthSquared() const
  {
    return x*x+y*y;
  }
  dfloat DistanceTo(const DVector2 *v) const
  // Returns distance to another point
  {
    dfloat dx=v->x-x,dy=v->y-y;
    return sqrt(dx*dx+dy*dy);
  }
  dfloat SquaredDistanceTo(const DVector2 *v) const
  // Returns squared distance to another point (faster than DistanceTo)
  {
    dfloat dx=v->x-x,dy=v->y-y;
    return dx*dx+dy*dy;
  }
  void   Normalize()
  {
    dfloat l=Length();
    if(l<D3_EPSILON)
    { // Zero-length vector
      x=y=0.0f;
    } else
    {
      dfloat scale=1.0f/l;
      x*=scale;
      y*=scale;
    }
  }
  void   NormalizeTo(DVector2 *v) const
  // Normalize into another vector
  {
    dfloat l=Length();
    if(l<D3_EPSILON)
    { // Zero-length vector
      v->x=v->y=0.0f;
    } else
    {
      dfloat scale=1.0f/l;
      v->x=x*scale;
      v->y=y*scale;
    }
  }
  dfloat Dot(const DVector2 &v) const
  {
    return x*v.x+y*v.y;
  }
  dfloat Dot(const DVector2 *v) const
  {
    return x*v->x+y*v->y;
  }
  dfloat DotSelf() const { return x*x+y*y; }
  void   SetToZero(){ x=y=0.0f; }
  void   Clear(){ x=y=0.0f; }
  void   Negate(){ x=-x; y=-y; }
  // Functional operations (faster than operator stack functions)
  void Add(const DVector2 *v)
  { x+=v->x;
    y+=v->y;
  }
  void Subtract(const DVector2 *v)
  { x-=v->x;
    y-=v->y;
  }
  void Scale(dfloat s)
  { x*=s;
    y*=s;
  }
  void Cross(const DVector2 *v1,const DVector2 *v2)
  // 'this' = crossproduct(v1,v2)
  // UNTESTED! Just-a-hunch function.
  {
    x=-v1->y;
    y=v1->x;
  }

  // Operators

  // Assignment
  DVector2& operator=(const DVector2 &v)
  { x=v.x; y=v.y; return *this; }
  DVector2& operator=(const dfloat &v);
  //{ x=v[0]; y=v[1]; z=v[2]; }

#ifdef FUTURE
  // Output
  friend ostream& operator<<(ostream&,const DVector3 &v);
  // Comparison
  friend int operator==(const DVector3 &a,const DVector3 &b);
  friend int operator!=(const DVector3 &a,const DVector3 &b);
  // Sum, difference, product
  //friend DVector3 operator+(const DVector3 &a,const DVector3 &b);
  //friend DVector3 operator-(const DVector3 &a,const DVector3 &b);
  //friend DVector3 operator-(const DVector3 &a);	// Unary minus
  //friend DVector3 operator*(const DVector3& v,const dfloat &s);
  //friend DVector3 operator*(const dfloat &s,const DVector3& v);
  friend DVector3 operator/(const DVector3& v,const dfloat &s);
  // Immediate
  DVector3& operator+=(const DVector3& v);
  DVector3& operator-=(const DVector3& v);
  DVector3& operator*=(const dfloat &s);
  DVector3& operator/=(const dfloat &s);
  //friend DVector3& operator*=(DVector3& v,const dfloat &s);
  // Cross product
  friend DVector3 operator*(const DVector3 &a,const DVector3 &b);
#endif

  // Debugging
  void DbgPrint(cstring s);
};

/***********
* DVECTOR3 *
***********/
class DVector3
// 3D vector
{
 public:
  // Public direct access to members
  dfloat x,y,z;

 public:
  // Constructors
  DVector3()
  { // Members are NOT initialized to 0 (!)
  }
  DVector3(dfloat _x,dfloat _y=0.0,dfloat _z=0.0)
    : x(_x), y(_y), z(_z)
  { }
  DVector3(const dfloat *v) : x(v[0]), y(v[1]), z(v[2])
  // Construct from an array of floats
  { }
  DVector3(const DVector3& v)
  // Copy constructor
  {
    x=v.GetX();
    y=v.GetY();
    z=v.GetZ();
  }

  void Set(dfloat _x,dfloat _y,dfloat _z)
  { x=_x; y=_y; z=_z; }
  void Set(DVector3 *v)
  { x=v->x; y=v->y; z=v->z; }

  // Attributes
  dfloat GetX() const { return x; }
  dfloat GetY() const { return y; }
  dfloat GetZ() const { return z; }
  void   SetX(dfloat _x){ x=_x; }
  void   SetY(dfloat _y){ y=_y; }
  void   SetZ(dfloat _z){ z=_z; }
#ifdef ND_FUTURE
  // Access like an array
  dfloat& operator[](int i) const { return ((dfloat*)this)[i]; }
  operator dfloat* () { return (dfloat*)this; }
#endif

  // Functions
  dfloat Length() const
  {
    return sqrt(x*x+y*y+z*z);
  }
  dfloat LengthSquared() const
  {
    return x*x+y*y+z*z;
  }
  dfloat DistanceTo(const DVector3 *v) const
  // Returns distance to another point
  {
    dfloat dx=v->x-x,dy=v->y-y,dz=v->z-z;
    return sqrt(dx*dx+dy*dy+dz*dz);
  }
  dfloat SquaredDistanceTo(const DVector3 *v) const
  // Returns squared distance to another point (faster than DistanceTo)
  {
    dfloat dx=v->x-x,dy=v->y-y,dz=v->z-z;
    return dx*dx+dy*dy+dz*dz;
  }
  void   Normalize()
  {
    dfloat l=Length();
    if(l<D3_EPSILON)
    { // Zero-length vector
      x=y=z=0.0f;
    } else
    {
      dfloat scale=1.0f/l;
      x*=scale;
      y*=scale;
      z*=scale;
    }
  }
  void   NormalizeTo(DVector3 *v) const
  // Normalize into another vector
  {
    dfloat l=Length();
    if(l<D3_EPSILON)
    { // Zero-length vector
      v->x=v->y=v->z=0.0f;
    } else
    {
      dfloat scale=1.0f/l;
      v->x=x*scale;
      v->y=y*scale;
      v->z=z*scale;
    }
  }
  dfloat Dot(const DVector3 &v) const
  {
    return x*v.x+y*v.y+z*v.z;
  }
  dfloat Dot(const DVector3 *v) const
  {
    return x*v->x+y*v->y+z*v->z;
  }
  dfloat DotSelf() const
  {
    return x*x+y*y+z*z;
  }
  void   SetToZero(){ x=y=z=0.0f; }
  void   Clear(){ x=y=z=0.0f; }         // Synonym for SetToZero()
  void   Negate()
  {
    x=-x; y=-y; z=-z;
  }
  // Functional operations (faster than operator stack functions)
  void Add(const DVector3 *v)
  { x+=v->x;
    y+=v->y;
    z+=v->z;
  }
  void Subtract(const DVector3 *v)
  { x-=v->x;
    y-=v->y;
    z-=v->z;
  }
  void Scale(dfloat s)
  { x*=s;
    y*=s;
    z*=s;
  }
  void Cross(const DVector3 *v1,const DVector3 *v2)
  // 'this' = crossproduct(v1,v2)
  {
    x=v1->y*v2->z-v1->z*v2->y;
    y=v1->z*v2->x-v1->x*v2->z;
    z=v1->x*v2->y-v1->y*v2->x;
  }

  // Operators

  // Assignment
  DVector3& operator=(const DVector3 &v)
  { x=v.x; y=v.y; z=v.z; return *this; }
  DVector3& operator=(const dfloat &v);
  //{ x=v[0]; y=v[1]; z=v[2]; }

  // Output
  friend ostream& operator<<(ostream&,const DVector3 &v);
  // Comparison
  friend int operator==(const DVector3 &a,const DVector3 &b);
  friend int operator!=(const DVector3 &a,const DVector3 &b);
  // Sum, difference, product
  //friend DVector3 operator+(const DVector3 &a,const DVector3 &b);
  //friend DVector3 operator-(const DVector3 &a,const DVector3 &b);
  //friend DVector3 operator-(const DVector3 &a);	// Unary minus
  //friend DVector3 operator*(const DVector3& v,const dfloat &s);
  //friend DVector3 operator*(const dfloat &s,const DVector3& v);
  friend DVector3 operator/(const DVector3& v,const dfloat &s);
  // Immediate
  DVector3& operator+=(const DVector3& v)
  { x+=v.x; y+=v.y; z+=v.z; return *this; }
  DVector3& operator-=(const DVector3& v);
  DVector3& operator*=(const dfloat &s);
  DVector3& operator/=(const dfloat &s);
  //friend DVector3& operator*=(DVector3& v,const dfloat &s);
  // Cross product
  friend DVector3 operator*(const DVector3 &a,const DVector3 &b);

  // Debugging
  void DbgPrint(cstring s);
};

inline DVector3 operator+(const DVector3 &u,const DVector3 &v)
{
  return DVector3(u.x+v.x,u.y+v.y,u.z+v.z);
}
inline DVector3 operator-(const DVector3 &u,const DVector3 &v)
{
//printf("DVector3:operator-()\n");
  return DVector3(u.x-v.x,u.y-v.y,u.z-v.z);
}
inline DVector3 operator*(const DVector3& v,dfloat const &s)
{
  return DVector3(s*v.x,s*v.y,s*v.z);
}
inline DVector3 operator*(dfloat const &s,const DVector3& v)
{
  return DVector3(s*v.x,s*v.y,s*v.z);
}

#ifdef ND_CANT_OVERLOAD_ON_MIPSPRO
// Dot product overloads the * operator
inline dfloat operator*(DVector3 const& u,DVector3 const& v)
{
  return u.x*v.x+u.y*v.y+u.z*v.z;
}
#endif

// Cross product overloads the ^ operator
// Use DVector3::Cross() preferably
inline DVector3 operator^(DVector3 const& u,DVector3 const& v)
{
  return DVector3
  ( (u.y*v.z-u.z*v.y), (u.z*v.x-u.x*v.z), (u.x*v.y-u.y*v.x) );
}

/*******
* DBOX *
*******/
class DBox
// A box, much like QRect, only in 3D
{public:
  DVector3 min,max;

  DBox();
  //DBox(dfloat x,dfloat y,dfloat z,dfloat wid=0,dfloat hgt=0,dfloat dep=0);
  DBox(DVector3 *min,DVector3 *max);
  DBox(DBox& box);
  DBox(DBox *box);
  ~DBox();

  // Information
  dfloat Volume();
  void   GetCenter(DVector3 *v);
  void   GetSize(DVector3 *v)
  { v->x=max.x-min.x;
    v->y=max.y-min.y;
    v->z=max.z-min.z;
  }
  dfloat GetRadius();

  // Boolean operations
  void Union(DBox *b);                         // Union with other box
  bool Contains(dfloat x,dfloat y,dfloat z);   // Point in rectangle?
#ifdef FUTURE
  // Boolean
  bool Overlaps(DBox *r);      // Do they touch?
  void Intersect(DBox *r);     // Intersection with other rect
#endif
  void DbgPrint(cstring s);
};

/**********
* DPLANE3 *
**********/
class DPlane3
// A 3-dimensional plane
// Parametric: ax+by+cz+d=0
// Stored as a normal and a distance; n and d
{
 public:
  enum Classification
  {
    FRONT,
    BACK,
    COPLANAR
  };
  DVector3 n;			// Normal of the plane (a,b,c)
  dfloat   d;			// Distance along normal to the origin

  // Ctor for ax+by+cz+d=0, note that (a,b,c) is the normal vector
  // and d is the distance along n to the origin
  DPlane3(dfloat a,dfloat b,dfloat c,dfloat _d) :
    n(a,b,c), d(_d)
  {
  }
  // Ctor for a normal and a distance.
  DPlane3(const DVector3& v,dfloat _d) :
    n(v), d(_d)
  {
  }
  // Ctor from 3 points in 3D
  DPlane3(const DVector3& p1,const DVector3& p2,const DVector3& p3);
  // Ctor from a normal and a point on the plane
  DPlane3(const DVector3& norm,const DVector3& v);
  // Empty ctor
  DPlane3(){}

  void Normalize();		// Make plane normal a unit vector
  dfloat DistanceTo(const DVector3 *v) const
  {
    return v->x*n.x+v->y*n.y+v->z*n.z+d;
  }
  int  Classify(const DVector3 *v) const;
  int  ClassifyFrontOrBack(const DVector3 *v) const;

  // Flip orientation
  void Flip();

  void DbgPrint(cstring s) const;
};

// DPlane3 inlines
inline DPlane3::DPlane3(const DVector3& u,const DVector3& v,const DVector3& w)
// Create plane from 3 points
// Right handed coordinate system
{
  // Left handed would be: n=(v-u)^(w-u)
  n=(u-w)^(v-w);
  n.Normalize();
  // Calculate d from n*p+d=0 (*=dot product, p=any point on plane)
  d=-n.Dot(u);
}
inline DPlane3::DPlane3(const DVector3& norm,const DVector3& v)
  : n(norm) , d(-(norm.Dot(v)))
{
}

inline void DPlane3::Normalize()
// Make the plane normal a unit vector
{
  dfloat t;
  t=1.0f/sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
  n.x*=t;
  n.y*=t;
  n.z*=t;
  d*=t;
}

#endif
