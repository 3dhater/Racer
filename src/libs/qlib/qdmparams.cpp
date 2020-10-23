/*
 * QDMParams - encapsulate DMparams type
 * 08-09-98: Created!
 * (C) MG/RVG
 */

#include <qlib/dmparams.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DME(f,s) if((f)!=DM_SUCCESS)qerr(s)

QDMParams::QDMParams()
{
  p=0;
#ifndef WIN32
  DME(dmParamsCreate(&p),"QDMParams ctor; can't allocate DMparams");
#endif
}

QDMParams::~QDMParams()
{
#ifndef WIN32
  if(p)dmParamsDestroy(p);
#endif
}

/******
* GET *
******/
int QDMParams::GetInt(const char *name)
{
#ifdef WIN32
  return 0;
#else
  return dmParamsGetInt(p,name);
#endif
}
int QDMParams::GetEnum(const char *name)
{
#ifdef WIN32
  return 0;
#else
  return dmParamsGetEnum(p,name);
#endif
}
const char *QDMParams::GetString(const char *name)
{
#ifdef WIN32
  return 0;
#else
  return dmParamsGetString(p,name);
#endif
}
double QDMParams::GetFloat(const char *name)
{
#ifdef WIN32
  return 0;
#else
  return dmParamsGetFloat(p,name);
#endif
}

DMfraction QDMParams::GetFract(const char *name)
{
#ifdef WIN32
  return 0;
#else
  return dmParamsGetFract(p,name);
#endif
}

/******
* SET *
******/
void QDMParams::SetImageDefaults(int wid,int hgt,DMimagepacking packing)
{ 
#ifndef WIN32
  DME(dmSetImageDefaults(p,wid,hgt,packing),"QDMParams::SetImageDefaults");
#endif
}
void QDMParams::SetAudioDefaults(int bits,int freq,int channels)
{
#ifndef WIN32
  DME(dmSetAudioDefaults(p,bits,freq,channels),"QDMParams::SetAudioDefaults");
#endif
}

void QDMParams::SetInt(const char *name,int n)
{
#ifndef WIN32
  dmParamsSetInt(p,name,n);
#endif
}
void QDMParams::SetFloat(const char *name,double v)
{
#ifndef WIN32
  dmParamsSetFloat(p,name,v);
#endif
}
void QDMParams::SetEnum(const char *name,int n)
{
#ifndef WIN32
  dmParamsSetEnum(p,name,n);
#endif
}
void QDMParams::SetString(const char *name,const char *v)
{
#ifndef WIN32
  dmParamsSetString(p,name,v);
#endif
}

/*******
* DEBUG *
********/
static void DbgPrintParams(const DMparams *p,int indent=0)
{
#ifndef WIN32
    int len = dmParamsGetNumElems( p );
    int i;
    int j;
    
    for ( i = 0;  i < len;  i++ ) {
        const char* name = dmParamsGetElem    ( p, i );
        DMparamtype type = dmParamsGetElemType( p, i );
        
        for ( j = 0;  j < indent;  j++ ) {
            printf( " " );
        }

        printf( "%8s: ", name );
        switch( type ) 
            {
            case DM_TYPE_ENUM:
                printf( "%d", dmParamsGetEnum( p, name ) );
                break;
            case DM_TYPE_INT:
                printf( "%d", dmParamsGetInt( p, name ) );
                break;
            case DM_TYPE_STRING:
                printf( "%s", dmParamsGetString( p, name ) );
                break;
            case DM_TYPE_FLOAT:
                printf( "%f", dmParamsGetFloat( p, name ) );
                break;
            case DM_TYPE_FRACTION:
                {
                    DMfraction f;
                    f = dmParamsGetFract( p, name );
                    printf( "%d/%d", f.numerator, f.denominator );
                }
                break;
            case DM_TYPE_PARAMS:
                DbgPrintParams( dmParamsGetParams( p, name ), indent + 4 );
                break;
            case DM_TYPE_ENUM_ARRAY:
                {
                    int i;
                    const DMenumarray* array = dmParamsGetEnumArray( p, name );
                    for ( i = 0;  i < array->elemCount;  i++ ) {
                        printf( "%d ", array->elems[i] );
                    }
                }
                break;
            case DM_TYPE_INT_ARRAY:
                {
                    int i;
                    const DMintarray* array = dmParamsGetIntArray( p, name );
                    for ( i = 0;  i < array->elemCount;  i++ ) {
                        printf( "%d ", array->elems[i] );
                    }
                }
                break;
            case DM_TYPE_STRING_ARRAY:
                {
                    int i;
                    const DMstringarray* array = 
                        dmParamsGetStringArray( p, name );
                    for ( i = 0;  i < array->elemCount;  i++ ) {
                        printf( "%s ", array->elems[i] );
                    }
                }
                break;
            case DM_TYPE_FLOAT_ARRAY:
                {
                    int i;
                    const DMfloatarray* array = 
                        dmParamsGetFloatArray( p, name );
                    for ( i = 0;  i < array->elemCount;  i++ ) {
                        printf( "%f ", array->elems[i] );
                    }
                }
                break;
            case DM_TYPE_FRACTION_ARRAY:
                {
                    int i;
                    const DMfractionarray* array = 
                        dmParamsGetFractArray( p, name );
                    for ( i = 0;  i < array->elemCount;  i++ ) {
                        printf( "%d/%d ", 
                                array->elems[i].numerator,
                                array->elems[i].denominator );
                    }
                }
                break;
            case DM_TYPE_INT_RANGE:
                {
                    const DMintrange* range = dmParamsGetIntRange( p, name );
                    printf( "%d ... %d", range->low, range->high );
                }
                break;
            case DM_TYPE_FLOAT_RANGE:
                {
                    const DMfloatrange* range = 
                        dmParamsGetFloatRange( p, name );
                    printf( "%f ... %f", range->low, range->high );
                }
                break;
            case DM_TYPE_FRACTION_RANGE:
                {
                    const DMfractionrange* range = 
                        dmParamsGetFractRange( p, name );
                    printf( "%d/%d ... %d/%d", 
                            range->low.numerator, 
                            range->low.denominator, 
                            range->high.numerator,
                            range->high.denominator );
                }
                break;
            defualt:
                printf( "UNKNOWN TYPE" );
            }
        printf( "\n" );
    }
  printf("---\n");
#endif
}

void QDMParams::DbgPrint()
{
  DbgPrintParams(p);
}
