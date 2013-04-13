#include <windows.h>
#include "ruby.h"

namespace Lumi
{
  using namespace RAL::CppExporter;

  void Lumi::test(char *msg)
  {
    MessageBox(NULL, msg, "test", MB_OK);
  }

  void init()
  {
    IRubyAbstractLayer* ral = GetIRAL();
    VALUE mLumi = ral->DefineModule("Lumi");

    _class<Lumi>(mLumi, "Lumi");
      //.alloc();
      //.def<1>("test", &Lumi::test);
  }

} // Lumi
