/*
 * QVector3 - a 3D vector class
 * 28-08-00: Created! (based on prof. Kenneth I. Joy's Vector.C)
 * (C) MarketGraph/Ruud van Gaal
 */

#include <qlib/vector.h>
#include <math.h>
#include <qlib/debug.h>
DEBUG_ENABLE

//
// Constructors
//
QVector3::QVector3()
// Empty constructor gives 0 vector
{
  x=y=z=0;
}
QVector3::QVector3(const double _x,const double _y,const double _z)
{
  x=_x;
  y=_y;
  z=_z;
}
QVector3::QVector3(const QVector3& b)
// Construct based on other QVector3
{
  x=b.GetX();
  y=b.GetY();
  z=b.GetZ();
}

double QVector3::Length() const
{
  double len;
  len=sqrt(x*x+y*y+z*z);
  return len;
}

void QVector3::SetToZero()
{
  x=y=z=0.0;
}

/************
* Operators *
************/
QVector3 operator*(const QVector3 &a,const double &s)
{
  QVector3 v;
  v.x=s*a.x;
  v.y=s*a.y;
  v.z=s*a.z;
  return v;
}
QVector3 operator*(const double &s,const QVector3 &a)
{
  QVector3 v;
  v.x=s*a.x;
  v.y=s*a.y;
  v.z=s*a.z;
  return v;
}
QVector3 operator+(const QVector3 &a,const QVector3 &b)
{
  QVector3 v;
  v.x=a.x+b.x;
  v.y=a.y+b.y;
  v.z=a.z+b.z;
  return v;
}

/*************
* Assignment *
*************/
QVector3& QVector3::operator=(const QVector3& v)
{
  // Avoid assigning to one's self
  if(this==&v)return *this;

  x=v.x;
  y=v.y;
  z=v.z;
  return *this;
}
QVector3& QVector3::operator=(const double& v)
{
qwarn("QVector3 assign to double is undefined\n");
  return *this;
}

/*********
* Output *
*********/
ostream& operator<< ( ostream& co, const QVector3& v )
{
  co << "< "
     << v.x 
     << ", "
     << v.y 
     << ", "
     << v.z 
     << " >" ;
  return co ;
}

/*************
* Comparison *
*************/
int operator== ( const QVector3& v1, const QVector3& v2 ) 
// Very dubious, because of floating point arithmetic
{
  if((v1.x==v2.x) &&
     (v1.y==v2.y) &&
     (v1.z==v2.z))
    return TRUE;
  else
    return FALSE;
}

int operator!= ( const QVector3& v1, const QVector3& v2 ) 
{
  if((v1.x!=v2.x) ||
     (v1.y!=v2.y) ||
     (v1.z!=v2.z))
    return TRUE;
  else
    return FALSE;
}

/*************
* Arithmetic *
*************/
QVector3 operator-(const QVector3& v1,const QVector3& v2)
{
  QVector3 vv ;

  vv.x = v1.x - v2.x;
  vv.y = v1.y - v2.y;
  vv.z = v1.z - v2.z;
  return vv;
}

QVector3 operator-(const QVector3& v)
{
  QVector3 vv ;
  vv.x=-v.x;
  vv.y=-v.y;
  vv.z=-v.z;
  return vv;
}

QVector3 operator/(const QVector3& v,const double& c)
{
  QVector3 vv;
  vv.x=v.x/c;
  vv.y=v.y/c;
  vv.z=v.z/c;
  return vv;
}

/***********************
* Immediate arithmetic *
***********************/
QVector3& QVector3 :: operator+=(const QVector3& v)
{
  x+=v.x;
  y+=v.y;
  z+=v.z;
  return *this;
}

QVector3& QVector3::operator-=(const QVector3& v)
{
  x-=v.x;
  y-=v.y;
  z-=v.z;
  return *this;
}

QVector3& QVector3::operator*=(const double &s)
{
  x*=s;
  y*=s;
  z*=s;
  return *this;
}

QVector3& QVector3::operator/=(const double& c)
{
  x/=c;
  y/=c;
  z/=c;
  return *this;
}

