#ifndef LUMI_RUBY_H
#define LUMI_RUBY_H
#include "ral/CppExporter.hpp"
#ifdef LUMI_WIN32
#include <windef.h>
#endif

namespace Lumi
{
  using namespace RAL::CppExporter;

  class Lumi
  {
    public:
      Lumi() {}
      ~Lumi() {}
    public:
      void test(char *);
  };

  void test(char *msg);
  void init();

} // Lumi

#endif //  LUMI_RUBY_H
