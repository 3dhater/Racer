// qlib/vector.h

#ifndef __QLIB_VECTOR_H
#define __QLIB_VECTOR_H

#include <qlib/types.h>
#include <iostream.h>

class QVector3
// 3D vector
{
 public:
  // Public direct access to members
  double x,y,z;

 public:
  // Constructors
  QVector3();
  QVector3(const double x,const double y=0.0,const double z=0.0);
  QVector3(const QVector3& b);

  void Set(const double x,const double y,const double z);
  // Info
  double GetX() const { return x; }
  double GetY() const { return y; }
  double GetZ() const { return z; }
  void   SetX(double _x){ x=_x; }
  void   SetY(double _y){ y=_y; }
  void   SetZ(double _z){ z=_z; }

  // Functions
  double Length() const;
  void   Normalize();
  double Dot(const QVector3 &v);
  double DotSelf();
  void   SetToZero();

  // Operators

  // Assignment
  QVector3& operator=(const QVector3 &v);
  QVector3& operator=(const double &v);
  // Output
  friend ostream& operator<<(ostream&,const QVector3 &v);
  // Comparison
  friend int operator==(const QVector3 &a,const QVector3 &b);
  friend int operator!=(const QVector3 &a,const QVector3 &b);
  // Sum, difference, product
  friend QVector3 operator+(const QVector3 &a,const QVector3 &b);
  friend QVector3 operator-(const QVector3 &a,const QVector3 &b);
  friend QVector3 operator-(const QVector3 &a);	// Unary minus
  friend QVector3 operator*(const QVector3& v,const double &s);
  friend QVector3 operator*(const double &s,const QVector3& v);
  friend QVector3 operator/(const QVector3& v,const double &s);
  // Immediate
  QVector3& operator+=(const QVector3& v);
  QVector3& operator-=(const QVector3& v);
  QVector3& operator*=(const double &s);
  QVector3& operator/=(const double &s);
  //friend QVector3& operator*=(QVector3& v,const double &s);
  // Cross product
  friend QVector3 operator*(const QVector3 &a,const QVector3 &b);
};

#endif
