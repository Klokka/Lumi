#ifndef RAL_RUBY_ABSTRACT_LAYER_H
#define RAL_RUBY_ABSTRACT_LAYER_H

#include "ral/IRubyAbstractLayer.h"
#include "ral/Macros.h"

#if defined(RUBY_1_9) || defined(RUBY_2_0)
    #include "RubyDefineV193.h"
#endif

namespace RAL
{
  class CRubyAbstractLayer : public IRubyAbstractLayer
  {
  private:
    typedef int           (*pfn_rb_type)        (VALUE obj);

    typedef char*         (*pfn_rb_string_ptr)  (VALUE str);
    typedef long          (*pfn_rb_string_len)  (VALUE str);
    typedef VALUE*        (*pfn_rb_array_ptr)   (VALUE ary);
    typedef long          (*pfn_rb_array_len)   (VALUE ary);
    typedef void*         (*pfn_rb_userdata_ptr)(VALUE d);

  public:
    CRubyAbstractLayer();
    ~CRubyAbstractLayer();

  public:
    bool                  Init();

  private:
    bool                  InitRuby193();
    bool                  InitRubyDifference();

  public:
    virtual unsigned long GetVersion() const       { return m_uVersionCode; }
    virtual bool          InterpreterInit();
    virtual void          InterpreterFinalize();

    virtual VALUE         GetCObject() const       { return ::rb_cObject; }
    virtual VALUE         GetCClass() const        { return ::rb_cClass; }
    virtual VALUE         GetCModule() const       { return ::rb_cModule; }
    virtual VALUE         GetCInteger() const      { return ::rb_cInteger; }
    virtual VALUE         GetCNumeric() const      { return ::rb_cNumeric; }
    virtual VALUE         GetMKernel() const       { return ::rb_mKernel; }
    virtual VALUE         GetEException() const    { return ::rb_eException; }
    virtual VALUE         GetESystemExit() const   { return ::rb_eSystemExit; }
    virtual VALUE         GetESyntaxError() const  { return ::rb_eSyntaxError; }
    virtual VALUE         GetENoMemError() const   { return ::rb_eNoMemError; }
    virtual VALUE         GetEArgError() const     { return ::rb_eArgError; }
    virtual VALUE         GetETypeError() const    { return ::rb_eTypeError; }
    virtual VALUE         GetERuntimeError() const { return ::rb_eRuntimeError; }

    virtual VALUE         ConstGet(VALUE obj, ID id)                            { return ::rb_const_get(obj, id); }
    virtual void          DefineConst(VALUE obj, const char* name, VALUE value) { ::rb_define_const(obj, name, value); }
    virtual void          DefineGlobalConst(const char* name, VALUE value)      { ::rb_define_global_const(name, value); }

    virtual VALUE         InstanceVarGet(VALUE obj, const char* vname)          { return ::rb_iv_get(obj, vname); }
    virtual VALUE         InstanceVarSet(VALUE obj, const char* vname, VALUE v) { return ::rb_iv_set(obj, vname, v); }

    virtual void*         XMalloc(size_t size) { return ::ruby_xmalloc(size); }
    virtual void          XFree(void* data)    { ::ruby_xfree(data); }

    virtual void          GcMark(VALUE obj) { ::rb_gc_mark(obj); }

    virtual VALUE         DefineClass(const char* klassname, VALUE parent) { return ::rb_define_class(klassname, parent); }
    virtual VALUE         DefineModule(const char* modname)                { return ::rb_define_module(modname); }
    virtual VALUE         DefineClassUnder(VALUE outer, const char* klassname, VALUE parent) { return ::rb_define_class_under(outer, klassname, parent); }
    virtual VALUE         DefineModuleUnder(VALUE outer, const char* modname)                { return ::rb_define_module_under(outer, modname); }

    virtual void          DefineMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc)       { ::rb_define_method(klass, name, cfunc, argc); }
    virtual void          DefineModuleFunction(VALUE mod, const char* name, RubyFunc cfunc, int argc) { ::rb_define_module_function(mod, name, cfunc, argc); }
    virtual void          DefineGlobalFunction(const char* name, RubyFunc cfunc, int argc)            { ::rb_define_global_function(name, cfunc, argc); }

