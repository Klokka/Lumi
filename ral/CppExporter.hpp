#include "ral/CppExporterTypes.h"
#include "ral/RubyAbstractLayer.h"

#include <new>
#include <typeinfo>

namespace RAL
{
  namespace CppExporter
  {
    /**
     *  类型特化处理
     */

    ///<  指针判断
    template<typename T> struct is_ptr          {enum{value=0};};
    template<typename T> struct is_ptr<T*>        {enum{value=1};};

    ///<  引用判断
    template<typename T> struct is_ref          {enum{value=0};};
    template<typename T> struct is_ref<T&>        {enum{value=1};};

    ///<  void类型判断
    template<typename T> struct is_void         {enum{value=0};};
    template<>       struct is_void<void>     {enum{value=1};};

    template<typename T> struct is_const        {enum{value=0};};
    template<typename T> struct is_const<const T>   {enum{value=1};};
    template<typename T> struct is_const<const T*>    {enum{value=1};};
    template<typename T> struct is_const<const T&>    {enum{value=1};};

    ///<  类型是否相同判断
    template<typename T1, typename T2> struct is_same {enum{value=0};};
    template<typename T1> struct is_same<T1, T1>    {enum{value=1};};

    ///<  移除常量特性
    template<typename T> struct remove_const      {typedef T  type;};
    template<typename T> struct remove_const<const T> {typedef T  type;};
    template<typename T> struct remove_const<const T*>  {typedef T* type;};
    template<typename T> struct remove_const<const T&>  {typedef T& type;};

    ///<  获取类型的基本类型 - 移除指针和引用类型
    template<typename T> struct base_type       {typedef T type;};
    template<typename T> struct base_type<T*>     {typedef T type;};
    template<typename T> struct base_type<T&>     {typedef T type;};

    ///<  移除常量指针和引用后的基本类型 ???
    template<typename T> struct class_type        {typedef typename remove_const<typename base_type<T>::type>::type type;};

    ///<  编译时条件判断 如果C为true返回T1 否则返回T2
    template<bool C, typename T1, typename T2>      struct if_{};
    template<typename T1, typename T2>          struct if_<true, T1, T2>  {typedef T1 type;};
    template<typename T1, typename T2>          struct if_<false, T1, T2> {typedef T2 type;};

    ///<  判断类T是否继承自TBase
    template<typename T, typename TBase>
    struct is_derived_from
    {
      static int  select(TBase*);
      static char select(void*);

      enum {value = (sizeof(int) == sizeof(select((T*)0)))};
    };

    ///<  运行时部分接口初始化
    static IRubyAbstractLayer*  spIral    = 0;

    static inline void SetCoreRuntime()
    {
      if (spIral)
        return;
      spIral = new CRubyAbstractLayer();
    }

    static inline IRubyAbstractLayer* GetIRAL()
    {
      RalAssert(spIral);
      return spIral;
    }

    ///<  Ruby中创建的扩展类对象的内存标记
    static const unsigned long RUBY_ALLOCATE_OBJECT_MEMORY_FLAG = 0xdeadbeef;

    ///<  Ruby类定义信息
    template<class T>
    struct _rb_class
    {
      static VALUE value;
    };
    template<class T> VALUE _rb_class<T>::value = Qnil;

    ///<  获取C++类对应的Ruby类
    template<class T>
    inline static VALUE _rb_class_get()   { return _rb_class<T>::value; }

    ///<  判断C++类对应的Ruby类是否存在
    template<class T>
    inline static bool  _rb_class_exist() { return (_rb_class_get<T>() != Qnil); }

    ///<  获取Ruby扩展对象的的UserData指针
    template<class T> 
    inline static T*  _rb_get_object_ptr(VALUE obj) { GetIRAL()->CheckTypeUserData(obj); return (T*)GetIRAL()->GetUserDataPtr(obj); }

    ///<  判断C++对象是否由Ruby创建
    template<class T>
    inline static bool  _is_rb_allocated(T* obj_ptr)  { return (*(unsigned long*)&((char*)obj_ptr)[sizeof(T)] == RUBY_ALLOCATE_OBJECT_MEMORY_FLAG); }

    ///<  Ruby对象的释放函数
    template<class T>
    static void RUBYCALL _rb_free(T* obj_ptr)
    {
      RalAssert(obj_ptr);
      RalAssert(_is_rb_allocated(obj_ptr));

      ///<  调用对象的析构函数
      obj_ptr->~T();

      ///<  释放对象内存
      GetIRAL()->XFree(obj_ptr);
      obj_ptr = 0;
    }

