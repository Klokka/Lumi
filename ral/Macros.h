#ifndef RAL_MACROS_H
#define RAL_MACROS_H
#include <new>

#ifdef NDEBUG
# define  RalAssert(exp)      ((void)0)
#else
# ifdef WIN32
#   include <assert.h>
#   define  RalAssert(exp)    assert(exp)
# else
#   define  RalAssert(exp)    ((void)0)
# endif //  WIN32
#endif  //  NDEBUG

#define RalArrayCount(ary)      (sizeof(ary) / sizeof((ary)[0]))

#define RalMin(v1, v2)        ((v1) < (v2) ? (v1) : (v2))
#define RalMax(v1, v2)        ((v1) > (v2) ? (v1) : (v2))
#define RalClamp(v, minv, maxv)   RalMin(maxv, RgeMax(v, minv))

#define RalText(txt)        L##txt

#define SAFE_DELETE(ptr)      {if (ptr) { delete (ptr); (ptr) = NULL; }}
#define SAFE_DELETE_ARRAY(ptr)    {if (ptr) { delete [] (ptr); (ptr) = NULL; }}
#define SAFE_RELEASE(ptr)     {if (ptr) { (ptr)->Release(); (ptr) = NULL; }}

#define WFILE_TEMP(x)       RalText(x)
#define __W_FILE__          WFILE_TEMP(__FILE__)

#endif  //  RAL_MACROS_H
