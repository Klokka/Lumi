#include "RubyAbstractLayer.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>

using namespace RAL;

CRubyAbstractLayer::CRubyAbstractLayer()
  : rb_string_ptr(0)
  , rb_string_len(0)
  , rb_array_ptr(0)
  , rb_array_len(0)
  , rb_userdata_ptr(0)

  , m_type_T_NIL(0)
  , m_type_T_DATA(0)
  , m_type_T_ARRAY(0)
  , m_type_T_SYMBOL(0)

  , m_pVersion(0)
  , m_pPlatform(0)
  , m_uVersionCode(0)
{
}

CRubyAbstractLayer::~CRubyAbstractLayer() {}

/**
 *  Init Ruby Abstract Layer
 */
bool CRubyAbstractLayer::Init()
{
  m_pVersion  = ::ruby_version;
  m_pPlatform = ::ruby_platform;
  if (!m_pVersion || !m_pPlatform)
    return false;

  if (memcmp(m_pVersion, "1.9.1", 5) == 0)      m_uVersionCode  = 191;
  else if (memcmp(m_pVersion, "1.9.2", 5) == 0) m_uVersionCode  = 192;
  else if (memcmp(m_pVersion, "1.9.3", 5) == 0) m_uVersionCode  = 193;
  else if (memcmp(m_pVersion, "2.0.0", 5) == 0) m_uVersionCode  = 200;
  else return false;

  if (!InitRubyDifference())
    return false;

  return true;
}

/**
 *  Init Ruby interpreter in 1.9.3
 */
bool CRubyAbstractLayer::InitRuby193()
{
  int    argc = 0;
  char** argv = 0;

  ::ruby_sysinit(&argc, &argv);
  {
    VALUE variable_in_this_stack_frame;
    ::ruby_init_stack(&variable_in_this_stack_frame);
    ::ruby_init();
    ::ruby_set_argv(argc - 1, argv + 1);
    ::ruby_init_loadpath();
  }

  return true;
}

/**
 *  Init the difference with Ruby's version.
 */
bool CRubyAbstractLayer::InitRubyDifference()
{
  switch (GetVersion())
  {
  case 191:
  case 192:
  case 193:
  case 200:
    {
      rb_type         = RubyStructV193::rb_type;
  
      rb_string_ptr   = RubyStructV193::RSTRING_PTR;
      rb_string_len   = RubyStructV193::RSTRING_LEN;
      rb_array_ptr    = RubyStructV193::RARRAY_PTR;
      rb_array_len    = RubyStructV193::RARRAY_LEN;
      rb_userdata_ptr = RubyStructV193::DATA_PTR;

      m_type_T_NIL    = RubyStructV193::T_NIL;
      m_type_T_DATA   = RubyStructV193::T_DATA;
      m_type_T_ARRAY  = RubyStructV193::T_ARRAY;
      m_type_T_SYMBOL = RubyStructV193::T_SYMBOL;
    }
    break;
  default:
    return false;
  }

  return true;
}

/**
 *  Init Ruby interpreter
 */
bool CRubyAbstractLayer::InterpreterInit()
{
  switch (GetVersion())
  {
  case 191: 
  case 192: 
  case 193: 
  case 200: return InitRuby193();
  }
  return false;
}

/**
 *  Finalize Ruby interpreter
 */
void CRubyAbstractLayer::InterpreterFinalize()
{
  switch (GetVersion())
  {
  case 191:
  case 192:
  case 193:
  case 200:
    {
      //::rb_thread_terminate_all();
      // Sleep(100);         //  等待线程结束
      //::ruby_cleanup(0);
    }
    break;
  default:
    break;
  }
}

VALUE CRubyAbstractLayer::Funcall(VALUE obj, ID mid, int argc, ...)
{
  va_list vargs;

  va_start(vargs, argc);

  VALUE argv[16];

  RalAssert(argc <= RalArrayCount(argv));

  for (int i = 0; i < argc; ++i)
  {
    argv[i] = va_arg(vargs, VALUE);
  }

  va_end(vargs);

  return ::rb_funcall2(obj, mid, argc, argv);
}