    ///<  Ruby对象的标记函数
    template<class T>
    static void RUBYCALL _rb_mark(T* obj_ptr)
    {
      RalAssert(obj_ptr);
      RalAssert(_is_rb_allocated(obj_ptr));

      obj_ptr->Mark();
    }

    ///<  包装本地指针为ruby对象（带free）
    namespace _rb_obj_alloc
    {
      template<typename T>
      struct _normal
      { 
        inline static VALUE invoke(VALUE klass, T* obj_ptr)
        {
          return GetIRAL()->UserDataObjectAlloc(klass, obj_ptr, /*(RubyDataFunc)_rb_mark<T>*/0, (RubyDataFunc)_rb_free<T>);
        }
      };

      template<typename T>
      struct _rbextclass
      { 
        inline static VALUE invoke(VALUE klass, T* obj_ptr)
        {
          RalAssert(obj_ptr);

          return obj_ptr->SetValue(GetIRAL()->UserDataObjectAlloc(klass, obj_ptr, (T::HaveMarkProc() ? (RubyDataFunc)_rb_mark<T> : 0), (RubyDataFunc)_rb_free<T>));
        } 
      };
    }

    ///<  Ruby对象的内存分配函数
    template<class T>
    VALUE _rb_allocate(VALUE klass)
    {
      ///<  对象有效性检测
      if (!_rb_class_exist<T>() || NIL_P(klass))
      {
        GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' object be allocated", typeid(T).name());
        return Qnil;
      }

      ///<  分配内存（对象大小 + 内存标记大小）
      void* obj_mem = GetIRAL()->XMalloc(sizeof(T) + sizeof(unsigned long));
      RalAssert(obj_mem);

      ///<  在指定位置构造对象（默认构造函数 不带参数）
      T* obj_ptr    = new (obj_mem) T;

      ///<  设置内存标记
      *(unsigned long*)&((char*)obj_ptr)[sizeof(T)] = RUBY_ALLOCATE_OBJECT_MEMORY_FLAG;

      ///<  返回Ruby对象
      return if_<is_derived_from<T, CRubyExtClassBase>::value, _rb_obj_alloc::_rbextclass<T>, _rb_obj_alloc::_normal<T> >::type::invoke(klass, obj_ptr);
    }

    ///<  C++指针的包装函数
    template<class T>
    VALUE _rb_wrap(T* obj_ptr)
    {
      ///<  如果空指针直接返回nil
      if (!obj_ptr)
        return Qnil;

      ///<  如果类T没有对应的Ruby类则抛出异常
      if (!_rb_class_exist<T>())
      {
        GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' pointer returned", typeid(T).name());
        return Qnil;
      }
      
      ///<  返回包装类对象
      return GetIRAL()->UserDataObjectAlloc(_rb_class_get<T>(), obj_ptr, 0, 0);
    }

    ///<  根据指定的导出类类型创建Ruby对象
    template<class T>
    inline VALUE  _rb_allocate_ruby_object()  { return _rb_allocate<T>(_rb_class_get<T>()); }

    ///<  根据指定的导出类类型创建C++对象
    template<class T>
    inline T*   _rb_allocate_cpp_object() { return (T*)GetIRAL()->GetUserDataPtr(_rb_allocate_ruby_object<T>()); }

    /**
     *  类型转换
     */
    namespace TypeConverter
    {
      namespace
      {
        ///<  VALUE 转换为 C++ 对象
        template<typename T>
        struct value2val 
        { 
          inline static T invoke(VALUE v)  ///<  VALUE只能转换到指针或引用 不能转换到对象 故该函数不实现
          {
              GetIRAL()->Raise(GetIRAL()->GetETypeError(), "VALUE can't be coerced into instance of type `%s'", typeid(T).name());
          }
        };

        template<typename T>
        struct value2ptr 
        { 
          inline static T* invoke(VALUE v)
          { 
            if (NIL_P(v)) return 0;     ///<  nil转换为空指针

            return _rb_get_object_ptr<T>(v);
          } 
        };

        template<typename T>
        struct value2ref 
        { 
          inline static T& invoke(VALUE v)
          {
            if (NIL_P(v))         ///<  nil不可转换为引用
              GetIRAL()->Raise(GetIRAL()->GetETypeError(), "NilClass can't be coerced into type `%s' reference", typeid(T).name());

            return *_rb_get_object_ptr<T>(v);
          } 
        };
      }

