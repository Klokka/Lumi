#ifndef RAL_INTERFACE_RUBY_ABSTRACT_LAYER_H
#define RAL_INTERFACE_RUBY_ABSTRACT_LAYER_H
#ifdef _MSVC_VER
    #include <Windows.h>
#endif
#include <stdlib.h>

namespace RAL
{
    typedef unsigned long VALUE;
    typedef unsigned long ID;

    #ifndef _MSVC_VER
        #define __cdecl __attribute__((__cdecl__))
    #endif
    #define RUBYCALL   __cdecl

    #ifdef _MSVC_VER
    typedef VALUE      (RUBYCALL* RubyFunc)(...);
    typedef void       (RUBYCALL* RubyDataFunc)(void*);
    #else
    typedef VALUE      (*RubyFunc)(...) RUBYCALL;
    typedef void       (*RubyDataFunc)(void*) RUBYCALL;
    #endif

    static const VALUE Qfalse  = 0;
    static const VALUE Qtrue   = 2;
    static const VALUE Qnil    = 4;
    static const VALUE Qundef  = 6;

    static inline bool RTEST(VALUE v)  { return ((v & ~Qnil) != 0); }
    static inline bool NIL_P(VALUE v)  { return (v == Qnil); }
} //  RAL

using namespace RAL;

//
// Ruby Abstract Layer for C++
// It defines some quick way to call ruby functions
//
struct IRubyAbstractLayer
{
virtual unsigned long GetVersion() const = 0;
  
virtual bool          InterpreterInit() = 0;
virtual void          InterpreterFinalize() = 0;

///<  Variable, Constant
virtual VALUE         GetCObject() const = 0;
virtual VALUE         GetCClass() const = 0;
virtual VALUE         GetCModule() const = 0;
virtual VALUE         GetCInteger() const = 0;
virtual VALUE         GetCNumeric() const = 0;
virtual VALUE         GetMKernel() const = 0;
virtual VALUE         GetEException() const = 0;
virtual VALUE         GetESystemExit() const = 0;
virtual VALUE         GetESyntaxError() const = 0;
virtual VALUE         GetENoMemError() const = 0;
virtual VALUE         GetEArgError() const = 0;
virtual VALUE         GetETypeError() const = 0;
virtual VALUE         GetERuntimeError() const = 0;
  
virtual VALUE         ConstGet(VALUE, ID) = 0;
virtual void          DefineConst(VALUE, const char*, VALUE) = 0;
virtual void          DefineGlobalConst(const char*, VALUE) = 0;
  
virtual VALUE         InstanceVarGet(VALUE obj, const char* vname) = 0;
virtual VALUE         InstanceVarSet(VALUE obj, const char* vname, VALUE v) = 0;

///<  Memory
virtual void*         XMalloc(size_t) = 0;
virtual void          XFree(void*) = 0;

///<  Garbage Collect
virtual void          GcMark(VALUE obj) = 0;

///<  Class, Module
virtual VALUE         DefineClass(const char* klassname, VALUE parent) = 0;
virtual VALUE         DefineModule(const char* modname) = 0;
virtual VALUE         DefineClassUnder(VALUE outer, const char* klassname, VALUE parent) = 0;
virtual VALUE         DefineModuleUnder(VALUE outer, const char* modname) = 0;
  
///<  Method  
virtual void          DefineMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) = 0;
virtual void          DefineModuleFunction(VALUE mod, const char* name, RubyFunc cfunc, int argc) = 0;
virtual void          DefineGlobalFunction(const char* name, RubyFunc cfunc, int argc) = 0;
virtual void          DefineProtectedMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) = 0;
virtual void          DefinePrivateMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) = 0;
virtual void          DefineSingletonMethod(VALUE klass, const char* name, RubyFunc cfunc, int argc) = 0;

///<  Special methods
virtual void          DefineAllocFunc(VALUE klass, VALUE (*c_alloc_func)(VALUE)) = 0;
virtual void          UndefAllocFunc(VALUE klass) = 0;
virtual VALUE         UserDataObjectAlloc(VALUE klass, void* userptr, RubyDataFunc cmark, RubyDataFunc cfree) = 0;
virtual void          UndefMethod(VALUE klass, const char* name) = 0;
virtual void          DefineAlias(VALUE klass, const char* newname,const char* oldname) = 0;

///<  ID & Name
virtual ID            Name2ID(const char*) = 0;
virtual const char*   ID2Name(ID) = 0;
virtual VALUE         ID2Sym(ID) = 0;

///<  Function call
virtual VALUE         Funcall(VALUE obj, ID mid, int argc, ...) = 0;
virtual VALUE         Funcall2(VALUE obj, ID mid, int argc, const VALUE* argv) = 0;
virtual VALUE         Funcall3(VALUE obj, ID mid, int argc, const VALUE* argv) = 0;
virtual int           ScanArgs(int argc, const VALUE* argv, const char* fmt, ...) = 0;
virtual VALUE         CallSuper(int argc, const VALUE* argv) = 0;
virtual int           RespondTo(VALUE obj, ID mid) = 0;
virtual VALUE         EvalString(const char*) = 0;
virtual VALUE         EvalStringProtect(const char*, int*) = 0;

