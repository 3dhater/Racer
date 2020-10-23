/* stub MG/RVG, 3-7-96 */
#ifndef __QLIB_DEBUG_H
#define __QLIB_DEBUG_H

#define DEBUG_ENABLE

void dprintf(char *fmt,...);

#include <alib/types.h>
#include <alib/callstak.h>
#include <alib/memorize.h>

#ifdef DEBUG

#define ASSERT(e) \
 if(!(e)){printf("Assert failed in %s/%d\n",__FILE__,__LINE__);exit(0);}
#define ASSERT_0(e) \
 if(!(e)){printf("Assert failed in %s/%d\n",__FILE__,__LINE__);return 0;}
#define ASSERT_V(e) \
 if(!(e)){printf("Assert failed in %s/%d\n",__FILE__,__LINE__);return;}

#define ASSERT_F(e) \
 if(!(e)){printf("Assert failed in %s/%d\n",__FILE__,__LINE__);return 0;}

#else

#define ASSERT(e)
#define ASSERT_0(e)
#define ASSERT_V(e)
#define ASSERT_F(e)

#endif

#endif