      template<class T>
      inline static T ValueTo(VALUE v, int argv_idx)
      {
        typedef value2ptr<typename base_type<T>::type> VALUE_PTR;
        typedef value2ref<typename base_type<T>::type> VALUE_REF;
        typedef value2val<typename base_type<T>::type> VALUE_VAL;

        return if_<is_ptr<T>::value, VALUE_PTR, typename if_<is_ref<T>::value, VALUE_REF, VALUE_VAL>::type>::type::invoke(v);
      }

      template<>
      inline static _rb_value ValueTo(VALUE v, int)     { return _rb_value(v); }

      template<>
      inline static char* ValueTo(VALUE v, int)       { return (NIL_P(v) ? 0 : GetIRAL()->GetStringPtr(v)); }

      template<>
      inline static const char* ValueTo(VALUE v, int)     { return (NIL_P(v) ? 0 : (const char*)GetIRAL()->GetStringPtr(v)); }
#if 0
      template<>
      inline static wchar_t* ValueTo(VALUE v, int idx)    { RalAssert(idx >= 0); return (NIL_P(v) ? 0 : GetIKconv()->UTF8ToUnicode(GetIRAL()->GetStringPtr(v), (unsigned long)idx)); }

      template<>
      inline static const wchar_t* ValueTo(VALUE v, int idx)  { return (const wchar_t*)ValueTo<wchar_t*>(v, idx); }
#endif
      template<>
      inline static char ValueTo(VALUE v, int)        { return (char)GetIRAL()->Fix2Int(v); }

      template<>
      inline static unsigned char ValueTo(VALUE v, int)   { return (unsigned char)GetIRAL()->Fix2Int(v); }

      template<>
      inline static short ValueTo(VALUE v, int)       { return (short)GetIRAL()->Fix2Int(v); }

      template<>
      inline static unsigned short ValueTo(VALUE v, int)    { return (unsigned short)GetIRAL()->Fix2Int(v); }

      template<>
      inline static long ValueTo(VALUE v, int)        { return GetIRAL()->Num2Long(v); }

      template<>
      inline static unsigned long ValueTo(VALUE v, int)   { return GetIRAL()->Num2Ulong(v); }

      template<>
      inline static int ValueTo(VALUE v, int)         { return GetIRAL()->Fix2Int(v); }

      template<>
      inline static unsigned int ValueTo(VALUE v, int)    { return GetIRAL()->Fix2Uint(v); }

      template<>
      inline static float ValueTo(VALUE v, int)       { return (float)GetIRAL()->Num2Dbl(v); }

      template<>
      inline static double ValueTo(VALUE v, int)        { return GetIRAL()->Num2Dbl(v); }

      template<>
      inline static bool ValueTo(VALUE v, int)        { return RTEST(v); }

      template<>
      inline static void ValueTo(VALUE v, int)        {}

      namespace
      {
        ///<  对象转为VALUE：普通类
        template<typename T>
        struct val2value_normal
        { 
          inline static VALUE invoke(T& v)
          {
            GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' object returned", typeid(T).name());
            return Qnil;
          }
        };

        ///<  对象转为VALUE：继承自CRubyExtClassBase类
        template<typename T>
        struct val2value_rbextclass
        { 
          inline static VALUE invoke(T& v)
          {
            if (!_rb_class_exist<T>())
            {
              GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' object returned", typeid(T).name());
              return Qnil;
            }

            //  allocate
            VALUE obj = _rb_allocate<T>(_rb_class_get<T>());

            //  clone
            *(T*)GetIRAL()->GetUserDataPtr(obj) = v;

            return obj;
          } 
        };

        ///<  对象转为VALUE：通用
        template<typename T>
        struct val2value 
        {
          inline static VALUE invoke(T& v)
          {
            return if_<is_derived_from<T, CRubyExtClassBase>::value, val2value_rbextclass<T>, val2value_normal<T> >::type::invoke(v);
          }
        };

        ///<  指针转为VALUE：普通类
        template<typename T>
        struct ptr2value_normal
        { 
          inline static VALUE invoke(T* v)  { return _rb_wrap(v); } 
        };

        ///<  指针转为VALUE：继承自CRubyExtClassBase类
        template<typename T>
        struct ptr2value_rbextclass
        { 
          inline static VALUE invoke(T* v)  
          { 
            if (!v) return Qnil;
            if (_is_rb_allocated(v)) return v->GetValue();
            return _rb_wrap(v);
          } 
        };