///<  Protect, Runtime
virtual VALUE         ProtectRun(VALUE (*proc)(VALUE), VALUE args, int* state) = 0;
virtual void          Raise(VALUE excp, const char* fmt, ...) = 0;
virtual void          RaiseNoMemory() = 0;
virtual VALUE         GetErrInfo(void) = 0;
  
///<  Types  
virtual VALUE         ObjectClass(VALUE obj) = 0;
virtual VALUE         ObjectSingletonClass(VALUE obj) = 0;
virtual VALUE         ObjectIsInstanceOf(VALUE obj, VALUE klass) = 0;
virtual VALUE         ObjectIsKindOf(VALUE obj, VALUE klass) = 0;
virtual const char*   Class2Name(VALUE klass) = 0;
virtual const char*   ObjectClassName(VALUE obj) = 0;
virtual int           Type(VALUE obj) = 0;
virtual void          CheckType(VALUE obj, int type) = 0;
virtual VALUE         ConvertType(VALUE val, int type, const char *tname, const char *method) = 0;
virtual int           Get_T_DATA() const = 0;
virtual int           Get_T_NIL() const = 0;
virtual int           Get_T_ARRAY() const = 0;
virtual int           Get_T_SYMBOL() const = 0;
virtual void          CheckTypeUserData(VALUE obj) = 0;
virtual void          SafeFixnumValue(VALUE obj) = 0;
virtual void          SafeIntegerValue(VALUE obj) = 0;
virtual void          SafeNumericValue(VALUE obj) = 0;
virtual void          SafeSpecClassValue(VALUE obj, VALUE klass) = 0;

virtual long          Fix2Long(VALUE x) = 0;
virtual unsigned long Fix2Ulong(VALUE x) = 0;
virtual int           Fix2Int(VALUE x) = 0;
virtual unsigned int  Fix2Uint(VALUE x) = 0;
virtual VALUE         Int2Fix(int i) = 0;
virtual VALUE         Long2Fix(long x) = 0;
virtual long          Num2Long(VALUE num) = 0;
virtual unsigned long Num2Ulong(VALUE num) = 0;
virtual double        Num2Dbl(VALUE num) = 0;
virtual VALUE         Int2Num(int v) = 0;
virtual VALUE         Uint2Num(unsigned int v) = 0;
virtual VALUE         Long2Num(long v) = 0;
virtual VALUE         Ulong2Num(unsigned long v) = 0;

///<  Built-in methods
virtual VALUE         StrNew(const char*, long) = 0;
virtual VALUE         StrNew2(const char*) = 0;
virtual VALUE         StrNew3(VALUE) = 0;
virtual void          StrModify(VALUE) = 0;
virtual VALUE         StrCat(VALUE, const char*, long) = 0;
virtual VALUE         StrBufNew(long) = 0;
virtual VALUE         StrBufAppend(VALUE, VALUE) = 0;
virtual VALUE         Inspect(VALUE) = 0;
virtual VALUE         ObjectAsString(VALUE) = 0;
virtual VALUE         ForceEncodingUTF8(VALUE str) = 0;
  
virtual VALUE         AryNew(void) = 0;
virtual VALUE         AryNew2(long len) = 0;
virtual VALUE         AryNew4(long len, const VALUE* values) = 0;
virtual void          AryStore(VALUE ary, long idx, VALUE v) = 0;
virtual VALUE         AryPush(VALUE ary, VALUE v) = 0;
virtual VALUE         AryPop(VALUE ary) = 0;
virtual VALUE         AryShift(VALUE ary) = 0;
virtual VALUE         AryUnshift(VALUE ary, VALUE v) = 0;
virtual VALUE         AryEntry(VALUE ary, long idx) = 0;
virtual VALUE         AryClear(VALUE ary) = 0;
  
virtual VALUE         FloatNew(double) = 0;
  
virtual int           BlockGivenP(void) = 0;
virtual VALUE         BlockProc(void) = 0;

virtual VALUE         StringValue(volatile VALUE*) = 0;
virtual char*         StringValuePtr(volatile VALUE*) = 0;
virtual char*         StringValueCstr(volatile VALUE*) = 0;

virtual char*         GetStringPtr(VALUE str) = 0;
virtual long          GetStringLen(VALUE str) = 0;
virtual VALUE*        GetArrayPtr(VALUE ary) = 0;
virtual long          GetArrayLen(VALUE ary) = 0;
virtual void*         GetUserDataPtr(VALUE d) = 0;
};

#endif // RAL_INTERFACE_RUBY_ABSTRACT_LAYER_H