/************
* Normalize *
************/
void QVector3::Normalize()
// Make the vector length 1
{
  double l=Length();
  // Avoid normalize zero vector
  if(l==0.0)return;
  
  x=x/l;
  y=y/l;
  z=z/l;
}

/**************
* Dot product *
**************/
double QVector3::Dot(const QVector3 &v1) 
// Returns dot product with another vector
{
  return v1.x*x+v1.y*y+v1.z*z;
}
double QVector3::DotSelf()
// Returns dot product of vector with itself
// Could also just call v.Dot(v), but this is perhaps nicer
{
  return x*x+y*y+z*z;
}

/****************
* Cross product *
****************/
QVector3 operator*(const QVector3& v1,const QVector3& v2)
{
  QVector3 vv;
  vv.x=v1.y*v2.z-v1.z*v2.y;
  vv.y=-v1.x*v2.z+v1.z*v2.x;
  vv.z=v1.x*v2.y-v1.y*v2.x;
  return vv;
}

#ifdef OBS_PERHAPS_FUTURE_STUFF
  
//
// Operators
//
QVector3& QVector3::operator=(const char* s)
{
//qdbg("QVector3 op=const\n");
//qdbg("  s='%s', p=%p, refs=%d\n",s,p,p->refs);
  if(p->refs>1)
  { // Detach our buffer; contents are being modified
    p->refs--;
    p=new srep;
  } else
  { // Delete old string buffer
//qdbg("  delete[] old p->s=%p (p=%p)\n",p->s,p);
    p->Free();
    //delete[] p->s;
  }
  // Copy in new string
  p->Resize(strlen(s)+1);
  //p->s=new char[strlen(s)+1];
//qdbg("  p->s=%p, len=%d (%s)\n",p->s,strlen(s)+1,p->s);
  strcpy(p->s,s);
  return *this;
}
QVector3& QVector3::operator=(const QVector3& x)
{
  x.p->refs++;			// Protect against 's==s'
  if(--p->refs==0)
  { // Remove our own rep
    //delete[] p->s;
    p->Free();
    delete p;
  }
  p=x.p;			// Take over new string's rep
  return *this;
}

char& QVector3::operator[](int i)
{
  if(i<0||i>=strlen(p->s))
  { qwarn("QVector3(%s): ([]) index %d out of range (0..%d)",
      p->s,i,strlen(p->s));
    i=strlen(p->s);		// Return the 0 (reference)
  }
  if(p->refs>1)
  { // Detach to avoid 2 string rep's mixing
    srep* np=new srep;
    //np->s=new char[strlen(p->s)+1];
    np->Resize(strlen(p->s)+1);
    strcpy(np->s,p->s);
    p->refs--;
    p=np;
  }
  return p->s[i];
}

const char& QVector3::operator[](int i) const
// Subscripting const strings
{
  if(i<0||i>=strlen(p->s))
  { qwarn("QVector3(%s): (const[]) index %d out of range (0..%d)",
      p->s,i,strlen(p->s));
    i=strlen(p->s);		// Return the 0 (reference)
  }
  return p->s[i];
}

//
// Type conversions
//
QVector3::operator const char *() const
{
  return p->s;
}
#ifdef ND_NO_MODIFIABLE_STRING_PLEASE
// This typecast cut out on 27-8-99, because it delivered too many
// late-captured problems
QVector3::operator char *() // const
{
  // Check refs
  if(p->refs>1)
  { // Detach; the returned pointer may be used to modify the buffer!
qdbg("QVector3 op char* detach\n");
    srep* np=new srep;
    //np->s=new char[strlen(p->s)+1];
    np->Resize(strlen(p->s)+1);
    strcpy(np->s,p->s);
    p->refs--;
    p=np;
  }
  return p->s;
}
#endif

/************
* COMPARIONS *
*************/
bool QVector3::operator==(const char *s)
{
  return (strcmp(p->s,s)==0);
}

// Information
bool QVector3::IsEmpty()
{
  if(*p->s)return FALSE;
  return TRUE;
}

#endif