        ///<  指针转为VALUE：通用
        template<typename T>
        struct ptr2value 
        {
          inline static VALUE invoke(T* v)
          { 
            return if_<is_derived_from<T, CRubyExtClassBase>::value, ptr2value_rbextclass<T>, ptr2value_normal<T> >::type::invoke(v);
          } 
        };

        ///<  引用转为VALUE：普通类
        template<typename T>
        struct ref2value_normal
        { 
          inline static VALUE invoke(T& v)
          {
            GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' reference returned", typeid(T).name());
            return Qnil;
          }
        };

        ///<  引用转为VALUE：继承自CRubyExtClassBase类
        template<typename T>
        struct ref2value_rbextclass
        { 
          inline static VALUE invoke(T& v)  
          { 
            if (!_rb_class_exist<T>())
            {
              GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "unexported type `%s' reference returned", typeid(T).name());
              return Qnil;
            }

            if (_is_rb_allocated(&v))
              return v.GetValue();

            GetIRAL()->Raise(GetIRAL()->GetERuntimeError(), "type `%s' native reference returned", typeid(T).name());
            return Qnil;
          } 
        };

        ///<  引用转为VALUE：通用
        template<typename T>
        struct ref2value
        {
          inline static VALUE invoke(T& v)
          {
            return if_<is_derived_from<T, CRubyExtClassBase>::value, ref2value_rbextclass<T>, ref2value_normal<T> >::type::invoke(v);
          }
        };
      }

      template<typename T>
      struct non_const_to_value
      { 
        inline static VALUE invoke(T v)
        {
          typedef ptr2value<typename class_type<T>::type> PTR_VALUE;
          typedef ref2value<typename class_type<T>::type> REF_VALUE;
          typedef val2value<typename class_type<T>::type> VAL_VALUE;

          return if_<is_ptr<T>::value, PTR_VALUE, typename if_<is_ref<T>::value, REF_VALUE, VAL_VALUE>::type>::type::invoke(v);
        }
      };

      template<typename T>
      struct const_to_value
      { 
        inline static VALUE invoke(T v) 
        { 
          typedef ptr2value<typename class_type<T>::type> PTR_VALUE;
          typedef ref2value<typename class_type<T>::type> REF_VALUE;
          typedef val2value<typename class_type<T>::type> VAL_VALUE;

          return if_<is_ptr<T>::value, PTR_VALUE, typename if_<is_ref<T>::value, REF_VALUE, VAL_VALUE>::type>::type::invoke(const_cast<typename remove_const<T>::type>(v));
        } 
      };

      template<class T> 
      inline static VALUE ToValue(T v)
      {
        return if_<is_const<T>::value, const_to_value<T>, non_const_to_value<T> >::type::invoke(v);
      }

      template<>
      inline static VALUE ToValue(_rb_value v)    { return (VALUE)v; }

      template<>
      inline static VALUE ToValue(char* v)      { return (v ? GetIRAL()->ForceEncodingUTF8(GetIRAL()->StrNew2(v)) : Qnil); }

      template<>
      inline static VALUE ToValue(const char* v)    { return (v ? GetIRAL()->ForceEncodingUTF8(GetIRAL()->StrNew2(v)) : Qnil); }
#if 0
      template<>
      inline static VALUE ToValue(wchar_t* v)     { return (v ? GetIRAL()->ForceEncodingUTF8(GetIRAL()->StrNew2(GetIKconv()->UnicodeToUTF8(v))) : Qnil); }

      template<>
      inline static VALUE ToValue(const wchar_t* v) { return (v ? GetIRAL()->ForceEncodingUTF8(GetIRAL()->StrNew2(GetIKconv()->UnicodeToUTF8(v))) : Qnil); }
#endif
      template<>
      inline static VALUE ToValue(char v)       { return GetIRAL()->Int2Fix((int)v); }

      template<>
      inline static VALUE ToValue(unsigned char v)  { return GetIRAL()->Int2Fix((int)v); }

      template<>
      inline static VALUE ToValue(short v)      { return GetIRAL()->Int2Fix((int)v); }

      template<>
      inline static VALUE ToValue(unsigned short v) { return GetIRAL()->Int2Fix((int)v); }

      template<>
      inline static VALUE ToValue(long v)       { return GetIRAL()->Long2Num(v); }

      template<>
      inline static VALUE ToValue(unsigned long v)  { return GetIRAL()->Ulong2Num(v); }

      template<>
      inline static VALUE ToValue(int v)        { return GetIRAL()->Int2Fix(v); }

