#ifndef RAL_RUBY_NATIVE_DEFINE_H
#define RAL_RUBY_NATIVE_DEFINE_H

#include <stddef.h>
#include "ral/IRubyAbstractLayer.h"

using namespace RAL;

#define NATIVE_EXTERN extern "C"

NATIVE_EXTERN VALUE rb_mKernel;
NATIVE_EXTERN VALUE rb_mComparable;
NATIVE_EXTERN VALUE rb_mEnumerable;
NATIVE_EXTERN VALUE rb_mPrecision;
NATIVE_EXTERN VALUE rb_mErrno;
NATIVE_EXTERN VALUE rb_mFileTest;
NATIVE_EXTERN VALUE rb_mGC;
NATIVE_EXTERN VALUE rb_mMath;
NATIVE_EXTERN VALUE rb_mProcess;

NATIVE_EXTERN VALUE rb_cObject;
NATIVE_EXTERN VALUE rb_cArray;
NATIVE_EXTERN VALUE rb_cBignum;
NATIVE_EXTERN VALUE rb_cClass;
NATIVE_EXTERN VALUE rb_cDir;
NATIVE_EXTERN VALUE rb_cData;
NATIVE_EXTERN VALUE rb_cFalseClass;
NATIVE_EXTERN VALUE rb_cFile;
NATIVE_EXTERN VALUE rb_cFixnum;
NATIVE_EXTERN VALUE rb_cFloat;
NATIVE_EXTERN VALUE rb_cHash;
NATIVE_EXTERN VALUE rb_cInteger;
NATIVE_EXTERN VALUE rb_cIO;
NATIVE_EXTERN VALUE rb_cModule;
NATIVE_EXTERN VALUE rb_cNilClass;
NATIVE_EXTERN VALUE rb_cNumeric;
NATIVE_EXTERN VALUE rb_cProc;
NATIVE_EXTERN VALUE rb_cRange;
NATIVE_EXTERN VALUE rb_cRegexp;
NATIVE_EXTERN VALUE rb_cString;
NATIVE_EXTERN VALUE rb_cSymbol;
NATIVE_EXTERN VALUE rb_cThread;
NATIVE_EXTERN VALUE rb_cTime;
NATIVE_EXTERN VALUE rb_cTrueClass;
NATIVE_EXTERN VALUE rb_cStruct;

NATIVE_EXTERN VALUE rb_eException;
NATIVE_EXTERN VALUE rb_eStandardError;
NATIVE_EXTERN VALUE rb_eSystemExit;
NATIVE_EXTERN VALUE rb_eInterrupt;
NATIVE_EXTERN VALUE rb_eSignal;
NATIVE_EXTERN VALUE rb_eFatal;
NATIVE_EXTERN VALUE rb_eArgError;
NATIVE_EXTERN VALUE rb_eEOFError;
NATIVE_EXTERN VALUE rb_eIndexError;
NATIVE_EXTERN VALUE rb_eRangeError;
NATIVE_EXTERN VALUE rb_eIOError;
NATIVE_EXTERN VALUE rb_eRuntimeError;
NATIVE_EXTERN VALUE rb_eSecurityError;
NATIVE_EXTERN VALUE rb_eSystemCallError;
NATIVE_EXTERN VALUE rb_eTypeError;
NATIVE_EXTERN VALUE rb_eZeroDivError;
NATIVE_EXTERN VALUE rb_eNotImpError;
NATIVE_EXTERN VALUE rb_eNoMemError;
NATIVE_EXTERN VALUE rb_eNoMethodError;
NATIVE_EXTERN VALUE rb_eFloatDomainError;

NATIVE_EXTERN VALUE rb_eScriptError;
NATIVE_EXTERN VALUE rb_eNameError;
NATIVE_EXTERN VALUE rb_eSyntaxError;
NATIVE_EXTERN VALUE rb_eLoadError;

NATIVE_EXTERN VALUE rb_stdin, rb_stdout, rb_stderr;
NATIVE_EXTERN VALUE ruby_errinfo;

NATIVE_EXTERN const char* ruby_version;
NATIVE_EXTERN const char* ruby_platform;

NATIVE_EXTERN void ruby_sysinit(int *, char***);

NATIVE_EXTERN void ruby_init_stack(VALUE*);
NATIVE_EXTERN void ruby_init(void);
NATIVE_EXTERN void ruby_set_argv(int, char**);
NATIVE_EXTERN void ruby_init_loadpath(void);
NATIVE_EXTERN void ruby_script(const char*);

NATIVE_EXTERN VALUE rb_const_get(VALUE, ID);
NATIVE_EXTERN void rb_define_const(VALUE, const char*, VALUE);
NATIVE_EXTERN void rb_define_global_const(const char*, VALUE);

NATIVE_EXTERN VALUE rb_iv_get(VALUE, const char*);
NATIVE_EXTERN VALUE rb_iv_set(VALUE, const char*, VALUE);

NATIVE_EXTERN void* ruby_xmalloc(size_t);
NATIVE_EXTERN void ruby_xfree(void*);

NATIVE_EXTERN void rb_gc_mark(VALUE);
    
NATIVE_EXTERN VALUE rb_define_class(const char*, VALUE);
NATIVE_EXTERN VALUE rb_define_module(const char*);
NATIVE_EXTERN VALUE rb_define_class_under(VALUE, const char*, VALUE);
NATIVE_EXTERN VALUE rb_define_module_under(VALUE, const char*);

