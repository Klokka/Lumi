#ifndef RAL_RUBY_DEFINE_GLOBAL_H
#define RAL_RUBY_DEFINE_GLOBAL_H

#include "limits.h"
#include "ral/IRubyAbstractLayer.h"
#include "ral/RubyNativeDefine.h"

namespace RAL
{
  namespace RubyCommon
  {
    typedef long SIGNED_VALUE;

    static const VALUE RUBY_FIXNUM_FLAG = 0x01;
    static const VALUE SYMBOL_FLAG = 0x0e;

    ///<  Type converter
    static inline long FIX2LONG(VALUE x)      { return (long)(((SIGNED_VALUE)x) >> 1); }
    static inline unsigned long FIX2ULONG(VALUE x) { return ((x >> 1) & LONG_MAX); }

    static inline int FIX2INT(VALUE x)        { return (int)FIX2LONG(x); }
    static inline unsigned int FIX2UINT(VALUE x) { return (unsigned int)FIX2ULONG(x); }

    static inline VALUE INT2FIX(int i)        { return ((VALUE)(((SIGNED_VALUE)(i)) << 1 | RUBY_FIXNUM_FLAG)); }

    static inline VALUE ID2SYM(ID id)         { return (((VALUE)(id) << 8)| SYMBOL_FLAG); }

    static const unsigned long FIXNUM_MAX = (unsigned long)(LONG_MAX >> 1);
    static const long FIXNUM_MIN          = ((long)LONG_MIN >> (int)1);

    static inline bool FIXNUM_P(VALUE f)      { return (((SIGNED_VALUE)f) & RUBY_FIXNUM_FLAG) != 0; }
    static inline bool POSFIXABLE(unsigned long f) { return (f <= FIXNUM_MAX); }
    static inline bool NEGFIXABLE(long f)     { return (f >= FIXNUM_MIN); }
    static inline bool FIXABLE(long f)        { return (NEGFIXABLE(f) && (f <= 0 || POSFIXABLE(f))); }
  } //  RubyCommon
} //  RAL

#endif  //  RAL_RUBY_DEFINE_GLOBAL_H