      template<>
      inline static VALUE ToValue(unsigned int v)   { return GetIRAL()->Uint2Num(v); }

      template<>
      inline static VALUE ToValue(float v)      { return GetIRAL()->FloatNew(v); }

      template<>
      inline static VALUE ToValue(double v)     { return GetIRAL()->FloatNew(v); }

      template<>
      inline static VALUE ToValue(bool v)       { return (v ? Qtrue : Qfalse); }

      //template<>
      //inline static VALUE ToValue(void);      ///<  没有void到VALUE的转换
    };


    ///<  函数信息结构体

    ///<  参数 -1
    template<int mid, typename MemberPtr, typename RVal, class T> 
    struct dm_method_m_1
    {
      static VALUE RUBYCALL invoke(int argc, VALUE* argv, VALUE obj)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(argc, argv));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T> MemberPtr dm_method_m_1<mid, MemberPtr, RVal, T>::_addr = 0;

    ///<  参数 -1 void类型偏特化
    template<int mid, typename MemberPtr, class T> 
    struct dm_method_m_1<mid, MemberPtr, void, T>
    {
      static VALUE RUBYCALL invoke(int argc, VALUE* argv, VALUE obj)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(argc, argv);
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T> MemberPtr dm_method_m_1<mid, MemberPtr, void, T>::_addr = 0;

