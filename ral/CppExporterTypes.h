#ifndef RAL_CPP_EXPORTER_TYPES_H
#define RAL_CPP_EXPORTER_TYPES_H

#include "ral/IRubyAbstractLayer.h"

namespace RAL
{
  namespace CppExporter
  {
    /**
     *  Ruby本地扩展类 类型的包装结构（用于特化处理 所有导出函数尽量使用包装类类型）
     */
    template<typename T, T _default>  
    struct _rb_ext_type_wrap
    {
    public:
      _rb_ext_type_wrap() : data(_default)
      {}

      _rb_ext_type_wrap(T _data) : data(_data)
      {}

      operator T() { return data; }

    private:
      T data;
    };

    template<typename T>
    struct _rb_ext_type_wrap_0
    {
    public:
      _rb_ext_type_wrap_0() : data(0)
      {}

      _rb_ext_type_wrap_0(T _data) : data(_data)
      {}

      operator T() { return data; }

    private:
      T data;
    };

    typedef _rb_ext_type_wrap<VALUE, Qnil>      _rb_value;        ///<  VALUE
    typedef _rb_ext_type_wrap_0<char*>        _rb_char_ptr;     ///<  char*
    typedef _rb_ext_type_wrap_0<const char*>    _rb_const_char_ptr;   ///<  const char*
    typedef _rb_ext_type_wrap_0<wchar_t*>     _rb_wchar_t_ptr;    ///<  wchar_t*
    typedef _rb_ext_type_wrap_0<const wchar_t*>   _rb_const_wchar_t_ptr;  ///<  const wchar_t*
    typedef _rb_ext_type_wrap_0<char>       _rb_char;       ///<  char
    typedef _rb_ext_type_wrap_0<unsigned char>    _rb_unsigned_char;    ///<  unsigned char
    typedef _rb_ext_type_wrap_0<short>        _rb_short;        ///<  short
    typedef _rb_ext_type_wrap_0<unsigned short>   _rb_unsigned_short;   ///<  unsigned short
    typedef _rb_ext_type_wrap_0<long>       _rb_long;       ///<  long
    typedef _rb_ext_type_wrap_0<unsigned long>    _rb_unsigned_long;    ///<  unsigned long
    typedef _rb_ext_type_wrap_0<int>        _rb_int;        ///<  int
    typedef _rb_ext_type_wrap_0<unsigned int>   _rb_unsigned_int;   ///<  unsigned int
    typedef _rb_ext_type_wrap_0<float>        _rb_float;        ///<  float
    typedef _rb_ext_type_wrap_0<double>       _rb_double;       ///<  double
    typedef _rb_ext_type_wrap<bool, false>      _rb_bool;       ///<  bool

    /**
     *  Ruby extension class base.
     *  All classes contian allocator must inherit this.
     */
    class CRubyExtClassBase
    {
    public:
      CRubyExtClassBase() : __obj(Qnil) {}
      virtual ~CRubyExtClassBase(){}

    public:
      virtual void  Mark()        {}
      static bool   HaveMarkProc()    { return false; }

    public:
      inline VALUE  GetValue()      { return __obj; }
      inline VALUE  SetValue(VALUE obj) { __obj = obj; return obj; }

    private:
      VALUE     __obj;
    };

  } //  CppExporter
} //  RAL

#endif  //  RAL_CPP_EXPORTER_TYPES_H
