/*
 * DVector2 - a 2D vector class
 * 07-11-01: Created!
 * NOTES:
 * - Most of the vector class is inlined for speed
 * (C) MarketGraph/Ruud van Gaal
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <math.h>
DEBUG_ENABLE

#ifdef FUTURE
/*********
* Output *
*********/
ostream& operator<< ( ostream& co, const DVector2& v )
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
#endif

/************
* Debugging *
************/
void DVector2::DbgPrint(cstring s)
{
  qdbg("( %.2f,%.2f ) [%s]\n",x,y,s);
}