    ///<  参数 -2
    template<int mid, typename MemberPtr, typename RVal, class T> 
    struct dm_method_m_2
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE args)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(args));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T> MemberPtr dm_method_m_2<mid, MemberPtr, RVal, T>::_addr = 0;

    ///<  参数 -2 void类型偏特化
    template<int mid, typename MemberPtr, class T> 
    struct dm_method_m_2<mid, MemberPtr, void, T>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE args)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(args);
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T> MemberPtr dm_method_m_2<mid, MemberPtr, void, T>::_addr = 0;

    ///<  参数 0
    template<int mid, typename MemberPtr, typename RVal, class T> 
    struct dm_method_m0
    {
      static VALUE RUBYCALL invoke(VALUE obj)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)());
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T> MemberPtr dm_method_m0<mid, MemberPtr, RVal, T>::_addr = 0;

    ///<  参数 0  void类型偏特化
    template<int mid, typename MemberPtr, class T> 
    struct dm_method_m0<mid, MemberPtr, void, T>
    {
      static VALUE RUBYCALL invoke(VALUE obj)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)();
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T> MemberPtr dm_method_m0<mid, MemberPtr, void, T>::_addr = 0;

    ///<  参数 1
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1> 
    struct dm_method_m1
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0)));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1> MemberPtr dm_method_m1<mid, MemberPtr, RVal, T, T1>::_addr = 0;

    ///<  参数 1  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1> 
    struct dm_method_m1<mid, MemberPtr, void, T, T1>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0));
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1> MemberPtr dm_method_m1<mid, MemberPtr, void, T, T1>::_addr = 0;

    ///<  参数 2
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2> 
    struct dm_method_m2
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1)));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2> MemberPtr dm_method_m2<mid, MemberPtr, RVal, T, T1, T2>::_addr = 0;

    ///<  参数 2  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1, typename T2> 
    struct dm_method_m2<mid, MemberPtr, void, T, T1, T2>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1));
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1, typename T2> MemberPtr dm_method_m2<mid, MemberPtr, void, T, T1, T2>::_addr = 0;

    ///<  参数 3
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3> 
    struct dm_method_m3
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1), TypeConverter::ValueTo<T3>(v3, 2)));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3> MemberPtr dm_method_m3<mid, MemberPtr, RVal, T, T1, T2, T3>::_addr = 0;

    ///<  参数 3  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3> 
    struct dm_method_m3<mid, MemberPtr, void, T, T1, T2, T3>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1), TypeConverter::ValueTo<T3>(v3, 2));
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3> MemberPtr dm_method_m3<mid, MemberPtr, void, T, T1, T2, T3>::_addr = 0;

    ///<  参数 4
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4> 
    struct dm_method_m4
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1), TypeConverter::ValueTo<T3>(v3, 2), TypeConverter::ValueTo<T4>(v4, 3)));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4> MemberPtr dm_method_m4<mid, MemberPtr, RVal, T, T1, T2, T3, T4>::_addr = 0;

    ///<  参数 4  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4> 
    struct dm_method_m4<mid, MemberPtr, void, T, T1, T2, T3, T4>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), TypeConverter::ValueTo<T2>(v2, 1), TypeConverter::ValueTo<T3>(v3, 2), TypeConverter::ValueTo<T4>(v4, 3));
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4> MemberPtr dm_method_m4<mid, MemberPtr, void, T, T1, T2, T3, T4>::_addr = 0;

    ///<  参数 5
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4, typename T5> 
    struct dm_method_m5
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4, VALUE v5)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), 
          TypeConverter::ValueTo<T2>(v2, 1), 
          TypeConverter::ValueTo<T3>(v3, 2), 
          TypeConverter::ValueTo<T4>(v4, 3),
          TypeConverter::ValueTo<T5>(v5, 4)
          ));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4, typename T5> MemberPtr dm_method_m5<mid, MemberPtr, RVal, T, T1, T2, T3, T4, T5>::_addr = 0;

    ///<  参数 5  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4, typename T5> 
    struct dm_method_m5<mid, MemberPtr, void, T, T1, T2, T3, T4, T5>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4, VALUE v5)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), 
          TypeConverter::ValueTo<T2>(v2, 1), 
          TypeConverter::ValueTo<T3>(v3, 2), 
          TypeConverter::ValueTo<T4>(v4, 3),
          TypeConverter::ValueTo<T5>(v5, 4)
          );
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4, typename T5> MemberPtr dm_method_m5<mid, MemberPtr, void, T, T1, T2, T3, T4, T5>::_addr = 0;

    ///<  参数 6
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> 
    struct dm_method_m6
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4, VALUE v5, VALUE v6)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        return TypeConverter::ToValue<RVal>((obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), 
          TypeConverter::ValueTo<T2>(v2, 1), 
          TypeConverter::ValueTo<T3>(v3, 2), 
          TypeConverter::ValueTo<T4>(v4, 3),
          TypeConverter::ValueTo<T5>(v5, 4),
          TypeConverter::ValueTo<T6>(v6, 5)
          ));
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, typename RVal, class T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> MemberPtr dm_method_m6<mid, MemberPtr, RVal, T, T1, T2, T3, T4, T5, T6>::_addr = 0;

    ///<  参数 6  void类型偏特化
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> 
    struct dm_method_m6<mid, MemberPtr, void, T, T1, T2, T3, T4, T5, T6>
    {
      static VALUE RUBYCALL invoke(VALUE obj, VALUE v1, VALUE v2, VALUE v3, VALUE v4, VALUE v5, VALUE v6)
      {
        T* obj_ptr = _rb_get_object_ptr<T>(obj);
        (obj_ptr->*_addr)(TypeConverter::ValueTo<T1>(v1, 0), 
          TypeConverter::ValueTo<T2>(v2, 1), 
          TypeConverter::ValueTo<T3>(v3, 2), 
          TypeConverter::ValueTo<T4>(v4, 3),
          TypeConverter::ValueTo<T5>(v5, 4),
          TypeConverter::ValueTo<T6>(v6, 5)
          );
        return Qnil;
      }

      static MemberPtr _addr;
    };
    template<int mid, typename MemberPtr, class T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> MemberPtr dm_method_m6<mid, MemberPtr, void, T, T1, T2, T3, T4, T5, T6>::_addr = 0;

    ///<  C++类导出模板
    template<class ExportClassType>
    struct _class
    {
      _class(const char* pKlassName, VALUE rb_cParent = Qnil)
      {
        if (NIL_P(rb_cParent)) rb_cParent = GetIRAL()->GetCObject();

        RalAssert(pKlassName);                                  ///<  防止空指针
        RalAssert(!_rb_class_exist<ExportClassType>());                     ///<  防止类重复导出
        RalAssert(RTEST(GetIRAL()->ObjectIsInstanceOf(rb_cParent, GetIRAL()->GetCClass())));  ///<  父类必须是Class类的实例

        _rb_class<ExportClassType>::value = GetIRAL()->DefineClass(pKlassName, rb_cParent);

        alloc(false);                                     ///<  默认不可构造对象
      }

      _class(VALUE rb_cOuter, const char* pKlassName, VALUE rb_cParent = Qnil)
      {
        if (NIL_P(rb_cParent)) rb_cParent = GetIRAL()->GetCObject();

        RalAssert(pKlassName);                                  ///<  防止空指针
        RalAssert(!_rb_class_exist<ExportClassType>());                     ///<  防止类重复导出
        RalAssert(RTEST(GetIRAL()->ObjectIsInstanceOf(rb_cParent, GetIRAL()->GetCClass())));  ///<  父类必须是Class类的实例
        RalAssert(RTEST(GetIRAL()->ObjectIsKindOf(rb_cOuter, GetIRAL()->GetCModule())));    ///<  Outer必须是类或者模块对象

        _rb_class<ExportClassType>::value = GetIRAL()->DefineClassUnder(rb_cOuter, pKlassName, rb_cParent);

        alloc(false);                         ///<  默认不可构造对象
      }

      ~_class()
      {
      }

      ///<  定义allocate方法
      // TODO: this is a dirty hack, need to use template
      _class& alloc(bool s = true)
      {
        if (s)
        {
          GetIRAL()->DefineAllocFunc(_rb_class_get<ExportClassType>(), _rb_allocate<ExportClassType>);
          return *this;
        }
        else
        {
          GetIRAL()->UndefAllocFunc(_rb_class_get<ExportClassType>());
          return *this;
        }
      }

      ///<  取消方法定义
      _class& undef(const char* pMethodName)
      {
        RalAssert(pMethodName);

        GetIRAL()->UndefMethod(_rb_class_get<ExportClassType>(), pMethodName);

        return *this;
      }

      ///<  定义方法别名
      _class& alias(const char* pNewName, const char* pOldName)
      {
        RalAssert(pNewName);
        RalAssert(pOldName);

        GetIRAL()->DefineAlias(_rb_class_get<ExportClassType>(), pNewName, pOldName);

        return *this;
      }
#ifdef _MSVC_VER
      ///<  *** __stdcall ***

      ///<  定义方法：参数 0
      template<int mid, typename RVal, typename T>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)())
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)();

        return def_core<struct dm_method_m0<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, 0);
      }

      ///<  定义方法：参数 0 const
      template<int mid, typename RVal, typename T>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)() const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)() const;

        return def_core<struct dm_method_m0<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, 0);
      }

      ///<  定义方法：参数 1
      template<int mid, typename RVal, typename T, typename T1>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1);

        return def_core<struct dm_method_m1<mid, MemberPtr, RVal, ExportClassType, T1> >(pMethodName, ptr, 1);
      }

      ///<  定义方法：参数 1 const
      template<int mid, typename RVal, typename T, typename T1>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1) const;

        return def_core<struct dm_method_m1<mid, MemberPtr, RVal, ExportClassType, T1> >(pMethodName, ptr, 1);
      }

      ///<  定义方法：参数 2
      template<int mid, typename RVal, typename T, typename T1, typename T2>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2);

        return def_core<struct dm_method_m2<mid, MemberPtr, RVal, ExportClassType, T1, T2> >(pMethodName, ptr, 2);
      }

      ///<  定义方法：参数 2 const
      template<int mid, typename RVal, typename T, typename T1, typename T2>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2) const;

        return def_core<struct dm_method_m2<mid, MemberPtr, RVal, ExportClassType, T1, T2> >(pMethodName, ptr, 2);
      }

      ///<  定义方法：参数 3
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3);

        return def_core<struct dm_method_m3<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3> >(pMethodName, ptr, 3);
      }

      ///<  定义方法：参数 3 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3) const;

        return def_core<struct dm_method_m3<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3> >(pMethodName, ptr, 3);
      }

      ///<  定义方法：参数 4
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4);

        return def_core<struct dm_method_m4<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4> >(pMethodName, ptr, 4);
      }

      ///<  定义方法：参数 4 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4) const;

        return def_core<struct dm_method_m4<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4> >(pMethodName, ptr, 4);
      }

      ///<  定义方法：参数 5
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4, T5))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5);

        return def_core<struct dm_method_m5<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5> >(pMethodName, ptr, 5);
      }

      ///<  定义方法：参数 5 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4, T5) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5) const;

        return def_core<struct dm_method_m5<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5> >(pMethodName, ptr, 5);
      }

      ///<  定义方法：参数 6
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4, T5, T6))
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5, T6);

        return def_core<struct dm_method_m6<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5, T6> >(pMethodName, ptr, 6);
      }

      ///<  定义方法：参数 6 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
      _class& def(const char* pMethodName, RVal (__stdcall T::*ptr)(T1, T2, T3, T4, T5, T6) const)
      {
        typedef RVal (__stdcall ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5, T6) const;

        return def_core<struct dm_method_m6<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5, T6> >(pMethodName, ptr, 6);
      }
