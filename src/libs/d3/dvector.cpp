/*
 * DVector3 - a 3D vector class
 * 28-08-00: Created! (based on prof. Kenneth I. Joy's Vector.C)
 * 11-12-00: Turned over to the D3 library; was QVector3.
 * NOTES:
 * - Most of the vector class is inlined for speed
 * (C) MarketGraph/Ruud van Gaal
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
DEBUG_ENABLE

/*************
* Assignment *
*************/
#ifdef OBS_INLINED
DVector3& DVector3::operator=(const DVector3& v)
{
  // Avoid assigning to one's self
  if(this==&v)return *this;

  x=v.x;
  y=v.y;
  z=v.z;
  return *this;
}
#endif
DVector3& DVector3::operator=(const dfloat& v)
{
qwarn("DVector3 assign to dfloat is undefined\n");
  return *this;
}

/*********
* Output *
*********/
ostream& operator<< ( ostream& co, const DVector3& v )
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
int operator== ( const DVector3& v1, const DVector3& v2 ) 
// Very dubious, because of floating point arithmetic
{
  if((v1.x==v2.x) &&
     (v1.y==v2.y) &&
     (v1.z==v2.z))
    return TRUE;
  else
    return FALSE;
}

int operator!= ( const DVector3& v1, const DVector3& v2 ) 
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
DVector3 operator-(const DVector3& v)
{
  DVector3 vv ;
  vv.x=-v.x;
  vv.y=-v.y;
  vv.z=-v.z;
  return vv;
}

DVector3 operator/(const DVector3& v,const dfloat& c)
{
  DVector3 vv;
  vv.x=v.x/c;
  vv.y=v.y/c;
  vv.z=v.z/c;
  return vv;
}

/***********************
* Immediate arithmetic *
***********************/
#ifdef OBS_INLINED
DVector3& DVector3::operator+=(const DVector3& v)
{
  x+=v.x;
  y+=v.y;
  z+=v.z;
  return *this;
}
#endif

DVector3& DVector3::operator-=(const DVector3& v)
{
  x-=v.x;
  y-=v.y;
  z-=v.z;
  return *this;
}

DVector3& DVector3::operator*=(const dfloat &s)
{
  x*=s;
  y*=s;
  z*=s;
  return *this;
}

DVector3& DVector3::operator/=(const dfloat& c)
{
  x/=c;
  y/=c;
  z/=c;
  return *this;
}

/****************
* Cross product *
****************/
DVector3 operator*(const DVector3& v1,const DVector3& v2)
{
  DVector3 vv;
  vv.x=v1.y*v2.z-v1.z*v2.y;
  vv.y=-v1.x*v2.z+v1.z*v2.x;
  vv.z=v1.x*v2.y-v1.y*v2.x;
  return vv;
}

/************
* Debugging *
************/
void DVector3::DbgPrint(cstring s)
{
  qdbg("( %.2f,%.2f,%.2f ) [%s]\n",x,y,z,s);
}

#ifdef OBS_PERHAPS_FUTURE_STUFF
  
//
// Operators
//
DVector3& DVector3::operator=(const char* s)
{
//qdbg("DVector3 op=const\n");
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
DVector3& DVector3::operator=(const DVector3& x)
{
  x.p->refs++;			// Protect against 's==s'
  if(--p->refs==0)
  { // Remove our own rep
    //delete[] p->s;
    p->Free();
    QDELETE(p);
  }
  p=x.p;			// Take over new string's rep
  return *this;
}

char& DVector3::operator[](int i)
{
  if(i<0||i>=strlen(p->s))
  { qwarn("DVector3(%s): ([]) index %d out of range (0..%d)",
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

const char& DVector3::operator[](int i) const
// Subscripting const strings
{
  if(i<0||i>=strlen(p->s))
  { qwarn("DVector3(%s): (const[]) index %d out of range (0..%d)",
      p->s,i,strlen(p->s));
    i=strlen(p->s);		// Return the 0 (reference)
  }
  return p->s[i];
}

//
// Type conversions
//
DVector3::operator const char *() const
{
  return p->s;
}
#ifdef ND_NO_MODIFIABLE_STRING_PLEASE
// This typecast cut out on 27-8-99, because it delivered too many
// late-captured problems
DVector3::operator char *() // const
{
  // Check refs
  if(p->refs>1)
  { // Detach; the returned pointer may be used to modify the buffer!
qdbg("DVector3 op char* detach\n");
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
bool DVector3::operator==(const char *s)
{
  return (strcmp(p->s,s)==0);
}

// Information
bool DVector3::IsEmpty()
{
  if(*p->s)return FALSE;
  return TRUE;
}

#endif