int CRubyAbstractLayer::ScanArgs(int argc, const VALUE* argv, const char* fmt, ...)
{
  int n, i = 0;
  const char *p = fmt;
  VALUE *var;
  va_list vargs;

  va_start(vargs, fmt);

  if (*p == '*') goto rest_arg;

  if (*p >= '0' && *p <= '9') {
    n = *p - '0';
    if (n > argc)
      Raise(GetEArgError(), "wrong number of arguments (%d for %d)", argc, n);
    for (i=0; i<n; i++) {
      var = va_arg(vargs, VALUE*);
      if (var) *var = argv[i];
    }
    p++;
  }
  else {
    goto error;
  }

  if (*p >= '0' && *p <= '9') {
    n = i + *p - '0';
    for (; i<n; i++) {
      var = va_arg(vargs, VALUE*);
      if (argc > i) {
        if (var) *var = argv[i];
      }
      else {
        if (var) *var = Qnil;
      }
    }
    p++;
  }

  if(*p == '*') {
rest_arg:
    var = va_arg(vargs, VALUE*);
    if (argc > i) {
      if (var) *var = AryNew4(argc-i, argv+i);
      i = argc;
    }
    else {
      if (var) *var = AryNew();
    }
    p++;
  }

  if (*p == '&') {
    var = va_arg(vargs, VALUE*);
    if (BlockGivenP()) {
      *var = BlockProc();
    }
    else {
      *var = Qnil;
    }
    p++;
  }
  va_end(vargs);

  if (*p != '\0') {
    goto error;
  }

  if (argc > i) {
    Raise(GetEArgError(), "wrong number of arguments (%d for %d)", argc, i);
  }

  return argc;

error:
  Raise(GetERuntimeError(), "bad scan arg format: %s", fmt);
  return 0;
}

void CRubyAbstractLayer::Raise(VALUE excp, const char* fmt, ...)
{
  char pTempBuffer[1024];

  va_list ap;
  va_start(ap, fmt);
#if _MSVC_VER
  vsprintf_s(pTempBuffer, 1024, fmt, ap);
#else
  vsnprintf(pTempBuffer, 1024, fmt, ap);
#endif
  va_end(ap);

  ::rb_raise(excp, pTempBuffer);
}
#if 0
void CRubyAbstractLayer::RaiseWinException(DWORD dwError)
{
  wchar_t* pTempBuffer = (wchar_t*)m_pRefCoreRuntime->GetTempBuffer01Ptr(sizeof(wchar_t) * 1024);

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 
    NULL,
    dwError, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    pTempBuffer, 
    1024, 
    NULL);

  rb_raise(GetERuntimeError(), m_pRefCoreRuntime->GetKconv()->UnicodeToUTF8(pTempBuffer));
}
#endif
void CRubyAbstractLayer::RaiseNoMemory()
{
  ::rb_raise(GetENoMemError(), "failed to allocate memory");
}

VALUE CRubyAbstractLayer::GetErrInfo(void)
{
  switch (GetVersion())
  {
  case 191:
  case 192:
  case 193:
  case 200: return ::rb_errinfo();
  }
  return Qnil;
}

void CRubyAbstractLayer::SafeFixnumValue(VALUE obj)
{
  if (!RubyCommon::FIXNUM_P(obj))
  {
    Raise(GetETypeError(), "wrong argument type %s (expected Fixnum)", ObjectClassName(obj));
  }
}

void CRubyAbstractLayer::SafeIntegerValue(VALUE obj)
{
  if (!ObjectIsKindOf(obj, GetCInteger()))
  {
    Raise(GetETypeError(), "wrong argument type %s (expected Integer)", ObjectClassName(obj));
  }
}

void CRubyAbstractLayer::SafeNumericValue(VALUE obj)
{
  if (!ObjectIsKindOf(obj, GetCNumeric()))
  {
    Raise(GetETypeError(), "wrong argument type %s (expected Numeric)", ObjectClassName(obj));
  }
}

void CRubyAbstractLayer::SafeSpecClassValue(VALUE obj, VALUE klass)
{
  if (ObjectClass(obj) != klass)
  {
    Raise(GetETypeError(), "wrong argument type %s (expected %s)", ObjectClassName(obj), Class2Name(klass));
  }
}

VALUE CRubyAbstractLayer::Int2Num(int v)
{
  if (RubyCommon::FIXABLE(v))
    return RubyCommon::INT2FIX(v);
  
  return ::rb_int2inum((long)v);
}

VALUE CRubyAbstractLayer::Uint2Num(unsigned int v)
{
  if (RubyCommon::POSFIXABLE(v))
    return RubyCommon::INT2FIX((int)v);

  return ::rb_uint2inum((unsigned long)v);
}

VALUE CRubyAbstractLayer::ForceEncodingUTF8(VALUE str)
{
  if (NIL_P(str))
    return Qnil;

  switch (GetVersion())
  {
  case 191:
  case 192:
  case 193:
  case 200:
    {
      return ::rb_enc_associate(str, ::rb_utf8_encoding());
    }
    break;
  }

  return str;
}