#endif
      ///<  *** __thiscall ***

      ///<  定义方法：不定参数 -1  方法原型： RVal T::func(int argc, VALUE* argv)
      template<int mid, typename RVal, typename T>
      _class& def_1(const char* pMethodName, RVal (T::*ptr)(int, VALUE*))
      {
        typedef RVal (ExportClassType::*MemberPtr)(int, VALUE*);

        return def_core<struct dm_method_m_1<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, -1);
      }

      ///<  定义方法：不定参数 -2  方法原型： RVal T::func(VALUE args)
      template<int mid, typename RVal, typename T>
      _class& def_2(const char* pMethodName, RVal (T::*ptr)(VALUE))
      {
        typedef RVal (ExportClassType::*MemberPtr)(VALUE);

        return def_core<struct dm_method_m_2<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, -2);
      }

      ///<  定义方法：参数 0
      template<int mid, typename RVal, typename T>
      _class& def(const char* pMethodName, RVal (T::*ptr)())
      {
        typedef RVal (ExportClassType::*MemberPtr)();

        return def_core<struct dm_method_m0<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, 0);
      }

      ///<  定义方法：参数 0 const
      template<int mid, typename RVal, typename T>
      _class& def(const char* pMethodName, RVal (T::*ptr)() const)
      {
        typedef RVal (ExportClassType::*MemberPtr)() const;

        return def_core<struct dm_method_m0<mid, MemberPtr, RVal, ExportClassType> >(pMethodName, ptr, 0);
      }

      ///<  定义方法：参数 1
      template<int mid, typename RVal, typename T, typename T1>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1);

        return def_core<struct dm_method_m1<mid, MemberPtr, RVal, ExportClassType, T1> >(pMethodName, ptr, 1);
      }

      ///<  定义方法：参数 1 const
      template<int mid, typename RVal, typename T, typename T1>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1) const;

        return def_core<struct dm_method_m1<mid, MemberPtr, RVal, ExportClassType, T1> >(pMethodName, ptr, 1);
      }

      ///<  定义方法：参数 2
      template<int mid, typename RVal, typename T, typename T1, typename T2>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2);

        return def_core<struct dm_method_m2<mid, MemberPtr, RVal, ExportClassType, T1, T2> >(pMethodName, ptr, 2);
      }

      ///<  定义方法：参数 2 const
      template<int mid, typename RVal, typename T, typename T1, typename T2>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2) const;

        return def_core<struct dm_method_m2<mid, MemberPtr, RVal, ExportClassType, T1, T2> >(pMethodName, ptr, 2);
      }

      ///<  定义方法：参数 3
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3);

        return def_core<struct dm_method_m3<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3> >(pMethodName, ptr, 3);
      }

      ///<  定义方法：参数 3 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3) const;

        return def_core<struct dm_method_m3<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3> >(pMethodName, ptr, 3);
      }

      ///<  定义方法：参数 4
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4);

        return def_core<struct dm_method_m4<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4> >(pMethodName, ptr, 4);
      }

      ///<  定义方法：参数 4 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4) const;

        return def_core<struct dm_method_m4<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4> >(pMethodName, ptr, 4);
      }

      ///<  定义方法：参数 5
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4, T5))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5);

        return def_core<struct dm_method_m5<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5> >(pMethodName, ptr, 5);
      }

      ///<  定义方法：参数 5 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4, T5) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5) const;

        return def_core<struct dm_method_m5<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5> >(pMethodName, ptr, 5);
      }

      ///<  定义方法：参数 6
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4, T5, T6))
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5, T6);

        return def_core<struct dm_method_m6<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5, T6> >(pMethodName, ptr, 6);
      }

      ///<  定义方法：参数 6 const
      template<int mid, typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
      _class& def(const char* pMethodName, RVal (T::*ptr)(T1, T2, T3, T4, T5, T6) const)
      {
        typedef RVal (ExportClassType::*MemberPtr)(T1, T2, T3, T4, T5, T6) const;

        return def_core<struct dm_method_m6<mid, MemberPtr, RVal, ExportClassType, T1, T2, T3, T4, T5, T6> >(pMethodName, ptr, 6);
      }

    private:
      ///<  实际的方法定义
      template<typename dm_method, typename MemberPtr>
      inline _class& def_core(const char* pMethodName, MemberPtr ptr, int argc)
      {
        RalAssert(pMethodName);
        RalAssert(!dm_method::_addr);

        dm_method::_addr = ptr;

        GetIRAL()->DefineMethod(_rb_class_get<ExportClassType>(), pMethodName, (RubyFunc)dm_method::invoke, argc);

        return *this;
      }
    };
  } //  CppExporter

} //  RAL