NATIVE_EXTERN void rb_define_method(VALUE, const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_class_method(VALUE, const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_module_function(VALUE, const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_global_function(const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_protected_method(VALUE, const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_private_method(VALUE, const char*, RubyFunc, int);
NATIVE_EXTERN void rb_define_singleton_method(VALUE, const char*, RubyFunc, int);

NATIVE_EXTERN void rb_define_alloc_func(VALUE, VALUE (*rb_alloc_func_t)(VALUE));
NATIVE_EXTERN void rb_undef_alloc_func(VALUE);
NATIVE_EXTERN VALUE rb_data_object_alloc(VALUE, void*, RubyDataFunc, RubyDataFunc);
NATIVE_EXTERN void rb_undef_method(VALUE, const char*);
NATIVE_EXTERN void rb_define_alias(VALUE, const char*, const char*);

NATIVE_EXTERN ID rb_intern(const char*);
NATIVE_EXTERN const char* rb_id2name(ID);

NATIVE_EXTERN VALUE rb_funcall(VALUE, ID, int, ...);
NATIVE_EXTERN VALUE rb_funcall2(VALUE, ID, int, const VALUE*);
NATIVE_EXTERN VALUE rb_funcall3(VALUE, ID, int, const VALUE*);
NATIVE_EXTERN int rb_scan_args(int, const VALUE*, const char*, ...);
NATIVE_EXTERN VALUE rb_call_super(int, const VALUE*);
NATIVE_EXTERN int rb_respond_to(VALUE, ID);

NATIVE_EXTERN VALUE rb_eval_string(const char*);
NATIVE_EXTERN VALUE rb_eval_string_protect(const char*, int*);

NATIVE_EXTERN VALUE rb_protect(VALUE (*)(VALUE), VALUE, int*);
NATIVE_EXTERN void rb_raise(VALUE, const char*, ...);
NATIVE_EXTERN VALUE rb_errinfo(void);

NATIVE_EXTERN VALUE rb_obj_class(VALUE);
NATIVE_EXTERN VALUE rb_singleton_class(VALUE);
NATIVE_EXTERN VALUE rb_obj_is_instance_of(VALUE, VALUE);
NATIVE_EXTERN VALUE rb_obj_is_kind_of(VALUE, VALUE);
NATIVE_EXTERN const char* rb_class2name(VALUE);
NATIVE_EXTERN const char* rb_obj_classname(VALUE);
NATIVE_EXTERN int rb_type(VALUE obj);
NATIVE_EXTERN void rb_check_type(VALUE, int);
NATIVE_EXTERN VALUE rb_convert_type(VALUE, int, const char *, const char *);

NATIVE_EXTERN long rb_num2long(VALUE);
NATIVE_EXTERN unsigned long rb_num2ulong(VALUE);
NATIVE_EXTERN double rb_num2dbl(VALUE);

NATIVE_EXTERN VALUE rb_int2inum(long);
NATIVE_EXTERN VALUE rb_uint2inum(unsigned long);
    
NATIVE_EXTERN VALUE rb_str_new(const char*, long);
NATIVE_EXTERN VALUE rb_str_new2(const char*);
NATIVE_EXTERN VALUE rb_str_new3(VALUE);
NATIVE_EXTERN void rb_str_modify(VALUE);
NATIVE_EXTERN VALUE rb_str_cat(VALUE, const char*, long);
NATIVE_EXTERN VALUE rb_str_buf_new(long);
NATIVE_EXTERN VALUE rb_str_buf_append(VALUE, VALUE);
NATIVE_EXTERN VALUE rb_inspect(VALUE);
NATIVE_EXTERN VALUE rb_obj_as_string(VALUE);

NATIVE_EXTERN VALUE rb_ary_new(void);
NATIVE_EXTERN VALUE rb_ary_new2(long);
NATIVE_EXTERN VALUE rb_ary_new4(long, const VALUE*);
NATIVE_EXTERN void rb_ary_store(VALUE, long, VALUE);
NATIVE_EXTERN VALUE rb_ary_push(VALUE, VALUE);
NATIVE_EXTERN VALUE rb_ary_pop(VALUE);
NATIVE_EXTERN VALUE rb_ary_shift(VALUE);
NATIVE_EXTERN VALUE rb_ary_unshift(VALUE, VALUE);
NATIVE_EXTERN VALUE rb_ary_entry(VALUE, long);
NATIVE_EXTERN VALUE rb_ary_clear(VALUE);

NATIVE_EXTERN VALUE rb_float_new(double);

NATIVE_EXTERN int rb_block_given_p(void);
NATIVE_EXTERN VALUE rb_block_proc(void);

NATIVE_EXTERN VALUE rb_string_value(volatile VALUE*);
NATIVE_EXTERN char* rb_string_value_ptr(volatile VALUE*);
NATIVE_EXTERN char* rb_string_value_cstr(volatile VALUE*);

NATIVE_EXTERN char* rb_string_ptr(VALUE str);
NATIVE_EXTERN long rb_string_len(VALUE str);
NATIVE_EXTERN VALUE* rb_array_ptr(VALUE ary);
NATIVE_EXTERN long rb_array_len(VALUE ary);
NATIVE_EXTERN void* rb_userdata_ptr(VALUE d);

NATIVE_EXTERN void* rb_utf8_encoding(void);
NATIVE_EXTERN VALUE rb_enc_associate(VALUE, void*);

#undef NATIVE_EXTERN

#endif //  RAL_RUBY_NATIVE_DEFINE_H