    virtual void          DefineProtectedMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) { ::rb_define_protected_method(klass, name, cfunc, argc); }
    virtual void          DefinePrivateMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc)   { ::rb_define_private_method(klass, name, cfunc, argc); }
    virtual void          DefineSingletonMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) { ::rb_define_singleton_method(klass, name, cfunc, argc); }

    virtual void          DefineAllocFunc(VALUE klass, VALUE (*c_alloc_func)(VALUE)) { ::rb_define_alloc_func(klass, c_alloc_func); }
    virtual void          UndefAllocFunc(VALUE klass)                                { ::rb_undef_alloc_func(klass); }
    virtual VALUE         UserDataObjectAlloc(VALUE klass, void* userptr, RubyDataFunc cmark, RubyDataFunc cfree) { return ::rb_data_object_alloc(klass, userptr, cmark, cfree); }
    virtual void          UndefMethod(VALUE klass, const char* name)                        { ::rb_undef_method(klass, name); }
    virtual void          DefineAlias(VALUE klass, const char* newname,const char* oldname) { ::rb_define_alias(klass, newname, oldname); }

    virtual ID            Name2ID(const char* name) { return ::rb_intern(name); }
    virtual const char*   ID2Name(ID id)            { return ::rb_id2name(id); }
    virtual VALUE         ID2Sym(ID id)             { return RubyCommon::ID2SYM(id); }

    virtual VALUE         Funcall(VALUE obj, ID mid, int argc, ...);
    virtual VALUE         Funcall2(VALUE obj, ID mid, int argc, const VALUE* argv) { return ::rb_funcall2(obj, mid, argc, argv); }
    virtual VALUE         Funcall3(VALUE obj, ID mid, int argc, const VALUE* argv) { return ::rb_funcall3(obj, mid, argc, argv); }
    virtual int           ScanArgs(int argc, const VALUE* argv, const char* fmt, ...);
    virtual VALUE         CallSuper(int argc, const VALUE* argv) { return ::rb_call_super(argc, argv); }
    virtual int           RespondTo(VALUE obj, ID mid)           { return ::rb_respond_to(obj, mid); }

    virtual VALUE         EvalString(const char* script)                    { return ::rb_eval_string(script); }
    virtual VALUE         EvalStringProtect(const char* script, int* state) { return ::rb_eval_string_protect(script, state); }

    virtual VALUE         ProtectRun(VALUE (*proc)(VALUE), VALUE args, int* state) { return ::rb_protect(proc, args, state); }
    virtual void          Raise(VALUE excp, const char* fmt, ...);
    virtual void          RaiseNoMemory();
    virtual VALUE         GetErrInfo(void);

    virtual VALUE         ObjectClass(VALUE obj)                     { return ::rb_obj_class(obj); }
    virtual VALUE         ObjectSingletonClass(VALUE obj)            { return ::rb_singleton_class(obj); }
    virtual VALUE         ObjectIsInstanceOf(VALUE obj, VALUE klass) { return ::rb_obj_is_instance_of(obj, klass); }
    virtual VALUE         ObjectIsKindOf(VALUE obj, VALUE klass)     { return ::rb_obj_is_kind_of(obj, klass); }
    virtual const char*   Class2Name(VALUE klass)                    { return ::rb_class2name(klass); }
    virtual const char*   ObjectClassName(VALUE obj)                 { return ::rb_obj_classname(obj); }
    virtual int           Type(VALUE obj)                            { return rb_type(obj); }
    virtual void          CheckType(VALUE obj, int type)             { ::rb_check_type(obj, type); }
    virtual VALUE         ConvertType(VALUE val, int type, const char *tname, const char *method) { return ::rb_convert_type(val, type, tname, method); }
    virtual int           Get_T_DATA() const                         { return m_type_T_DATA; }
    virtual int           Get_T_NIL() const                          { return m_type_T_NIL; }
    virtual int           Get_T_ARRAY() const                        { return m_type_T_ARRAY; }
    virtual int           Get_T_SYMBOL() const                       { return m_type_T_SYMBOL; }
    virtual void          CheckTypeUserData(VALUE obj)               { RalAssert(m_type_T_DATA); ::rb_check_type(obj, m_type_T_DATA); }
    virtual void          SafeFixnumValue(VALUE obj);
    virtual void          SafeIntegerValue(VALUE obj);
    virtual void          SafeNumericValue(VALUE obj);
    virtual void          SafeSpecClassValue(VALUE obj, VALUE klass);

    virtual long          Fix2Long(VALUE x)          { return RubyCommon::FIX2LONG(x); }
    virtual unsigned long Fix2Ulong(VALUE x)         { return RubyCommon::FIX2ULONG(x); }
    virtual int           Fix2Int(VALUE x)           { return RubyCommon::FIX2INT(x); }
    virtual unsigned int  Fix2Uint(VALUE x)          { return RubyCommon::FIX2UINT(x); }
    virtual VALUE         Int2Fix(int i)             { return RubyCommon::INT2FIX(i); }
    virtual VALUE         Long2Fix(long x)           { return RubyCommon::INT2FIX((int)x); }
    virtual long          Num2Long(VALUE num)        { return ::rb_num2long(num); }
    virtual unsigned long Num2Ulong(VALUE num)       { return ::rb_num2ulong(num); }
    virtual double        Num2Dbl(VALUE num)         { return ::rb_num2dbl(num); }
    virtual VALUE         Int2Num(int v);
    virtual VALUE         Uint2Num(unsigned int v);
    virtual VALUE         Long2Num(long v)           { return Int2Num((int)v); }
    virtual VALUE         Ulong2Num(unsigned long v) { return Uint2Num((unsigned int)v); }

    virtual VALUE         StrNew(const char* str, long len)        { return ::rb_str_new(str, len); }
    virtual VALUE         StrNew2(const char* str)                 { return ::rb_str_new2(str); }
    virtual VALUE         StrNew3(VALUE str)                       { return ::rb_str_new3(str); }
    virtual void          StrModify(VALUE str)                     { ::rb_str_modify(str); }
    virtual VALUE         StrCat(VALUE str, const char* s, long l) { return ::rb_str_cat(str, s, l); }
    virtual VALUE         StrBufNew(long len)                      { return ::rb_str_buf_new(len); }
    virtual VALUE         StrBufAppend(VALUE str1, VALUE str2)     { return ::rb_str_buf_append(str1, str2); }
    virtual VALUE         Inspect(VALUE obj)                       { return ::rb_inspect(obj); }
    virtual VALUE         ObjectAsString(VALUE obj)                { return ::rb_obj_as_string(obj); }
    virtual VALUE         ForceEncodingUTF8(VALUE str);

    virtual VALUE         AryNew(void)                           { return ::rb_ary_new(); }
    virtual VALUE         AryNew2(long len)                      { return ::rb_ary_new2(len); }
    virtual VALUE         AryNew4(long len, const VALUE* values) { return ::rb_ary_new4(len, values); }
    virtual void          AryStore(VALUE ary, long idx, VALUE v) { return ::rb_ary_store(ary, idx, v); }
    virtual VALUE         AryPush(VALUE ary, VALUE v)            { return ::rb_ary_push(ary, v); }
    virtual VALUE         AryPop(VALUE ary)                      { return ::rb_ary_pop(ary); }
    virtual VALUE         AryShift(VALUE ary)                    { return ::rb_ary_shift(ary); }
    virtual VALUE         AryUnshift(VALUE ary, VALUE v)         { return ::rb_ary_unshift(ary, v); }
    virtual VALUE         AryEntry(VALUE ary, long idx)          { return ::rb_ary_entry(ary, idx); }
    virtual VALUE         AryClear(VALUE ary)                    { return ::rb_ary_clear(ary); }

    virtual VALUE         FloatNew(double f) { return ::rb_float_new(f); }

    virtual int           BlockGivenP(void) { return ::rb_block_given_p(); }
    virtual VALUE         BlockProc(void)   { return ::rb_block_proc(); }

    virtual VALUE         StringValue(volatile VALUE* str)     { return ::rb_string_value(str); }
    virtual char*         StringValuePtr(volatile VALUE* str)  { return ::rb_string_value_ptr(str); }
    virtual char*         StringValueCstr(volatile VALUE* str) { return ::rb_string_value_cstr(str); }

    virtual char*         GetStringPtr(VALUE str) { return rb_string_ptr(str); }
    virtual long          GetStringLen(VALUE str) { return rb_string_len(str); }
    virtual VALUE*        GetArrayPtr(VALUE ary)  { return rb_array_ptr(ary); }
    virtual long          GetArrayLen(VALUE ary)  { return rb_array_len(ary); }
    virtual void*         GetUserDataPtr(VALUE d) { return rb_userdata_ptr(d); }

  private:
    pfn_rb_type           rb_type;

    pfn_rb_string_ptr     rb_string_ptr;
    pfn_rb_string_len     rb_string_len;
    pfn_rb_array_ptr      rb_array_ptr;
    pfn_rb_array_len      rb_array_len;
    pfn_rb_userdata_ptr   rb_userdata_ptr;

    int            m_type_T_NIL;
    int            m_type_T_DATA;
    int            m_type_T_ARRAY;
    int            m_type_T_SYMBOL;

  private:
    const char*    m_pVersion;
    const char*    m_pPlatform;
    unsigned long  m_uVersionCode;
  };

} //  RAL

#endif  //  RAL_RUBY_ABSTRACT_LAYER_H
